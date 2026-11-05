# M8-S15.4 — Hợp đồng điều khiển Voice/VC-02 tách 4 bánh và 2 motor lái (04/11/2026)

**QUYẾT ĐỊNH MỚI NHẤT TỪ USER:** Bộ điều khiển hai nhóm phải độc lập. Bảng 49 lệnh VC-02 đã chốt: `docs/VC02_OPERATOR_COMMAND_MAP_2026_11_04.md`. USER đã báo VC-02 COM3 có trạng thái nạp hoàn tất trong UniOne, xác nhận đã nói “rồi ok” sau thử lệnh; **chưa có bản đo UART byte nhận thực tế trên STM32 và chưa nạp firmware STM32 tích hợp**. Không chuyển trạng thái nạp VC-02 thành test 6 motor PASS.

## 1. Định nghĩa chuỗi lệnh và thời điểm thực thi

| Nhóm cơ cấu | Byte UART (HEX) | Cách nói / hành vi | Cách chốt |
|---|---|---|---|
| **Bốn bánh (wheel group)** | `A2 D0 XX` | Go forward + duration | **Bắt buộc có D0 XX** để chốt, không chạy khi chỉ có A2 |
| Bốn bánh | `A3 D0 XX` | Go backward + duration | Bắt buộc TIME |
| Bốn bánh | `A4 D0 XX` | Rotate left + duration | Bắt buộc TIME |
| Bốn bánh | `A7 D0 XX` | Rotate right + duration | Bắt buộc TIME; A7 chưa decode ở STM32 hiện hành |
| **Hai motor lái (steering group)** | `A5 B1..B9` | Turn left + 5°..45° | **Chốt ngay khi có hướng và góc**, không cần TIME |
| Hai motor lái | `A6 B1..B9` | Turn right + 5°..45° | Chốt ngay khi có hướng và góc, không cần TIME |
| Hai motor lái | `B0` | Center / Zero degrees | Lệnh yêu cầu về vị trí CENTER, không cần TIME. `B0` đã có 2 alias tiếng nói |
| Toàn xe | `FF` | STOP | Hủy pending, ready, executing của **cả wheel và steer** rồi dừng 6 motor qua đường STOP an toàn |
| Pending voice | `F0` | Clear command | Theo giao thức hiện hành: xóa chuỗi đang chờ, **không đồng nhất F0 với STOP** nếu người dùng chưa yêu cầu |
| Speed | `C1..C5` | Speed level1..5 | Tham số wheel, **không tự bật motor**; steering speed derive from both endpoints/feasibility, không lấy speed Cn để áp vào lái |

**XX** là giá trị giây 05,0A,...,78 (5..120s bước 5) và `D0 XX` là đúng hai byte UART. TURN và ANGLE cũng là từng byte, nhưng hai lời nói độc lập; không yêu cầu TIME để chốt. **Không biến duration thành thời gian chung giữa wheel và steer.**

## 2. Hai owner / hai state machine Voice độc lập

- **WHEEL state:** `idle → pending_movement → active_until_deadline → stopping → idle`. Chỉ start khi hợp lệ (A2/A3/A4/A7 + TIME), không bơm bánh khi chỉ nghe một từ. Deadline từ `HAL_GetTick()`/elapsed (không blocking). Bốn bánh dùng owner đã có với fixed speed/ramp hoặc spin. Nếu có lệnh lái độc lập, không cắt wheel active chỉ vì steering đang ra target.
- **STEER state:** `idle → pending_direction → active_target → done/stop/fault`. `A5/A6` chọn hướng, `B0..B9` chốt target cho **LEFT và RIGHT riêng** theo phép mapping/hiệu chuẩn đã được kiểm chứng; không yêu cầu TIME. Cần phân biệt: `B0` nói độc lập là yêu cầu CENTER, còn `A5 B0` hoặc `A6 B0` cũng tương đương đích 0°, không là nút xóa pending như code cũ. Không tự suy ra hai góc bánh xe thực từ một góc khẩu lệnh khi chưa có mapping cơ khí.
- **S15.4 angle trajectory:** đo vị trí ban đầu của từng trục, target độc lập, hành trình còn lại, max/min PWM, vùng giảm tốc, NC/SOFT, nguồn động lực; tính tốc độ/đường đi cho hai trục với mục tiêu phối hợp **common-T khi khả thi**, còn không feasible thì dùng position owner độc lập hoặc báo không feasible đúng lộ trình. Không tự giảm LEFT manual trong S15.3. Mỗi trục DONE phải giữ CCR0, không phát lại. Không tự giả định độ trễ <2%.
- **Concurrency bắt buộc:** một lệnh voice bánh đã chốt (vd `A2 D0 0A`) có thể chạy cùng lúc với lệnh voice steering được chốt sau đó (vd `A5 B6`), không làm mất wheel deadline. Ngược lại voice steering đang chạy không chặn hợp lệ một wheel command mới, nếu các điều kiện an toàn/owner cho phép. Không để `Turn_Motors_Stop()` của Wheel executor xóa steering đang active.

