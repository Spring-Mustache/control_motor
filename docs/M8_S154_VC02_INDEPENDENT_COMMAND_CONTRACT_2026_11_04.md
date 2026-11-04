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
