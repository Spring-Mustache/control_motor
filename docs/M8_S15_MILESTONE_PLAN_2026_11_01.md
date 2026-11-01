# M8-S15 — Milestone kế hoạch điều khiển đánh lái (chốt định hướng 2026-11-01)

**Dự án:** xe tăng cướp bóng, STM32F767ZI, 4 motor bánh + 2 motor lái.  
**Nhánh:** `feature/m8-s15-dual-target-common-time`; xác minh HEAD thực tế trước mọi thao tác.  
**Tình trạng tài liệu:** kế hoạch dựa trên yêu cầu người vận hành mới nhất, không phải nghiệm thu firmware hoặc phần cứng. Giữ nguyên các kết quả S14, S15 pilot và S15.1 lịch sử ở tài liệu cũ.

## QUYẾT ĐỊNH PHẠM VI MỚI NHẤT — 2026-11-01

**[USER-APPROVED] DỪNG PHẠM VI PHÁT TRIỂN M8-S15 HIỆN TẠI SAU S15.4.** Thực hiện theo thứ tự S15.2 (manual angle-relation SHADOW) → S15.3 (RIGHT-only manual, khi đủ bằng chứng) → S15.4 (R1/R2 position/common-T có điều kiện). Sau khi đạt tiêu chí kiểm chứng tương ứng của S15.4, **chuyển trọng tâm phát triển sang tích hợp lệnh VC-02 với hệ thống điều khiển xe**. Nếu một bước chưa đạt, phải báo CHƯA ĐẠT/CHƯA KIỂM CHỨNG và xử lý các rủi ro liên quan; không tự ghi S15 đã PASS.

**[DEFERRED, NOT CANCELED]** S15.5 target góc độc lập bất kỳ / Ackermann vì **chưa có động học hoàn chỉnh**, không tự giả định ánh xạ góc bánh thực và không nhận lệnh VC-02 góc tùy ý là đã hỗ trợ. Mục S15.6 cũ (regression & six-motor Gate A) **không nằm trong chuỗi S15 ưu tiên trước VC-02**; chuyển kiểm thử tương ứng sang các cổng tích hợp VC-02 / Gate A / Gate B riêng, không bỏ hay mặc nhiên PASS. Không triển khai S15.5/S15.6 chỉ để đóng mốc trước khi quay về VC-02.

### Nhánh kế tiếp: tích hợp VC-02 vào cơ cấu điều khiển xe (sau S15.4)

- **Hiện trạng source được đọc:** `VC02_M2_ProcessEvent` đã nhận event MOVE_FORWARD/BACKWARD, ROTATE, TURN_LEFT/RIGHT, ANGLE, SPEED, DURATION, CLEAR, STOP; `Voice_FinalizeCommand` đóng lệnh khi có DURATION. Nhưng `Voice_IsOpenLoopCommandSupported` hiện **chỉ cho thực thi tiến/lùi thẳng, không TURN/ANGLE/ROTATE**, và `Voice_ExecuteMotorControl` cấp target cho bốn bánh rồi `Turn_Motors_Stop()`. Vì vậy **đã có parser/builder không đồng nghĩa đã tích hợp VC-02 với cơ cấu lái**.
- **Đầu việc VC-02 tiếp theo (mốc công việc mới VC02-I1; kế hoạch, chưa code):** kiểm tra bảng sự kiện/giá trị góc thật sự từ VC-02; đối chiếu arbitration `STOP > PS2 > Voice > Idle`, quyền sở hữu motor và điều kiện PS2 takeover; thiết kế liên kết command hợp lệ tới bốn bánh (giữ M6 wheel owner/PI và profile đã có) và các mode đánh lái đã kiểm chứng đến S15.4. Không tự sử dụng Ackermann hoặc hai góc target vật lý chưa có mapping.
- **Trình tự dự kiến VC02-I1 → VC02-I2 → VC02-I3:** (I1) source audit + HOST shadow mapping/validator, từ chối an toàn các lệnh chưa hỗ trợ; (I2) opt-in tích hợp executor và trạng thái hoàn thành/cancel/timeout theo quyền sở hữu, không chồng PS2/Voice; (I3) regression VC-02/PS2, STOP/NC/soft, bánh/lái trên giá treo **Gate A** và ground-load **Gate B riêng** sau đủ điều kiện. Đây là ID mới để lập kế hoạch, chưa phải milestone firmware hay PASS.
- **Bảo vệ:** một command voice không tự bỏ qua START/arm/neutral, STOP, NC, SOFT, fault, stall, driver cutoff; không thể dùng mốc thử trên giá để suy loaded PASS. Chưa chốt diễn giải cơ khí cho lệnh TURN/ANGLE; việc tích hợp cần phân biệt command parser hợp lệ với lệnh motor có đích/động học đã hỗ trợ.
- **Thực thi ở commit này:** cập nhật docs only; không sửa `Core/`, PWM, Level2/Fast Start, route PS2, VC-02 hoặc safety. Không đặt hạn hay dữ liệu test giả định.