## 3. STOP, PS2 takeover và neutral latch

**Priority:** `STOP > PS2 > Voice (wheel+steer độc lập) > Idle`. Một STOP `FF` hoặc emergency stop cắt tất cả nhóm, xóa pending/ready/active của cả hai state machine; không tự tái khởi chạy lệnh voice vừa hủy khi STOP nhả. STOP safety latch / RF loss / NC/EXTI / fault vẫn có quyền cao nhất.

**Bất kỳ thao tác PS2 điều khiển hợp lệ** trong lúc có voice pending/ready/active ở BẤT KỲ nhóm nào: trước tiên hủy **cả wheel và steer voice** và yêu cầu dừng hai nhóm theo đường an toàn hiện hành. Thao tác PS2 đó **CHỈ CANCEL**, không tự chạy motor. Chốt `ps2_cancel_hold=1` hoặc một trạng thái neutral rearm tương đương; không kích hoạt lại PS2 nếu phím vẫn giữ/joystick vẫn lệch. Sau khi tất cả nút điều khiển thả và LY/RX về neutral trên gói PS2 Analog hợp lệ, xóa hold; **chỉ edge/thao tác PS2 mới sau đó** mới được điều khiển. Nếu mất RF, giữ bảo vệ radio-loss và yêu cầu neutral hợp lệ trước khi rearm, không coi disconnected=neutral.

PS2 đang điều khiển bình thường có thể giữ input để duy trì thao tác, không tự bắt nhả sau từng poll. Quy tắc bắt buộc thả và thao tác lại áp dụng cho **một input đã được tiêu thụ để hủy Voice hoặc sau STOP/fault**, không bắt người dùng nhả sau mỗi gói PS2 thông thường. Voice command mới không chạy trong thời gian PS2 nắm quyền và không tự khôi phục các lệnh đã hủy.

**Thiết kế cổng an toàn:** STOP phải giữ cắt motor có thẩm quyền, không dựa riêng vào wheel ramp nếu là emergency; PS2 cancellation có thể ramp theo owner bình thường chỉ khi không vi phạm STOP/NC. No unattended queue replay, no startup auto-resume.

## 4. So sánh với mã nguồn HIỆN TẠI ở baseline f2137556 (= main.c commit be26f52)

- Có `VC02_M2_ProcessEvent`: parser nhận A2/A3/A4, A5/A6, B0..B9, C1..C5 và `D0 XX` để chốt command chung ở `Voice_FinalizeCommand`; hiện chỉ một pending/completed/active voice.
- **Không đúng yêu cầu mới:** `Voice_FinalizeCommand` hiện đòi MOVEMENT trước TIME kể cả khi có hướng+góc; chưa chốt lệnh lái ngay A5+Bn. `Voice_IsOpenLoopCommandSupported` chỉ cho bánh tiến/lùi không có góc; `Voice_ExecuteMotorControl` luôn gọi `Turn_Motors_Stop()` và có thể đè steering. `vc02.c` thiếu A7, A4 chưa mang hướng quay, B0 đang `Voice_SetStraight` thay vì lệnh CENTER.
- **Đã có một phần phù hợp:** `Control_Arbitrate` có logic PS2 first input cancels Voice và chốt hold đến PS2 không active, rồi mới cho input mới; **nhưng** ở trạng thái hiện tại `PS2_IsManualControlActive()` không phải là kiểm tra từng nút đã nhả và LY/RX về neutral một cách tường minh trong mọi trường hợp, và chỉ có Voice state chung. Khi tách hai owner phải kiểm tra lại neutral logic cũng như PS2 mất kết nối trước khi cho điều khiển sau latch.
- PS2 L1/L2 spin đã có nhưng bắt buộc hai NC MIN vật lý tác động và MAX clear; không bỏ qua trên Voice Rotate. Driver cutoff độc lập, dòng và coast/tình trạng cơ khí chưa chứng minh đầy đủ, không cho phép RUN thử motor chỉ vì docs/host tests PASS.

