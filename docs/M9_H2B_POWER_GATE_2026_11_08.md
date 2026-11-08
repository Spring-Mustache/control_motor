
# M9-H2-B0 — Chính sách giữ góc gần đích, công suất KHÓA (2026-11-08)

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
