# M8-S14.9 — RIGHT terminal-lag forecast (opt-in) + RAW-bin diagnostics

**2026-10-31 — SOURCE-STAGED.** Dựa trên báo cáo riêng tư `M8_S14_8_PI_ON_deep_diagnosis_20261031.md` của phiên Level2/Hybrid **ON** 17:22. Không đưa CSV/M8S00 riêng tư lên repository public; chỉ dùng thống kê tổng hợp.

## Bằng chứng, giả thuyết

- Bốn lượt D-pad LEFT đầy đủ trong log ON: RIGHT trễ LEFT tại mốc 180 RAW trung bình **18,4ms**, tại SOFT **355,4ms**; chênh thêm **337,1ms** sau khi coordinator ngừng boost. RIGHT đi từ 180RAW tới SOFT **782,8ms**, LEFT **445,7ms**.
- M8-S14.8 giữ cả hai gần nhau tại 180RAW; tăng gain PI không đổi đoạn <=180RAW. M8-S14.9 thử **dẫn trước RIGHT ở vùng xa** bằng dự đoán thời gian còn tới 180RAW, không chứng minh sẽ cải thiện SOFT.
- Host-only replay xấp xỉ tốc độ từ CSV trên 4 lượt x 19 mẫu >180RAW: forecast median khoảng 332–338ms; khoảng **84% mẫu** yêu cầu tăng RIGHT dương, median **23–24CCR**, so với chỉ khoảng **8% mẫu UART** cũ RIGHT>175CCR. Đây chỉ là **counterfactual command request**, không phải mô phỏng vật lý hoặc chứng cứ an toàn.

## Firmware chính xác

- Level2 boot `steer_s142_trial_level=2`, owner LEFT405/RIGHT175, cap LEFT450/RIGHT200; Level1 cap 375/150. `steer_s147_velocity_pi_enable=0` và `steer_s149_early_lead_enable=0` đều **boot-OFF**. Chỉ khi operator chủ động bật cả hai trước **lệnh mới sau neutral** mới latch phiên M8-S14.9. `steer_s149_last` ghi dấu latch.
- D-pad LEFT hướng R_MIN, Level1/Level2, S14.5 OFF, hai owner MOVING, sensor hợp lệ và cả hai cách SOFT >180RAW: `M8S149_Predict` dùng tốc độ góc AS5600 từng trục, biến đổi với chiều dài hành trình RAW/góc để ước lượng thời gian tới 180RAW rồi cộng **337ms** terminal skew từ log. Khi prediction >50ms, yêu cầu boost `(prediction-50)/12` CCR, tối đa **25 CCR**. Tốc độ ước lượng được kẹp 250–2000 RAW/s để tránh mẫu cực đoan; sample không hợp lệ không tạo yêu cầu.
- `M8S149_ChooseRight` lấy MAX giữa hybrid PI/P+D cũ và ứng viên `ownerR+leadRequest`, **không cộng**, áp dụng lại cap RIGHT hiện hành và tối đa +12CCR qua previous final. Không giảm LEFT hoặc cấp motor PWM=0; NC/STOP/EXTI/late atomic, SOFT và owner taper 100RAW giữ nguyên.
- Toàn bộ 180RAW cuối do owner điều khiển, không tăng PWM ở cả hai trục khi một bên vào vùng này. Không đổi cap 450/200, không đổi R1/R2/RX/D-pad RIGHT, wheel PID hoặc việc disarm lúc boot.
- `M8S00_S149`: enable,last,frames,requests,wins,predMs,requestR. Bốn dòng `M8S00_S149_BIN` cho RIGHT: >320, (180,320], (100,180], <=100RAW; frames/active và **LAST observed values** targetR,measR,errR,ownerR,capR,piR,pdR,leadR,appliedR. pi/pd/lead là ứng viên, appliedR là PWM sau atomic. Nếu active=0, targetR=0 nghĩa là không update, không phải mục tiêu tốc độ 0.

## Trạng thái và kiểm chứng

- C host test exact production helper `tools/test_m8_s149_terminal_lead.c`, verifier `tools/verify_m8_s149_terminal_lead.py`, CI đồng thời chạy tất cả hồi quy M8-S14.2/3/5/7/8 và bảo toàn hồ sơ 17 mục + nguyên văn 386 dòng. Chỉ xác nhận HOST PASS sau khi xem GitHub Actions tại exact SHA.
- **CHƯA** CubeIDE Clean Build, Flash, đo dòng/current, coast, NC clearance, independent motor power cut hoặc HARDWARE PASS. Lead sớm có thể làm tăng năng lượng/coast dù vẫn dưới 200CCR, nên cần gate điện-cơ trước khi vận hành.
- Người dùng chỉ muốn thử **controller ON**, không lặp OFF. Khi và chỉ khi đảm bảo ngắt nguồn động lực độc lập, NC/STOP, coast/clearance/current và flashed đúng SHA: kiểm arm + STOP, xung ngắn giữa hành trình, rồi D-pad LEFT ON-only trên giá treo; thu M8S11 complete/M8S00, so với mốc ON cũ. Ưu tiên RIGHT/LEFT absolute SOFT arrival, last180/100RAW, actual CCR, NC/trip/current/coast, không chỉ skew.
- Nếu requests cao nhưng wins hoặc appliedR không tăng, xem headroom/slew/safety; không tự nâng limit. Nếu trễ chủ yếu xuất hiện <=180RAW, phải xem xét profile coast riêng khi đã có đo an toàn, không dùng tăng PI gain để xử lý vùng bị khóa.


## 2026-10-31: ON-only 18:05 evidence and passive-bin fix

User's ON-only test showed full UART capture 845/845 rows, 5 MAX→SOFT D-pad LEFT Level2/PI/S149 strokes. Mean RIGHT arrival 1838.3ms, LEFT1541.7ms, skew296.6ms versus S14.8 Level2/PI ON means RIGHT1917.6ms, LEFT1562.2ms, skew355.4ms (4 strokes). Compared within approximate UART interpolation, RIGHT improves79.3ms and skew58.9ms. S149 enable1,last1,frames466,requests359,wins254. No confirmed current/coast/clearance safety; comparisons do not isolate all external variables.

**Diagnostic source bug identified:** four M8S00_S149_BIN lines all report zero because the telemetry-only guard checked both left_direction<0 and right_direction<0. In main.c command sign logic, left_direction=-command and right_direction=command; D-pad LEFT uses +1/-1. Guard corrected to left_direction>0 and right_direction<0; exact routing static regression added. Does not alter motor PWM behavior, gates, cutoff, target, max caps, S149 enable default or latch. Historic BIN=0 lines remain invalid; re-test ON-only after correct Build/Flash and safety gate if finer diagnosis is needed.
