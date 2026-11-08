
# M9-H2-B0 — Chính sách giữ góc gần đích, công suất KHÓA (2026-11-08)

> Tình trạng: đã lập trình source H2-B0, cần xác minh GitHub CI và build STM32 riêng. **Không phải H2-B1 giữ góc bằng mô-men motor.** Mọi đầu ra motor do H2-B0 đều bằng 0.

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

