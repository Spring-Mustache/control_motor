# Milestones — Xe tăng cướp bóng (STM32F767ZI)

**Cập nhật:** 2026-10-17  
**Firmware đang phát triển:** `feature/m7-s02-limit-run-10pct`; M7-S01 PASS theo xác nhận thực tế của người dùng; M7-S02 chờ nghiệm thu trên giá treo  
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
| M7-S01 | Nhận/kiểm tra 4 công tắc hành trình lái | **PASS — trên bo, thử bằng tay** | Người dùng xác nhận ngày 2026-10-17: cả 4 công tắc chuyển trạng thái đúng và ổn định; chưa thử cắt PWM motor |
| M7-S02 | Chặn motor lái ở giới hạn, cho phép thoát khỏi giới hạn, motor PWM thấp | **ĐÃ CODE RUN-UNTIL-LIMIT 10%, CHỜ TEST TRÊN BO** | Giai đoạn 1: PWM 100/999, cắt PWM ngay khi LIMIT mở hoặc tự dừng sau 5 s dự phòng; chưa cho phép thoát limit khi chưa xác định chiều DIR thực tế |
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

## M7-S01 — PASS đọc 4 công tắc; M7-S02 là mốc tiếp theo

### Bản đồ tín hiệu hiện có trong source

| Tín hiệu | GPIO | IRQ |
|---|---|---|
| `LIMIT_R_MAX` | PF5 | EXTI9_5 |
| `LIMIT_R_MIN` | PF8 | EXTI9_5 |
| `LIMIT_L_MIN` | PD10 | EXTI15_10 |
| `LIMIT_L_MAX` | PD11 | EXTI15_10 |

Source `Core/Src/main.c` giữ `GPIO_MODE_IT_RISING` + `GPIO_PULLUP`, đúng theo sơ đồ đã chọn **NC -> GPIO, COM -> GND** (tiếp điểm NC đóng = LOW; nhấn công tắc/mất dây NC = HIGH). Nhánh M7-S01 bổ sung callback đếm cạnh RISING và hàm đọc 4 chân tại main loop với chống dội 40 ms. **Người dùng đã xác nhận trên bo cả 4 công tắc đọc đúng và ổn định theo bài test thao tác tay**. Chưa có xác nhận giới hạn đã chặn PWM motor thực tế. Không thay đổi PWM motor lái hay thuật toán bánh.

### Mục tiêu M7-S01 (chưa cấp PWM cho motor đánh lái)

- Đọc trực tiếp từng mức GPIO ở trạng thái công tắc **nhả/ấn** (độc lập cho 4 chân).
- Ghi biến Live Expressions: mức logic, trạng thái active, đếm cạnh/chuyển trạng thái, phát hiện dội nút nếu có.
- Xác minh đúng công tắc ↔ GPIO; xác định cực tính bằng bằng chứng trên bo, không đoán NO/NC.
- Nhận đủ 4 công tắc khi thao tác bằng tay; không làm ảnh hưởng logger hoặc điều khiển bánh.
- **Debug trong Live Expressions:** `debug_limit_r_max`, `debug_limit_r_min`, `debug_limit_l_min`, `debug_limit_l_max` (mở rộng struct để xem `.raw`, `.active`, `.activate_count`, `.release_count`, `.exti_rise_count`); `debug_limit_raw_mask` và `debug_limit_active_mask` (bit0 R_MAX, bit1 R_MIN, bit2 L_MIN, bit3 L_MAX), `debug_limit_same_side_both_active` (bit0 phải, bit1 trái).
- **Test lần đầu không chạy motor:** khi 4 công tắc NC nhả, chờ ≥0,1 s, cả 4 `raw/active=0` và hai mask =0. Bấm giữ từng công tắc ≥0,2 s: chỉ công tắc đó lên `raw=1,active=1`, `activate_count` tăng 1, `exti_rise_count` tăng ít nhất 1 (có thể >1 do dội). Nhả ≥0,2 s: trở về `0`, `release_count` tăng 1. Lặp từng công tắc đúng một lần.
- Nếu chân HIGH liên tục khi chưa nhấn: có thể đang đứt dây/lắp sai tiếp điểm/công tắc đang bị chạm, không tự kết luận module hỏng. Hai biên cùng phía HIGH đồng thời làm `debug_limit_same_side_both_active` khác 0, cần kiểm tra dây trước khi cấp PWM.
- **M7-S01 đã PASS** (nhận 4 công tắc khi nhấn/nhả bằng tay); chưa cấp PWM. M7-S02 tiếp tục thiết lập an toàn về chiều motor và chặn tại giới hạn trước khi chạy cơ cấu.

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

## Biên bản cập nhật sau bài thử M7-S01 (2026-10-17)

- Bài test do người dùng thao tác trực tiếp trên bo với cả 4 công tắc NC–COM nối GND: **cả bốn đều đúng và ổn định**. Mốc **M7-S01 PASS** chỉ trong phạm vi đọc tín hiệu GPIO/Live Expressions.
- Không có bằng chứng mới về hoạt động motor đánh lái hoặc hiệu chuẩn góc; không suy rộng PASS sang M7-S02, M7-S03, M7-S04, hoặc Gate A.
- Tiếp theo M7-S02: xác nhận đường PWM/DIR, kiểm tra ngắt giới hạn, xác định ánh xạ chiều thực tế của motor trái/phải đến MIN/MAX; thử xung PWM thấp, giới hạn thời gian, dừng an toàn, và chỉ thử từng motor trên giá treo khi đủ điều kiện bảo vệ.

## M7-S02 — Bài thử nhận dạng chiều motor đánh lái PWM 10% (CHƯA PASS)

**Phiên bản:** `feature/m7-s02-steering-pulse-10pct`. Đây mới là firmware và hướng dẫn, chưa có log/quan sát chạy motor từ người dùng.

**Đấu nối hiện có:** LEFT PWM PB15/TIM12_CH2, DIR PD14; RIGHT PWM PF9/TIM14_CH1, DIR PF10. Các kênh được start ở duty 0 khi boot; không tự chạy.

**Cố định cho giai đoạn 1:** PWM 100 / (999 + 1) = 10%; xung 200 ms qua phần mềm. Một lệnh cần arm riêng, không chỉnh được duty/time từ Live Expressions. Phải giữ **xe treo chắc và cơ cấu lái cách xa hai đầu hành trình**. Không dừng debugger/breakpoint trong lúc motor đang chạy: nếu CPU halt, bộ hẹn giờ phần mềm có thể không kịp tắt PWM. Chuẩn bị cắt nguồn motor bằng tay khi cần.

### Live Expressions

