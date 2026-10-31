# M8-S14.10 — RIGHT speed-adaptive terminal taper SHADOW (ON-only)

**2026-10-31 — SOURCE-STAGED, non-actuating, motor output UNCHANGED.**
Mục tiêu: nghiên cứu vùng RIGHT còn 180 RAW cuối sau kết quả S14.9 ON; **chưa** đủ bằng chứng về quãng trôi, khoảng hở NC/dòng để thay thế đường giảm tốc thật. User chỉ yêu cầu test **controller ON**; không lặp PI/S149 OFF.

## Bằng chứng làm cơ sở

- Phiên 18:13 (3 lượt D-pad LEFT đầy đủ) Level2 + PI/S149 ON: RIGHT SOFT khoảng **1801ms**, LEFT **1560ms**, chênh **241ms**; RIGHT dẫn khoảng **44ms** ở mốc 180 RAW, nhưng mất lợi thế sau đoạn này. So S14.8 ON 17:22: RIGHT 1918ms, LEFT1562ms, chênh355ms (bốn lượt); khác số lượt và điều kiện, không tuyên bố causal proof.
- S14.9 sau sửa telemetry hiển thị vùng 100-180RAW: `frames=83, active=0, ownerR=100` (snapshot cuối), vùng <=100RAW: `frames=185, active=0, ownerR=0` (snapshot sau dừng); dữ liệu snapshot cuối **không** phải phân bố PWM/tốc độ toàn vùng. Dữ liệu AS5600 UART chu kỳ ~50ms nên phép nội suy không phải dòng điện hay coast.
- Chưa xác nhận **điện/cơ:** current/temperature driver, clearance tại NC khi ngắt PWM, measured coast, independent power cut, MCU flashed ELF verification. Source/HOST/GitHub CI ≠ HARDWARE PASS.

## Thay đổi source

- Thêm `Core/Inc/steer_s1410_right_terminal_shadow.h`, hàm C thuần `M8S1410_Plan` **KHÔNG IO**, dùng filtered RIGHT speed RAW/s đã có trong `Steering_ManualAxisUpdate` và timestamp từ đúng sensor. **Không đọc I2C thêm**; chỉ tính khi D-pad LEFT R_MIN, S149 ON đã latch, RIGHT MOVING, distance<=180RAW, PWM>0, tốc độ valid và tuổi mẫu<=100ms. Có sample invalid thì không suy ra tốc độ hay ứng viên.
- Đường tốc độ giả định: target `150+2*remaining RAW/s` (180RAW=510, 100RAW=350, 40RAW=230). Nếu speed thấp hơn target, tính **ứng viên shadow** tăng tối đa 20CCR trên PWM thực, nhưng vẫn bị chặn bởi `M8S142_Envelope` cũ; không yêu cầu tăng trong <=40RAW. Đây là **giả thuyết nghiên cứu**, không phải quyết định thay motor PWM.
- `M8S1410_Plan` trả `candidate_pwm`, `measured/target_raw_s`, `estimated_to_soft_ms`, và **travel-in-50ms proxy**. Proxy 50ms KHÔNG đo được quãng trôi khi PWM=0, KHÔNG thay cho clearance/NC; có flag proxy nếu travel>=remaining, chỉ quan sát.
- Hàm được gọi trong `Steering_ManualUpdate` **sau atomic late NC/STOP gate và sau ghi CCR**, đọc `steer_manual_right_pwm` và `steer_manual_right_axis.speed_filtered_raw_s`. **KHÔNG chép shadow candidate vào `ra`, PWM, owner hay TIM/CCR**, không sửa NC, sensor guards, stall, taper hiện hữu, cap Level2 450/200, cutoff 180, taper 100, +12 rise, S149 forecast, arm/disarm, R1/R2/RX/D-pad RIGHT.
- `M8S00_S1410`: `mode=shadow`, frames/valid/would/proxy50. `M8S00_S1410_BIN`: band0 (140,180], 1 (100,140], 2 (60,100], 3 (0,60] RAW; frames/valid/would/proxy50, average speed/actual PWM/candidate PWM **trong các khung VALID**, peak RAW/s, thông số mẫu cuối. `avgShadow` chỉ là ứng viên, **không phải PWM đã cấp**. Không tăng lưu lượng UART lúc motor ON; chỉ dump M8S00 sau disarm theo gate cũ.

## Test và quyết định tiếp

- `tools/test_m8_s1410_right_terminal_shadow.c` host GCC strict tests fresh/stale, candidate cap, 40RAW no-raise, PWM0, speed0/proxy; `tools/verify_m8_s1410_right_terminal_shadow.py` kiểm source zero-actuation và đường đọc đúng, đi kèm workflow regression S14.2–S14.9 và nguyên văn handoff 386 dòng/17 mục.
- **Không tự động bật thay đổi giảm tốc motor trong phiên này**, bởi việc tăng PWM gần SOFT có thể thay đổi coast tới NC dù không tăng trần 200CCR. Tăng PI gain không giải quyết <=180RAW vì coordinator vẫn OFF theo thiết kế. Không hứa cải thiện tốc độ motor từ S14.10 shadow.
- Bước tiếp theo: CI **exact HEAD SHA** → Windows git fetch/ff-only, CubeIDE Clean Build/Flash (SHA evidence) → sau gate hiện hữu về cắt nguồn độc lập, NC/STOP, current/coast/clearance mới thu một log **ON-only**: M8S11 complete, M8S00_S1410 bands, SOFT raw cuối, thật sự dừng trước NC; không thử OFF. Từ band averages + đo coast thực, quyết định có đủ căn cứ xin chỉnh PWM trong một bản riêng biệt hay không. Nếu thiếu data, giữ S14.9 actuation và S14.10 shadow như hiện tại.
