# Milestones — Xe tăng cướp bóng (STM32F767ZI)

**Cập nhật:** 2026-10-17  
**Firmware đang dùng:** `feature/m6-s03-log-retention`  
**Quy tắc nghiệm thu:** Chỉ đánh dấu PASS trong điều kiện đã kiểm chứng. PASS trên giá treo **không** có nghĩa đã PASS trên mặt đất có tải.

## Quyết định triển khai (2026-10-17)

- Người dùng xác nhận **cả 4 bánh treo lên** trong các bài điều khiển tốc độ và thử logger vừa qua.
- **Tạm PASS phần chạy bánh trên giá treo**, giữ nguyên bộ điều khiển hiện tại trừ khi tích hợp phát sinh lỗi có bằng chứng.
- Chuyển sang **đánh lái**, bắt đầu bằng **đọc chính xác tín hiệu 4 công tắc hành trình** khi chưa cho motor đánh lái chạy.
- Hai hệ thống chạy bánh và đánh lái phải **cùng hoàn thành trên giá treo**, kiểm tra tích hợp, **sau đó mới cùng nghiệm thu trên mặt đất có tải**.
- Nút RESET khởi động lại chương trình nhưng không chủ ý xóa log đã hoàn thành. **Chỉ giữ USER đủ 3 giây**, trong điều kiện an toàn được firmware kiểm tra, mới xóa log đã hoàn thành.

## Trạng thái các milestone

| Mốc | Phạm vi | Tình trạng | Bằng chứng/giới hạn |
|---|---|---|---|
| M0 | NUCLEO-F767ZI, nguồn E5V, cấu hình peripheral, motor/encoder, kiến trúc firmware | Nền tảng đã triển khai | JP3 1–2 (E5V), JP1 tháo; cần giữ E5V liên tục khi lấy log |
| M1 | VC-02 UART/parser sự kiện giọng nói | Có trong firmware | Chưa tái nghiệm thu toàn hệ thống ở đợt M6 |
| M2 | Voice command builder (movement, turn, angle, time) | Có trong firmware | Chưa nghiệm thu điều khiển đánh lái thực |
| M3 | Arbitration STOP > PS2 > Voice > Idle | Có trong firmware | Chưa nghiệm thu tích hợp chạy + lái dưới tải |
| M4/M4' | Đường lệnh chạy bánh từ PS2/Voice, chạy open-loop/ramp | Đã triển khai | Là nền chuyển sang closed-loop M6 |
| M6-S01 | Baseline điều khiển tốc độ bánh | Lưu baseline | `baseline/m6-s01-speed-v1` tại `8721bde3` |
| M6-S02 | FIXED S-curve 1 s, PI + FF, dấu chiều động cơ và encoder, ±40 RPM | **PASS trên giá treo** | Bài 4 bánh × 4 plateau đạt 16/16 sai số trung bình trong ±0,5 RPM; log tiến/lùi tiếp theo 8/8 đoạn plateau đạt |
| M6-S03a | Giữ RAM log qua RESET / cắm lại USB, CRC, giữ USER 3 s để xóa | **PASS chức năng trên giá treo** | Đã xuất CSV sau reset/rút-cắm USB; bài thử 1.128 mẫu và bài có điều khiển 1.714 mẫu không thấy thiếu dòng/cột |
| M6-S03b | Tốc độ bánh **trên mặt đất có tải** | **CHƯA LÀM** | Không được gọi M6-S03 loaded PASS vì cả 4 bánh vẫn đang treo |
| M7-S01 | Nhận/kiểm tra 4 công tắc hành trình lái | **ĐÃ CODE, CHỜ THỬ TRÊN BO** | Nhánh `feature/m7-s01-limit-switch-inputs`; xem bài test bên dưới |
| M7-S02 | Chặn motor lái ở giới hạn, cho phép thoát khỏi giới hạn, motor PWM thấp | Chưa làm | Cần M7-S01 PASS trước |
| M7-S03 | Phản hồi encoder/AS5600 hai bên, hiệu chuẩn tâm và biên lái | Chưa nghiệm thu | Có mã debug và khởi tạo, chưa có bằng chứng đủ điều kiện đóng vòng |
| M7-S04 | Điều khiển vị trí đánh lái trái/phải, dừng đúng góc, lỗi an toàn | Chưa làm | Test trên giá treo |
| M7-S05 | Tích hợp **chạy bánh + đánh lái** trên giá treo | Chưa làm | Điều kiện kết thúc giai đoạn giá treo |
| M8 | Thử **cả chạy bánh + đánh lái** trên mặt đất, chịu tải thực tế | Chưa làm | Chỉ vào sau khi M7-S05 PASS |