- `steer_test_arm` (mặc định 0) → đặt 1 để cho phép đúng **một** xung.
- `steer_test_request` → đặt 1 (LEFT, DIR=0), 2 (LEFT, DIR=1), 3 (RIGHT, DIR=0), 4 (RIGHT, DIR=1). **DIR điện 0/1 chưa đồng nghĩa đi về MIN/MAX.**
- `steer_test_running`, `steer_test_pwm_applied`, `steer_test_elapsed_ms`, `steer_test_encoder_delta`, `steer_test_last_command`, `steer_test_trip_mask`, `steer_test_result`, `steer_test_run_count`, `steer_test_rejected_count`, `steer_test_limit_stop_count`.
- `steer_test_result`: 0 chưa chạy, 1 đang chạy, 2 xung kết thúc bình thường, 3 từ chối do chưa arm/không an toàn, 4 LIMIT ngắt, 5 bị hủy, 6 mã lệnh không hợp lệ.

### Điều kiện và tiêu chí giai đoạn 1

- **Trước thử:** Bốn limit NC đóng (`debug_limit_raw_mask=0` và `debug_limit_active_mask=0`), cơ cấu cách xa cả hai đầu giới hạn, bánh xe và giọng nói/PS2 đứng yên, không đang ghi/xuất CSV; `debug_as5600_poll_enable=0`; kiểm tra nguồn và dây driver.
- Chỉ thử **LEFT DIR=0** đầu tiên: đặt `steer_test_arm=1` rồi `steer_test_request=1`. Để chương trình chạy tự do; không nhấn Pause. Motor không được tự chạy trước lệnh. Sau một xung, xem `steer_test_result` và `steer_test_encoder_delta`; đồng thời quan sát cơ cấu tiến về công tắc nào, hay không chuyển động.
- Sau khi xác nhận kết quả xung 1 mới chuyển DIR=1 hoặc motor phải. Không tự tăng PWM, kéo dài xung, hoặc lặp khi chưa rõ tình trạng.
- Trong mỗi xung: **bất cứ LIMIT nào mở (raw lên HIGH) thì EXTI lập tức đặt PWM hai kênh lái về 0**, khóa xung đó và đặt `steer_test_result=4`. Nếu limit mở sẵn, firmware **từ chối mọi xung**, kể cả xung có thể thoát giới hạn.
- Nếu STOP/PS2/Voice/bánh chạy/đang logger thì xung bị từ chối hoặc hủy. Reset xóa trạng thái xung, **không chủ ý xóa log hoàn tất**.
- **Phần chưa triển khai:** ánh xạ chiều DIR0/1 ↔ MIN/MAX, cho phép thoát khỏi limit một cách có kiểm soát; kiểm thử thời gian đáp ứng motor khi bị kẹt. Chỉ làm khi có bằng chứng vật lý từ xung đầu.


## M7-S02 — Bản sửa 2026-10-20: chạy PWM 10% cho tới LIMIT hoặc timeout

**Nhánh mới:** `feature/m7-s02-limit-run-10pct` (độc lập với nhánh xung 200 ms). Đã code, **chưa xác minh build trên máy người dùng hoặc thử motor trên phần cứng**.

Quan sát Live Expressions bản cũ của người dùng: `steer_test_last_command=1` (LEFT DIR=0); `steer_test_result=2` (đã hết xung 200 ms), `steer_test_encoder_delta=0`, `steer_test_running=0` và `steer_test_pwm_applied=0`. Không thể kết luận chắc motor đã quay từ log này.

**Mặc định an toàn:** không tự chạy khi nạp/khởi động/reset; cần `steer_test_arm=1` rồi `steer_test_request=1`. Giữ duty 100/1000=10%, chỉ chạy motor LEFT/DIR0 cho lần đầu và xem hướng cơ khí. Có thể dùng request 2/3/4 ở các bước riêng sau khi đánh giá kết quả lần đầu; không phải PS2.

- Dừng sớm nếu BẤT CỨ limit NC nào mở: ISR EXTI RISING thực thi `Steering_TestFinish(4)`, làm cả hai PWM lái về zero; `steer_test_trip_mask` nêu chân tác động (1 R_MAX, 2 R_MIN, 4 L_MIN, 8 L_MAX).
- Thời gian tối đa **5.000 ms**: `steer_test_result=2` = **timeout (chưa chạm công tắc)**; khác `4` = **đã phát hiện limit**.
- `steer_test_abort=1` yêu cầu hủy bằng main loop (`result=5`). STOP/PS2/Voice/xe chạy có thể hủy qua kiểm tra trạng thái; trước khi test phải để xe ở IDLE, không ghi/xuất logger, tất cả limit LOW.
- Dữ liệu encoder `steer_test_encoder_delta` được cập nhật trong khi đang chạy và khi kết thúc, giúp biết encoder có ghi nhận count hay không; =0 KHÔNG khẳng định chắc chắn motor không quay.
- **PWM về 0 không tương đương ngắt nguồn cấp driver.** Muốn ngắt nguồn vật lý phải xác nhận chân EN hoặc mạch ngắt nguồn/driver cụ thể; không tự kết nối trực tiếp GPIO STM32 với đường điện motor.
- Không dùng Pause/breakpoint trong lúc chạy; CPU halt có thể khiến bộ đếm 5 giây bằng HAL_GetTick ngừng cập nhật trong khi PWM vẫn tồn tại. Cần có phương án ngắt nguồn motor vật lý, quan sát cơ khí, và đảm bảo công tắc được tác động **trước chặn cơ khí**.
- Thử an toàn theo hai bước: **(1)** nếu thuận tiện ngắt riêng nguồn cấp động lực driver, giữ STM32 chạy; kích hoạt test và thao tác công tắc L_MIN/L_MAX bằng tay để xem `result=4` và `pwm_applied=0`. **(2)** Chỉ sau khi xác nhận đường dừng và khoảng cách cơ khí mới cho motor chạy thật; mỗi lần phải arm lại, dừng nếu âm thanh/độ nóng/chuyển động bất thường.
- Đường ngắt qua firmware/EXTI không thay thế cầu dao bảo vệ quá dòng, phần ngắt cứng hoặc E-stop phần cứng cho xe vận hành thực tế.


## M7-S02 — Xác nhận thực tế motor phải (2026-10-20)

