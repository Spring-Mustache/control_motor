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
| **M9-H0 — Passive drift baseline** | Chỉ ghi góc mục tiêu/thực, error, tốc độ, tuổi mẫu, state owner, NC, STOP, thời điểm motor PWM về 0 và độ trôi sau DONE; báo log ở UART idle, không thêm PWM | HOST verifier nguồn thực và CI đúng SHA; xác nhận dữ liệu không làm thay đổi timing/owner/motor; nếu làm test vật lý thì log riêng tư nhận được | **PLANNED** |
| **M9-H1 — HOLD PD shadow** | Tính `Kp*e-Kd*v`, lọc nhiễu, saturate, anti-windup bằng không dùng I; phân tích trường hợp thả tải đột ngột trong unit/simulation; không ghi CCR/DIR | HOST C tests: vượt góc, đảo lực, stale, dt=0, sensor jump, wrap, NC, STOP, owner priority; H1 thực tế vẫn PWM0 | **PLANNED** |
| **M9-H2 — Bounded active hold** | Opt-in duy nhất sau hoàn thành goal hợp lệ, PWM/DIR hạn chế theo trục; giám sát phản hồi và duration/duty budget, hủy lực khi lỗi/STOP/NC/owner change | CI + CubeIDE Clean Build + flash đúng SHA; bench **có bảo vệ/ngắt nguồn motor độc lập**; xác minh đầu ra CCS_SHB12 PWM0/brake-coast, current/thermal nếu đo được, từng trục trước; test tải và nhả tải đột ngột có quan sát overshoot/reversal | **PLANNED, chưa được phép coi là thử motor** |
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

**Nhánh M9 tại thời điểm lập kế hoạch:** chỉ có commit tài liệu; *không có HOLD firmware, không có Build/Flash và không có hardware HOLD PASS*.
