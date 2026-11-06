# Xe tăng cướp bóng — STM32F767ZI

Firmware 4 motor bánh + 2 motor đánh lái, PS2, VC-02, encoder/AS5600, công tắc NC/STOP.

## Mốc hiện hành (2026-11-06)

**M9 — Steering Active Position Hold**: [nhánh phát triển M9](https://github.com/Spring-Mustache/control_motor/tree/feature/m9-steering-active-position-hold) đã được tạo từ exact `d367618d`; người dùng chọn **software-only** STM32F767ZI + AS5600 + CCS_SHB12, đã có H0 read-only observer (chưa có bộ điều khiển HOLD ACTIVE). H0 đã có **4/4 log trên MCU, DATA CAPTURE PASS giới hạn tĩnh**. **H1 PD SHADOW** mới được đưa vào source, cần CI exact SHA/CubeIDE/real log; H2/H3 vẫn **PLANNED**. M8 Voice 15°/30°/40° six-case được người dùng xác nhận PASS riêng; M8 RIGHT CENTER R1/Voice trial chưa xác nhận trên motor.

## Mở cuộc hội thoại ChatGPT mới

**[Sao chép lời nhắn khởi đầu tại đây](docs/CHAT_B_START_PROMPT.md)** vào hội thoại mới. Bạn không cần tải lại file nếu AI có quyền đọc repository.

Nguồn chính của dự án là **Git commit + tài liệu được cập nhật tại SHA của đúng nhánh**. GitHub Actions **kiểm tra** mỗi push. Context Release/ZIP **chỉ tạo theo yêu cầu hoặc milestone**, không còn được tạo sau mọi push. **Không có Release cho HEAD là bình thường**, không phải thiếu ngữ cảnh.

## Hướng dẫn AI tiếp quản

1. Xác minh HEAD SHA thực tế nhánh **M9 đang làm** `feature/m9-steering-active-position-hold` (fork chính xác từ `d367618d4d9c4c382a9a7208f9b878dfefa21ace`, không coi base là HEAD vĩnh viễn). Nếu người dùng chuyển nhánh, dùng nhánh họ chỉ định; đừng lấy HEAD `main`, M8 cũ hoặc Release.
2. Đọc `CONTEXT_HANDOFF.md` **mục 1 hiện hành**, rồi **toàn bộ 17 mục (0–16) và cập nhật lịch sử**, `docs/PROJECT_MILESTONES.md` ở đầu và cuối, chính sách, đặc tả gốc nguyên văn cùng [kế hoạch M9](docs/M9_STEERING_ACTIVE_POSITION_HOLD_PLAN_2026_11_06.md) và các tài liệu M7/M8 có liên quan. Tất cả ở cùng SHA.
3. Xem 20 commit gần đây, diff mới nhất và kết quả [GitHub Actions](https://github.com/Spring-Mustache/control_motor/actions/workflows/context-handoff-release.yml) trên đúng SHA. Phân biệt SOURCE-STAGED, HOST PASS, CubeIDE Build, Flash, giá treo và mặt đất có tải.
4. Tìm [Release](https://github.com/Spring-Mustache/control_motor/releases) **chỉ nếu muốn gói ZIP/milestone**. Kiểm tra tag `context-<FULL_SHA>`; nếu vắng, tiếp tục trực tiếp từ repository. Không coi bản của commit/nhánh khác là hiện hành.
5. **M9 H0 đã có dữ liệu tĩnh**; **H1 PD shadow đã có code**, tính toán thụ động không ghi motor. H2 opt-in ACTIVE → H3 tích hợp vẫn chưa triển khai. Không có HOLD ACTIVE hoặc hardware PASS. Bảo toàn M8 Voice 15/30/40 six-case USER PASS tại SHA `9db12228…`; trial M8 RIGHT R1/Voice CENTER nền `d367618d…` chưa có hardware PASS. Sau khi sửa code/ra quyết định mới, cập nhật handoff/roadmap và kiểm tra CI; không mặc nhiên có log riêng từ cuộc trò chuyện khác.

## Tài liệu tham khảo

- [Lời nhắn chuyển tiếp cho B/C/D](docs/CHAT_B_START_PROMPT.md)
- [Đặc tả gốc của người vận hành — nguyên văn 386 dòng](docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md)
- [Hồ sơ 17 mục: lịch sử bảo tồn + trạng thái cập nhật](CONTEXT_HANDOFF.md)
- [Roadmap và mốc đã/chưa kiểm chứng](docs/PROJECT_MILESTONES.md)
- [Milestone M9 — Steering Active Position Hold: thiết kế và acceptance gates](docs/M9_STEERING_ACTIVE_POSITION_HOLD_PLAN_2026_11_06.md)
- [M9-H0 — Passive post-DONE drift telemetry, test và collector](docs/M9_H0_PASSIVE_DRIFT_2026_11_06.md)
- [M9-H1 — PD SHADOW chống nhiễu và mô phỏng thả tải, không cấp PWM](docs/M9_H1_PD_SHADOW_2026_11_06.md)
- [Chính sách và cách phát hành ZIP tùy chọn](docs/CONTEXT_RELEASE_POLICY.md)
- [Workflow CI kiểm tra và nút tạo Release thủ công](https://github.com/Spring-Mustache/control_motor/actions/workflows/context-handoff-release.yml)

**Lưu ý:** Source trên GitHub, CI PASS, Release ZIP, CubeIDE Clean Build, Flash và nghiệm thu phần cứng là **những trạng thái khác nhau**. FIRST-STOP RIGHT cấp điện khi CCR=0 đã được người dùng xác nhận fix; không coi các cảnh báo lịch sử cũ là lỗi mới. Không xóa Release, tag hoặc dữ liệu lịch sử.
