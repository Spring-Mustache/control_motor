# Xe tăng cướp bóng — STM32F767ZI

Firmware điều khiển 4 motor bánh + 2 motor đánh lái, PS2, VC-02, AS5600, NC/STOP.

## Dành cho người vận hành — mở hội thoại ChatGPT mới

**Bạn chỉ cần [mở lời nhắn bàn giao dành cho B](docs/CHAT_B_START_PROMPT.md), sao chép toàn bộ và dán làm tin nhắn đầu tiên trong hội thoại mới.**

Không bắt buộc tải lại hay đính kèm `CONTEXT_HANDOFF.md` khi hội thoại B có công cụ và quyền truy cập GitHub. B sẽ tự thực hiện các bước tìm nguồn được mô tả trong lời nhắn; nếu không truy cập được, B phải nói rõ tài nguyên còn thiếu.

Bạn có thể xem trước hồ sơ tại [GitHub Releases](https://github.com/Spring-Mustache/control_motor/releases). Mỗi lần push lên nhánh có workflow tạo một bản phát hành ngữ cảnh ứng với HEAD của lần push đó.

## Dành cho AI tiếp quản — hội thoại B, C, D...

**Nếu đang đọc README theo yêu cầu của lời nhắn bàn giao, đây là bản đồ tài nguyên, KHÔNG phải yêu cầu người dùng mở thêm hội thoại.**

1. Xác minh HEAD SHA thực tế của **nhánh phát triển được chỉ định** (hiện là `feature/m7-s15-ps2-controls-restoration-staged`); không nhầm với `main`.
2. Tìm bản [Context Release](https://github.com/Spring-Mustache/control_motor/releases) có tag `context-<FULL_HEAD_SHA>`. Không lấy Release trên cùng nếu thuộc nhánh khác hoặc SHA cũ.
3. Đọc `RELEASE_STATUS.md` để xác nhận SHA, thay đổi và lịch sử commit.
4. Đọc toàn bộ [CONTEXT_HANDOFF.md](CONTEXT_HANDOFF.md) (mục 0–16), [roadmap](docs/PROJECT_MILESTONES.md), tài liệu M7/M8 liên quan và mã nguồn ở **cùng SHA**. Nếu không thể truy cập ZIP, đọc trực tiếp từ repository; báo rõ phạm vi truy cập.
5. Xác định phần đã thực hiện, đã kiểm chứng, chưa kiểm chứng và đúng điểm dừng; tiếp tục việc đủ dữ liệu, công cụ và quyền. Sau thay đổi, cập nhật roadmap/tài liệu cùng commit và kiểm tra Release mới.

## Tài liệu tham chiếu

- [Lời nhắn đầu tiên để sao chép vào B](docs/CHAT_B_START_PROMPT.md)
- [Hồ sơ bàn giao lịch sử 17 mục](CONTEXT_HANDOFF.md)
- [Roadmap và trạng thái dự án](docs/PROJECT_MILESTONES.md)
- [Quy tắc phát hành và giới hạn](docs/CONTEXT_RELEASE_POLICY.md)
- [Theo dõi GitHub Actions](https://github.com/Spring-Mustache/control_motor/actions/workflows/context-handoff-release.yml)

**Giới hạn quan trọng:** GitHub Release `context-*` là **prerelease ngữ cảnh**, không phải bản firmware đã Clean Build, Flash hoặc nghiệm thu hardware. Hồ sơ A là snapshot lịch sử, không tự biết kết quả mới chỉ gửi trong chat. FIRST-STOP RIGHT cấp điện khi CCR=0 đã được người vận hành xác nhận fix; không coi là lỗi đang mở chỉ vì ghi chú lịch sử cũ.

Workflow tự tạo **một bản trên mỗi lần push** ở SHA cuối cùng của lần push; nếu Actions hoặc quyền tạo Release gặp lỗi, kiểm tra log thay vì coi bản cũ là hiện hành.