## 5. Đặc tả kiểm thử trước khi chạy motor

1. **HOST event only:** A2 riêng → bánh không chạy; A2 D0 05 → wheel active5s; A4 D0 0A → rotate left10s; A7 D0 0A → rotate right10s; thử cả thiếu D0/xx hợp lệ/invalid xx, F0.
2. **HOST event only:** A5 riêng → steering không chạy; A5 B6 → LEFT/RIGHT target được tạo đúng từ hướng trái+30°, không TIME; A6 B9 → phải45°. B0 độc lập → CENTER, A5 B0 → CENTER. Incomplete TURN must time out/reset safely.
3. **HOST overlap:** A2 D0 0A rồi A5 B6 → cùng lúc wheel vẫn chạy đúng deadline và steering hướng+góc; A5 B6 đang chạy rồi A3 D0 05 → wheel mới chạy nếu an toàn. Lệnh lái mới phải có policy thay đổi/cancel đích an toàn; không đổi hướng motor trực tiếp khi đang chạy.
4. **HOST safety:** FF tại bất kỳ trạng thái nào xóa toàn bộ pending và active, wheel/steer output0, không replay sau FF; STOP vs buffered UART; PS2 first input cancels both, giữ phím/LY/RX≠neutral không chạy, reset state sau valid PS2 neutral, input MỚI mới chạy; mất RF không được tính neutral; kiểm nút Dpad/R1/R2/SELECT/L1/L2/LY/RX.
5. **HOST guards:** wheel rotate chỉ khi đồng thời L_MIN/R_MIN NC thật, MAX clear; NC nhả trong khi spin thì dừng khẩn; steering stale AS5600, invalid mapping, impossible targets/T, STOP, NC, SOFT, owner conflict, DONE no re-power; fallback infeasible phải rõ.
6. **Gates:** SOURCE+host CI exact SHA → STM32CubeIDE Clean Build → flashed firmware SHA → UART và state without motor power → electrical safety (independent physical cutoff, NC/STOP, coast/current) → supported suspended-motor S15.4 hardware runs. Không có log nào hiện tại xác minh user speech-to-COM3-to-STM32 run S15.4.

## 6. Phạm vi & không quyết định thay USER

**Không sửa firmware trong commit kế hoạch này**. Giữ nguyên f2137556 function/PWM, PS2 mapping, S15.3 result 4/4 without skew<2. Kế hoạch người dùng mới thay **cách chốt và xử lý lệnh VOICE** chứ không tự xác lập mapping cơ khí của cặp target. S15.5 arbitrary Ackermann vẫn hoãn; S15.4 chỉ được coi hoàn thành khi đích hai trục và phối hợp thời gian đã được xác minh trên dữ liệu chuẩn và kiểm thử VC-02, không phải chỉ có bảng A/B byte.

## 2026-11-04 — USER sửa đặc tả: SPEED là lệnh độc lập; ROTATE = R2 → L2/L1

