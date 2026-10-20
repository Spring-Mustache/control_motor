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
