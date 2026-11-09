**2026-11-09 — First MCU powered-tuning register-output/cutoff evidence (NOT HOLD response PASS):** private UART M9H2B1_HOLD_20261109_172834.txt has3 complete rows, SHA621a5ce87385600314404bc8866662c0102f4df4 seq1/source2 CENTER/profile1. WAIT_DATA at37ms -> HOLDING at109ms -> FAULT reason11(NO_PROGRESS) at1868ms, active0/lease0/faultAxes1. LEFT peakCCR30, acknowledged nonzero output1007ms, final/peak error23RAW (goal1886,raw1909); RIGHT peakCCR/output-time0, peak error3RAW. Firmware cut both on its response guard, not a captured Voice STOP. Driver torque, release timing, current and thermal result are not established. No automatic PWM increase; next required observation is whether LEFT was released promptly or still externally held during the no-progress interval. Docs-only update: keep MCU621a5ce, do not reflash or repeat old static logs for this record.

**2026-11-09 — USER-authorized powered H2-B1 tuning:** USER requests actual continuous two-axis HOLD using existing STOP/NC, without a prerequisite motor-supply disconnect test. New profile1=TUNING permits PWM after fresh eligible BOTH-DONE; POWER_COMMISSIONED remains0 and means current/thermal acceptance is pending, not power locked in profile1. Initial engineering caps LEFT30/RIGHT10CCR (3%/1%), independent PD/slew,200ms reversal blank; no-progress1s and recoverable output-time budget cut BOTH axes on tuning fault, with no auto-resume. No fixed HOLD session expiry. Boot has no target/output. Source/HOST verified locally; new CubeIDE build/flash/physical test still pending. See docs/M9_H2B1_POWERED_TUNING_2026_11_09.md. Older closed-power/pending notes below are dated history; do not demand the rejected disconnect pretest or repeat validated H1/H2-B0/unpowered B1 logs.

**2026-11-08 — H2-B1 MCU uncommissioned integration observed:** private UART firmware `4d004757f48f67c1b1b5192b8020c3e8d196ba9a`, four complete lines across seq1/source2 CENTER and seq2/source3 Voice angle. Both retain two goals beyond5s (5002/5001ms), active1/state2/reason7, commissioned0/lease0, all PWM/peakPWM/appliedMs/everApplied0. This is LIMITED MCU target-retention/telemetry PASS, not powered HOLD, current/thermal/load or fault-injection acceptance. Latest DOCX identifies DC775 geared motors: wheel110rpm x4, TURN LEFT30rpm, TURN RIGHT32rpm; manufacturer/winding/gearbox model and safe stationary current are not specified. See docs/M9_H2B1_FULL_HOLD_2026_11_08.md. Earlier pending/819f635 notes below are dated history.


# M9-H2-B0 — Chính sách giữ góc gần đích, công suất KHÓA (2026-11-08)

