# M8 — VC-02 I1: hiện thực lệnh bánh và lái độc lập, trước S15.4 đồng bộ tốc độ

**2026-11-04.** Phạm vi người vận hành đã duyệt trong docs/M8_S154_VC02_INDEPENDENT_COMMAND_CONTRACT_2026_11_04.md. Code triển khai theo SOURCE, không tự coi GitHub CI HOST là target CubeIDE, flashed firmware, thử motor hay nghiệm thu vật lý.

## Phạm vi đã viết vào firmware

- Core/Inc/vc02.h và Core/Src/vc02.c: decode byte A7 thành VC02_EVENT_ROTATE_RIGHT mới, không làm xê dịch ID enum cũ. A4 vẫn là loại enum VC02_EVENT_ROTATE_IN_PLACE (tên cũ) nhưng ở executor mới được hiểu là rotate left; A5/A6 dành cho turn+angle.
- Core/Inc/voice_s154_route.h: hàm thuần C kiểm tra duration 5..120s/bước5, xác nhận bit NC vật lý đúng mẫu MIN/MIN và MAX clear, chọn chiều A4=-1 (L2) / A7=+1 (L1), ánh xạ B1..B9 ra LEFT/RIGHT SOFT RAW riêng. **Ánh xạ tỉ lệ tuyến tính hành trình trục** là proxy thử nghiệm, KHÔNG chứng minh độ bánh xe 5..45 thực tế hoặc Ackermann. CENTER B0 dùng đường R1 đã có.
- Core/Src/main.c: Voice_FinalizeCommand chỉ chốt lệnh bánh A2/A3/A4/A7 khi D0 XX hợp lệ. Voice_S154StageSteerAngle xử lý A5/A6+B1..B9 ngay khi đủ hai thành phần, B0 độc lập CENTER. C1..C5 là tốc độ độc lập, không tự chạy motor; speed snapshot cho lệnh bánh kế tiếp.
- Voice_ExecuteMotorControl: bánh tiến/lùi giữ owner/ramp hiện có; không còn gọi Turn_Motors_Stop trong khi steering Voice đang hoạt động, để cho bánh và lái đồng thời. R2 first: A4/A7+duration chờ motor position R2, xác nhận cả L_MIN/R_MIN vật lý và cả MAX không tác động, rồi gọi WheelControl_SetSpinDirection(-1/+1). Nếu R2 fail, timeout, STOP, NC release, lỗi owner: KHÔNG quay hoặc cắt bánh. PWM quay voice cap <= MOTOR_DPAD_PWM=400 CCR của L1/L2. Duration start sau R2 thành công, là giải pháp hiện thực được chú thích cần người vận hành quan sát, không mạo nhận chính sách đã kiểm chứng.
- Steering_PositionUpdate: hỗ trợ voice pos request qua **owner** mới chứ không sửa PS2_Pad, dùng chính hàm Steering_PositionAxisUpdate, AS5600, NC, timeout, atomic CCR và fine CENTER đã có. Command4 dành cho angle để không xung đột bitmask3 của PS2 R1+R2. Hai target khác nhau trong SOFT, mỗi trục tự đến target, chưa có bộ sync common-T S15.4.
- Control_Arbitrate: STOP>PS2>Voice; đầu tiên PS2 khi có bất kỳ Voice pending/ready/active chỉ CANCEL + ngắt/yêu cầu dừng, chốt ps2_cancel_hold tới lúc tất cả input điều khiển về neutral (LY/RX & nút) trên PS2 Analog hợp lệ. Chỉ input mới sau release được chạy, không tự resume. STOP FF xóa pending và active ở cả hai nhóm. Không biến Voice-owned R2/Spin thành PS2 input hoặc nhầm RF loss do Voice-owned actuator.

## Các biến boot OFF / phạm vi cho phép

| Biến | Boot | Vai trò |
|---|---:|---|
| voice_pos_enable | 0 | Chốt vật lý cho Voice CENTER/R2/ANGLE qua existing position owner |
| voice_angle_mapping_enable | 0 | Chấp nhận ánh xạ B1..B9 normalized-shaft RAW (chưa hiệu chuẩn góc bánh) |
| voice_pivot_enable | 0 | Chốt cho Rotate A4/A7 phát lệnh R2 rồi spin (vẫn cần voice_pos_enable) |
| voice_speed_mode | C3 | Mode tốc độ độc lập cho lệnh bánh, không thay tốc độ servo lái |

Các biến trên chỉ được cân nhắc thay đổi khi đã có CubeIDE Build/Flash đúng SHA, UART đúng lệnh, STOP/NC/nguồn cắt độc lập, đo dòng/quãng trôi và chassis treo an toàn. **Không khuyên bật cả ba hoặc chạy đột ngột trên nền chưa chứng nhận.** Theo mặc định lệnh bánh A2/A3 giữ hành vi baseline; Voice steering/pivot nhận lệnh nhưng không được tự tăng PWM ở boot.

## Xác minh & cổng nghiệm thu

- tools/verify_m8_vc02_i1.py: biên dịch thật Core/Src/vc02.c với stub HAL, kiểm tra A4/A7, tốc độ, B0/B9, D0 XX hợp lệ/sai, F0, FF, plus pure-C voice_s154_route.h và các chốt source.
- tools/verify_m8_vc02_command_core.py: trích xuất/biên dịch **hàm C thực** Voice_ResetPendingCommand, Voice_SetMovement/Turn/Angle/Straight, Voice_S154StageSteerAngle, Voice_FinalizeCommand, VC02_M2_ProcessEvent trong môi trường HOST stub. Kiểm thử forward+TIME, B0 CENTER, turn+30/45 không TIME, overlap wheel READY+steer, C5 speed độc lập, A4/A7, STOP, CLEAR, duplicate/conflict/invalid.
- GitHub Actions chạy cả hai test cùng verifiers M8 cũ. Chưa chứng minh full STM32 MCU build/flash, ISR timing, actual GPIO/CCR, NC thời gian thực, hay khoảng trôi vật lý.
- Người dùng thử với **nguồn động lực 6 motor bị ngắt**, nạp firmware STM32 sau Build, kiểm tra VC02 raw UART LastEvent và lệnh nguồn. Khi đã có cắt nguồn độc lập, NC và giới hạn điện/cơ được kiểm tra, mới mở từng tính năng có giám sát và làm A/B.
- M8-S15.4 **chưa thực hiện bộ điều khiển chung T cho hai target**: đó là milestone SAU khi lệnh/route độc lập được xác nhận hoạt động. S15.3 manual f2137556 giữ nguyên, không đưa LEFT pacing trở lại. S15.5 Ackermann thực vẫn hoãn.

## Ghi chú về version và tính xác thực

Branch feature/m8-s15-dual-target-common-time tiếp nối nền f2137556 và tài liệu chốt 49 câu VC-02 tại docs/VC02_OPERATOR_COMMAND_MAP_2026_11_04.md. Người dùng đã chụp UniOne COM3 báo Download Complete cho VC-02, nhưng không có log UART so với STM32/SHA firmware đang nạp ở board. Nếu command fail, đọc trạng thái flags, event và owner trước khi thay PWM.
