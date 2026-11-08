# Xe tăng cướp bóng — STM32F767ZI

> **2026-11-08 — USER clarified FULL H2-B1 (not H1):** delivery is persistent TWO-axis position HOLD, no3s expiry and no single-axis/short-session substitute. Full C controller + atomic HAL owner route + main idle/PS2/Voice/NC handoff are now SOURCE-STAGED with local strict GCC tests120s(controller)/60s(main hooks) PASS. Physical commissioning remains CLOSED0; PWM caps/gains/reversal timings are unverified, so no powered HOLD is enabled or claimed. GitHub CI on the new source and CubeIDE/MCU validation remain separate. See docs/M9_H2B1_FULL_HOLD_2026_11_08.md.

> **Motor-lead current baseline (USER,2026-11-08):** khi CENTER trái0,8–1A/phải≈2A; sau dừng đồng hồ đọc0A. Mức5/10A từ công suất chỉ là ước lượng, không phải giới hạn HOLD. [B1 bench design và giới hạn dữ liệu](docs/M9_H2B_POWER_GATE_2026_11_08.md). H2-B1 chưa cấp lực; firmware819f635.

> **Trạng thái mới 2026-11-08:** H2-B0 MCU **STATIC INTEGRATION/TELEMETRY PASS một lượt Voice CENTER**, 3003ms tại firmware819f635; đầu ra HOLD0, candidate0. H2-B1 cấp lực/H3 tích hợp chưa hoàn thành. [Kết quả và giới hạn](docs/M9_H2B_POWER_GATE_2026_11_08.md). **USER clarification (2026-11-08):** Động học chưa hoàn thành để tích hợp/nạp vào firmware; SELECT tạm hoãn. Không tự đồng nhất hai trạng thái hoặc coi nội suy RAW của Voice là động học bánh xe hoàn chỉnh.

Firmware 4 motor bánh + 2 motor đánh lái, PS2, VC-02, encoder/AS5600, công tắc NC/STOP.

## Mốc hiện hành (2026-11-06)

**M9 — Steering Active Position Hold**: [nhánh phát triển M9](https://github.com/Spring-Mustache/control_motor/tree/feature/m9-steering-active-position-hold) đã được tạo từ exact `d367618d`; người dùng chọn **software-only** STM32F767ZI + AS5600 + CCS_SHB12, đã có H0 read-only observer (chưa có bộ điều khiển HOLD ACTIVE). H0 đã có **4/4 log trên MCU, DATA CAPTURE PASS giới hạn tĩnh**. **H1 PD SHADOW SOURCE + HOST CI PASS**, đã thu **4/4 cặp UART trên MCU** với SHA `00ccaeace` và xác nhận bộ lọc ổn định khi đứng yên, **chưa thử phản ứng ngoại lực**; H2/H3 vẫn **PLANNED**. M8 Voice 15°/30°/40° six-case được người dùng xác nhận PASS riêng; M8 RIGHT CENTER R1/Voice trial chưa xác nhận trên motor.

**M9-H1 REAL data recorder (source 2026-11-07):** Đã thêm bộ chụp chuyển động thật của M8 khi chạy lệnh vị trí, lưu RAW + measured CCR + PD SHADOW trong RAM rồi xuất UART chỉ khi idle, không mô phỏng/chỉnh Live Expressions/đổi motor control. Thu 4 lượt với `tools/capture_m9_h1_real.ps1`. **SOURCE + HOST CI PASS và đã có 4/4 lượt REAL trên MCU** tại `3dbdfaf`, đủ130 frame AS5600/CCR/u_shadow; chưa thử ngoại lực hoặc giữ góc powered H2; không phải H2 HOLD ACTIVE. [Hướng dẫn một lượt REAL](docs/M9_H1_REAL_DRIVE_CAPTURE_2026_11_07.md).

**M9-H2-A GUARD (SOURCE/HOST CI + one-run ON-MCU 3s PASS at `5dd1502`, 2026-11-08):** Đã có log MCU 13ms `reason3/blockBits256`, source xác định VOICE->IDLE sau DONE bị guard cũ coi mất quyền, đã sửa narrow normal handoff; new 3s MCU log/CI SHA mới cần xác minh. **M9-H2-A trước đó (SOURCE + HOST CI PASS `55db22f`, 2026-11-07):** Đã thêm supervisor OFF/MONITOR/SHADOW/FAULT chỉ sau M8 DONE hợp lệ, kiểm tra STOP/NC/owner/sensor/wheel, near-goal PD score; **duty=0 và compile hard lock M9H2_POWER_PERMITTED=0U**, không HOLD cấp lực. H2-B powered chưa có vì chưa đo dòng/nhiệt/PWM0 driver. [M9-H2-A hướng dẫn](docs/M9_H2_GUARD_SOURCE_2026_11_07.md).

**M9-H2-B0 SOURCE + HOST CI PASS (2026-11-08, 9e4f4533):** bounded candidate HOLD policy after H2-A guard, hyst 8/4 RAW, veto score ngược chiều mục tiêu; power output vẫn hard lock 0, không phải active holding. H2-B1 sẽ cần dữ liệu dòng/nhiệt motor và PWM0 driver. See docs/M9_H2B_POWER_GATE_2026_11_08.md.

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
- [M9-H1 — PD SHADOW chống nhiễu, không cấp PWM](docs/M9_H1_PD_SHADOW_2026_11_06.md)
- [M9-H1 REAL — Một lần thu vận động xe thật, RAW/CCR/u_shadow](docs/M9_H1_REAL_DRIVE_CAPTURE_2026_11_07.md)
- [Chính sách và cách phát hành ZIP tùy chọn](docs/CONTEXT_RELEASE_POLICY.md)
- [Workflow CI kiểm tra và nút tạo Release thủ công](https://github.com/Spring-Mustache/control_motor/actions/workflows/context-handoff-release.yml)

**Lưu ý:** Source trên GitHub, CI PASS, Release ZIP, CubeIDE Clean Build, Flash và nghiệm thu phần cứng là **những trạng thái khác nhau**. FIRST-STOP RIGHT cấp điện khi CCR=0 đã được người dùng xác nhận fix; không coi các cảnh báo lịch sử cũ là lỗi mới. Không xóa Release, tag hoặc dữ liệu lịch sử.
