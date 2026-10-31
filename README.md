# Xe tăng cướp bóng — STM32F767ZI

Firmware 4 motor bánh + 2 motor đánh lái, PS2, VC-02, encoder/AS5600, công tắc NC/STOP.

## Mở cuộc hội thoại ChatGPT mới

**[Sao chép lời nhắn khởi đầu tại đây](docs/CHAT_B_START_PROMPT.md)** vào hội thoại mới. Bạn không cần tải lại file nếu AI có quyền đọc repository.

Nguồn chính của dự án là **Git commit + tài liệu được cập nhật tại SHA của đúng nhánh**. GitHub Actions **kiểm tra** mỗi push. Context Release/ZIP **chỉ tạo theo yêu cầu hoặc milestone**, không còn được tạo sau mọi push. **Không có Release cho HEAD là bình thường**, không phải thiếu ngữ cảnh.

## Hướng dẫn AI tiếp quản

1. Xác minh HEAD SHA thực tế của nhánh M8-S15 đang làm `feature/m8-s15-dual-target-common-time` (hoặc nhánh người dùng chỉ định mới); nhánh M7-S15 cũ là mốc trước PI. Đừng lấy HEAD của `main`, SHA cũ hay Release mới nhất.
2. Đọc `CONTEXT_HANDOFF.md` **mục 1 hiện hành**, rồi **toàn bộ 17 mục (0–16) và cập nhật lịch sử**, `docs/PROJECT_MILESTONES.md` ở đầu và cuối, chính sách, đặc tả gốc nguyên văn cùng các tài liệu M7/M8 có liên quan. Tất cả ở cùng SHA.
3. Xem 20 commit gần đây, diff mới nhất và kết quả [GitHub Actions](https://github.com/Spring-Mustache/control_motor/actions/workflows/context-handoff-release.yml) trên đúng SHA. Phân biệt SOURCE-STAGED, HOST PASS, CubeIDE Build, Flash, giá treo và mặt đất có tải.
4. Tìm [Release](https://github.com/Spring-Mustache/control_motor/releases) **chỉ nếu muốn gói ZIP/milestone**. Kiểm tra tag `context-<FULL_SHA>`; nếu vắng, tiếp tục trực tiếp từ repository. Không coi bản của commit/nhánh khác là hiện hành.
5. Chốt việc còn dang dở, những gì đã kiểm chứng và bước tiếp tục; M8-S15 hiện chưa đạt hai đích SOFT cùng T theo log 19:34–19:35 và chưa có chứng cứ SHA flash/điện-cơ. Sau khi sửa code/ra quyết định mới, cập nhật đủ handoff/roadmap và kiểm tra CI. Không mặc nhiên có file/log riêng từ hội thoại khác.

## Tài liệu tham khảo

- [Lời nhắn chuyển tiếp cho B/C/D](docs/CHAT_B_START_PROMPT.md)
- [Đặc tả gốc của người vận hành — nguyên văn 386 dòng](docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md)
- [Hồ sơ 17 mục: lịch sử bảo tồn + trạng thái cập nhật](CONTEXT_HANDOFF.md)
- [Roadmap và mốc đã/chưa kiểm chứng](docs/PROJECT_MILESTONES.md)
- [Chính sách và cách phát hành ZIP tùy chọn](docs/CONTEXT_RELEASE_POLICY.md)
- [Workflow CI kiểm tra và nút tạo Release thủ công](https://github.com/Spring-Mustache/control_motor/actions/workflows/context-handoff-release.yml)

**Lưu ý:** Source trên GitHub, CI PASS, Release ZIP, CubeIDE Clean Build, Flash và nghiệm thu phần cứng là **những trạng thái khác nhau**. FIRST-STOP RIGHT cấp điện khi CCR=0 đã được người dùng xác nhận fix; không coi các cảnh báo lịch sử cũ là lỗi mới. Không xóa Release, tag hoặc dữ liệu lịch sử.