**Chưa đánh dấu nghiệm thu:** Điều khiển bắt bóng/hoạt động toàn xe và chuỗi lệnh giọng nói phối hợp với đánh lái thực.

## Những điều đã xác nhận ở M6

1. Chiều motor + dấu encoder bốn bánh đã chuẩn hóa để RPM dương là tiến, RPM âm là lùi.
2. PI + feedforward bật mặc định; FIXED S-curve 1 giây; đảo chiều +40 tới −40 RPM được thử khi bánh treo.
3. Bài M6-S02 trước đó đạt 16/16 đoạn tốc độ trung bình với sai lệch trong ±0,5 RPM. Bài CSV sau sửa logger (1.714 mẫu, 42,825 s) có chuỗi đứng yên → tiến → lùi → dừng, 8/8 đoạn ổn định được đánh giá đạt trong điều kiện treo bánh.
4. M6-S03 giữ 6.000 mẫu trong RAM `.noinit`, đánh dấu log hoàn tất kèm CRC, khôi phục khi SRAM còn nguồn qua reset. Dữ liệu **không** được bảo đảm sau mất nguồn/brownout/nạp flash khiến RAM bị ghi đè.
5. USER ngắn: start → stop → dump CSV; USER giữ ≥3 giây: xóa khi xe đứng yên, log xong và không dump; RESET: phục hồi log hợp lệ. Đã thử trên board, người dùng xác nhận chức năng đạt.
6. CSV đã sửa lỗi metadata vượt bộ đệm UART (tăng line buffer lên 512 và chặn length vượt). File thử cuối hợp lệ. Metadata cấu hình log đặt 20 ms nhưng thời gian các mẫu thực tế trong CSV khoảng 25 ms; khi phân tích dùng timestamp thực tế.

**Mốc firmware:** `feature/m6-s02-fixed-scurve` ~ `fbaa452e` (không thay đổi); `feature/m6-s03-log-retention` firmware commit `4697e11f`. Không sửa baseline A/B.

## M7-S01 — Bước kế tiếp: kiểm chứng công tắc hành trình

### Bản đồ tín hiệu hiện có trong source

| Tín hiệu | GPIO | IRQ |
|---|---|---|
| `LIMIT_R_MAX` | PF5 | EXTI9_5 |
| `LIMIT_R_MIN` | PF8 | EXTI9_5 |
| `LIMIT_L_MIN` | PD10 | EXTI15_10 |
| `LIMIT_L_MAX` | PD11 | EXTI15_10 |

Source `Core/Src/main.c` giữ `GPIO_MODE_IT_RISING` + `GPIO_PULLUP`, đúng theo sơ đồ đã chọn **NC -> GPIO, COM -> GND** (tiếp điểm NC đóng = LOW; nhấn công tắc/mất dây NC = HIGH). Nhánh M7-S01 bổ sung callback đếm cạnh RISING và hàm đọc 4 chân tại main loop với chống dội 40 ms. **Mạch thực tế chưa được đo xác nhận**, không đánh dấu PASS trước khi xem Live Expressions. Không thay đổi PWM motor lái hay thuật toán bánh.

### Mục tiêu M7-S01 (chưa cấp PWM cho motor đánh lái)