- Người dùng quan sát motor đánh lái RIGHT chuyển động **về phía R_MIN** sau lệnh `steer_test_request=3` (RIGHT, DIR=0 ở PF10), PWM 10%.
- Live Expressions sau dừng: `steer_test_last_command=3`, `steer_test_result=4` (limit stop), `debug_limit_raw_mask=2`, `debug_limit_active_mask=2` (R_MIN/PF8), `steer_test_encoder_delta=-1224`, `steer_test_pwm_applied=0`, `steer_test_running=0`. Kết hợp quan sát thực tế, xác nhận **RIGHT DIR=0 → R_MIN**, encoder phải có đếm và phần mềm đã dừng PWM theo tín hiệu giới hạn.
- Chưa xác nhận **RIGHT DIR=1 → R_MAX**, chưa đo độc lập tốc độ cắt PWM/nguồn cấp của driver và chưa nghiệm thu cơ cấu trái. Không đánh dấu M7-S02 PASS toàn bộ.
- RIGHT đang ở R_MIN (mask=2): firmware giai đoạn này **không cho chạy bất kỳ motor nào nếu có LIMIT active**, nên không yêu cầu thử đảo chiều bằng debug khi limit chưa nhả; phải có quy trình thoát giới hạn an toàn sau khi xác nhận hướng vật lý.
- Motor LEFT DIR=0 trước đó có lệnh PWM nhưng encoder delta=0 và motor không thấy quay. Driver LEFT CCS_SHB12 vẫn cần kiểm tra chế độ PWM/DIR (RUN nháy 2), trạng thái ERR/iOVER, dây PB15→IN1, PD14→IN2, GND chung và nguồn motor; không kết luận driver hỏng khi chưa kiểm tra.


## M7-S02 — Chẩn đoán mã hủy 5 (2026-10-20)

- Sau một lần thử LEFT DIR=0, người dùng quan sát `steer_test_result=5` và `steer_test_encoder_delta=0` với tất cả các cờ `control_mode=IDLE`, `ps2_manual_active=0`, `voice_execution_active=0`, `wheel_ff_test_active=0`, `wheel_log_active=0`, `steer_test_abort=0`, và limit mask=0 **tại thời điểm chụp ảnh**. Các giá trị hiện tại không chứng minh nguyên nhân hủy đã biến mất trước đó.
- Bổ sung hai biến Live Expressions trong `Core/Src/main.c`, lưu tại thời điểm hủy:
  - `steer_test_abort_cause`: 0 = chưa từng bị hủy trong lượt hiện tại; 1 = có request thứ hai trong khi đang chạy; 2 = cờ `steer_test_abort` được đặt; 3 = mất điều kiện an toàn khi đang chạy.
  - `steer_test_abort_reason_mask`: nếu cause=3, mỗi bit là điều kiện đang vi phạm: b0 control_mode!=IDLE, b1 stop latch, b2 PS2 manual, b3 PS2 cancel-hold, b4 voice executing, b5 voice ready, b6 wheel target/current PWM, b7 wheel PWM outputs, b8 wheel_pid_target_rpm, b9 wheel FF test, b10 wheel logger active, b11 CSV dump/request, b12 AS5600 polling enabled, b13 bất kỳ wheel RPM>2. Mask có thể chứa nhiều bit. Ví dụ 4 nghĩa b2, 256 nghĩa b8.
- Hai biến giữ nguyên kết quả sau dừng và được xóa khi **một lệnh mới thực sự được chấp nhận**. Các thay đổi chỉ ghi thêm debug; **không** sửa PWM 100/1000, thời hạn 5 s, hướng motor hay điều kiện bảo vệ giới hạn.
- Chưa build/đo trên phần cứng phiên bản bổ sung debug mới. Khi thử lại phải giữ cơ cấu cách xa chặn, có công tắc giới hạn tốt và dự phòng nguồn motor; ưu tiên chưa chạy motor khi chưa xác định an toàn trên bo.


## M7-S02 — Phát hiện đấu nối/hàn dây phần cứng (2026-10-20)

- Người dùng báo cáo trong lần chẩn đoán đã từng dùng ngõ ra cầu H trái để cấp cho bánh LU và thấy bánh quay khi tháo encoder LU. Đây là **quan sát của người dùng**, chưa phải phép đo xác nhận nguyên nhân firmware, cũng không chứng minh vòng điều khiển LU hoạt động khi encoder bị tháo.
- Người dùng phát hiện **mối hàn dây vào motor đánh lái trái tiếp xúc không tốt**; hiện được xem là **nguyên nhân nghi ngờ chính cho hiện tượng đánh lái trái không chuyển động**. Chưa xác nhận sau khi hàn lại bằng bài thử thực tế.
- Quyết định: ưu tiên sửa và kiểm tra thông mạch mối hàn khi đã ngắt nguồn, phục hồi encoder LU, bảo đảm mỗi đầu ra cầu H nối đúng motor được chỉ định; chưa sửa firmware hoặc tăng PWM thử.
- Sau khi sửa, thử đánh lái trái với PWM 10%, DIR=0, vị trí giữa hành trình, giới hạn NC hoạt động, có đường ngắt nguồn công suất thủ công; nghiệm thu theo chuyển động/encoder/limit. M7-S02 chưa PASS toàn bộ.


## M7-S02 — Bài thử tăng PWM ngắn cho motor đánh lái trái (2026-10-20)

**Nhánh chẩn đoán tách biệt:** `feature/m7-s02-left-startup-20pct`. Không chỉnh nhánh 10% trước đó và không thay đổi firmware bánh xe.

**Dữ kiện mới do người dùng quan sát:** kết nối motor đánh lái trái với CCS_SHB12, chạy lệnh `steer_test_arm=1` rồi `steer_test_request=1` ở PWM 10%, nghe tiếng rít/chói tai, không thấy quay; sau 5 giây tiếng kết thúc. Có thể do motor thiếu mô-men khởi động, stall cơ khí, tiếp xúc dây/mối hàn kém, hoặc giới hạn dòng/bảo vệ driver; **chưa phân biệt được nguyên nhân**.

**Cấu hình thử có giới hạn an toàn:**
- `STEER_TEST_PWM_DUTY=200` trên ARR=999: đúng 20% duty.
- `STEER_TEST_MAX_RUN_MS=500`: một lệnh dài tối đa 500 ms. **Không giữ PWM 20% liên tục 5 giây khi chưa xác định tiếng rít.**
- Chỉ cho phép `steer_test_request=1` (LEFT DIR0); `request=2/3/4` bị từ chối với `result=6`. Phải đặt `steer_test_arm=1` cho mỗi lần thử mới; tự reset arm khi chấp nhận yêu cầu. Không tự chạy sau khởi động.
- Bốn công tắc NC đều phải LOW trước khi bắt đầu; RISING EXTI bất kỳ lập tức đặt PWM lái về 0. STOP/PS2/Voice/xe chạy/logger vẫn ngăn hoặc hủy thử.
- `steer_test_result=2` = hết 500ms (không chứng minh motor đã quay); `4` = chạm limit; `5` = hủy; xem `steer_test_abort_cause` và `steer_test_abort_reason_mask` nếu có lỗi.
- Đặt cơ cấu gần giữa hành trình, đảm bảo mối hàn đã xử lý và tay tránh xa cơ khí. Chuẩn bị ngắt nguồn động lực motor độc lập. Không Pause/breakpoint CPU trong lúc PWM đang phát (HAL_GetTick sẽ bị dừng). Nếu tiếng rít lớn, đứng yên, nóng, iOVER hay ERR sáng, ngừng thử và kiểm tra phần cứng/dòng.
- Sau test đọc `steer_test_result`, `steer_test_elapsed_ms`, `steer_test_encoder_delta`; quan sát có chuyển động về L_MIN/L_MAX và trạng thái ERR/iOVER trên CCS_SHB12.
- **Chưa build/kiểm thử phiên bản 20% này trên máy hay bo của người dùng**; chưa được PASS.