> **H2-B1 SOURCE/HOST CI VERIFIED (2026-11-08):** source `ac4fc082167bb13b299cba089bc8998e4d1bd2da`, GitHub Actions [42467841303](https://github.com/Spring-Mustache/control_motor/actions/runs/42467841303) completed SUCCESS. Linux full persistent two-axis controller + production main hooks and all legacy regressions PASS; Windows native PowerShell5.1 parser PASS. Physical commissioning remains0; CubeIDE/MCU/HOLD torque/thermal/load validation pending. A following docs-only HEAD is not a new actuator implementation.

> **2026-11-08 — USER clarified FULL H2-B1 (not H1):** delivery is persistent TWO-axis position HOLD, no3s expiry and no single-axis/short-session substitute. Full C controller + atomic HAL owner route + main idle/PS2/Voice/NC handoff are now SOURCE-STAGED with local strict GCC tests120s(controller)/60s(main hooks) PASS. Physical commissioning remains CLOSED0; PWM caps/gains/reversal timings are unverified, so no powered HOLD is enabled or claimed. GitHub CI on the new source and CubeIDE/MCU validation remain separate. See docs/M9_H2B1_FULL_HOLD_2026_11_08.md.

> **2026-11-08 — USER-REPORTED motor-lead current baseline:** User measured on wires FROM driver TO motor (not driver input). During CENTER travel LEFT0.8–1A and RIGHT≈2A; after stopping meter reads0A on both. Earlier LEFT5A/RIGHT10A were estimates from discussed motor powers80/200W, NOT verified continuous/stall/HOLD ratings. These readings are a running-current baseline, not safe HOLD limits; meter type/bandwidth and thermal/standstill-current envelope remain unverified. MCU firmware stays819f635; H2-B1 powered output has not been implemented.

> **Hiện hành 2026-11-08:** SOURCE/HOST CI PASS; đã có một lượt H2-B0 MCU STATIC INTEGRATION/TELEMETRY PASS tại firmware819f635, 3003ms. Candidate vẫn0; H2-B1 giữ góc bằng mô-men motor chưa thực hiện. Các phần SOURCE/MCU pending bên dưới lưu trạng thái trước kết quả này.

## Căn cứ và giới hạn

Đã xem manual CCS_SHB12 do người dùng cung cấp: driver hỗ trợ PWM/DIR, chỉnh ACCE/DECCE, iLIMIT và tự bảo vệ dòng/nhiệt. Manual cho biết 12A liên tục và 30A đỉnh là khả năng **driver**, không phải giới hạn dòng giữ/stall an toàn của motor đánh lái đang lắp. Chưa đo được nhiệt motor, dòng đứng yên, PWM=0 có brake/coast trong chế độ hiện tại, trễ tác động và cơ chế ngắt nguồn vật lý độc lập. Dữ liệu H1 130 mẫu là motor đang di chuyển bằng M8, không chứng nhận lực giữ lâu.

## Mã đã triển khai H2-B0

- Core/Inc/m9_h2b_hold_policy.h: cấu trúc OFF/MONITOR/CANDIDATE/COMPLETE/REVOKED, error reasons; cổng công suất bị khóa cố định.
- Core/Src/m9_h2b_hold_policy.c: policy C độc lập, chỉ theo phiên H2-A sau M8 DONE hợp lệ. Chỉ tính candidate khi H2-A ở SHADOW và qua STOP/NC/owner/sensor/wheels. Ngưỡng vào 8 RAW, nhả 4 RAW, cắt đề xuất ngược dấu lỗi, chặn vượt phiên/time budget, giới hạn score ±20 và thay đổi tối đa 3 score mỗi lượt. Candidate chỉ là đơn vị thuật toán, không phải PWM.
- Core/Src/main.c: gọi H2B_Begin sau H2A_Begin và H2B_Step sau H2A_Step. Không thêm đọc I2C, không tạo owner motor, không ghi TIM CCR/DIR. Nguồn M8, H0, H1 và H2-A tiếp tục không đổi.
- M9H2B_POWER_STAGE_ENABLED=0, M9H2B_VERIFIED_MOTOR_PWM_LIMIT=0, M9H2B_VERIFIED_ON_TIME_MS=0, kèm tiền xử lý compile #error khi định mở khóa trong khi thiết kế chưa có bảo vệ thật. H2-A vẫn độc lập với M9H2_POWER_PERMITTED=0. Không thể bật PWM HOLD bằng Live Expressions hay script.
- Tận dụng M9H2_GUARD UART3 (safe idle), thêm bState/bReason/bPeakL/bPeakR/bEligibleL/bEligibleR/bVetoL/bVetoR/bBlocked/bCcrL/bCcrR, vẫn giữ dutyL/R=0, powered=0.
- tools/m9_h2b_policy_host_test.c và tools/verify_m9_h2b_policy.py kiểm tra C thật, khi guard revoked, NC/STOP/owner, score ngược dấu, policy hết 3s, các đầu ra motor luôn 0.

## Test SOURCE/HOST và MCU

CI chỉ xác minh build GCC/thử logic, không phải kiểm tra motor. Sau khi CI PASS và bảo vệ các thay đổi local, cập nhật project CubeIDE hiện có; Clean Build, Debug/Flash và Resume. Trên giá an toàn, kiểm tra NC/STOP rồi chạy một Voice CENTER đã được chứng minh an toàn, thu M9H2_GUARD (không làm lại 130 frame H1):

    powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\capture_m9_h2_guard.ps1" -TimeoutSeconds 300 -MaxEvents 1 -OutputDirectory ".\logs"

Yêu cầu firmware SHA mới; bState=3 (COMPLETE), bReason=1, bCcrL=0,bCcrR=0, dutyL=0,dutyR=0,powered=0. Với góc đứng yên gần đích, bPeakL/R=0 có thể đúng; không thể coi đây là đã thử lực ngoại tác.

## H2-B1 (phần ACTIVE thật chưa thực hiện)

Trước khi code đường xuất TIM12/TIM14/DIR cho HOLD cần xác minh motor cụ thể, dòng liên tục và stalled theo nhiệt, nguồn và ngắt nguồn động lực độc lập, trạng thái coast/brake khi PWM=0, cấu hình ACCE/DECCE thực, quyền STOP/NC ISR/PS2/Voice của từng trục. Sau đó mới xác định mức PWM và thời lượng thử với đo dòng/nhiệt và kiểm tra từng trục, rollback khi mất tín hiệu. **Không thay các define khóa thành 1 chỉ để thử.** Cần commit kiểm tra actuator lease riêng sau khi có dữ liệu, không tự suy motor PWM từ score.


## 2026-11-08 — Exact source CI verified

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


## 2026-11-09 — H2-B1 powered tuning source

**2026-11-09 — USER-authorized powered H2-B1 tuning:** USER requests actual continuous two-axis HOLD using existing STOP/NC, without a prerequisite motor-supply disconnect test. New profile1=TUNING permits PWM after fresh eligible BOTH-DONE; POWER_COMMISSIONED remains0 and means current/thermal acceptance is pending, not power locked in profile1. Initial engineering caps LEFT30/RIGHT10CCR (3%/1%), independent PD/slew,200ms reversal blank; no-progress1s and recoverable output-time budget cut BOTH axes on tuning fault, with no auto-resume. No fixed HOLD session expiry. Boot has no target/output. Source/HOST verified locally; new CubeIDE build/flash/physical test still pending. See docs/M9_H2B1_POWERED_TUNING_2026_11_09.md. Older closed-power/pending notes below are dated history; do not demand the rejected disconnect pretest or repeat validated H1/H2-B0/unpowered B1 logs.

Cấu hình thử có thể cấp lực thật, không tương đương nghiệm thu motor. Xem [quy trình hiện hành](M9_H2B1_POWERED_TUNING_2026_11_09.md). Bản cập nhật thay Core/tests/collector nên cần build/flash mới; không dùng log4d004757 để nghiệm thu bản này.


## 2026-11-09 — First powered-tuning MCU log received

**2026-11-09 — First MCU powered-tuning register-output/cutoff evidence (NOT HOLD response PASS):** private UART M9H2B1_HOLD_20261109_172834.txt has3 complete rows, SHA621a5ce87385600314404bc8866662c0102f4df4 seq1/source2 CENTER/profile1. WAIT_DATA at37ms -> HOLDING at109ms -> FAULT reason11(NO_PROGRESS) at1868ms, active0/lease0/faultAxes1. LEFT peakCCR30, acknowledged nonzero output1007ms, final/peak error23RAW (goal1886,raw1909); RIGHT peakCCR/output-time0, peak error3RAW. Firmware cut both on its response guard, not a captured Voice STOP. Driver torque, release timing, current and thermal result are not established. No automatic PWM increase; next required observation is whether LEFT was released promptly or still externally held during the no-progress interval. Docs-only update: keep MCU621a5ce, do not reflash or repeat old static logs for this record.

Số liệu và giới hạn: [báo cáo powered tuning](M9_H2B1_POWERED_TUNING_2026_11_09.md). Chưa có dữ liệu nhả tác động/dòng/nhiệt; không kết luận nguyên nhân hoặc tự tăng cap.