**[USER DECISION, có hiệu lực thay các mô tả chưa đủ ở trên]**
1. **Cấp tốc độ là độc lập:** `C1,C2,C3,C4,C5` là các khẩu lệnh đơn tự hoàn chỉnh, không cần A2/A3/A4/A7, không cần Bn hay TIME. Chỉ cập nhật cấp tốc độ Voice; **không gây chuyển động**. Có thể nói ở thời điểm khác với câu chuyển động. Giữ quy tắc cũ trong STM32: speed mode chọn riêng, có thể tác động tới lệnh bánh **kế tiếp**; việc đổi speed tức thời của lệnh bánh đã chạy không được user yêu cầu nên không tự kích hoạt. Tốc độ của 2 motor đánh lái vẫn suy từ 2 target và giới hạn cơ cấu, không đồng nhất với C1..C5.
2. **A4 Rotate left + D0 XX:** khi chuỗi movement + TIME hoàn chỉnh và tất cả điều kiện STOP/PS2/NC/owner cho phép, **yêu cầu thao tác vị trí tương đương R2** để đưa 2 cơ cấu lái về đích MIN như đã hiệu chuẩn; chỉ sau khi R2 thành công **và cả NC L_MIN, R_MIN tác động vật lý; L_MAX, R_MAX không tác động**, mới cấp lệnh wheel-spin **tương đương L2** = `WheelControl_SetSpinDirection(-1, ...)`.
3. **A7 Rotate right + D0 XX:** quy trình R2 tương tự; sau khi R2 đạt trạng thái hợp lệ thì cấp wheel-spin **tương đương L1** = `WheelControl_SetSpinDirection(+1, ...)`.
4. **Không phát R2 trước khi nhận đủ TIME**: A4/A7 đơn lẻ là pending, không motor; TIME chỉ chốt yêu cầu tổng hợp; không được bypass NC bằng việc chỉ thấy RAW tới MIN, và không fake state PS2/ghi trực tiếp nút `PS2_Pad`. Phải gọi đường position-owner có bảo vệ hiện có hoặc bộ route tương đương với cùng gates.
5. **Thời gian xoay được tính từ lúc bắt đầu quay bánh**, tức là sau giai đoạn chờ R2/đủ NC (lựa chọn triển khai hợp lý từ thứ tự USER, không phải lời USER khẳng định về mốc thời gian). Nếu R2 thất bại, timeout, fault, NC không khớp hoặc đang bị quyền điều khiển khác chiếm: **không quay bánh**, ghi rõ nguyên nhân. Nếu mất điều kiện NC giữa chừng, dừng wheel-spin ngay và không tự quay lại. Không để quãng chạy bánh trong giai đoạn steering positioning.
6. **Phụ thuộc quan trọng:** Về tổng thể Voice wheel và voice steering độc lập, NHƯNG Rotate là trường hợp chủ động cần dùng cả steering owner R2 và wheel owner spin. Không được chạy đồng thời một lệnh Turn+Angle khác có đích xung đột với R2, hoặc chiếm quyền PS2. Thiết kế phải từ chối/đợi được quyết định an toàn, không âm thầm ghi đè target đánh lái đang chạy.
7. **STOP/PS2** hủy cả giai đoạn `WAIT_R2` và `SPINNING`, xóa command pending, TIME latch; PS2 action đầu chỉ cancel, đợi nhả hết button và cả joystick neutral/Analog hợp lệ rồi thao tác mới. Không auto-resume chuỗi quay trước đó.

### Bảng state ROTATE dành cho kiểm thử

| State | Điều kiện chuyển | Wheel PWM | Steering owner |
|---|---|---|---|
| `PENDING_ROTATE` | A4/A7 chưa có D0 XX | 0 | Không tự chạy R2 |
| `REQUEST_R2` | Đủ D0 XX và quyền thực thi hợp lệ | 0 | Gửi yêu cầu vị trí tương đương R2 |
| `WAIT_R2_AND_NC` | R2 đang đưa 2 trục về MIN | 0 | Giữ owner vị trí đến khi hoàn tất; xác nhận NC L/R MIN vật lý và MAX clear |
| `SPINNING` | R2 thành công + NC đúng + không STOP/trip/PS2 | Theo voice wheel-speed và giới hạn spin | Không cho lệnh steering khác xung đột; A4=>L2 (-1), A7=>L1 (+1) |
| `DONE/ABORT` | Hết duration, STOP, PS2, RF loss, NC release, fault | 0/dừng an toàn; NC release phải cắt ngay | Không phát lại R2/Spin khi không có lệnh mới |

### Bổ sung kiểm thử HOST bắt buộc

- C1..C5 đơn lẻ lưu speed mode, không lệnh motor, không TIME; speed mode không xóa pending R2/Turn và giữ được qua từng lệnh theo quy tắc hiện hữu.
- A4/A7 không kèm TIME **không được** phát R2 hoặc spin; sau D0 XX kiểm chứng trình tự `R2 → NC physically valid → L2/L1`.
- A4 với `R2` chưa hoàn tất / NC thiếu: spin PWM luôn0; đúng NC MIN nhưng MAX đang tác động: từ chối; khi spinning NC nhả: ngắt bánh ngay, ghi lỗi, không tự khởi động lại.
- Phân biệt dấu: A4 => L2 => -1, A7 => L1 => +1. Không dùng A5/A6 làm rotate direction.
- FF STOP hoặc PS2 input tại `REQUEST_R2`, `WAIT_R2_AND_NC`, `SPINNING` hủy cả 2 nhóm; PS2 input không điều khiển đến khi có neutral và tác động MỚI.
- Khi lệnh steering A5/A6 đang active, incoming Rotate không được bỏ qua owner arbitration và tự chạy R2; khi Rotate chiếm steering, lệnh Turn+Angle khác không thể ghi đè target trong giai đoạn spin.

**Trạng thái:** Đây chỉ là hợp đồng/kế hoạch để tích hợp STM32 S15.4, chưa có firmware điều khiển mới, target build hay bằng chứng motor thực tế. Giữ nguyên `f2137556` code, STOP/NC/atomic và quyền PS2.