## M7-S02 — Thử LEFT DIR0 PWM 20% trong 1 giây (2026-10-20)

- Người dùng đã hàn lại cực motor lái trái và thử phiên bản **PWM 20%, 500 ms** trên giá treo; quan sát motor **có quay nhẹ / nhích một đoạn**, nhưng chưa xác định liệu chuyển động nhỏ do thiếu mô-men hay thời gian ngắn. `steer_test_result=2`, `steer_test_elapsed_ms=500`, `steer_test_encoder_delta=0`, hai limit mask=0.
- Theo yêu cầu người dùng, tăng **chỉ** thời gian tối đa thành `STEER_TEST_MAX_RUN_MS=1000U`, tương đương **1.000 ms (1 giây)**. Giữ `STEER_TEST_PWM_DUTY=200U` (**20%**), **chỉ cho phép LEFT DIR0** (`steer_test_request=1`) và giữ nguyên ngắt limit EXTI, kiểm tra an toàn, cấp quyền `arm` trước mỗi lệnh.
- Đây là thử nghiệm **chưa chạy trên bo**. `steer_test_result=2` trong bản này là đã đủ 1 giây, không chứng minh motor quay liên tục. Nếu tiếng rít bất thường, motor giật/kẹt hoặc driver báo lỗi, ngừng thử; không tăng PWM/thời gian tiếp khi chưa phân biệt nguyên nhân. CPU dừng ở breakpoint sẽ làm mất đảm bảo giới hạn thời gian phần mềm.
- **M7-S02 chưa PASS:** cần quan sát chuyển động trơn tru, hướng tiến về L_MIN/L_MAX, encoder trái và phản ứng chạm limit.


## M7-S02 — Quan sát motor trái quay về phía LU ở 20%/1s

- Người dùng xác nhận sau khi hàn lại dây, chạy lệnh LEFT DIR0 (`steer_test_arm=1`, `steer_test_request=1`) trên nhánh `feature/m7-s02-left-startup-20pct`, PWM 20% tối đa 1 giây, **motor đánh lái trái đã quay thực tế**.
- Quan sát cơ khí: cơ cấu đang dịch chuyển **về phía LU (Left Up)**; đây là mô tả hướng quan sát, **chưa đồng nhất với tên công tắc L_MIN hoặc L_MAX** vì chưa có xác nhận vị trí tương ứng.
- Chuyển động chưa tới công tắc giới hạn trong thời gian thử; **chưa nghiệm thu khả năng dừng tại L_MIN/L_MAX cho motor trái**. Chưa có giá trị encoder delta và kết quả `steer_test_result` của riêng lượt thử này.
- Chưa xác định chắc nguyên nhân tốc độ chậm/quãng đường ngắn: duty, gia tốc mềm của CCS_SHB12, tải cơ khí hoặc thời gian thử; không tự tăng PWM/thời gian khi chưa làm rõ tình trạng tiếng rít/stall bất thường được báo trước đó.
- Trạng thái M7-S02: RIGHT DIR0→R_MIN đã quan sát dừng; LEFT DIR0 hiện **có chuyển động** nhưng chưa map MIN/MAX, chưa kiểm chứng dừng tại limit. **Chưa PASS toàn bộ**.


## M7-S02 — Thử PWM 30% LEFT 500 ms / khôi phục RIGHT 10% (2026-10-20)

- Theo quan sát thực tế của người dùng, LEFT ở 20%/1 s đã quay được về phía bánh LU nhưng còn **tiếng rít**, chưa tới công tắc hành trình; chưa biết là cơ cấu tải lớn, điều kiện gia tốc của CCS_SHB12, hoặc motor/driver đang stall hay giới hạn dòng.
- **Theo yêu cầu người dùng**, tạo nhánh riêng `feature/m7-s02-left30-right10-diagnostic` để tăng duty LEFT DIR0 **30%**, chỉ trong **500 ms** nhằm hạn chế rủi ro khi tiếng rít vẫn chưa giải thích được. Không suy ra duty cao hơn chắc giải quyết tiếng rít, không tăng thêm nếu vẫn có tiếng rít/driver quá dòng.
- Khôi phục lệnh `steer_test_request=3` RIGHT DIR0 với **10%**, giới hạn phần mềm **5000 ms** như bài thử đã đạt; chú ý RIGHT DIR0 đã được xác nhận chạy về R_MIN. **Không thử RIGHT DIR0 nếu R_MIN còn HIGH hoặc cơ cấu sát R_MIN**.
- Chỉ chấp nhận `steer_test_request=1` (LEFT DIR0) hoặc `3` (RIGHT DIR0), phải dùng `steer_test_arm=1` cho từng lần riêng, chỉ chạy một motor mỗi lúc; lệnh 2/4 bị từ chối (`steer_test_result=6`).
- Tất cả công tắc hành trình NC phải nhả (bốn bit = 0) trước khi chạy; bất kỳ RISING EXTI limit nào cũng cắt PWM cả hai motor và đặt `steer_test_result=4`. STOP/PS2/Voice/xe chạy/logger chặn hoặc hủy lệnh như trước.
- PWM 30% không đồng nghĩa dòng/mô-men đã kiểm soát, và tiếng rít khi motor không quay có thể đi kèm dòng lớn. Không thử đi thử lại liên tục. Cần chuẩn bị ngắt nguồn động lực riêng và không ngừng CPU ở breakpoint khi PWM đang chạy.
- Xem sau lượt LEFT: có chuyển động thật không, tốc độ tăng ra sao, tiếng rít tăng/giảm, `steer_test_result`, `steer_test_elapsed_ms`, `steer_test_encoder_delta`, trạng thái đèn ERR/iOVER của CCS_SHB12.
- **Trạng thái:** Firmware cập nhật trên GitHub; **chưa có kết quả build CubeIDE và thử nghiệm bo cho phiên bản 30%**; M7-S02 vẫn CHƯA PASS toàn bộ.


## M7-S02 — Thử chạm hành trình motor trái 20% tối đa 5 giây (2026-10-20)

