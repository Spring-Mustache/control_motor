#!/usr/bin/env python3
"""Enforce the operator's VERBATIM context-handoff specification on every push.

This checks source integrity, 17-section structure and required document
updates whenever firmware/code changes. It CANNOT reconstruct ChatGPT chats,
prove the quality of prose, or declare STM32/hardware acceptance.
"""
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import sys


REPO = Path(__file__).resolve().parents[1]
SPEC = REPO / "docs" / "CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md"
HANDOFF = REPO / "CONTEXT_HANDOFF.md"
ROADMAP = REPO / "docs" / "PROJECT_MILESTONES.md"
EXPECTED_ORIGINAL_SHA256 = "13d8976ec0d74057af94bf84c8445bddbb48715b308964e22eb6684711f708a4"


def git(*arguments: str) -> str:
    result = subprocess.run(
        ["git", "-C", str(REPO), *arguments],
        capture_output=True, text=True, encoding="utf-8", check=True,
    )
    return result.stdout.strip()


def updated_code(path: str) -> bool:
    p = Path(path)
    firmware = path.startswith(("Core/", "Drivers/", "Middlewares/"))
    if firmware and p.suffix.lower() in {".c", ".h", ".s", ".cpp", ".hpp", ".asm"}:
        return True
    if p.suffix.lower() in {".ioc", ".ld"}:
        return True
    # Changes to the checker, packaging script and workflow itself are
    # changes to project behavior: require updated context documents too.
    if path.startswith("tools/") and p.suffix.lower() in {".py", ".c", ".h"}:
        return True
    if path.startswith(".github/workflows/") and p.suffix.lower() in {".yml", ".yaml"}:
        return True
    return False


def check_structure() -> list[str]:
    errors: list[str] = []
    if not SPEC.is_file():
        errors.append("missing original 386-line handoff requirements")
    else:
        raw = SPEC.read_bytes()
        if hashlib.sha256(raw).hexdigest() != EXPECTED_ORIGINAL_SHA256:
            errors.append("ORIGINAL REQUIREMENTS CHANGED: restore exact user-provided bytes")
        if len(raw.splitlines()) != 386:
            errors.append("original requirements line count differs from 386")
    if not HANDOFF.is_file():
        errors.append("missing CONTEXT_HANDOFF.md")
    else:
        content = HANDOFF.read_text(encoding="utf-8")
        matches = re.findall(r"^##\s+(\d+)\.\s+.*$", content, re.MULTILINE)
        if matches != [str(i) for i in range(17)]:
            errors.append("CONTEXT_HANDOFF.md must have ordered sections 0 through 16")
        if not content.rstrip().endswith("END OF CONTEXT HANDOFF"):
            errors.append("missing required END OF CONTEXT HANDOFF footer")
        for i in range(17):
            m = re.search(
                r"^##[ \t]+" + str(i) + r"\.[ \t]+[^\n]*\n(.*?)(?=^##[ \t]+\d+\.|\Z)",
                content, re.MULTILINE | re.DOTALL,
            )
            if m is None or not m.group(1).strip():
                errors.append(f"section {i} is empty")
    if not ROADMAP.is_file():
        errors.append("missing docs/PROJECT_MILESTONES.md")
    return errors


def diff_files(before: str, head: str) -> set[str]:
    """Check the push range, or the commit's parent on manual dispatch.

    Never silently skip the update gate merely because workflow_dispatch
    does not provide github.event.before.
    """
    if not before:
        try:
            before = git("rev-parse", f"{head}^")
        except subprocess.CalledProcessError as exc:
            raise ValueError("no previous SHA and cannot resolve head parent") from exc
    if set(before) == {"0"}:
        # First push of a branch: use the commit parent (if available).
        # Missing parent must fail closed, rather than silently skip.
        try:
            before = git("rev-parse", f"{head}^")
        except subprocess.CalledProcessError as exc:
            raise ValueError("branch creation without resolvable head parent") from exc
    return set(git("diff", "--name-only", before, head).splitlines())


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--before", default="", help="push event's previous SHA")
    parser.add_argument("--head", default="HEAD", help="current release SHA")
    args = parser.parse_args()
    errors = check_structure()
    # Run the content-update gate for BOTH push and manual dispatch.
    try:
        changed = diff_files(args.before, args.head)
    except (subprocess.CalledProcessError, ValueError) as exc:
        errors.append(f"cannot inspect code changes; refusing to verify: {exc}")
        changed = set()
    code_changes = sorted(x for x in changed if updated_code(x))
    if code_changes:
        print("CODE/WORKFLOW/TOOL CHANGES DETECTED:", ", ".join(code_changes))
        for required in ("CONTEXT_HANDOFF.md", "docs/PROJECT_MILESTONES.md"):
            if required not in changed:
                errors.append(
                    f"code updated without revising {required} in the checked range. "
                    "Update it under the original 17-section specification first."
                )
    else:
        print("No code updates in checked range; historical material is retained.")
    if errors:
        for error in errors:
            print("FAIL:", error, file=sys.stderr)
        raise SystemExit(1)
    print("PASS: exact 386-line original specification SHA-256")
    print("PASS: full 0-16 structured handoff and required footer")
    print("PASS: code/workflow/tool changes require handoff + roadmap updates (push/dispatch)")
    print("NOTE: source checks do NOT establish full prose accuracy or hardware PASS")


if __name__ == "__main__":
    main()
