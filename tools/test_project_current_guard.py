#!/usr/bin/env python3
"""Offline regression: new active milestone must outrank historical main/M7."""
import base64
import unittest

from verify_project_current import (
    GuardError, check_bootstrap, parse_status, verify_status,
)

M9 = "7fd8ab82552efb59de12d12e6960eaeda8149d16"
FIRMWARE = "b1c1e9c4d8f7d191b21123a328ee1dea550595da"
OLD = "bcf6c2a4e122de91ee8fdb864f5fe7cfd10cca5d"

DOC = f"""# PROJECT CURRENT

| Trường | Giá trị |
|---|---|
| Milestone hiện hành | M9 — Steering Active Position Hold |
| Nhánh phát triển được chỉ định | feature/m9-steering-active-position-hold |
| HEAD nhánh M9 đã xác minh | {M9} |
| Mốc source firmware M9 Profile 4 | {FIRMWARE} |
| Hồ sơ bàn giao | docs/M9_SESSION_HANDOFF_2026_11_11.md |
"""


def fake_api(endpoint):
    if endpoint.startswith("git/ref/heads/"):
        return {"object": {"sha": M9}}
    if endpoint.startswith("compare/"):
        return {"status": "ahead", "behind_by": 0}
    if endpoint.startswith("contents/"):
        return {
            "encoding": "base64",
            "content": base64.b64encode(
                b"# CONTEXT HANDOFF\nM9 active two-axis position hold\n"
            ).decode("ascii"),
        }
    raise AssertionError(f"Unexpected endpoint: {endpoint}")


class ProjectCurrentTest(unittest.TestCase):
    def test_current_m9_valid(self):
        result = verify_status(DOC, fake_api)
        self.assertEqual(result["milestone"], "M9")
        self.assertEqual(result["head"], M9)

    def test_wrong_old_m7_branch_blocked(self):
        wrong = DOC.replace(
            "feature/m9-steering-active-position-hold",
            "feature/m7-s15-ps2-controls-restoration-staged",
        )
        with self.assertRaisesRegex(GuardError, "Milestone and active branch"):
            parse_status(wrong)

    def test_stale_m9_head_fails_closed(self):
        with self.assertRaisesRegex(GuardError, "HEAD mismatch"):
            verify_status(DOC.replace(M9, OLD), fake_api)

    def test_unrelated_firmware_baseline_blocked(self):
        def unrelated(endpoint):
            if endpoint.startswith("compare/"):
                return {"status": "diverged", "behind_by": 1}
            return fake_api(endpoint)
        with self.assertRaisesRegex(GuardError, "not an ancestor"):
            verify_status(DOC, unrelated)

    def test_wrong_handoff_fails(self):
        def wrong_handoff(endpoint):
            if endpoint.startswith("contents/"):
                return {
                    "encoding": "base64",
                    "content": base64.b64encode(b"# Old M7 handoff").decode(),
                }
            return fake_api(endpoint)
        with self.assertRaisesRegex(GuardError, "does not mention"):
            verify_status(DOC, wrong_handoff)

    def test_missing_milestone_field(self):
        with self.assertRaisesRegex(GuardError, "Missing status field"):
            parse_status(DOC.replace("Milestone hiện hành", "Khac"))

    def test_truncated_firmware_sha(self):
        with self.assertRaisesRegex(GuardError, "complete lowercase SHA"):
            parse_status(DOC.replace(FIRMWARE, "b1c1e9c4"))

    def test_old_main_bootstrap_rejected(self):
        with self.assertRaisesRegex(GuardError, "obsolete M7 branch"):
            check_bootstrap(
                "Read docs/PROJECT_CURRENT.md "
                "feature/m7-s15-ps2-controls-restoration-staged",
                "Read docs/PROJECT_CURRENT.md",
            )

    def test_new_main_bootstrap_passes(self):
        check_bootstrap(
            "Read docs/PROJECT_CURRENT.md before determining milestone",
            "Read docs/PROJECT_CURRENT.md and verify the active branch HEAD",
        )


if __name__ == "__main__":
    unittest.main()