## 1. Quyết định điều khiển hiện hành và điều đã bị thay thế

1. **D-pad LEFT/RIGHT và joystick RX là MANUAL ANGLE-RELATION:** mục tiêu là giữ **quan hệ góc đánh lái mong muốn** giữa LEFT/RIGHT trong khi vận hành, có dự đoán sai lệch góc/vận tốc. Không yêu cầu đồng tốc độ góc tuyệt đối, không ép hoàn thành chung T khi nhả bất kỳ lúc nào. Khác nhau về cung hành trình khiến `v_L=v_R` hoặc `angle_travel_L=angle_travel_R` không tự bảo đảm quan hệ góc đúng.
2. **LEFT:** đã có đáp ứng tốt theo người vận hành; không dùng taper, hold hay slowdown LEFT làm chiến lược đồng bộ. Chỉ các bảo vệ/owner có thẩm quyền mới được hạn chế hoặc dừng.
3. **RIGHT:** trục ưu tiên cải tiến khả năng bám quan hệ góc, đo bằng AS5600 và nghiên cứu encoder vận tốc sau khi kiểm chứng tỷ số, chiều, timestamp. Nếu bị bão hòa và không theo được quan hệ góc, công khai trạng thái không khả thi; không hứa duy trì sai số vô hạn trong giới hạn PWM.
4. **Giữ lại các ý tưởng tốt S14:** dự đoán sai lệch góc (P+D có timestamp); Fast Start S14.3 và **Level 2 làm mặc định nguồn** theo mã hiện hành; không khôi phục S14/S14.1 taper-only làm chủ trương.
5. **Các lệnh có góc đích:** R1 tới CENTER có thể chọn common-T nếu cả hai còn hành trình có ý nghĩa và khả thi, nếu không dùng owner vị trí độc lập. R2 chỉ được xét common-T khi **cả hai ở CENTER hợp lệ** tại lúc nhận lệnh; từ trạng thái lệch tâm/sau D-pad, một bên gần hoặc đã MIN, R2 độc lập. R2 có **MIN vật lý/NC** riêng, không biến thành đích SOFT giả và không ép chạm NC cùng T. Lệnh góc bất kỳ và Ackermann tương lai dùng hai góc riêng, common-T khi khả thi; trục đã DONE không được cấp PWM lại.
6. **Giữ nguyên PS2 semantics:** D-pad/RX nhả thì stop manual như source hiện hành, không tự quay CENTER; đổi chiều phải qua neutral/chờ thích hợp. STOP, NC/EXTI, SOFT, fault, stall, START arm, atomic late check và wheel-owner ưu tiên cao hơn coordinator.
7. **Giới hạn nguồn hiện có:** `steer_s142_trial_level=2` mặc định, LEFT Level2 active cap **450 CCR**, RIGHT active cap **200 CCR**; `steer_s143_fast_start_enable=1`. RIGHT **300 CCR** chỉ *proposed/dormant* của S15.1, **không cấp motor**, không thay cap hoặc profile giảm tốc 320→180/100 RAW, +12 CCR mỗi pass, dòng/quãng trôi chưa đo. Không tự nâng PWM gần NC.
8. **Kiến trúc một owner cho mỗi trục:** mode chọn/latch lúc bắt đầu lệnh hợp lệ, không chồng S14 PI/P+D/S149/S15 hay đổi mode đột ngột giữa stroke. Có thể dùng chung observer nhưng từng mode chỉ có một đường quyết định candidate; owner an toàn vẫn có quyết định cuối.