- Người dùng báo cáo **LEFT DIR0 ở 30% chạy hơi nhanh**, mong muốn giữ **15% hoặc 20%** nhưng tăng thời gian giống bài RIGHT để quan sát **dừng ở công tắc hành trình**; chọn **20%** vì đã quan sát có chuyển động ở mức này.
- Nhánh thử riêng: `feature/m7-s02-left20-limit-5s`. Đặt `STEER_TEST_LEFT_PWM_DUTY=200U` (20%, ARR 999), `STEER_TEST_LEFT_MAX_RUN_MS=5000U` (5 giây tối đa), `steer_test_request=1` là LEFT DIR0. `steer_test_request=3` vẫn là RIGHT DIR0 10% tối đa 5 giây. Các lệnh 2/4 vẫn bị khóa.
- Nếu LEFT chạm bất kỳ công tắc NC nào trước 5 giây, EXTI dừng PWM và kết quả mong đợi `steer_test_result=4`, `steer_test_trip_mask=4` (L_MIN) hoặc `8` (L_MAX) nếu switch tương ứng; nếu `result=2` nghĩa là **hết 5 giây nhưng chưa có limit trip**. Tiến về phía LU là mô tả hướng quan sát, chưa có bằng chứng nó tương ứng L_MIN hay L_MAX.
- Chỉ thử khi bốn bit `debug_limit_raw_mask` và `debug_limit_active_mask` đều 0; nếu R_MIN phía phải vẫn đang tác động, firmware sẽ không cho LEFT chạy. Bắt đầu khi không sát đầu hành trình và có phương án ngắt nguồn động lực độc lập.
- Cơ cấu còn có tiếng rít ở 20% theo báo cáo trước; không coi thời gian dài hơn là phương án chữa tiếng rít. Nếu rít lớn, kẹt, động cơ nóng hoặc driver ERR/iOVER, **ngắt nguồn và không chờ đủ 5 giây**. Không pause/debug-halt MCU trong khi motor chạy vì phần mềm timeout dựa trên `HAL_GetTick`.
- **Chưa build và chưa xác minh trên bo**; chỉ khi quan sát limit thực sự tác động, PWM=0, `result=4` và trip mask chính xác mới ghi PASS phần LEFT DIR0→limit. M7-S02 chưa PASS toàn bộ.


## M7-S02 — LEFT DIR0 chạm L_MIN và kế hoạch thu AS5600 bốn đầu hành trình (2026-10-21)

- Người dùng xác nhận **build thành công**, thử LEFT DIR0 PWM 20% tối đa 5 giây vận hành với tốc độ phù hợp. Ảnh Live Expressions sau thử: `steer_test_result=4`, `steer_test_elapsed_ms=4404`, `debug_limit_raw_mask=4`, `debug_limit_active_mask=4`, `steer_test_pwm_applied=0`, `steer_test_running=0`, `steer_test_last_command=1`, `steer_test_abort_cause=0`. Bit 2 (mask 4) là L_MIN/PD10: **LEFT DIR0→L_MIN, ngắt PWM khi limit tác động đã xác nhận**. Bằng chứng đã đủ PASS riêng cho chiều LEFT DIR0 và limit L_MIN; chưa có bằng chứng chiều LEFT DIR1→L_MAX và RIGHT DIR1→R_MAX.
- `steer_test_encoder_delta=0` tại cuối lượt chạy dù có quan sát motor di chuyển: **encoder quadrature trái TIM4 chưa được xác minh**, không đồng nghĩa AS5600 trả về 0. Ghi riêng để kiểm tra sau.
- Người dùng muốn đo AS5600 RAW (0..4095) ở L_MIN, L_MAX, R_MIN, R_MAX trước; giá trị CENTER sẽ do người dùng tự căn chỉnh cung cấp sau. Dung sai giữa dự kiến ±3 **chưa xác định đơn vị counts hay độ**; chưa dùng để điều khiển.
- Firmware hiện tại **đã có các biến/calibration capture**: đặt `debug_as5600_poll_enable=1` **khi motor đã dừng**; xem `debug_as5600_{left,right}_raw`, `_comm_ok`, `_magnet_detected`, `_magnet_too_weak`, `_magnet_too_strong`. `debug_as5600_cal_command`: 1=L_MIN, 3=L_MAX, 4=R_MIN, 6=R_MAX; 2/5 là LEFT/RIGHT CENTER (đợi người dùng), 7=xóa RAM capture (không dùng tùy tiện). Giá trị chốt vào `debug_as5600_left_min_raw`, `_left_max_raw`, `_right_min_raw`, `_right_max_raw`, có cờ `_valid`. Chỉ ghi nhận thành công nếu `debug_as5600_cal_last_result=1` và `_valid=1`.
- **An toàn:** bit12 trong `Steering_TestUnsafeMask` chặn/hủy thử motor khi `debug_as5600_poll_enable=1`; phải đặt polling về 0 trước bài chạy sau. Nếu bất kỳ limit đang HIGH, firmware từ chối **mọi** lệnh chạy, kể cả chuyển về đầu kia: **không ép motor thoát limit bằng cách bỏ bảo vệ**. Phải bổ sung quy trình thoát limit có xác nhận chiều hoặc đưa cơ cấu về giữa ở trạng thái ngắt nguồn, rồi mới đo các mốc còn lại.
- Hiện **chưa có số RAW tại bất kỳ đầu hành trình nào** do chưa có ảnh các biến AS5600; không bịa giá trị. Các giá trị capture lưu trong RAM và reset khi MCU khởi động lại, cần chép lại để dùng lâu dài.


## M7-S02 — L_MIN escape→L_MAX bằng LEFT DIR1 có bảo vệ

**Trạng thái:** đã thay đổi code trên nhánh riêng `feature/m7-s02-left-lmin-escape-to-lmax`, **chưa build/nạp hoặc kiểm thử trên phần cứng**.

