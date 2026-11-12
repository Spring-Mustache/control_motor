#!/usr/bin/env python3
"""Read-only canonical status validator. No Release dependency."""
from __future__ import annotations

import base64
import json
import os
import re
import sys
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class GuardError(Exception):
    pass


def parse_status(content: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for key, value in re.findall(
        r"^\|\s*([^|]+?)\s*\|\s*([^|]+?)\s*\|\s*$",
        content, flags=re.MULTILINE,
    ):
        key, value = key.strip(), value.strip().strip(chr(96))
        if key in fields:
            raise GuardError(f"Duplicate field: {key}")
        fields[key] = value

    def require(key: str) -> str:
        value = fields.get(key, "")
        if not value:
            raise GuardError(f"Missing status field: {key}")
        return value

    def prefix(name: str) -> str:
        matches = [v for k, v in fields.items() if k.startswith(name)]
        if len(matches) != 1:
            raise GuardError(f"Expected one status field starting with: {name}")
        return matches[0]

    milestone = require("Milestone hiện hành")
    match = re.match(r"^(M[0-9]+)\b", milestone, flags=re.IGNORECASE)
    if not match:
        raise GuardError("Milestone must begin with M and a number")
    milestone_id = match.group(1).upper()

    branch = require("Nhánh phát triển được chỉ định")
    if (
        not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._/-]*", branch)
        or ".." in branch
        or "//" in branch
        or branch.endswith(("/", ".", ".lock"))
        or branch == "main"
    ):
        raise GuardError("Invalid active branch name")

    branch_match = re.search(
        r"(?:^|[/_-])(m[0-9]+)(?=$|[/_-])", branch, flags=re.IGNORECASE,
    )
    if not branch_match or branch_match.group(1).upper() != milestone_id:
        raise GuardError("Milestone and active branch name disagree")

    snapshot = prefix("HEAD nhánh ")
    baseline = prefix("Mốc source firmware ")
    for label, sha in (("HEAD snapshot", snapshot), ("firmware baseline", baseline)):
        if not re.fullmatch(r"[0-9a-f]{40}", sha):
            raise GuardError(f"{label} must be a complete lowercase SHA")

    handoff = require("Hồ sơ bàn giao")
    if (
        not handoff.startswith("docs/")
        or not handoff.endswith(".md")
        or ".." in Path(handoff).parts
        or "\\" in handoff
        or "//" in handoff
    ):
        raise GuardError("Handoff document path is invalid")

    return {
        "milestone": milestone_id, "branch": branch, "head": snapshot,
        "baseline": baseline, "handoff": handoff,
    }


def check_bootstrap(readme: str, prompt: str) -> None:
    stale = "feature/m7-s15-ps2-controls-restoration-staged"
    for name, content in (("README.md", readme), ("CHAT_B_START_PROMPT.md", prompt)):
        if "docs/PROJECT_CURRENT.md" not in content:
            raise GuardError(f"{name} does not link to canonical status")
        if stale in content:
            raise GuardError(f"{name} still pins obsolete M7 branch")


def verify_status(content: str, get_json) -> dict[str, str]:
    status = parse_status(content)
    branch, head, baseline = status["branch"], status["head"], status["baseline"]

    ref = get_json("git/ref/heads/" + urllib.parse.quote(branch, safe="/"))
    observed = ref.get("object", {}).get("sha")
    if observed != head:
        raise GuardError(
            f"Active branch HEAD mismatch: recorded={head}, observed={observed}. "
            "Reconcile and update PROJECT_CURRENT.md."
        )

    comparison = get_json(f"compare/{baseline}...{head}")
    if comparison.get("status") not in ("ahead", "identical"):
        raise GuardError("Firmware baseline is not an ancestor of active HEAD")
    if comparison.get("behind_by", 0):
        raise GuardError("Firmware baseline comparison reports divergence")

    path = urllib.parse.quote(status["handoff"], safe="/")
    query = urllib.parse.quote(branch, safe="")
    payload = get_json(f"contents/{path}?ref={query}")
    if payload.get("encoding") != "base64" or not payload.get("content"):
        raise GuardError("Cannot read designated handoff from active branch")
    try:
        handoff = base64.b64decode(payload["content"]).decode("utf-8")
    except (ValueError, UnicodeError) as exc:
        raise GuardError(f"Invalid handoff encoding: {exc}") from exc
    if not re.search(r"\b" + re.escape(status["milestone"]) + r"\b", handoff):
        raise GuardError("Designated handoff does not mention active milestone")

    return status


def github_get(endpoint: str):
    repo = os.environ.get("GITHUB_REPOSITORY", "")
    token = os.environ.get("GH_TOKEN", "")
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repo):
        raise GuardError("Invalid GITHUB_REPOSITORY")
    if not token:
        raise GuardError("Missing GH_TOKEN")
    request = urllib.request.Request(
        f"https://api.github.com/repos/{repo}/{endpoint}",
        headers={
            "Accept": "application/vnd.github+json",
            "Authorization": f"Bearer {token}",
            "User-Agent": "project-current-validator",
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=25) as response:
            return json.load(response)
    except (urllib.error.URLError, json.JSONDecodeError) as exc:
        raise GuardError(f"GitHub API read failed: {exc}") from exc


def main() -> int:
    try:
        content = (ROOT / "docs/PROJECT_CURRENT.md").read_text(encoding="utf-8")
        readme = (ROOT / "README.md").read_text(encoding="utf-8")
        prompt = (ROOT / "docs/CHAT_B_START_PROMPT.md").read_text(encoding="utf-8")
        check_bootstrap(readme, prompt)
        result = verify_status(content, github_get)
    except (GuardError, OSError) as exc:
        print(f"::error::{exc}", file=sys.stderr)
        return 1
    print(
        f"PASS: {result['milestone']} | {result['branch']} | "
        f"HEAD={result['head']} | firmware={result['baseline']}"
    )
    print("NOTE: no MCU build/flash/hardware acceptance is inferred")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