## 2. Phân biệt ba đại lượng

- **Góc tuyệt đối hiệu chuẩn:** `theta_L,theta_R` từ hai AS5600 theo mapping CENTER/MIN/MAX riêng; `-45°..+45°` hiện là tọa độ phần mềm, chưa phải đo trực tiếp góc bánh xe thực tế.
- **Quan hệ góc mong muốn:** `theta_R_ref=f(theta_L, direction, geometry/calibration)`; sai số `e=theta_R_ref-theta_R`. Chưa có hàm `f` hình học đã hiệu chuẩn thực tế. Dùng normalized fraction từng cung CENTER→biên chỉ là **proxy để SHADOW**, không gắn nhãn Ackermann/đúng vật lý.
- **Dự đoán sai lệch:** `e_pred=e+tau*(d theta_R_ref/dt - omega_R)`, dùng timestamp riêng, độ mới AS5600 và/hoặc encoder đã hiệu chuẩn. `tau=120ms` trong S14 là mốc thử lịch sử, **không phải gain chốt**. Khi dữ liệu stale, đảo chiều, đích/đo không hợp lệ, không dùng dự đoán như căn cứ tăng PWM.

## 3. Milestone và cổng kiểm chứng

| ID | Đầu ra | Tiêu chí chuyển bước | Trạng thái |
|---|---|---|---|
| **S15.0 (lịch sử)** | C kernel common-T hai target, pilot D-pad LEFT Level2 | HOST đã kiểm pure C; pilot thực tế RIGHT tới SOFT sau LEFT 448/669ms, không nhận là T vật lý PASS | **Đã triển khai; CHƯA ĐẠT common-T ngoài thực tế** |
| **S15.1 (hiện tại)** | RAM trace 96 samples của stroke S15 cuối, AS5600/encoder delta/T/ref/owner/cap/applied, M8S151 dump; RIGHT 300 chỉ proposed | SOURCE+HOST/CI exact SHA; **còn cần** CubeIDE Clean Build, flash SHA và xác minh log khi có điều kiện; trace hiện chỉ cho S15 pilot, chưa bao phủ toàn manual | **SOURCE/HOST-CI PASS; HARDWARE CHƯA KIỂM** |
| **S15.2 — Manual relation SHADOW** | Một helper C thuần tính quan hệ góc, sai số dự đoán, marker stale/saturation; telemetry D-pad cả hai hướng & RX; chỉ đọc, không ghi CCR | HOST unit tests: cung L/R khác nhau, CENTER/biên, đổi hướng, nhấn ngắn, nhả, stale/invalid, nhiều timestamp, NC/SOFT. So offline với log, ghi rõ sai số proxy và hạn chế mapping | **CHƯA BẮT ĐẦU; ưu tiên kế tiếp** |
| **S15.3 — RIGHT-only manual tracking** | Một coordinator opt-in mới, chỉ can thiệp RIGHT khi owner bảo vệ cho phép, không cắt/hãm LEFT để làm đẹp skew; lấy candidate boost/trim RIGHT hợp lệ (encoder chỉ sau validation) | HOST/CI và CubeIDE đúng SHA trước thử; khi an toàn điện/cơ cho phép, A/B có cùng mốc đầu và RX magnitude, đo max/mean/time-over-limit của sai số quan hệ, RIGHT response/arrival, LEFT response không suy giảm, NC/STOP/SOFT/coast/current. Không có ngưỡng góc nghiệm thu được đo/duyệt thì chưa PASS | **CHƯA BẮT ĐẦU; phụ thuộc S15.2** |
| **S15.4 — Chọn mode điều khiển vị trí** | Route `MANUAL_RELATION`, `POSITION_COMMON_T`, `POSITION_INDEPENDENT`, `SAFETY_STOP`; R1 về CENTER common-T có điều kiện; R2 chỉ xét common-T từ CENTER cả hai, còn lại independent và NC-authoritative | HOST đầy đủ điều kiện CENTER, một trục gần MIN/đã DONE, R2 NC, short stroke, infeasible T, neutral/release, ưu tiên STOP. CubeIDE/Flash và đo motor từng mode trước khi bật mặc định. Không tự chuyển chế độ giữa stroke | **CHƯA BẮT ĐẦU; thiết kế thống nhất** |
| **S15.5 — Hai target góc bất kỳ / Ackermann** | Route owner dừng ở góc trung gian L/R riêng, dùng common-T được kiểm tra feasibility, mapping góc bánh thực tế/động học | +35°/+30° và nhiều dấu/góc/điểm đầu phải đến từng đích trong dung sai đo, không chạm NC, T feasible thực tế; **chưa có mô hình hình học và dung sai được nghiệm thu** | **TẠM HOÃN theo yêu cầu USER; chờ động học hoàn chỉnh, không chặn VC-02** |
| **S15.6 — Regression và kết thúc S15 (mốc lịch sử đã đề xuất)** | Kiểm thử D-pad/RX/R1/R2/SELECT, PS2/Voice, STOP/NC/soft, 4 bánh, phối hợp sáu motor | **Gate A** treo an toàn PASS có bằng chứng; thử mặt đất có tải thuộc **Gate B riêng**, không suy tự động từ Gate A hoặc CI | **TẠM HOÃN dưới nhãn S15.6; chuyển các bài hồi quy và Gate A/B sang giai đoạn tích hợp VC-02; chưa PASS** |

