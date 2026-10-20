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