## 2026-11-04 — Firmware VC02-I1 source staged BEFORE S15.4 synchronization

User yêu cầu **viết firmware nhận đủ lệnh và kiểm tra đúng trước**, sau đó mới bắt đầu đồng bộ tốc độ hai motor lái S15.4. Theo đúng thứ tự, code bổ sung A7 decoder, hai luồng Voice wheel-TIME và steering-angle, B0 CENTRE độc lập, speed C1..C5 độc lập, STOP/PS2 neutral latch, Rotate R2 → physical MIN NC → L2/L1. Steering angle chạy owner vị trí độc lập (command4) với hai target SOFT RAW suy từ hành trình chuẩn hóa; **chưa có common-T hay dynamic velocity synchronization** và chưa được coi đó là góc bánh xe vật lý được hiệu chuẩn.

Các đường motor VC02 steering/rotate mới đều boot disabled: voice_pos_enable=0, voice_angle_mapping_enable=0, voice_pivot_enable=0. A2/A3 tiếp tục khả năng wheel Voice trước đây. Giao thức/host tests không chứng minh bảo đảm điện hoặc cắt nguồn; ghi nguồn bộ cắt độc lập, NC, dòng driver, Coast và target MCU Build/Flash đang chờ. Chi tiết implementation và test đúng SHA ở docs/M8_VC02_I1_IMPLEMENTATION_2026_11_04.md.


## 2026-11-04 — Đặc tả mới nhất từ USER: kiểu PLC, latest partial wins

**Chốt thay thế mọi mô tả "conflict" cũ:** mỗi nhóm có thanh ghi lệnh dở riêng. WHEEL: lệnh mới nhất A2/A3/A4/A7 ghi đè movement chưa hoàn tất; chỉ khi có cả D0 XX mới chốt bốn bánh. STEER: hướng mới nhất A5/A6 ghi đè hướng chưa hoàn tất; chỉ khi có đủ B1..B9 mới tính hai target. Có thể nhận TIME trước movement hoặc ANGLE trước turn trong cửa sổ 20 giây; không ghép dữ liệu quá cũ. B0 tự chốt CENTER. C1..C5 chỉ đổi tốc độ Voice bánh, độc lập, không làm motor chạy.

**F0 là CLEAR vùng nhớ lệnh chưa hoàn tất** của cả hai nhóm, không STOP, không hủy lệnh đã chốt/đang thực hiện, không xóa mức speed. **FF STOP** và input điều khiển PS2 đầu tiên trong khi Voice có pending/ready/active hủy toàn bộ cả wheel và steer. PS2 input đó chỉ dùng để hủy; phải thả hết 16 nút và trả LY/RX/LX/RY về neutral với gói Analog hợp lệ, sau đó thao tác mới mới được điều khiển. Wheel+steer vẫn hai owner đồng thời. Riêng A4/A7 yêu cầu R2 → hai NC MIN thực tác động, hai MAX không tác động → L2/L1 spin, không được override R2 trong lúc quay.

**Source branch:** `feature/m8-vc02-plc-latest-command` từ đúng `e96ad4f`, không reset nhánh cũ. Pilot cho 3 Voice route flags boot 1 sau lệnh USER yêu cầu triển khai; **master `steer_safety_motion_permit` vẫn boot 0**, chỉ PS2 START kiểm tra neutral/AS5600/NC/wheel-stop mới cấp quyền. Không đổi PWM, gain, S15.3 LEFT, STOP/NC/ISR/FAULT hay SOFT. Thời gian 20s là mặc định xử lý command dở trong phiên triển khai này, không phải yêu cầu đã đo của VC-02.

**Khả năng chẩn đoán UART:** các biến mới `vc02_uart_rx_byte_count`, `vc02_uart_last_byte`, `vc02_uart_error_count`, `vc02_uart_rearm_error_count`, `vc02_uart_begin_status` để phân biệt nguyên nhân không có decoded event. CI chỉ xác minh HOST C với stub; chưa CubeIDE Clean Build, chưa flash thực, chưa có nhận UART/hardware PASS. S15.4 common-T dựa hai target vẫn milestone kế tiếp.


## 2026-11-04 — USER chỉnh SPEED C1..C5 áp dụng NGAY khi đang chạy