Không đặt hạn, phần trăm tiến độ hoặc ngưỡng độ lệch góc cụ thể khi chưa có dữ liệu/đồng thuận. `S15.2–S15.6` là ID lập kế hoạch mới, chưa phải các bản firmware đã commit. **Phạm vi S15 ưu tiên hiện hành chỉ đến S15.4; S15.5/S15.6 tạm hoãn và sau đó chuyển VC-02.**

## 4. Kế hoạch triển khai ngay sau tài liệu này

**Bước trực tiếp:** S15.2 SHADOW, KHÔNG đổi PWM. Tạo hàm C thuần và bộ test với hai cung khác nhau; tính quan hệ góc trên tọa độ tương ứng của từng bên và một khoảng dự đoán có kiểm soát; bổ sung log cho manual D-pad/RX trong RAM có giới hạn; đối chiếu trên dữ liệu sẵn có khi đủ. Chưa khẳng định proxy chuẩn hóa là quan hệ góc bánh thực tế.

**Điều cần xác định trước khi chuyển sang active:** mapping `f` đúng cơ cấu thực tế, dung sai góc chấp nhận, độ mới và chất lượng encoder/AS5600, headroom RIGHT trước 180 RAW và vùng cuối, cùng bằng chứng an toàn điện-cơ. Có thể phát triển HOST chỉ với dữ liệu source sẵn có, không bắt người dùng đo motor ngay để bắt đầu S15.2.

**Cách đánh giá FAIL/PASS:** tách `SOURCE/STATIC`, `HOST`, `CUBEIDE`, `FLASHED SHA`, `HARDWARE`; tách lỗi script verifier, phản hồi motor, chênh lệch góc, thời gian arrival và an toàn. Không đánh dấu PASS chỉ vì skew giảm do LEFT chậm, hoặc vì đạt SOFT cả hai khi RIGHT trễ. Báo riêng số khung S15/RIGHT tracking thực thi; `M8S00` last-session không phải lịch sử mọi stroke.

