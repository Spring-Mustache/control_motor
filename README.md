# Xe tăng cướp bóng — STM32F767ZI

**CẬP NHẬT HIỆN HÀNH 2026-11-09 — SỬA HỦY R2 DO RPM BÁNH THỤ ĐỘNG.**
Log MCU `d4894029` xác nhận 4/4 Rotate bị hủy trước spin: R2 chỉ chạy 170–350ms,
`pos=9`, `result=4`, `started=0`, `nc=0`, `trip=0`. Log cũ chưa có mask của điều kiện
hủy. HOST đã tái hiện đúng chuỗi lỗi này khi bánh quay thụ động trong lúc đánh lái.
Sửa gate Voice R2/45: đứng yên trước khi nhận lệnh; khi đã đánh lái chỉ yêu cầu
không có lệnh/PWM kéo bánh. RPM thụ động không tự hủy owner. Spin vẫn chờ RPM
về mức hợp lệ qua wheel API gốc, thời gian xoay chỉ bắt đầu khi API nhận lệnh.
UART thêm `passiveFrames`, `abortSrc`, `abortMask`, `permit`. M9 vẫn tạm dừng;
nhánh hiện hành `feature/m8-r1-voice-center-right-late-fine-10raw`.
Chưa có kiểm chứng trên xe của bản sửa mới. Xem [báo cáo sửa R2](docs/M8_VC02_ROTATE_R2_CONTINUATION_2026_11_09.md).


**HIỆN HÀNH 2026-11-09 — TẠM DỪNG M9, SỬA VOICE ROTATE TRÊN NỀN M8.**
Theo chỉ thị mới của người vận hành, nhánh đang làm là `feature/m8-r1-voice-center-right-late-fine-10raw`,
phát triển trực tiếp từ mốc tiền đề M9 `d367618d4d9c4c382a9a7208f9b878dfefa21ace`.
Xác minh HEAD của nhánh này; các chỉ dẫn M8-S15 cũ bên dưới là lịch sử.
M9 HOLD tạm dừng, được giữ riêng tại `feature/m9-steering-active-position-hold`
(HEAD đã kiểm tra `3df43edde8f53b5e84561704189a83ac8286a56b`);
MCU cuối cùng có log M9 xác nhận là `b89b4d5c45c6173f5d726b9b1d9f7f755af05fe1`,
chưa được coi là đã flash 3df43ed. Không merge M9 vào nhánh sửa M8.
Mục tiêu hiện tại: Rotate LEFT = R2 → L2, RIGHT = R2 → L1, giữ xoay đủ số giây
sau khi bộ điều khiển bánh chấp nhận khởi động; giữ nguyên NC/STOP/PS2/PID.
Chi tiết, giới hạn bằng chứng và cách cập nhật/test: [M8_VC02_ROTATE_STARTUP_2026_11_09.md](docs/M8_VC02_ROTATE_STARTUP_2026_11_09.md).


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