- Đọc trực tiếp từng mức GPIO ở trạng thái công tắc **nhả/ấn** (độc lập cho 4 chân).
- Ghi biến Live Expressions: mức logic, trạng thái active, đếm cạnh/chuyển trạng thái, phát hiện dội nút nếu có.
- Xác minh đúng công tắc ↔ GPIO; xác định cực tính bằng bằng chứng trên bo, không đoán NO/NC.
- Nhận đủ 4 công tắc khi thao tác bằng tay; không làm ảnh hưởng logger hoặc điều khiển bánh.
- **Debug trong Live Expressions:** `debug_limit_r_max`, `debug_limit_r_min`, `debug_limit_l_min`, `debug_limit_l_max` (mở rộng struct để xem `.raw`, `.active`, `.activate_count`, `.release_count`, `.exti_rise_count`); `debug_limit_raw_mask` và `debug_limit_active_mask` (bit0 R_MAX, bit1 R_MIN, bit2 L_MIN, bit3 L_MAX), `debug_limit_same_side_both_active` (bit0 phải, bit1 trái).
- **Test lần đầu không chạy motor:** khi 4 công tắc NC nhả, chờ ≥0,1 s, cả 4 `raw/active=0` và hai mask =0. Bấm giữ từng công tắc ≥0,2 s: chỉ công tắc đó lên `raw=1,active=1`, `activate_count` tăng 1, `exti_rise_count` tăng ít nhất 1 (có thể >1 do dội). Nhả ≥0,2 s: trở về `0`, `release_count` tăng 1. Lặp từng công tắc đúng một lần.
- Nếu chân HIGH liên tục khi chưa nhấn: có thể đang đứt dây/lắp sai tiếp điểm/công tắc đang bị chạm, không tự kết luận module hỏng. Hai biên cùng phía HIGH đồng thời làm `debug_limit_same_side_both_active` khác 0, cần kiểm tra dây trước khi cấp PWM.
- Kết thúc M7-S01 mới cho phép test motor lái PWM thấp và xây dựng chặn quá giới hạn.

### Nền đánh lái đã có trong firmware nhưng chưa nghiệm thu

- Hai motor đánh lái có PWM và DIR; `Turn_Motors_Stop()` được gọi khi emergency stop.
- Encoder bên LEFT: TIM4 (PD12/PD13); bên RIGHT: LPTIM1 (PG12/PE1); có biến debug mức A/B và CNT.
- AS5600 LEFT: I2C2 (PB10/PB11), RIGHT: I2C1 (PB8/PB9), khởi tạo để debug/hiệu chuẩn tay; `debug_as5600_poll_enable` mặc định 0 để tránh trì hoãn vòng PI bánh.
- **Không suy ra** encoder/AS5600/điều khiển góc đã PASS chỉ từ việc đã có mã khởi tạo.

## Cửa nghiệm thu song song

**Gate A — giá treo:** chạy bánh PASS (tạm đạt) + đánh lái PASS (chưa) + thử phối hợp không va giới hạn (chưa) → mới đóng Gate A.

**Gate B — mặt đất có tải:** trên cơ sở Gate A đã đóng, đo lại startup, sai số tốc độ, PWM và đáp ứng góc trong khi chạy + lái; kiểm chứng dừng an toàn, giới hạn hành trình, thay đổi hướng có tải. Gate B hiện **CHƯA LÀM**.

## Nguyên tắc làm việc

- Mỗi test có tiêu chí PASS/FAIL và file log/bằng chứng cụ thể.
- Không sửa lại PI/FF/S-curve đã tạm PASS trên giá treo nếu không có chứng cứ lỗi do tích hợp hoặc dưới tải.
- Không ép motor lái chạy tới chặn cơ khí trước khi xác nhận từng công tắc.
- Không xóa log trước khi xuất và kiểm tra CSV; thao tác xóa chủ động là giữ USER 3 giây.
- Giữ nhánh baseline, tránh tự ghi đè những mốc đã nghiệm thu.
