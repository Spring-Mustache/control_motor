#!/usr/bin/env python3
"""Package an OPTIONAL versioned, source-grounded handoff for milestones.

Do not infer hardware PASS, regenerate private ChatGPT history, or alter git.
No third-party packages required.
"""
from __future__ import annotations

import argparse
import datetime as dt
import os
from pathlib import Path
import subprocess
import zipfile


def git(*args: str) -> str:
    return subprocess.check_output(["git", *args], text=True, encoding="utf-8").strip()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", default="dist/context-release")
    args = parser.parse_args()
    root = Path(git("rev-parse", "--show-toplevel"))
    os.chdir(root)
    sha = git("rev-parse", "HEAD")
    branch = os.environ.get("GITHUB_REF_NAME") or git("branch", "--show-current") or "detached"
    repo = os.environ.get("GITHUB_REPOSITORY", "Spring-Mustache/control_motor")
    output = (root / args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    recent = git("log", "-n", "20", "--date=iso-strict", "--pretty=format:%h | %ad | %s")
    changed = git("diff-tree", "--root", "--no-commit-id", "--name-status", "-r", "HEAD")
    stamp = dt.datetime.now(dt.timezone.utc).strftime("%Y-%m-%d %H:%M UTC")
    # Files are always read from the exact COMMITTED tree, never a dirty
    # working copy, so local manual packaging cannot silently mix versions.
    tracked = set(git("ls-tree", "-r", "--name-only", sha).splitlines())
    handoff = root / "CONTEXT_HANDOFF.md"
    original_spec = root / "docs" / "CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md"
    if "docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md" not in tracked or not original_spec.is_file():
        raise SystemExit("Missing verbatim original handoff requirements")
    if "CONTEXT_HANDOFF.md" not in tracked or not handoff.is_file():
        raise SystemExit("Missing CONTEXT_HANDOFF.md at repository root: cannot publish an incomplete handoff")
    if "docs/PROJECT_MILESTONES.md" not in tracked or not (root / "docs" / "PROJECT_MILESTONES.md").is_file():
        raise SystemExit("Missing docs/PROJECT_MILESTONES.md")

    status = f"""# PROJECT RELEASE HANDOFF — {sha[:12]}

> **Bắt đầu từ đây khi chuyển sang cuộc hội thoại B.** Bản này được tạo tự động từ commit thật. Nội dung chat/kiểm thử ngoài GitHub **không thể tự suy ra**. Xem thêm `CONTEXT_HANDOFF.md` và tài liệu trạng thái mới nhất.

- Repository: https://github.com/{repo}
- Nhánh của lần cập nhật: `{branch}`
- Commit chính xác: `{sha}`
- Mã nguồn tại commit: https://github.com/{repo}/tree/{sha}
- Bản diff: https://github.com/{repo}/commit/{sha}
- Giờ tạo gói (UTC): {stamp}
- Mốc hiện hành: xem **mục 1 của CONTEXT_HANDOFF.md**, phần cập nhật mới nhất của roadmap, code và commit diff ở SHA này. Không suy từ release hoặc ghi chú lịch sử.
- **Trạng thái Build/Flash/hardware của commit này: CHƯA KIỂM CHỨNG trong gói bàn giao**, trừ khi có bằng chứng kiểm thử riêng, ghi rõ trong commit/release. GitHub Actions tạo file này **không** phải build STM32 hoặc thử trên xe.
- `CONTEXT_HANDOFF.md` gồm **lịch sử gốc được giữ nguyên + cập nhật tích lũy 17 mục**; những SHA trong phần lịch sử có thể cũ. Luôn đối chiếu HEAD thực của đúng nhánh và trạng thái hiện hành ở đầu hồ sơ. Release này là bản tùy chọn, không phải điều kiện để tiếp quản.
- Trạng thái lỗi FIRST-STOP RIGHT được **người dùng báo đã fix** (xem hồ sơ), không coi là lỗi còn mở chỉ vì tài liệu cũ viết khác; nguồn/commit sửa chi tiết có thể chưa được đối chiếu.

## Đúng commit này thay đổi những gì

```text
{changed or '(không có diff được git diff-tree hiển thị)'}
```

## 20 commit gần nhất mà bản chụp có thể đọc

```text
{recent}
```

## Tài liệu để B kiểm tra theo thứ tự

1. Xác minh nhánh và HEAD SHA độc lập trên GitHub; không dùng release cũ để chọn HEAD.
2. `CONTEXT_HANDOFF.md` (lịch sử + mục cập nhật do người thực hiện commit; không tự nhập chat).
3. `docs/CHAT_B_START_PROMPT.md` (lời nhắn khởi đầu cho hội thoại mới).
4. `docs/PROJECT_MILESTONES.md` (roadmap tích lũy; ưu tiên mục ghi ngày mới nhất, bảng mở đầu có thể cũ).
5. `docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md` (bản gốc nguyên văn 386 dòng, không thay chữ; yêu cầu cho TỪNG lần bàn giao).
6. `docs/CONTEXT_RELEASE_POLICY.md` (nguyên tắc cập nhật trạng thái và phát hành tự động).
7. Tài liệu kỹ thuật `docs/M8_*.md`, `docs/M7_*.md` và mã nguồn tại SHA này khi quyết định cách sửa.
8. Log CSV/TXT chỉ khi đã commit công khai có chủ ý hoặc người dùng cung cấp lại; **không mặc định** có log thử xe trong gói.

## Trình tự tiếp quản

- Xác minh repo/nhánh/SHA trên GitHub (không đồng nhất SHA trong hồ sơ tĩnh với HEAD mới).
- Đọc commit diff cùng milestone mới nhất, phân biệt code-staged với build, flash và hardware PASS.
- Chốt vấn đề chưa làm và test cần có; không tự suy dữ liệu ngoài repo.
- Sau kết quả thử hoặc quyết định quan trọng, cập nhật tài liệu và commit; không cần Release cho mỗi push. Khi cần mốc offline, chạy workflow_dispatch với publish_release=true.
"""
    status_file = output / "RELEASE_STATUS.md"
    status_file.write_text(status, encoding="utf-8")
    zip_file = output / f"context-handoff-{sha[:12]}.zip"
    required = {
        "README.md",
        "CONTEXT_HANDOFF.md",
        "docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md",
        "docs/PROJECT_MILESTONES.md",
        "docs/CONTEXT_RELEASE_POLICY.md",
        "docs/CHAT_B_START_PROMPT.md",
        "tools/verify_context_handoff_contract.py",
        "tools/build_context_release.py",
        ".github/workflows/context-handoff-release.yml",
    }
    optional = sorted(
        p for p in tracked
        if (p.startswith("docs/M7_") and p.endswith(".md"))
        or (p.startswith("docs/M8_") and p.endswith(".md"))
        or (p.startswith("tools/verify_m8_") and p.endswith(".py"))
        or (p.startswith("tools/test_m8_s") and p.endswith(".c"))
    )
    missing = required - tracked
    if missing:
        raise SystemExit(f"Missing committed handoff inputs: {sorted(missing)}")
    members = sorted(required | set(optional))
    with zipfile.ZipFile(zip_file, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        archive.writestr("RELEASE_STATUS.md", status)
        for rel in members:
            blob = subprocess.check_output(["git", "show", f"{sha}:{rel}"])
            archive.writestr(rel, blob)
    seen = set(members)
    print(f"COMMIT={sha}")
    print(f"BRANCH={branch}")
    print(f"STATUS={status_file}")
    print(f"ASSET={zip_file}")
    print(f"FILES={len(seen) + 1}")


if __name__ == "__main__":
    main()