- Tại L_MIN/PD10, người dùng gửi ảnh: `debug_limit_raw_mask=4`, `debug_limit_active_mask=4`, `steer_test_result=4`, `steer_test_elapsed_ms=4404`, PWM=0. AS5600 trái khi nghỉ: `debug_as5600_left_raw=2314`, `debug_as5600_left_min_raw=2314`, `debug_as5600_left_comm_ok=1`, magnet detected=1, ML=0, MH=0. Chưa nhìn thấy `debug_as5600_left_min_valid`, `debug_as5600_cal_last_result` trên ảnh; chưa xác nhận hai cờ đó.
- Đã biết LEFT DIR0→L_MIN. Để lấy L_MAX mà không vô hiệu hóa công tắc đang tác động, bổ sung yêu cầu `steer_test_request=2` (LEFT DIR1) theo **hai giai đoạn, không tự chuyển giai đoạn**:
  1. Khi **chỉ** L_MIN đang tác động (raw mask và active mask cùng bằng 4; các limit khác không tác động), với polling AS5600 **tắt**, motor trái chạy DIR1 PWM 20% trong **tối đa 1.000 ms**. Nếu L_MIN nhả (GPIO raw về 0), PWM ngắt ngay ở lần cập nhật kế tiếp, `steer_test_result=7`, `steer_test_left_dir1_verified=1` = **đã quan sát L_MIN nhả**, không phải đã tới L_MAX. Nếu không nhả trong 1s, `result=8` và `verified=0`: kiểm tra cơ cấu/chiều, **không yêu cầu chạy lại liên tục**.
  2. Sau khi đã quan sát cơ khí đúng chiều và cả raw/active mask đều về 0 (bao gồm debounce), **nạp lại arm=1 rồi request=2 lần thứ hai**. DIR1 PWM 20% chạy tới công tắc L_MAX (hoặc tối đa 5s); mong `result=4`, `steer_test_trip_mask=8`, `debug_limit_raw_mask=8`, PWM=0. Nếu result=2 là hết 5s chưa có limit.
- Tất cả công tắc có EXTI RISING: bất kỳ công tắc mới tác động trong mọi pha đều ngắt PWM. Giữ chống chạy chồng motor, chống PS2/Voice/STOP, wheel PWM, logger, và chỉ chạy sau explicit arm. Đường ngắt nguồn động lực độc lập vẫn cần vì PWM=0 không phải cắt cấp nguồn driver.
- **Bật AS5600 polling chỉ khi motor đã dừng** (`debug_as5600_poll_enable=1`), để đọc/chụp `debug_as5600_cal_command=3` tại L_MAX (`debug_as5600_left_max_raw`, `_max_valid`). Bật polling khi đang có motor command khiến safety từ chối/hủy test. Tắt polling trước khi chạy. Dữ liệu capture ở RAM, không tự lưu flash.
- RIGHT DIR0 vẫn 10%, tối đa 5 giây; LEFT DIR0 vẫn 20%, tối đa 5 giây; RIGHT DIR1 vẫn **khóa** (`request=4`).
- Đã kiểm tra logic tĩnh và cấu hình guards. **Chưa nghiệm thu chiều LEFT DIR1, giải phóng L_MIN và L_MAX trên bo**. Giá trị CENTER và dung sai ±3 vẫn chờ hiệu chuẩn thực tế.


## M7-S02 — Thu AS5600 RIGHT R_MIN/R_MAX với lệnh thoát limit

- **Người dùng đã gửi ảnh thực tế:** LEFT L_MIN `AS5600_LEFT_RAW = 2314`, `debug_limit_raw_mask=4`, `result=4`; LEFT L_MAX `AS5600_LEFT_RAW = 1429`, `debug_limit_raw_mask=8`, `result=4`, `steer_test_last_command=2`, `steer_test_elapsed_ms=962`, `debug_as5600_left_comm_ok=1`. Hai số này là giá trị quan sát lúc LIMIT tác động, **chưa xác nhận có được lưu đồng thời trong RAM**; sau flash/reset cần chụp lại nếu muốn RAM calibration.
- Người dùng muốn đo tiếp `RIGHT R_MIN` và `RIGHT R_MAX`. Ở ảnh cuối trước khi đo RIGHT, LEFT L_MAX đang active (mask 8), khiến lệnh RIGHT bị firmware cũ từ chối. Vì vậy nhánh `feature/m7-s02-right-limits-capture` đã bổ sung thoát LEFT L_MAX (DIR0) và thoát RIGHT R_MIN (DIR1) dạng hai bước có bảo vệ, không tự tăng tốc và không bỏ qua limit:
  - **L_MAX** mask 8, `steer_test_request=1` LEFT DIR0 20%, tối đa **1000ms**, lập tức dừng khi L_MAX nhả, `result=7` `steer_test_exit_released_mask=8`. Chỉ chấp nhận khi L_MAX là limit DUY NHẤT đang active (raw và debounced cùng =8).
  - Khi **tất cả limit raw/active mask=0**, `steer_test_request=3` RIGHT DIR0 10%, tối đa **5000ms**, dừng tại R_MIN (`result=4`, raw/active=2). Motor phải DIR0→R_MIN đã có bằng chứng trước đó; nếu đang ở R_MIN thì không phát lại lệnh3. Chụp RIGHT_MIN qua AS5600 `debug_as5600_cal_command=4` khi motor dừng/poll enabled.
  - **R_MIN** mask 2, `steer_test_request=4` RIGHT DIR1 10%, tối đa **1000ms** nhằm **xác nhận R_MIN thật sự nhả**. Lệnh chỉ cho phép khi duy nhất R_MIN raw/active=2. Khi nhả → `result=7`, `steer_test_exit_released_mask=2`, `steer_test_right_dir1_verified=1`; nếu không nhả trước 1s → `result=8`, không tự tiếp tục và cần kiểm tra chiều/vướng cơ khí.
  - Sau khi `result=7` và **tất cả limit mask=0**, lặp `arm=1` + `request=4` bằng thao tác mới để RIGHT DIR1 10% **tối đa 5000ms** tiến về R_MAX, kỳ vọng limit mask=1, `result=4`, PWM=0. Chụp RIGHT_MAX qua `debug_as5600_cal_command=6` sau khi dừng.
- Các lệnh phải bắt đầu từ xe IDLE, PS2/voice/bánh xe không hoạt động và **`debug_as5600_poll_enable=0` trước khi chạy motor**; bật polling=1 chỉ khi motor dừng, ghi số rồi tắt trước lệnh tiếp. Mỗi lần đều phải `steer_test_arm=1`; không có chuyển tiếp tự động. **Không thực hiện khi nhiều limit cùng HIGH**, driver ERR/iOVER, motor rít/kẹt; phải có ngắt nguồn động lực độc lập. Không dừng CPU bằng breakpoint trong khi PWM chạy.
- Đọc `debug_as5600_right_raw`, `debug_as5600_right_comm_ok`, `debug_as5600_right_magnet_detected`, `debug_as5600_right_min_raw`/`max_raw`, cờ `_valid` và `debug_as5600_cal_last_result`. Hai giá trị RIGHT hiện **chưa đo**; không tự suy từ LEFT. `debug_as5600_cal_command=5` RIGHT CENTER chờ căn chỉnh, dung sai ±3 chưa rõ đơn vị.
- Đã cập nhật mã và rà soát logic tĩnh, **chưa build/flash hoặc thử trên bo phiên bản mới**. LEFT DIR0→L_MIN và LEFT DIR1→L_MAX có bằng chứng thực tế; RIGHT DIR1→R_MAX chưa PASS.