- **Quy tắc mới nhất thay thế câu cũ `speed only for next wheel command`:** SPEED vẫn là lệnh đơn độc, không cần TIME, không tự khởi chạy bánh. Nhưng nếu FORWARD/BACKWARD/ROTATE đang active, khi STM32 giải mã C1..C5 hợp lệ thì ở vòng điều khiển Voice kế tiếp phải cập nhật ngay **setpoint tốc độ của chính lệnh đang chạy** mà không chờ hết TIME. Giữ nguyên hướng, thời gian kết thúc, hành trình lái và quyền ưu tiên STOP/PS2.
- `Voice_ExecuteMotorControl` đồng bộ `voice_active_command.speed_level` từ `voice_speed_mode` mỗi vòng khi còn active. FORWARD/BACKWARD tính lại `voice_execution_pwm` giữ dấu và gửi qua `WheelControl_SetTargetPWM` (FIXED shaper/ramp/PID cũ); Rotate R2 WAIT có thể đổi speed nhưng không spin trước NC, sau SPIN thì gọi `WheelControl_SetSpinDirection` với mức mới, vẫn cap `MOTOR_DPAD_PWM=400`.
- **Lưu ý về giới hạn:** các mức hiện hành là C1=250, C2=400, C3=550, C4=750, C5=999 (requested PWM counts before ramp). Trong Rotate, trần spin 400 nên C2..C5 cùng bị clamp còn 400; chúng không phải năm tốc độ xoay phân biệt. Không âm thầm tăng cap hoặc đổi tuning để tạo đủ năm mức khi chưa có kiểm thử cơ cấu.
- SPEED không làm motor nhảy tức thời tới RPM vật lý: **mới chỉ có target được cập nhật ngay**, ramp/PID cần thời gian đáp ứng. FF STOP, PS2 cancel, NC, current/stall, và owner steering độc lập giữ nguyên. Không thay đổi START-arming flags hay thời gian command.
- Thêm `voice_speed_active_update_count` (debug) đếm những lần owner active thực sự nhận thay đổi speed từ `voice_speed_mode`. HOST test của code C thực kiểm tra cập nhật giữa FORWARD/BACKWARD, trong WAIT_R2, trong SPIN trái/phải, giữ duration/sign/400 cap và STOP/NC. CubeIDE Clean Build, flash và kiểm thử hardware chưa thực hiện.


## 2026-11-04 — USER: hoán đổi chức năng STOP giọng nói / PS2 hủy Voice

- **Yêu cầu mới nhất, thay cho mô tả FF hard STOP cũ:** FF của VC-02 hủy cả lệnh Voice bánh và lái nhưng dùng **giảm tốc mềm M6 đã có** cho 4 bánh: yêu cầu `WheelControl_SetSpinDirection(0,0)`, `WheelControl_SetTargetPWM(0)`, không gọi emergency cut ở FF. Hai motor lái vẫn ngắt PWM theo owner cũ. Voice FF **không phải E-stop vật lý**.
- **PS2 input đầu tiên khi có Voice pending/ready/active:** hủy Voice và **ngay lập tức** gọi đường hiện hữu `Wheel_EmergencyStop()`, không cho chính input đầu đó điều khiển; `ps2_cancel_hold` giữ đến khi cả nút/trục về neutral hợp lệ và người dùng tạo input mới.
- **Khi bánh còn đang hãm mềm sau FF:** giữ `control_stop_latched` cho đến khi logic `Steering_PositionWheelsStopped()` xác nhận dừng; PS2 tác động lúc đang hãm mềm sẽ cắt khẩn và vào `ps2_cancel_hold`. Nếu mất RF khi wheel spin đang hãm, giữ đường emergency RF đã có.
- **Không hoán đổi các đường dừng an toàn:** NC/EXTI, safety trip, lỗi encoder/steer, RF-loss fail-safe và dừng khẩn cấp độc lập không bị đổi thành ramp. Không thay PID, M6 S-curve, PWM, lệnh SPEED, lệnh F0, nhánh PLC latest-wins, R2/NC pivot, hay S15.4.
- **Điều kiện kiểm chứng:** test trích đúng C của `Voice_S154CancelAll`, `Voice_CancelByPS2`, `Control_RequestStop`, `Control_Arbitrate` dùng HOST stub, không chứng minh PWM/độ trôi thực. Build target STM32CubeIDE, flash và thử trên xe là bước riêng, chưa thực hiện.


## 2026-11-04 — Newest operator requirement: guarded auto-ARM without PS2 START