**Không đổi trong commit lập milestone:** `Core/`, `tools/`, `stm32...`, công tắc an toàn, Level2, Fast Start, các flag, PWM, NC, STOP, phần mềm logger hoặc dữ liệu log riêng tư. Cần kiểm GitHub Actions đúng SHA tài liệu sau khi commit.

## Implementation S15.2 — Manual angle-relation SHADOW (2026-11-01)

**SOURCE firmware patch only, hardware NOT validated.** `Core/Inc/steer_s152_manual_relation.h` is a pure C observer; `M8S152_Record` runs AFTER post-atomic PWM decision in `Steering_ManualUpdate`, for **D-pad LEFT/RIGHT and joystick RX** regardless of active S14/S15 control. Read-only RAM retains 96 samples at ~50ms or state transitions for the most recent manual stroke, with dropped count and terminal zero-PWM marker; `M8S152_META`, `M8S152_A` and `M8S152_B` print only within the original fully disarmed M8S00 gate. No extra I2C, UART during movement, changes to owner/CCR/STOP/NC/SOFT, or PWM gains. Level2 LEFT450/RIGHT200 and Fast Start boot ON unchanged; RIGHT300 remains only dormant proposal.

**What is compared:** `Steering_AngleFromRaw` maps different LEFT/RIGHT raw MIN/CENTER/MAX arcs to software calibrated +/-4500 centidegrees. `qL=1000*aL/4500`, `qR=-1000*aR/4500` in signed permille; `e=directionL*(qL-qR)` sign aligned to command. `e0` is the first valid BOTH-MOVING error, `dE=e-e0`. This is a **proxy only** for two physically different wheel-angle relations: NOT a measured Ackermann curve or proven real wheel angles.

**Prediction:** distinct timestamps per AS5600 (age<=100ms, inter-axis difference<=60ms, consecutive dt 7–150ms); bounded delta and low-pass signed speed. `ePred=e+0.120*directionL*(vL-vR)`. Tau 120ms is from S14 historical experiment and not calibrated. `predValid=0` on stale, invalid geometry, or non-dual-MOVING. `shadowR` is an illustrative, **never applied** RIGHT PWM candidate: predicted error>20permille, up to +25CCR within current RIGHT active envelope, both distances>180RAW, both MOVING. Not authorized gains or current/clearance evidence. No physical motor change at S15.2.

**Tests:** `tools/test_m8_s152_manual_relation.c` and `tools/verify_m8_s152_manual_relation.py` (pure HOST C, static safety/route checks) added to workflow. CI PASS at exact SHA is separate from STM32CubeIDE Clean Build, ELF SHA/Flash and real suspended hardware A/B; Gate A/B and electrical cut/current/coast/NC clearance remain open. `S15.3` RIGHT-only active tracking is NOT started. S15 scope still stops after S15.4 before VC-02 integration, S15.5 Ackermann deferred.

## S15.2 operator data collection correction (2026-11-01)

**[USER REQUIREMENT] PowerShell-only data transmission / receipt.** Do **not** recommend serial terminal apps, Live Expressions or manual GUI export. Added `tools/capture_m8_s152.ps1`: native `System.IO.Ports.SerialPort`, reads FOUR existing `M8S00` exports with embedded `M8S152` data into ONE TXT, validates markers, four `kind/dirL` pairs, final 0-CCR and run sequence. Operation requires short NUCLEO USER physical press **after PS2 START disarm for each stroke**, so exporter remains behind the existing safe M8S00 disarm gate. The new script **does not change firmware PWM, UART ISR or Flash**; S11 already had a PowerShell COM capture and S00 already had a single-dump PowerShell script. One shell command, no PC apps. See [exact four-run workflow](M8_S152_POWERSHELL_CAPTURE.md).

Static protocol check `tools/verify_m8_s152_capture.py` added to HOST CI; Windows PowerShell native Parser syntax is checked on windows-latest without serial device. Real USB serial/hardware capture not yet verified. This supplements, does not replace, prior S15.2 source/host PASS.