## M7-S02 — AS5600 bốn mốc hành trình đã đo, RIGHT hai chiều PASS

- **Nguồn:** hai ảnh STM32CubeIDE Live Expressions người dùng gửi 2026-10-21, cộng hai ảnh đo motor trái trước đó. Các giá trị sau là quan sát thực tế; không phải thông số giả định hoặc giá trị mặc định firmware.
- AS5600 RAW tại bốn công tắc: `L_MIN=2314`, `L_MAX=1429`, `R_MIN=2929`, `R_MAX=3694` (0–4095). **L_CENTER và R_CENTER chưa xác định**, người dùng sẽ căn chỉnh và cung cấp riêng; dung sai dự kiến ±3, **chưa chốt đơn vị**.
- RIGHT DIR0, `steer_test_last_command=3`: `debug_limit_raw_mask=2`, `debug_limit_active_mask=2` (R_MIN), `steer_test_result=4`, `steer_test_elapsed_ms=2137`, `steer_test_encoder_delta=-2034`, `steer_test_pwm_applied=0`, `debug_as5600_right_raw=2929`, `debug_as5600_right_min_raw=2929`.
- RIGHT DIR1, `steer_test_last_command=4`: `debug_limit_active_mask=1` (R_MAX), `steer_test_result=4`, `steer_test_elapsed_ms=2237`, `steer_test_encoder_delta=+2010`, `steer_test_pwm_applied=0`, `debug_as5600_right_raw=3694`, `debug_as5600_right_max_raw=3694`. `steer_test_right_dir1_verified=1` tại ảnh tương ứng. Như vậy **RIGHT DIR0→R_MIN và DIR1→R_MAX đã có bằng chứng dừng LIMIT và encoder hai chiều**.
- LEFT DIR0→L_MIN và LEFT DIR1→L_MAX đã có bằng chứng dừng LIMIT; encoder quadrature motor trái từng trả delta=0, còn cần kiểm chứng riêng.
- Các biến capture AS5600 thuộc RAM, ở hai ảnh RIGHT có một đầu lưu RAW còn đầu kia hiển thị 0; **chưa có bằng chứng bốn số cùng lưu trong RAM một lần**. Đã ghi bốn giá trị vào milestone để bảo toàn số liệu giữa các lần flash/reset. `debug_as5600_right_comm_ok` và các cờ `_valid` không hiển thị trong cả hai ảnh, nên đánh giá chính là căn cứ RAW đã quan sát tại đúng công tắc, không suy ra đã xác minh tất cả cờ chất lượng cảm biến.
- **Trạng thái:** kiểm chứng giới hạn hai chiều motor phải PASS; hai chiều motor trái đã quan sát dừng LIMIT; M7-S02 toàn bộ còn chờ các phần kiểm tra cần thiết khác (encoder trái, quy trình thoát limit tin cậy, calibration center) trước khi kết luận nghiệm thu tổng thể.


## M7-S02 — AS5600 CENTER do người dùng căn chỉnh (2026-10-21)

- Người dùng cung cấp trực tiếp hai giá trị **CENTER sau căn chỉnh cơ khí**: **`L_CENTER=1857`**, **`R_CENTER=3398`** (đơn vị RAW AS5600 12-bit, 0–4095). Đây là giá trị **được người dùng báo cáo**, không phải kết quả mình đo trực tiếp trên bo.
- Bảng 6 điểm hiệu chuẩn đã biết:

  | Mốc | LEFT RAW | RIGHT RAW |
  |---|---:|---:|
  | MIN | 2314 | 2929 |
  | CENTER | **1857** | **3398** |
  | MAX | 1429 | 3694 |

- Chênh lệch RAW có dấu tính từ CENTER bằng hiệu ngắn nhất (modulo 4096): `LEFT CENTER→MIN +457`, `LEFT CENTER→MAX -428`, `RIGHT CENTER→MIN -469`, `RIGHT CENTER→MAX +296`. Không suy rằng các mốc CENTER phải là trung bình số học hai LIMIT: người dùng căn chỉnh vị trí cân bằng thực tế.
- User trước đó dự kiến **sai số cân bằng ±3**, nhưng chưa xác định **±3 count RAW AS5600 hay ±3 độ cơ khí**; **chưa hard-code deadband**, chưa điều khiển motor theo các mốc này.
- Những giá trị trên đã ghi tài liệu để không mất khi flash/reset. Calibration capture của firmware hiện tại nằm ở RAM và reset khi khởi động, **không tự nhận 6 số trong tài liệu**. Để điều khiển vị trí tự động cần xác nhận đơn vị, kiểm tra độ lặp/độ rơ hai chiều và thiết kế state machine giới hạn an toàn trước khi triển khai.


## M7 — PS2 mapping bench check (2026-10-21, user-confirmed PASS)

- **Branch:** `feature/m7-ps2-steering-wheel-controls`, code entry `1e47b348`; child of `feature/m7-s02-right-limits-capture`. The PS2 extension in `Core/Src/main.c` records requests only and **does not drive new motor outputs**.
- **Observed and user-confirmed:** after replacing the weak PS2 controller battery, wireless response returned. User then tested the mapped PS2 controls and replied **"ok pass"**. Treat as **PASS for Live Expressions input mapping on the board**; this is a user-reported result, not an independent log of every button.
- **Mapping covered by code:** R1 request CENTER, R2 request MIN; L1 left wheels forward/right backward, L2 reverse; D-pad left/right -> both steering motors towards MIN/MAX; **right joystick RX** -> proportional steering request, left joystick LY remains forward/reverse. D-pad requests: LEFT compare 200/100 (20%/10%); RX full-scale request 400/200 (40%/20%) in the read-only telemetry. Full-scale PWM **not approved by loaded physical test**.
- **Not yet PASS:** R1/R2 closed-loop AS5600 positioning, L1/L2 split wheel PI speed targets, D-pad/RX motor actuation, interaction/priority testing, STOP and RF-loss handling under motor load. No build/flash log attached to this milestone record.
- **Next work:** integrate motor paths **without removing old functionality**, preserve STOP > PS2 > Voice > Idle and M6 PI+FF/S-curve. Before steering actuation, validate sensor polling and limit escape safeguards (especially R_MAX) and test new PWM progressively on a supported, suspended chassis. Treat GPIO limit cutoff and PWM=0 as distinct from hard motor supply cutoff.
- **Calibration policy (latest user-approved):** L_CENTER=1857 RAW, R_CENTER=3398 RAW, target balance tolerance **±5 RAW counts**, superseding the older ±3 (undefined unit) note above. MIN/MAX observed values remain L_MIN=2314, L_MAX=1429, R_MIN=2929, R_MAX=3694. These figures in the documentation are not proof that runtime firmware calibration persists in RAM.