- ARM is a **permission**, not a motor-start command. On boot `steer_safety_motion_permit`, `steer_manual_enable`, `steer_pos_enable`, `steer_ps2_manual_armed` remain zero. Existing `Steering_PS2ManualArmUpdate` now automatically grants 1 only after connected analog PS2 full neutral, stationary wheels, all four initialized noncontradictory NC switches, no STOP/PS2 cancellation/busy owner, BOTH AS5600 fresh reads and existing physical envelope checks pass. No START hold, no START press, no alternative ARM button.
- Any **transient unmet prerequisite before ARM** is retried automatically, no user reset required: neutral, wheel-stop, disconnected PS2, NC contradictory/not initialized, AS5600 transport/magnet/geometry failed before grant. Current retry cadence is once per 500ms (not a 1.5s START hold). A successful later check yields `steer_ps2_manual_arm_status=2`, permit=1. After PS2 disconnect, old behavior disarms; once reconnected with neutral and valid sensors, rechecks and automatically arms.
- A **latched hard steering incident** `steer_safety_trip_latched=1` is still an active fault even if its transient physical symptom has disappeared; the current firmware intentionally has no automatic latch clear. It does not auto-ARM across such a fault. This limitation preserves physical NC/EXTI emergency rules and is not an additional manual ARM procedure. Fault resolution/requalification is a separate safety-design task; do not silently reset a trip.
- START is removed from PS2 manual-takeover detection and the auto-ARM function does not read START for arming. START remains part of full-neutral preflight (must be released) and part of other existing neutral checks. **TODO FOR FUTURE COMMIT**: remap PS2 START to logging control. This commit intentionally retains `RamLogger_ButtonUpdate` on its existing USER-button route without implementing or guessing new START log semantics.
- Prior braking swap remains: VC-02 FF uses existing M6 soft-wheel stop; first PS2 input taking Voice or interrupting soft-coast calls existing emergency cut. C1..C5 live, F0 pending-only, latest-wins and R2/NC spin retained. No changes to S15.4 timing, PID gains, motor PWM limits, software/hardware emergency mechanisms.
- HOST regression extracts production C functions, tests auto-ARM without START, START no longer toggles, transient errors heal without a button, disconnected/reconnected PS2, and latched fault blocking. HOST CI is not CubeIDE firmware build, flash, or physical motor safety validation.


## 2026-11-04 — Chuỗi lệnh VC-02 theo thứ tự nghiêm ngặt, không ghép tham số rơi rớt

**Yêu cầu người vận hành (thay quy tắc input đảo thứ tự trước đây):** Nếu VC-02 không nhận ra lời điều khiển chính mà chỉ nhận TIME hoặc ANGLE, tuyệt đối không được giữ tham số đó để kích hoạt motor khi người dùng nói lại lệnh chính sau. Khi một lệnh chính mới ghi đè lệnh đang nhập dở, phải làm trắng toàn bộ trường chưa chốt của chuỗi đó, bao gồm tham số cũ, ngay cả khi lệnh chính mới trùng lệnh cũ.

- **Bánh:** Chỉ `A2/A3/A4/A7` **rồi mới đến** `D0 XX` (XX hợp lệ) được chốt. `D0 XX` xuất hiện khi chưa có movement hợp lệ: bỏ đi, không lưu `duration_sec` hay `duration_valid`; sau đó `A2` một mình tuyệt đối không làm bánh chạy. `A2 A3 D0 0A` thành BACKWARD 10s, `A2 A3` không chạy, `D0 05 A2` không chạy cho đến khi có D0 mới.
- **Lái:** Chỉ `A5/A6` **rồi mới đến** `B1..B9` được chốt. `B6` không có hướng trước đó: bỏ đi, không lưu `angle_valid`; sau đó `A5` một mình tuyệt đối không đánh lái. `A5 A6 B6` thành RIGHT 30°, `B6 A5` không chạy cho đến khi có B mới. **B0 vẫn là CENTER độc lập**, không cần lệnh A5/A6 trước.
- **Thay thế toàn bộ chuỗi chưa hoàn tất của CÙNG nhóm:** mỗi PRIMARY mới làm trắng toàn bộ movement+duration của nhóm bánh hoặc turn+angle của nhóm lái trước khi bắt đầu chuỗi mới; cả khi PRIMARY trùng. Không xóa nhóm kia vì bánh và lái vốn có owner song song. Không hủy lệnh đã chốt/đang thực thi: STOP/PS2 vẫn quyết định hủy. `F0` chỉ xóa pending chưa chốt; `FF` giữ chính sách STOP mềm và dừng khẩn PS2 như nhánh nền.
- SPEED `C1..C5` độc lập, cập nhật setpoint của bánh đang chạy như trước, không làm tự khởi động. Giữ expiry 20s cho primary không đủ tham số; dữ liệu đến quá muộn sau expiry bị coi như tham số mồ côi và bỏ. `voice_orphan_duration_discard_count` và `voice_orphan_angle_discard_count` là counter debug mới.
- Kiểm thử: `tools/verify_m8_vc02_command_core.py` trích đúng hàm C sản xuất để mô phỏng missed primary, orphan parameters, replace cùng/khác direction, 2 owner độc lập, expiry, F0/FF; `tools/verify_m8_vc02_i1.py` static contract cập nhật. CI HOST không chứng minh CubeIDE target build, nạp flash hoặc hành vi phần cứng thực.


