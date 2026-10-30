#!/usr/bin/env python3
"""Package a versioned, source-grounded handoff for GitHub Releases.

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
    handoff = root / "CONTEXT_HANDOFF.md"
    if not handoff.is_file():
        raise SystemExit("Missing CONTEXT_HANDOFF.md at repository root: cannot publish an incomplete handoff")
    if not (root / "docs" / "PROJECT_MILESTONES.md").is_file():
        raise SystemExit("Missing docs/PROJECT_MILESTONES.md")

    status = f"""# PROJECT RELEASE HANDOFF — {sha[:12]}

> **Bắt đầu từ đây khi chuyển sang cuộc hội thoại B.** Bản này được tạo tự động từ commit thật. Nội dung chat/kiểm thử ngoài GitHub **không thể tự suy ra**. Xem thêm `CONTEXT_HANDOFF.md` và tài liệu trạng thái mới nhất.

- Repository: https://github.com/{repo}
- Nhánh của lần cập nhật: `{branch}`
- Commit chính xác: `{sha}`
- Mã nguồn tại commit: https://github.com/{repo}/tree/{sha}
- Bản diff: https://github.com/{repo}/commit/{sha}
- Giờ tạo gói (UTC): {stamp}
- Mốc nghiên cứu hiện hành tại lần tạo hồ sơ ban đầu: M8-S14.2, cross-coupled steering (đọc lại roadmap và diff để xác minh mốc đã đổi hay chưa).
- **Trạng thái Build/Flash/hardware của commit này: CHƯA KIỂM CHỨNG trong gói bàn giao**, trừ khi có bằng chứng kiểm thử riêng, ghi rõ trong commit/release. GitHub Actions tạo file này **không** phải build STM32 hoặc thử trên xe.
- `CONTEXT_HANDOFF.md` là hồ sơ lịch sử chụp tại thời điểm ban đầu; **SHA nằm trong hồ sơ có thể cũ**. Ưu tiên SHA và lịch sử commit nêu trong file release này; không tự nhận văn bản tĩnh đã cập nhật diễn biến chat.
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

1. File này: `RELEASE_STATUS.md` (SHA hiện hành và các thay đổi gần nhất).
2. `CONTEXT_HANDOFF.md` (hồ sơ lịch sử A→B, **không tự cập nhật từ chat**).
3. `docs/CHAT_B_START_PROMPT.md` (lời nhắn khởi đầu cho hội thoại mới).
4. `docs/PROJECT_MILESTONES.md` (roadmap tích lũy; ưu tiên mục ghi ngày mới nhất, bảng mở đầu có thể cũ).
5. `docs/CONTEXT_RELEASE_POLICY.md` (nguyên tắc cập nhật trạng thái và phát hành tự động).
6. Tài liệu kỹ thuật `docs/M8_*.md`, `docs/M7_*.md` và mã nguồn tại SHA này khi quyết định cách sửa.
7. Log CSV/TXT chỉ khi đã commit công khai có chủ ý hoặc người dùng cung cấp lại; **không mặc định** có log thử xe trong gói.

## Trình tự tiếp quản

- Xác minh repo/nhánh/SHA trên GitHub (không đồng nhất SHA trong hồ sơ tĩnh với HEAD mới).
- Đọc commit diff cùng milestone mới nhất, phân biệt code-staged với build, flash và hardware PASS.
- Chốt vấn đề chưa làm và test cần có; không tự suy dữ liệu ngoài repo.
- Sau mỗi lần có kết quả thử thực tế, cập nhật tài liệu trạng thái/roadmap và commit để phiên bản release kế tiếp mang bằng chứng mới.
"""
    status_file = output / "RELEASE_STATUS.md"
    status_file.write_text(status, encoding="utf-8")
    zip_file = output / f"context-handoff-{sha[:12]}.zip"
    include = [handoff, root / "docs" / "PROJECT_MILESTONES.md", root / "docs" / "CONTEXT_RELEASE_POLICY.md", root / "docs" / "CHAT_B_START_PROMPT.md"]
    include += sorted((root / "docs").glob("M7_*.md"))
    include += sorted((root / "docs").glob("M8_*.md"))
    include += sorted((root / "tools").glob("verify_m8_*.py"))
    include += sorted((root / "tools").glob("test_m8_s*.c"))
    seen: set[str] = set()
    with zipfile.ZipFile(zip_file, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        archive.writestr("RELEASE_STATUS.md", status)
        for path in include:
            if not path.is_file():
                continue
            rel = path.relative_to(root).as_posix()
            if rel in seen:
                continue
            seen.add(rel)
            archive.write(path, rel)
    print(f"COMMIT={sha}")
    print(f"BRANCH={branch}")
    print(f"STATUS={status_file}")
    print(f"ASSET={zip_file}")
    print(f"FILES={len(seen) + 1}")


if __name__ == "__main__":
    main()