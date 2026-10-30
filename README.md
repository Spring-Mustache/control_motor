# Xe tăng cướp bóng — STM32F767ZI

Firmware điều khiển 4 motor bánh + 2 motor đánh lái, PS2, VC-02, AS5600, NC/STOP.

## Khi cần chuyển sang cuộc hội thoại ChatGPT mới

**Bắt đầu từ [GitHub Releases](https://github.com/Spring-Mustache/control_motor/releases).**

1. Chọn bản Context handoff gắn SHA của **nhánh đang phát triển** (hiện: feature/m7-s15-ps2-controls-restoration-staged).
2. Mở RELEASE_STATUS.md của Release: xác nhận nhánh, SHA, file vừa đổi và lịch sử commit.
3. Tải context-handoff-<short-SHA>.zip và đọc CONTEXT_HANDOFF.md + docs/PROJECT_MILESTONES.md.
4. Gửi link Release cho cuộc hội thoại B. B phải đối chiếu mã nguồn đúng SHA trước khi sửa firmware.
5. Khi có kết quả thử hoặc quyết định mới từ chat: cập nhật docs/PROJECT_MILESTONES.md và commit/push để bản Release kế tiếp phản ánh thông tin đó.

- [Lời nhắn mở đầu cuộc hội thoại B (sao chép để sử dụng)](docs/CHAT_B_START_PROMPT.md)
- [Hồ sơ bàn giao 17 mục](CONTEXT_HANDOFF.md)
- [Roadmap](docs/PROJECT_MILESTONES.md)
- [Quy tắc phát hành và giới hạn](docs/CONTEXT_RELEASE_POLICY.md)
- [Theo dõi GitHub Actions](https://github.com/Spring-Mustache/control_motor/actions/workflows/context-handoff-release.yml)

**Phân biệt rõ:** mỗi bản context-* là prerelease ngữ cảnh của một Git commit, KHÔNG phải firmware được Clean Build/Flash hoặc hardware PASS. Hồ sơ A là bản chụp lịch sử; kết quả ngoài GitHub không thể tự xuất hiện khi chat B thay đổi. Sự cố FIRST-STOP RIGHT đã được người dùng xác nhận fix, không tiếp tục xem là lỗi mở chỉ vì ghi chú cũ.

Workflow tự phát hành cho mọi push vào nhánh mang workflow, nếu GitHub Actions và quyền contents:write được cho phép. Một push nhiều commit tạo một bản tại HEAD sau push.
