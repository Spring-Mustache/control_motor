#!/usr/bin/env python3
"""Regress contract gate + resume from a real Git checkout with NO Release.

No GitHub token, network, release, firmware, motor or external dependency.
"""
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SPEC = ROOT / "docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md"
REQUIREMENTS_SHA = "13d8976ec0d74057af94bf84c8445bddbb48715b308964e22eb6684711f708a4"


def run(cwd, *args, ok=True):
    r = subprocess.run(args, cwd=cwd, text=True, capture_output=True)
    if ok and r.returncode != 0:
        raise AssertionError(f"{args}: rc={r.returncode}\n{r.stdout}\n{r.stderr}")
    return r


def main():
    assert hashlib.sha256(SPEC.read_bytes()).hexdigest() == REQUIREMENTS_SHA
    assert len(SPEC.read_bytes().splitlines()) == 386
    workflow = (ROOT / ".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
    prompt = (ROOT / "docs/CHAT_B_START_PROMPT.md").read_text(encoding="utf-8")
    assert "publish_release:" in workflow
    assert "github.event_name == 'workflow_dispatch' && inputs.publish_release == true" in workflow
    assert "needs: verify-handoff" in workflow
    assert "contents: read" in workflow and "contents: write" in workflow
    assert "Release" in prompt and "không" in prompt and "HEAD" in prompt
    assert "CONTEXT_HANDOFF.md" in prompt and "PROJECT_MILESTONES.md" in prompt

    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp) / "repo"
        (root / "tools").mkdir(parents=True)
        (root / "docs").mkdir()
        shutil.copy2(ROOT / "tools/verify_context_handoff_contract.py",
                     root / "tools/verify_context_handoff_contract.py")
        shutil.copy2(SPEC, root / "docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md")
        shutil.copy2(ROOT / "CONTEXT_HANDOFF.md", root / "CONTEXT_HANDOFF.md")
        shutil.copy2(ROOT / "docs/PROJECT_MILESTONES.md",
                     root / "docs/PROJECT_MILESTONES.md")
        run(root, "git", "init", "-q")
        run(root, "git", "config", "user.name", "Context CI")
        run(root, "git", "config", "user.email", "ci@example.invalid")
        run(root, "git", "add", ".")
        run(root, "git", "commit", "-qm", "baseline")
        base = run(root, "git", "rev-parse", "HEAD").stdout.strip()

        # CHANGE TO SOURCE with no docs must fail on push AND manual
        (root / "Core/Src").mkdir(parents=True)
        (root / "Core/Src/demo.c").write_text("int demo(void){return 1;}\n")
        run(root, "git", "add", ".")
        run(root, "git", "commit", "-qm", "source-only invalid update")
        changed = run(root, "git", "rev-parse", "HEAD").stdout.strip()
        checker = str(root / "tools/verify_context_handoff_contract.py")
        push_fail = run(root, sys.executable, checker,
                        "--before", base, "--head", changed, ok=False)
        manual_fail = run(root, sys.executable, checker, "--head", changed, ok=False)
        assert push_fail.returncode != 0 and "CONTEXT_HANDOFF.md" in push_fail.stderr
        assert manual_fail.returncode != 0 and "PROJECT_MILESTONES.md" in manual_fail.stderr

        # A subsequent commit changing code AND both living docs must pass.
        (root / "Core/Src/demo.c").write_text("int demo(void){return 2;}\n")
        h = root / "CONTEXT_HANDOFF.md"
        h.write_text(h.read_text(encoding="utf-8").replace(
            "\nEND OF CONTEXT HANDOFF",
            "\n### Synthetic test update, do not publish\n\nVerified source change.\n\nEND OF CONTEXT HANDOFF"),
            encoding="utf-8")
        milestone = root / "docs/PROJECT_MILESTONES.md"
        milestone.write_text(milestone.read_text(encoding="utf-8")
                             + "\nSynthetic CI test marker, not a real milestone.\n",
                             encoding="utf-8")
        run(root, "git", "add", ".")
        run(root, "git", "commit", "-qm", "source plus context update")
        new_sha = run(root, "git", "rev-parse", "HEAD").stdout.strip()
        run(root, sys.executable, checker, "--before", changed, "--head", new_sha)
        run(root, sys.executable, checker, "--head", new_sha)

        # SPEC modification must fail EVEN when code/docs are present.
        spec = root / "docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md"
        spec.write_bytes(spec.read_bytes() + b"\n")
        tampered = run(root, sys.executable, checker, "--head", new_sha, ok=False)
        assert tampered.returncode != 0 and "ORIGINAL REQUIREMENTS CHANGED" in tampered.stderr

    # No Releases directory/tag was created, yet the exact SHA and all
    # required repo docs were readable and the contract was verifiable.
    sha = run(ROOT, "git", "rev-parse", "HEAD").stdout.strip()
    for p in ("README.md", "CONTEXT_HANDOFF.md",
              "docs/PROJECT_MILESTONES.md", "docs/CHAT_B_START_PROMPT.md",
              "docs/CONTEXT_RELEASE_POLICY.md"):
        blob = run(ROOT, "git", "show", f"{sha}:{p}").stdout
        assert blob.strip(), p
    # Optional packaging can be verified WITHOUT publishing a Release.
    # Compare ZIP bytes to the exact immutable commit tree, not local files.
    with tempfile.TemporaryDirectory() as out:
        run(ROOT, sys.executable, str(ROOT / "tools/build_context_release.py"),
            "--output", out)
        package = Path(out) / f"context-handoff-{sha[:12]}.zip"
        assert package.is_file(), package
        with zipfile.ZipFile(package) as z:
            names = set(z.namelist())
            assert {"README.md", "CONTEXT_HANDOFF.md", "RELEASE_STATUS.md",
                    "docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md",
                    "docs/PROJECT_MILESTONES.md"} <= names
            for name in names - {"RELEASE_STATUS.md"}:
                exact = subprocess.check_output(["git", "show", f"{sha}:{name}"],
                                                cwd=ROOT)
                assert z.read(name) == exact, f"mixed commit bytes in {name}"
        print("PASS: offline ZIP built on request from exact committed blobs")
    print("PASS: push code-only blocked; code+two-docs accepted")
    print("PASS: manual dispatch checks parent, cannot bypass missing docs")
    print("PASS: 386-line specification integrity protected")
    print("PASS: direct repository/SHA handoff without GitHub Release")


if __name__ == "__main__":
    main()