## M7 PS2 — L1/L2 spin motor integration (2026-10-21, CODED / NOT YET BENCH PASS)

- **Implementation:** `Core/Src/main.c` routes L1/L2 through `PS2_ApplyManualControl()` and the existing `STOP > PS2 > Voice > Idle` arbitration. `Core/Inc/wheel_control.h` and `Core/Src/wheel_control.c` add `WheelControl_SetSpinDirection()`. No M6 PID coefficients, FF calibrations, S-curve parameters, original LY/D-pad Up/Down/Voice/CSV functions or pin mappings were removed.
- **Requested operation:** hold L1 -> LU/LD **forward (+)** and RU/RD **reverse (-)**; hold L2 -> opposite. Magnitude uses existing `MOTOR_DPAD_PWM=400` command (~40 RPM target through existing M6 mapping when settled). It is a **target**; motor actual rotation is unverified on this branch. Holding L1 and L2 at once requests zero, not an arbitrary winner. LY/D-pad forward/reverse remain available when spin is inactive; L1/L2 take priority over LY when held.
- **Closed loop:** unchanged fixed-profile speed shaper and 150 ms reverse dead-time set the common signed left target; right RPM target is its negative, and wheel-specific PID, feedforward (including RD reverse offset), dynamic FF and output sign bounds use their respective signed target. Cross-wheel same-direction sync and RU capture-assist are bypassed only while spin is active. All baseline PID gains retained.
- **Safety:** spin entry deferred until old wheel commands are zero and observed RPM settles; spin mode latches through stop ramp to prevent sudden change of right-side sign; radio loss, PS2 leaving Analog, STOP, PID-off, or an encoder availability flag turned off forces emergency wheel PWM cut. After RF loss a neutral Analog PS2 packet is required before rearming. PWM zero is not a power disconnect.
- **Bench debug:** `PS2_Pad.L1/L2`, `PS2_Pad.Connected/Mode`, `wheel_spin_active`, `wheel_spin_direction`, `wheel_spin_right_target_rpm`, `wheel_pid_target_rpm`, `wheel_pwm_lu/ld/ru/rd`, `Motor_LeftUp/LeftDown/RightUp/RightDown.current_rpm`, `wheel_spin_start_count`, `wheel_spin_deferred_count`, `ps2_spin_rf_latched`.
- **Verification state:** GitHub changes were submitted and source inspected for balanced delimiters and references. **No STM32CubeIDE cross-compilation, flash verification, or motor test was executed in this environment; NOT PASS**. Requires board suspended with all four wheels free, independent motor-power cutoff, clean `Debug` build, first L1 hold/release then L2 hold/release, STOP and radio loss tests; observe sign and ramp vs each wheel's measured RPM.
- **Out of scope for this implementation:** R1/R2 steering position control and D-pad/RX steering PWM remain observation-only. R2-to-MIN steering posture combined with L1/L2 must not be tested as an integrated physical maneuver until M7 steering gates are PASS.


## M7-S04 — PS2 R1/R2 AS5600 position controller (2026-10-21; CODED, bench test pending)

- **Source branch:** `feature/m7-s04-ps2-r1-r2-position` branched from existing `feature/m7-ps2-steering-wheel-controls`. Retains M6 wheel PI/FF, fixed S-curve, L1/L2 wheel-spin, PS2 LY/D-pad Up/Down, Voice, logger, M7-S02 diagnostic steering pulses, existing GPIO/timer wiring and STOP arbitration.
- **Requirements:** PS2 `R1` requests both steering units to measured **CENTER**, `R2` requests both to measured **MIN**. New rising press starts a bounded one-shot target move; release need not be held; STOP, loss of PS2 Analog mode, conflict with driving inputs, sensor/limit trouble cancel. Only one R1/R2 command at a time.
- **Hard-coded agreed calibration (AS5600 RAW):** LEFT MIN/CENTER/MAX = `2314/1857/1429`; RIGHT = `2929/3398/3694`. Arrival band `±5 RAW counts` on both. LEFT positive RAW error -> DIR0 (towards MIN), negative -> DIR1. RIGHT negative RAW error -> DIR0 (towards MIN), positive -> DIR1. No encoder-left dependency; absolute sensors are used as the feedback.
- **Staged enable:** `steer_pos_enable=0` on every reboot (**motor actuation DISABLED by default**). Only set `steer_pos_enable=1` in Live Expressions after confirming live sensor reads/magnet status and all four NC limit signals, on a suspended chassis with independent motor-power stop. Revert it to `0` to cancel the active move. Keep `debug_as5600_poll_enable=0` while positioning so I2C debug telemetry does not compete. It is NOT a guarantee of safe automatic positioning until bench tests are complete.
- **Safety:** position only starts while four wheels are stopped, no simultaneous PS2 drive, no diagnostic steering pulse, no STOP/Voice/cancel, sensor buses startup recovered, and no conflicting limits. Each cycle reads AS5600 twice through the existing HAL reader (one read per side), requires healthy magnet, checks known mechanical range and implausible jumps. Bounded PWM: LEFT <= 200/999 (~20%), RIGHT <= 100/999 (~10%), tapered below these near target. Any rising NC limit IRQ immediately cuts both steering PWM. Normal position timeout 5 s; sensor motion stall 500 ms; move-away-from-goal detection. PWM=0 is not physical motor-power isolation.
- **Limit escape behavior:** If R1 starts while ONLY L_MIN and/or R_MIN is/are asserted, a <=1 s DIR1 escape is allowed; motion stops immediately after the originating limit(s) release. A NEW R1 press is required for CENTER. Existing incomplete R_MAX escape stays blocked; no automatic escape from either MAX. Pressing R2 when an endstop is already active is rejected rather than applying force.
- **Debug variables:** `steer_pos_enable`, `steer_pos_active`, `steer_pos_result`, `steer_pos_command`, `steer_pos_left_raw`, `steer_pos_right_raw`, `steer_pos_left_error`, `steer_pos_right_error`, `steer_pos_left_pwm`, `steer_pos_right_pwm`, `steer_pos_limit_trip_mask`, `steer_pos_run_count`, `steer_pos_reject_count`. Result codes: 0 idle; 1 active; 2 both inside tolerance; 3 limit; 4 AS5600 read/quality/range; 5 timeout; 6 disallowed start; 7 escaped MIN only (repress); 8 stall/wrong-way; 9 cancelled.
- **Verification state:** source committed, static delimiter/macro and call-path checks performed; **no arm-none-eabi-gcc/STM32CubeIDE build, flash, or physical position test is available in this session**. Do NOT mark M7-S04 PASS. Need bench calibration quality/repeatability and carefully staged first motion tests before enabling any subsequent steering function.
