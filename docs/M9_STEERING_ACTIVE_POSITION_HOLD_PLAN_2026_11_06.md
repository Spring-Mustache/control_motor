# M9 — Steering Active Position Hold (kế hoạch chương trình, 2026-11-06)

> **2026-11-08 — H2-B1 MCU uncommissioned integration observed:** private UART firmware `4d004757f48f67c1b1b5192b8020c3e8d196ba9a`, four complete lines across seq1/source2 CENTER and seq2/source3 Voice angle. Both retain two goals beyond5s (5002/5001ms), active1/state2/reason7, commissioned0/lease0, all PWM/peakPWM/appliedMs/everApplied0. This is LIMITED MCU target-retention/telemetry PASS, not powered HOLD, current/thermal/load or fault-injection acceptance. Latest DOCX identifies DC775 geared motors: wheel110rpm x4, TURN LEFT30rpm, TURN RIGHT32rpm; manufacturer/winding/gearbox model and safe stationary current are not specified. See docs/M9_H2B1_FULL_HOLD_2026_11_08.md. Earlier pending/819f635 notes below are dated history.

> **H2-B1 SOURCE/HOST CI VERIFIED (2026-11-08):** source `ac4fc082167bb13b299cba089bc8998e4d1bd2da`, GitHub Actions [42467841303](https://github.com/Spring-Mustache/control_motor/actions/runs/42467841303) completed SUCCESS. Linux full persistent two-axis controller + production main hooks and all legacy regressions PASS; Windows native PowerShell5.1 parser PASS. Physical commissioning remains0; CubeIDE/MCU/HOLD torque/thermal/load validation pending. A following docs-only HEAD is not a new actuator implementation.

> **2026-11-08 — USER clarified FULL H2-B1 (not H1):** delivery is persistent TWO-axis position HOLD, no3s expiry and no single-axis/short-session substitute. Full C controller + atomic HAL owner route + main idle/PS2/Voice/NC handoff are now SOURCE-STAGED with local strict GCC tests120s(controller)/60s(main hooks) PASS. Physical commissioning remains CLOSED0; PWM caps/gains/reversal timings are unverified, so no powered HOLD is enabled or claimed. GitHub CI on the new source and CubeIDE/MCU validation remain separate. See docs/M9_H2B1_FULL_HOLD_2026_11_08.md.

> **2026-11-08 — USER-REPORTED motor-lead current baseline:** User measured on wires FROM driver TO motor (not driver input). During CENTER travel LEFT0.8–1A and RIGHT≈2A; after stopping meter reads0A on both. Earlier LEFT5A/RIGHT10A were estimates from discussed motor powers80/200W, NOT verified continuous/stall/HOLD ratings. These readings are a running-current baseline, not safe HOLD limits; meter type/bandwidth and thermal/standstill-current envelope remain unverified. MCU firmware stays819f635; H2-B1 powered output has not been implemented.

> **2026-11-08 — H2-B0 MCU STATIC INTEGRATION/TELEMETRY PASS (1 healthy Voice CENTER run):** Private user log `M9H2_GUARD_20261108_151606.txt`, MCU firmware `819f635aafbe9dde3ee0e4b9c5c8e6cbc282e0c2`, seq1/source2. H2-A state6/reason1 and H2-B0 bState3/bReason1 completed 3003ms, 268 guard updates; freshL/R267/268; sensorErr/deniedNC/deniedOwner/blockBits/dropped0; normal VOICE1->IDLE0 accepted. bPeakL/R0 and bEligibleL/R0; bCcrL/R=dutyL/R=0, powered0. This validates one quiet static integration window, NOT nonzero-candidate/disturbance/STOP-NC fault-injection or powered HOLD. H2-B1/H3 remain pending; no firmware changes in this documentation update.

> **USER clarification (2026-11-08):** Động học chưa hoàn thành để tích hợp/nạp vào firmware; SELECT tạm hoãn. Không tự đồng nhất hai trạng thái hoặc coi nội suy RAW của Voice là động học bánh xe hoàn chỉnh.

## 0. Quyết định, nền tảng và ranh giới bằng chứng

- **[USER, 2026-11-06]** Không muốn đầu tư thêm phần cứng; ưu tiên xây dựng chương trình giữ góc/trục lái bằng mô-men của motor DC hiện tại, hai AS5600, STM32F767ZI và driver CCS_SHB12.
- **[USER, 2026-11-06]** Yêu cầu tạo milestone **M9** trên nhánh riêng `feature/m9-steering-active-position-hold`, xuất phát **chính xác từ** `d367618d4d9c4c382a9a7208f9b878dfefa21ace`. Nhánh đã được tạo và kiểm tra HEAD đúng SHA nền trước cập nhật tài liệu.
- **[USER, 2026-11-06]** Yêu cầu cập nhật các file Markdown về tiến độ/lộ trình. Commit tài liệu M9 **không** phải triển khai firmware và **không** xác nhận H0/H1/H2/H3 PASS.
- **[REPO/M8]** Source có bộ điều khiển vị trí đưa LEFT/RIGHT đến mục tiêu; khi hoàn thành các nhánh `Steering_PositionFinish()` / `Steering_PositionAxisDone()` ngắt PWM của trục liên quan. Chưa có bộ servo HOLD chủ động sau hoàn thành. `Steering_PositionTelemetryIdleUpdate()` lấy mẫu idle luân phiên khoảng 50 ms nên mỗi bên khoảng 100 ms ở chế độ không bị chặn; chưa được chứng nhận là tần suất đủ cho HOLD.
- **[REPO/M8]** AS5600 12-bit raw, LEFT CENTER mặc định 1886, RIGHT CENTER 662; endpoint thực tế có thể bị SELECT homing thay thế runtime; `STEER_POS_TOL_RAW=5`, LEFT position PWM 175..300, RIGHT 50..100, RIGHT fine 85CCR/12ms, tùy state. Giá trị trên là thông số **điều khiển vị trí M8**, tuyệt đối **không** dùng làm giới hạn HOLD mặc định.
- **[EVIDENCE]** Người vận hành đã ghi nhận PASS S15.4/S15.6 Voice 15°/30°/40° LEFT/RIGHT tại firmware `9db12228f38c1589bc6229409e3e210f598561ed`, peak-gap 28..56‰, tương đương 94.4..97.2% worst-point progress, đủ hai target và RIGHT zero fine pulses. Không suy ra thử 45°/NC/STOP/có tải.
- **[PENDING M8]** Trial RIGHT CENTER 10 RAW tại `d367618d` (PS2 R1 và Voice B0) có CI pass ở bản nền, **chưa có** xác nhận CubeIDE Build/Flash/giảm hunting trên phần cứng. M9 kế thừa đúng source này, không gọi R1 10 RAW là đã nghiệm thu và không âm thầm sửa hành vi M8.

## 1. Lý do cần bộ điều khiển

Motor DC chổi than **có thể tạo mô-men đứng yên khi cấp dòng**, gần đúng `T=Kt*I`, nhưng dòng/PWM một chiều cố định **không** khóa được trục trước ngoại lực hai chiều. Nếu tải biến mất khi còn mô-men bù, bánh có thể vượt góc, bật lại hoặc dao động. M9 thiết kế một vòng **servo giữ vị trí chủ động**: đo vị trí và tốc độ AS5600, điều chỉnh mô-men theo phản hồi, giảm lực nhanh khi bánh đang quay ngược chiều/tiệm cận mục tiêu. Đây không phải khóa cơ khí khi mất nguồn.

CCS_SHB12 hỗ trợ PWM/DIR, có bảo vệ giới hạn dòng/ACCE-DECCE; manual chưa chứng minh có lệnh BRAKE hoặc closed-loop current/torque riêng, cũng chưa xác định chắc PWM=0 dẫn đến brake hay coast. Không dùng `ENA` như chân BRAKE. Giữ trục khi stalled có thể gây `I²R` và nóng; điện áp/PWM **không bằng** số đo mô-men/dòng thực tế.

## 2. Kiến trúc phần mềm dự kiến

```text
PS2 / VC-02 / position goal
          |
     M8 position controller ------ DONE hợp lệ + mục tiêu giữ
          |                                   |
          |                              M9 hold supervisor
          |                      AS5600 L/R -> e, tốc độ, PD
          |                                   |
          +---------- motor owner / safety arbitration -------+
                                                               |
                                        giới hạn PWM/DIR, timeout, NC/STOP
                                                               |
                                                   CCS_SHB12 LEFT / RIGHT
```

Đây là **kiến trúc dự kiến**, chưa có mã. Hợp nhất quyền cấp lệnh tại motor arbitration; HOLD không được ghi CCR/DIR tranh chấp Position, Manual, Voice, SELECT, NC recovery, STOP, hoặc bất kỳ fault latch hiện hành. Phải xét **mỗi trục** riêng khi một trục DONE còn trục kia đang MOVING; chỉ cấp HOLD khi quyền được bàn giao hợp lệ. Mới vào nguồn / DISARM / STOP / không có mục tiêu đã hoàn thành => HOLD không tự bật.

**State machine đề xuất:** `HOLD_OFF` (mặc định boot), `HOLD_MONITOR` (read-only), `HOLD_SHADOW` (tính lệnh nhưng không cấp), `HOLD_ACTIVE` (opt-in và đủ liên động), `HOLD_FAULT` (PWM0 + latch). Chuyển từ OFF/FAULT sang ACTIVE phải có điều kiện rõ, không auto-arm sau reset/lỗi.

**PD ban đầu, chưa có tích phân:**

```text
error_raw = shortest_valid_calibrated_position_error(target_raw, measured_raw)
velocity_raw_s = filtered_delta_AS5600_over_measured_dt
u_candidate = Kp * error_raw - Kd * velocity_raw_s
u = limit_and_guard(u_candidate, duty_cap, direction, age, timeout, mode)
```

Cần xử lý wrap-around 0..4095 AS5600, hướng tăng/giảm encoder từng cơ cấu, dải hiệu chuẩn hợp lệ, lọc vi phân nhiễu, deadband/hysteresis và vùng thả lực. Không suy `RAW` là độ bánh lái khi chưa biết tỷ số cơ cấu. `Kp`/`Kd`, chu kỳ lấy mẫu, PWM cap, tốc độ đổi lệnh, nhiệt/duty budget và thời gian giữ **đều TBD dựa trên đo thực**; không đưa ra một trị số tùy ý làm cấu hình chạy motor. Có thể cần giới hạn tốc độ **tăng** mô-men nhưng ưu tiên thoát lực nhanh an toàn khi sensor chứng tỏ trục đang văng.

## 3. Lộ trình và tiêu chí nghiệm thu

| Giai đoạn | Thực hiện | Cổng chấp nhận | Trạng thái |
|---|---|---|---|
| **M9-H0 — Passive drift baseline** | Source post-DONE 3s log AS5600 cached goal/start/end, peak drift/error/velocity, sample/invalid/jump, owners/safety termination; báo một M9H0_DRIFT line khi UART idle; không thêm I2C/PWM | GCC unit + wiring verifier và CI SHA; CubeIDE Build/Flash/log MCU cần bằng chứng riêng | **SOURCE-STAGED; target/hardware pending** |
| **M9-H1 — HOLD PD shadow** | Pure-C PD SHADOW Kp·e−Kd·v, velocity window≥100ms, lọc 1RAW, hysteresis/stale/jump, ±40 shadow units (không PWM), UART idle summary | GCC HOST synthetic release-load + no motor writes + PowerShell syntax PASS on exact CI SHA `7d8c98e` | **SOURCE + HOST CI PASS; target/real log pending** |
| **M9-H2 — Bounded active hold** | **H2-A** đã thêm pure-C guard OFF/MONITOR/SHADOW/FAULT/COMPLETE, post-DONE goal transfer, stale/NC/STOP/owner/wheel monitoring, near-goal PD shadow & safe-idle UART; **H2-B** là bench power owner/PWM hạn chế sau kiểm chứng điện | H2-A GCC/CI exact-SHA và MCU status cần xác minh riêng; H2-B current/thermal/driver brake-coast, nguồn ngắt độc lập + NC/STOP + tải ngoài | **H2-A SOURCE-STAGED, PWM HARD-LOCKED=0; H2-B powered chưa được phép** |
| **M9-H3 — Integration** | Phối hợp HOLD với R1/Voice CENTER, lệnh vị trí Voice5..40, manual PS2, VC-02 và wheel drive; phân tích NC, R2/Voice45, SELECT, STOP, mất nguồn | Nghiệm thu giá treo trước; bài thử có tải riêng, hồi quy M8 và xác minh không phát xung bất ngờ khi STOP/NC hoặc khi lệnh lái mới | **PLANNED** |

Mỗi H0/H1/H2/H3 phải phân biệt `SOURCE-STAGED -> HOST CI PASS -> CubeIDE BUILD PASS -> FLASHED -> HARDWARE PASS bench -> HARDWARE PASS loaded`; không nhảy cổng. Đối với H0/H1 **không được** báo `HARDWARE HOLD PASS` vì chưa hề tác động lực.

## 4. Bất biến an toàn và điều kiện chặn ACTIVE

1. **STOP, mất liên lạc/neutral, NC, error latch, sensor invalid/stale, chuyển quyền điều khiển, khởi động và DISARM** có mức ưu tiên cao hơn HOLD, cắt PWM theo đường ngắt hiện có, không giữ lại xung cuối.
2. Không cố áp lực vào **endstop NC**; lệnh R2 hoặc Voice45 kết thúc tại NC phải tắt HOLD (trừ khi sau này có đặc tả riêng đã nghiệm thu). Không dùng việc DONE hoặc PWM0 để suy chắc driver đang short-brake.
3. H0/H1 không được tự kích hoạt motor. H2 bắt buộc **opt-in boot OFF**, hạn chế lực và liên động fault cứng. Nếu không có phép đo dòng thật, giới hạn PWM/thời gian chỉ là biện pháp giảm rủi ro, **không** chứng minh an toàn nhiệt hoặc dòng stalled.
4. Phải điều tra tác động ACCE/DECCE của CCS_SHB12 lên độ trễ giảm mô-men, điều kiện motor nóng/stall, play/backlash, AS5600 gắn trên trục nào và thực tế PWM0/dir khi driver enable. Không tự tuyên bố driver có brake mode.
5. Thử powered cần gá cố định, tay/chân ngoài vùng cơ cấu, thử nhỏ từng trục, preflight NC/STOP khi ngắt nguồn động lực, chuẩn bị công tắc cắt **nguồn motor độc lập**. Lệnh thử không được làm vượt hành trình hoặc làm chạm NC với tốc độ.
6. Không đăng private UART/CSV/TXT người dùng lên repository public; tài liệu chỉ ghi kết luận được phép công khai. Giữ nguồn rollback đã nghiệm thu, không force-update nhánh M8.

## 5. Công việc ngay sau khi chốt tài liệu M9

- **Đầu tiên:** rà soát cách motor owner ngắt PWM tại `Steering_PositionFinish`/`AxisDone`, luồng idle AS5600, trạng thái NC/STOP, tính thật của driver khi PWM=0. Viết đặc tả dữ liệu/điều kiện H0 từ code, chưa phát xung HOLD.
- **Kế đó:** thực hiện H0 *read-only* trên branch M9, unit/actual-C verifier; chỉ tạo telemetry lúc đường UART idle, không làm tăng tải thời gian điều khiển.
- **Sau H0:** có log drift/response và thống nhất tiêu chí thì xây H1 shadow PD, sau đó mới quyết định H2 powered và H3 tích hợp.
- Mỗi commit thay code/tools phải cập nhật **cả** `CONTEXT_HANDOFF.md` 17 mục và `docs/PROJECT_MILESTONES.md`, đúng CI SHA; giữ đặc tả nguyên bản 386 dòng. Không yêu cầu Release cho mỗi commit.

## 6. Câu hỏi mở cần kiểm tra bằng dữ liệu, không tự gán kết quả

- PWM0 của CCS_SHB12 là coast, short-brake, hay trạng thái khác? Mức độ trễ do ramp ACCE/DECCE?
- Dòng stalled và nhiệt của motor ở tải thực, dòng tối đa an toàn liên tục, khả năng driver giới hạn dòng ở vùng HOLD? **Chưa đo**.
- AS5600 đo trực tiếp góc đầu ra hay trước độ rơ cơ khí; mức 5 RAW bằng bao nhiêu độ bánh lái thực?
- Mức ngoại lực/độ lệch cho phép, độ văng sau khi tải bỏ đột ngột, tần suất và độ trễ vòng lặp phù hợp? **Chưa đo**.
- M8 R1/Voice CENTER hunting trial 10RAW xuất phát d367 chưa có xác nhận powered improvement; phải tách riêng với kết quả M9.

**Cập nhật 2026-11-06 sau dữ liệu người dùng:** H0 SOURCE + HOST CI PASS và người dùng đã gửi 4 UART MCU log đầy đủ (mỗi bên peak drift ≤1 RAW trong 3s, sensor invalid=0). **H0 DATA CAPTURE PASS giới hạn tĩnh**, không chứng minh giữ góc trước tải. Bộ đo thực tế ~89Hz/trục do vòng safety hiện có đọc 2 AS5600, ngoài idle poll 50ms luân phiên; đỉnh vận tốc 90 RAW/s có thể chỉ 1 RAW/11ms, nên H1 phải lọc derivative. Đọc `docs/M9_H0_PASSIVE_DRIFT_2026_11_06.md`. CubeIDE Build/Flash log độc lập chưa có; H1/H2/H3 chưa code, **không có HARDWARE HOLD PASS**.

## 2026-11-06 — Cập nhật triển khai H1 SHADOW

- Module `m9_h1_shadow.c/.h` đã được ghép vào cửa sổ H0 post-DONE, không thay vòng vị trí hoặc quyền motor.
- Bộ lọc vận tốc theo chênh lệch >=100ms, <2RAW coi là không chuyển động; position hysteresis 8/4RAW, effective deadband 5RAW; stale sau 250ms và jump >90RAW bị đánh dấu và loại.
- Cặp tham số `Kp_q8=256`, `Kd_q8=128`, ±40 shadow units là **giả định mô phỏng để kiểm thử**, không được chuyển sang CCR, mô-men hoặc dòng điện. Chưa đủ chứng cứ xác định gain giữ góc thật.
- `M9H1_SHADOW` xuất log chỉ sau khi H0 kết thúc và mọi motor/UART idle; no motor write/no extra I2C. Simulated release-load in GCC không chứng minh an toàn đẩy ngược trên xe.
- H2 ACTIVE vẫn chưa được thực hiện; chỉ nghiên cứu tiếp khi có H1 on-MCU data và đánh giá các cổng current/thermal/NC/STOP. Xem `docs/M9_H1_PD_SHADOW_2026_11_06.md`.

### Cập nhật chứng cứ M9-H1 — 2026-11-07

Người dùng cung cấp 4 cặp UART H0/H1 trên MCU `sha=00ccaeace99a6948880ecf85061dea5d2f77514e`: 3 Voice vị trí và 1 Voice CENTER, mỗi window 3000–3010ms, peak drift≤1RAW/trục, H1 peakVel0, peakCmd0, noise-suppressed tổng 28, không có error/stale/jump/drop. **H1 SHADOW static on-MCU data capture PASS giới hạn**; các err đo |e|≤6RAW thấp hơn ngưỡng engage8RAW nên không tạo candidate. Vì vậy chưa có bằng chứng motor damping/load release hoặc giữ góc khi có ngoại lực. H2 ACTIVE vẫn PENDING; `docs/M9_H1_PD_SHADOW_2026_11_06.md` chứa bảng kết quả.

## 2026-11-07 — Quyết định đo động học H1 bằng motor thật

Theo yêu cầu người dùng, H1 bổ sung `M9H1_REAL_BEGIN/FRAME/END` khi *M8 thực sự lái bánh* (không injection AS5600/sim); raw, error, filtered velocity, shadow candidate và actual M8 CCR/time/NC được lưu vào RAM và gửi chỉ khi an toàn đứng yên. Một script `tools/capture_m9_h1_real.ps1` thu bốn lệnh trong cùng một lần. Chờ log MCU thật và phân tích đáp ứng trước khi đổi thông số PD. H2 vẫn chưa cấp lực, không có H1 loaded release test. [Hướng dẫn](M9_H1_REAL_DRIVE_CAPTURE_2026_11_07.md).

### 2026-11-07 — Rút ra từ 130 mẫu chuyển động thật M8

4/4 lệnh REAL motor pass result2, 130 frames 84ms, exact SHA `3dbdfaf`, AS5600 ages≤1ms, no overflow/stale/bad. M8 đạt sai số cuối ≤5RAW/trục; `u_shadow` saturate±40 trên nhiều mẫu và có thể đề xuất hãm khi còn e≈150–180RAW trong lúc M8 còn PWM cao. Do vậy trước H2 không chuyển trực tiếp `Kp256/Kd128/cap40` thành motor PWM; phải thiết kế riêng hold-only gating, giới hạn dòng/nhiệt, NC/STOP và đánh giá ngoại lực. H0 reason3 lượt3 sau1180ms chưa xác định trigger cụ thể. [Báo cáo](M9_H1_REAL_DRIVE_CAPTURE_2026_11_07.md).

## 2026-11-07 — H2-A đã lập trình; chưa có H2-B cấp lực

Theo yêu cầu thực hiện H2, tách hai cổng kỹ thuật để không biến dữ liệu H1 thành dòng giữ trục không kiểm soát: **H2-A guard supervisor** và **H2-B active motor commissioning**. Mã H2-A có `Core/Inc/m9_h2_hold_guard.h`, `Core/Src/m9_h2_hold_guard.c` (pure C), wiring `Core/Src/main.c`, GCC verifier, một dòng UART `M9H2_GUARD` cho mỗi cửa sổ sau DONE và PowerShell `tools/capture_m9_h2_guard.ps1`.

H2-A giữ quy tắc: hai trục DONE2 (R1 CENTER hoặc Voice5..40), no NC/STOP, goal hợp lệ; mỗi trục sensor valid/fresh ≤250ms, reject RAW jump, speed 100ms, near-goal deviation ≤32RAW, STOP/mode/owner/NC/wheels đang chạy => mất quyền giám sát hoặc FAULT, post-DONE window ≤3000ms. Tính shadow score riêng `Kp_Q8=128`, `Kd_Q8=32`, score cap ±20 **không phải motor PWM**; khác H1 transit `Kp256/Kd128` đã thấy hãm sớm. `M9H2_POWER_PERMITTED=0U` được hardcode, `dutyL=0`, `dutyR=0`, `powered=0` trên mọi đường đi, bench request không thể bật motor. H2 KHÔNG thay M8 servo, motor arbiter hoặc NC ISR.

**Cổng H2-B còn khóa**: chưa đo dòng stalled/nhiệt motor đánh lái, PWM=0 driver CCS_SHB12 coast/brake, độ trễ ACCE/DECCE, DIR khi đổi lực, giới hạn duty/duration từ dữ liệu thực. Driver manual có bảo vệ over-current/over-temp chung nhưng không chứng nhận an toàn dòng đứng yên của motor này. Phải có gá và nguồn motor ngắt độc lập, powered test từng trục sau review. Chưa đủ quyền kỹ thuật để tự gán cap vật lý. Đọc `docs/M9_H2_GUARD_SOURCE_2026_11_07.md`.

### 2026-11-07 — H2-A SOURCE/HOST CI kết quả xác minh

Source `55db22f67c81517ada48106f5a506b7b367b7e4c` Actions `42370515934` PASS (C-core hard power-off/STOP/NC/owner/stale plus Win PowerShell syntax). Không có target CubeIDE/flash/H2 MCU log nên **H2-A hardware pending**; không có H2-B Active PWM. Bản đầu `b9b75a` Win parser fail dấu `:` sau tên biến rồi đã sửa, không phải lỗi motor.

### 2026-11-08 — H2-A sửa VOICE → IDLE sau DONE

MCU SHA8214610 source3 H2_GUARD abort 13ms reason3/blockBits256, duty0, powered0, sensorErr0/deniedNC0. Source kiểm tra xác định điều kiện so sánh `control_mode` tuyệt đối: VOICE lúc DONE sau đó IDLE mặc định. Sửa riêng normal-mode handoff trong pure-C/MAIN, chỉ cho phép khi Voice/PS2 không có công việc mới và các cổng STOP/NC/owner/motion/sensor vẫn nguyên; UART log thêm modeStart/modeNow/modeOK. Chưa có 3s H2-A hardware PASS; H2-B vẫn không cấp motor. File khác H0_DRIFT bị ghép với M8R1_FINE không đủ để kết luận nguyên nhân UART. [Chi tiết](M9_H2_GUARD_SOURCE_2026_11_07.md).

### 2026-11-08 — H2-A guard 3s one-run MCU PASS

Firmware `5dd1502` đã chạy một lệnh Voice CENTER source2, cả H0/H1/H2 quan sát 3000ms, H2 reason1 state6 updates294, modeStart1->modeNow0 modeOK1, blockedOwner/NC0, sensorErr0, dutyL/R0 powered0. Bình thường VOICE->IDLE được chấp nhận, không còn abort sớm. Bằng chứng này **chỉ** xác nhận supervisor tĩnh một lượt; chưa chứng minh lỗi/ngoại lực trên thực địa hay giữ góc bằng lực motor. H2-B vẫn cần kiểm tra current/thermal/driver PWM0 và bảo vệ vật lý trước khi lập trình ACTIVE output. [H2 real UART summary](M9_H2_GUARD_SOURCE_2026_11_07.md).


## 2026-11-08 — H2-B0 candidate SOURCE, physical power hard-locked

- USER yêu cầu tiến hành H2-B sau khi H2-A 3s trên MCU PASS (source firmware 5dd1502). Source mới xây pure C bounded near-goal policy m9_h2b_hold_policy.c/.h, liên kết H2-A guard cùng seq/timestamp; hysteresis 8/4 RAW, score cap 20 arbitrary, slew 3/tick, reject sign away from goal; STOP/NC/owner/sensor/timeout trúng guard thì policy hủy.
- Không có H2 GPIO/DIR/CCR write. Compile hard locks M9H2B_POWER_STAGE_ENABLED=0, verified motor PWM0 và on-time0 cùng H2-A PERMITTED0. Candidate score tuyệt đối không có đơn vị PWM/torque. Các bản ghi M9H2_GUARD mới gồm bState/bReason/bPeak/bEligible/bVeto/bBlocked/bCcr nhưng powered/duty vẫn0.
- Manual CCS_SHB12 uploaded by USER ghi board 12A continuous/30A peak, iLIMIT and overcurrent/overtemperature, tùy chỉnh ACCE/DECCE; đây KHÔNG phải đặc tính stall/thermal an toàn của motor DC riêng. PWM0 coast/brake, motor current/temp và cắt nguồn độc lập chưa xác minh.
- Test source tools/m9_h2b_policy_host_test.c + tools/verify_m9_h2b_policy.py với GitHub Actions strict Linux GCC và Windows PS parser. MCU H2B0 3s log cần Build/Flash sau exact CI; không phải H2-B1 powered PASS.
- H2-B1 active motor duty/wiring cần thêm dữ liệu vật lý, STOP/NC owner safety + current/thermal commissioning; chưa code bật công suất. Tài liệu docs/M9_H2B_POWER_GATE_2026_11_08.md.

**M9 H2-B0 SOURCE CI PASS 2026-11-08:** Firmware commit 9e4f4533b9c7de820295f9c36ceab28541048dc0, GitHub Actions 42460572191 completed SUCCESS: Linux GCC exact-C H2-B0 candidate + hard-power-lock verifier and H0/H1/H2-A regressions; Windows PowerShell 5.1 syntax PASS. Target CubeIDE Build/Flash/H2B0 MCU log remains PENDING. H2-B1 powered hold not implemented (unknown motor stall current/temp, driver PWM0 coast/brake and independent cutoff). Docs-only HEAD after this report is not new firmware.


## 2026-11-08 — H2-B0 on-MCU: one healthy static 3003ms window PASS

Nguồn: file UART riêng tư do người dùng gửi, `M9H2_GUARD_20261108_151606.txt`. Đã kiểm tra đủ ba dòng H0/H1/H2, cùng firmware `819f635aafbe9dde3ee0e4b9c5c8e6cbc282e0c2`, `seq=1`, `source=2` (Voice CENTER). Chỉ công khai kết quả tổng hợp này; không đưa TXT gốc vào repository.

| Hạng mục | Kết quả quan sát |
|---|---|
| Phiên H2-A | COMPLETE state6, reason1; 3003ms; 268 updates; fresh L/R267/268 |
| H2-B0 | COMPLETE bState3, bReason1; bPeak L/R0, bEligible L/R0, bVeto L/R0, bBlocked0 |
| Quyền / cảm biến | deniedOwner0, deniedNC0, sensorErr0, blockBits0; modeStart1 -> modeNow0, modeOK1, voiceBusy0 |
| Đầu ra HOLD | bCcrL/R0, dutyL/R0, powered0, shadowOnly1, dropped0 |
| H0 đối chiếu cùng phiên | 3003ms; peak drift1/1RAW; samples268/269; bad/jump/drop0; wheelsMoved0 |
| H1 đối chiếu cùng phiên | error cuối5/2RAW; filtered peak velocity0/0, peak command0/0; noise suppressed7/0; bad/stale/jump/drop0 |

**Kết luận:** H2-B0 **MCU STATIC INTEGRATION/TELEMETRY PASS giới hạn một lượt Voice CENTER khỏe mạnh**. H0/H1/H2 khớp phiên; bình thường VOICE->IDLE không hủy guard, policy kết thúc đủ cửa sổ và các giá trị đầu ra HOLD bằng0. UART chứng minh firmware đã chạy trên MCU tại SHA nêu trên; phiên này không cung cấp riêng transcript lệnh CubeIDE Build/Flash.

**Giới hạn:** H2 peak error6/3RAW, thấp hơn ngưỡng engage8RAW; không xuất hiện candidate khác0. Vì vậy chưa kiểm chứng thực tế ngưỡng vào/nhả, chặn sai dấu, phản ứng ngoại lực/nhả tải, fault-injection STOP/NC/sensor hoặc mô-men giữ. Những bài HOST đã có vẫn là bằng chứng phần mềm riêng. Không mở khóa PWM hoặc suy an toàn điện/nhiệt từ cửa sổ tĩnh.

**Điểm tiếp tục:** Không yêu cầu người dùng lặp H0/H1 130 frame hay chính lượt CENTER này. H2-B1 powered và H3 integration còn chờ đặc tính motor current/thermal/stall, CCS_SHB12 PWM0 coast/brake/ACCE-DECCE và thiết kế cấp quyền actuator có STOP/NC/cắt nguồn phù hợp. Bản cập nhật này chỉ sửa tài liệu; firmware đã chạy vẫn là `819f635`, không yêu cầu build/flash lại vì commit docs mới.

**USER clarification (2026-11-08):** Động học chưa hoàn thành để tích hợp/nạp vào firmware; SELECT tạm hoãn. Không tự đồng nhất hai trạng thái hoặc coi nội suy RAW của Voice là động học bánh xe hoàn chỉnh.


## 2026-11-08 — Motor-lead current baseline and H2-B1 bench design

**Nguồn:** người dùng báo số đo trực tiếp trong chat, xác nhận vị trí đo là dây từ driver ra motor. Không phải dòng nguồn cấp driver và không phải telemetry dòng do STM32 ghi.

| Trạng thái được người dùng mô tả | LEFT | RIGHT |
|---|---:|---:|
| Đang chạy về CENTER | 0,8–1 A | khoảng 2 A |
| Sau khi đến đích, đã dừng | đồng hồ đọc 0 A | đồng hồ đọc 0 A |

**Phân loại dữ liệu:** số đo do USER cung cấp, chưa có tệp đo theo thời gian, model/băng thông đồng hồ, giá trị peak/RMS hoặc nhiệt độ. Chưa thể coi là dòng khởi động đỉnh, dòng stall hoặc dòng HOLD liên tục an toàn. Các mức 5A trái/10A phải trước đó chỉ được ước lượng từ công suất80/200W đang thảo luận; không lưu chúng như rating dòng của nhà sản xuất. Driver12A là thông số khả năng driver trong manual; chưa xác minh ngưỡng bảo vệ/iLIMIT thực trên từng board.

**Ý nghĩa đối với M9:** đã có baseline dòng chạy trên motor để đối chiếu bài thử sau. Đồng hồ đọc0A khi dừng phù hợp firmware819f635 không cấp PWM HOLD; riêng quan sát này không xác định PWM0 là coast hay short-brake. Không suy PWM thành ampere hoặc cho rằng LEFT1A/RIGHT2A là giới hạn giữ lâu. Firmware hiện không có đường phản hồi dòng/nhiệt từ đồng hồ; giới hạn PWM/thời gian không được gọi là giới hạn dòng thực.

### Phần thiết kế H2-B1 có thể tiếp tục từ source đã kiểm tra

- B1 bench phải thử riêng từng trục; bốn bánh và trục không được thử giữ đầu ra0; boot OFF, một yêu cầu kích hoạt có chủ đích và phiên hữu hạn. Cơ chế kích hoạt, PWM cap, on-time và duty/thermal budget chưa chốt; không thay khẩu lệnh49 byte hoặc nút PS2 đang dùng mà không có đặc tả.
- Source `Steering_PositionFinish()` tắt cả hai CCR trước khi gọi H2; `Control_Arbitrate()` ở IDLE gọi `Turn_Motors_Stop()`, còn `M9H2_IdleUpdate()` hiện yêu cầu cả hai CCR0. Vì vậy H2-B1 cần thiết kế quyền actuator riêng có giám sát, không nối thẳng score vào TIM và không bỏ điều kiện chủ sở hữu của H2-A để lách gate.
- NC ISR `Steering_SafetyOnLimitInterrupt()` đang cắt trực tiếp TIM12_CH2 hoặc TIM14_CH1 theo trục; STOP, fault, sensor stale/invalid, NC, chủ sở hữu mới và hết phiên phải hủy quyền HOLD, ngăn superloop cấp lại sau cắt. M8 Position/Manual/SELECT/Voice45/R2 giữ quyền ưu tiên hiện có.
- Bài thử đầu cho phần cấp lực cần mục tiêu là đo dòng khi HOLD thực sự hoạt động, trên gá, giới hạn phiên và từng trục; chưa đưa ngoại lực/nhả tải vào bài đầu. Mức thử dựa đặc tính motor/driver và phép đo thực, không lấy arbitrary score20 hoặc dòng chạy vừa báo làm giới hạn an toàn.
- Phản ứng ngoại lực/nhả tải và nhiệt độ dưới thời gian giữ dài hơn là nghiệm thu tiếp theo; phối hợp khi xe chạy thuộc H3. Không đánh dấu HOLD/thermal/load PASS từ baseline dòng chạy.

**Trạng thái:** đây là cập nhật dữ liệu và thiết kế bài thử, KHÔNG phải firmware H2-B1 đã triển khai hoặc được phép cấp điện. Không thay Core/tools/driver setting. Firmware đang chạy vẫn819f635; không yêu cầu build/flash hoặc lặp130 frame H1 vì commit docs này.

## 2026-11-08 — Phạm vi và source H2-B1 liên tục hai trục

**2026-11-08 — USER clarified FULL H2-B1 (not H1):** delivery is persistent TWO-axis position HOLD, no3s expiry and no single-axis/short-session substitute. Full C controller + atomic HAL owner route + main idle/PS2/Voice/NC handoff are now SOURCE-STAGED with local strict GCC tests120s(controller)/60s(main hooks) PASS. Physical commissioning remains CLOSED0; PWM caps/gains/reversal timings are unverified, so no powered HOLD is enabled or claimed. GitHub CI on the new source and CubeIDE/MCU validation remain separate. See docs/M9_H2B1_FULL_HOLD_2026_11_08.md.

**Nguồn hiện hành:** [H2-B1 full HOLD](M9_H2B1_FULL_HOLD_2026_11_08.md). Các đề xuất bench từng trục/phiên ngắn phía trước được giữ làm lịch sử, không phải phạm vi bàn giao đã chốt. H2-B0 và guard3s vẫn là quan sát cũ riêng; H2-B1 không dùng timer đó để kết thúc mục tiêu.



## 2026-11-08 — MCU H2-B1 chưa commissioning và nhận diện motor

**2026-11-08 — H2-B1 MCU uncommissioned integration observed:** private UART firmware `4d004757f48f67c1b1b5192b8020c3e8d196ba9a`, four complete lines across seq1/source2 CENTER and seq2/source3 Voice angle. Both retain two goals beyond5s (5002/5001ms), active1/state2/reason7, commissioned0/lease0, all PWM/peakPWM/appliedMs/everApplied0. This is LIMITED MCU target-retention/telemetry PASS, not powered HOLD, current/thermal/load or fault-injection acceptance. Latest DOCX identifies DC775 geared motors: wheel110rpm x4, TURN LEFT30rpm, TURN RIGHT32rpm; manufacturer/winding/gearbox model and safe stationary current are not specified. See docs/M9_H2B1_FULL_HOLD_2026_11_08.md. Earlier pending/819f635 notes below are dated history.

Thông số motor theo DOCX mới: DC775 giảm tốc, bánh110rpm x4, láiLEFT30rpm, RIGHT32rpm. Chưa có hãng/mã cuộn dây/hộp số hoặc giới hạn dòng đứng giữ. `dropped=1` ở seq2 là trạng thái chờ bị thay thế khi Begin phiên mới theo adapter, không phải bộ đếm lỗi cảm biến. Bản ghi tổng hợp và giới hạn: [H2-B1 full HOLD](M9_H2B1_FULL_HOLD_2026_11_08.md). Không thay source hoặc cần nạp lại cho commit tài liệu này.