## 2026-11-05 — USER chốt: 0° CENTER, 5–40° nội suy NC MIN/MAX, 45° phải xác nhận NC

**Đây là chính sách Voice Steering mới nhất, thay nội suy 45° theo SOFT hoặc coi RAW đích = 45° hoàn tất ở các ghi chú cũ.** Nhánh thử nghiệm `feature/m8-vc02-45deg-nc-endstops` từ mốc an toàn `b407c0dbb9f40c2843763483f55ce5bee936990b`; mốc gốc và các nhánh trước giữ nguyên.

- `B0`: CENTER bằng các RAW CENTER hai bên như cũ. `B1..B8` (5°..40°): nội suy tuyến tính, độc lập giữa LEFT và RIGHT, từ CENTER đến mốc vật lý MIN/MAX đã lưu bằng `steer_cal_*_runtime`. **Không** dùng giá trị SOFT làm điểm 45°. Nếu NC bất ngờ tác động ở 5–40°, báo lỗi, không coi là thành công.
- `A5 B9` LEFT45: trục LEFT phải chạm `L_MAX`, trục RIGHT chạm `R_MIN`. `A6 B9` RIGHT45: LEFT phải chạm `L_MIN`, RIGHT chạm `R_MAX`. Khởi chạy `steer_pos_command=5` riêng cho Voice45, không dùng `command=4` của RAW 5–40°, không dùng `command=2` chỉ tìm MIN của R2.
- **45 chỉ DONE khi NC đúng hướng:** bộ ngắt GPIO EXTI cắt ngay PWM từng bên, các kiểm tra ở đầu lệnh, pending IRQ, late IRQ và atomic GPIO recheck xác nhận NC riêng từng motor. Một motor về đích trước dừng riêng; motor còn lại vẫn chịu watchdog, bảo vệ và không ép quá mốc. `steer_pos_left_result=3` và `steer_pos_right_result=3` là bắt buộc để báo thành công; `steer_pos_result=2` là hoàn thành tổng hợp khi cả hai đạt NC. Mọi NC bất ngờ/đôi mâu thuẫn vẫn lỗi.
- **Không ép cơ khí:** mốc `steer_cal_*_runtime` là RAW **giới hạn tới hạn để dừng lỗi nếu NC chưa tác động**, không phải ngưỡng cho phép tiếp tục quá giới hạn. Khi RAW tới mốc mà NC vẫn chưa tác động, trục dừng và báo lỗi (code8); không tăng PWM, không thay mốc safety, không kéo dài timeout/stall. Voice45 phải đợi bốn bánh đứng yên trước khi được phép chạy; STOP/PS2 takeover/NC/fault vẫn có ưu tiên.
- **Phụ thuộc cơ khí bắt buộc trước test powered:** công tắc NC phải thực sự báo đúng khi cơ cấu còn có khoảng trôi an toàn, và mốc runtime phải phản ánh dữ liệu đo mới nhất; nếu NC tác động SAU ranh RAW, firmware fail-closed, không cố ép đến NC. `45°` ở đây là nhãn giao thức tương ứng giới hạn NC do người dùng quy ước, không phải đo góc bánh thực bằng thước.
- Bộ test HOST C thực: `tools/verify_m8_vc02_45_nc.py`, `tools/verify_m8_vc02_command_core.py`, `tools/verify_m8_vc02_i1.py`, chạy trong workflow exact SHA. Host CI không chứng minh STM32 target compile/flash, tính đúng góc vật lý, tiếp điểm NC thực hoặc độ trôi sau cắt PWM. Không đổi câu lệnh UART, C1..C5, FF soft STOP, PS2 hard cancel, auto-ARM, điều phối bánh/lái riêng và S15.4 common-T.
