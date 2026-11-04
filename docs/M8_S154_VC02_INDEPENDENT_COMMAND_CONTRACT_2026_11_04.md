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
