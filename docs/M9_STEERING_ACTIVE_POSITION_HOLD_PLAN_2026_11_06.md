# M9 — Steering Active Position Hold (kế hoạch chương trình, 2026-11-06)

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
