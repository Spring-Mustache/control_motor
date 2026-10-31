# M8-S14.7 — Hai vòng PI tốc độ AS5600 cho đánh lái, thử nghiệm D-pad LEFT → R_MIN

**2026-10-31 — SOURCE-STAGED, boot-OFF; host/CI phải kiểm tra ở SHA mới.** Không có bằng chứng STM32CubeIDE Clean Build, flashed SHA hoặc hardware A/B cho bản này. Không đổi cơ chế dừng, NC/STOP, trần PWM hoặc hiệu chuẩn. Không thay thế hồ sơ 17 mục và đặc tả gốc 386 dòng.

## Tại sao cần một bộ điều khiển khác

Trong log `M8S11_six_motor_20261031_160757.csv` (M8-S00 16:09, S14.5 OFF), ba lượt D-pad LEFT toàn hành trình: RIGHT chậm LEFT 547/548/607ms (~567ms). Các lượt OFF/ON S14.5 trước đó ~512/~547ms; dù ON có 350 khung bias, chưa đạt mục tiêu giảm lag. Các mẫu RIGHT gần R_MIN dùng duty ~52–100CCR và tốc độ suy từ AS5600 giảm đáng kể; tăng duty cuối hành trình khi không biết coast/current/clearance là chưa có căn cứ.

Đã từng có `Steering_SyncApply` PI kiểu **RIGHT master / LEFT follower** ở M7-S16. Khi `steer_s142_latched!=0` (Level1), đường `Steering_SyncApply` bị bypass và thay bằng `Steering_S142SyncApply` P+D ưu tiên bên chậm theo góc. **M8-S14.7 không chỉnh gain PI cũ** vì PI cũ không thể chủ động tăng RIGHT.

## Kiến trúc và phạm vi cho thử đầu

- **Chế độ chọn một lần tại khởi động lệnh:** `steer_s147_velocity_pi_enable=0` lúc boot; tại *lệnh manual mới sau neutral*, latch `steer_s147_latched=1` khi `steer_s142_trial_level=1`, PS2 **D-pad LEFT**, RIGHT về R_MIN, và `steer_s144_rmin_catchup_enable=0`. Đổi flag khi đang chạy không chuyển thuật toán giữa hành trình. RX, D-pad RIGHT, R1 CENTER, R2 MIN/NC, SELECT, Level2 giữ nguyên code hiện tại. Chủ động giới hạn pilot do dữ liệu đo coast R_MAX còn thiếu; đây là **hai PI thực sự nhưng mới kích hoạt thử trong một hướng**.
- **Vòng ngoài:** normalized travel `progressL/progressR` lấy góc hiệu chuẩn từ hai AS5600, tính mục tiêu tốc độ `560 permille stroke/s` LEFT và `680 permille stroke/s` RIGHT (ưu tiên RIGHT sớm vì terminal taper dài). Nếu một bên chậm tiến độ, mục tiêu tốc độ của *bên chậm* tăng thêm tối đa `120 permille/s`, không bao giờ ra lệnh hãm bên nhanh. Đây là các giá trị khởi đầu *source-only*, **không phải gain nghiệm thu**.
- **Vòng trong:** PI tốc độ riêng LEFT/RIGHT (`M8S147_AxisStep`), sai số `target_pms − measured_pms`; P = `error*10/100` CCR, I = `integral/400`, cập nhật I chỉ ở mẫu AS5600 mới bằng `error*dt_ms/40`, reset I khi tốc độ vượt mục tiêu, đóng băng I nếu bão hòa cap; I chặn 0..6000. Thành phần D = 0 để tránh khuếch đại nhiễu. Có dữ liệu cần chỉnh lại mới được sửa gain.
- **Phản hồi:** dùng `Steering_S14Observe` từ chính mẫu/timestamp đã xác thực của chủ trục, không thêm giao dịch I2C. Tốc độ `centidegrees/s` được đổi thành `permille stroke/s` theo quãng góc hiệu chuẩn mỗi trục. Không lấy số lượng frame UART 50ms thay cho dt thực; khi không có mẫu mới, không tích lũy I. Khi stale >100ms hoặc hình học/dữ liệu không hợp lệ, bỏ qua PI và để nguyên duty của owner.
- **Output:** **BOOST-ONLY**. PI không thể giảm duty về 0, không thay đổi chiều, không khởi động động cơ đang CCR0, và không cộng PWM sau khi chủ trục đã DONE/FAULT. Mức bù yêu cầu tối đa +40CCR mỗi bên nhưng luôn phải nằm dưới `M8S142_Envelope` của chủ trục: Level1 cap tuyệt đối LEFT375/RIGHT150; RIGHT baseline125 nên headroom thực ở xa SOFT tối đa25CCR. Tăng <=12 CCR mỗi vòng điều khiển qua previous final. Mỗi owner kiểm tra riêng về SOFT và taper 100 RAW cuối **giữ y nguyên**; PI bị bỏ qua khi khoảng cách của **bất kỳ trục nào** <=180 RAW đến SOFT.
- **Loại trừ:** Khi PI latched, S14.2 P+D và S14.5 bias không chạy chồng. Khi PI OFF, mọi câu lệnh cũ vẫn dùng S14.2 không đổi. Các kiểm tra NC, sensor, sai hướng, timeout/stall và late-EXTI/atomic GPIO/CCR đặt TRƯỚC và SAU coordinator như cũ; STOP và permit vẫn có thẩm quyền cuối. Không có active brake.

## Chẩn đoán sau khi STOP/đóng M8-S11

`M8S00_S147,enable=...,last=...,frames=...,skip=...,boostLf=...,boostRf=...,satLf=...,satRf=...`

`M8S00_S147_PI,targetL=...,targetR=...,measL=...,measR=...,errL=...,errR=...,boostL=...,boostR=...`

Live Expressions (ghi đúng tên): `steer_s147_velocity_pi_enable`, `steer_s147_latched`, `steer_s147_last`, `steer_s147_frames`, `steer_s147_skipped`, `steer_s147_boost_l_frames`, `steer_s147_boost_r_frames`, `steer_s147_sat_l_frames`, `steer_s147_sat_r_frames`, `steer_s147_target_l_pms`, `steer_s147_target_r_pms`, `steer_s147_measured_l_pms`, `steer_s147_measured_r_pms`, `steer_s147_error_l_pms`, `steer_s147_error_r_pms`, `steer_s147_boost_l_ccr`, `steer_s147_boost_r_ccr`.

`last=1` và `frames>0` chứng minh nhánh PI được thực thi, `boostRf>0` chứng minh đã tăng duty RIGHT so với owner; **không chứng minh RIGHT thực đến nhanh hơn**. Kết quả phải đo từ CSV. Hai file CSV OFF/ON phải có meta complete, cùng điều kiện, các lượt MAX→MIN đủ đến mốc, so skew, 100 RAW cuối, sai hướng, coast, current, STOP/NC.

## Hướng dẫn cập nhật và thử nghiệm (chỉ khi an toàn điện/cơ đã được kiểm chứng)

**Windows PowerShell** (không ghi đè thay đổi cục bộ):

```powershell
cd "D:\STM32 task\control_motor"
git status --short
git fetch origin
git merge --ff-only origin/feature/m7-s15-ps2-controls-restoration-staged
git rev-parse HEAD
python tools/verify_m8_s142_speed.py
python tools/verify_m8_s147_velocity_pi.py
python tools/verify_m8_s143_faststart.py
```

So HEAD với SHA của commit triển khai được báo riêng. STM32CubeIDE **Clean Project → Build → Flash** chính HEAD mới; GitHub Actions host PASS không phải cross-build/Flash PASS. Trên gá treo an toàn, có nguồn cắt động lực độc lập và NC/STOP hoạt động thực tế, **khi motor dừng hoàn toàn**:

| Live Expressions | OFF baseline | ON pilot |
|---|---:|---:|
| `steer_s142_trial_level` | 1 | 1 |
| `steer_s143_fast_start_enable` | 1 | 1 |
| `steer_s144_rmin_catchup_enable` | 0 | 0 |
| `steer_s147_velocity_pi_enable` | **0** | **1** |
| `m8_s11_stream_period_ms` | 50 | 50 |
| `m8_s11_stream_enable` | 0 → 1 → 0 khi capture | 0 → 1 → 0 khi capture |

Lệnh bật **chỉ có hiệu lực với một D-pad LEFT mới sau trung tính**, không áp dụng nếu đang giữ phím từ trước khi bật. Để quan sát UART: chạy `powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s11.ps1 -Port COM4 -Baud 115200` trước; khi PowerShell báo Listening mới đặt `m8_s11_stream_enable=1`; sau khi nhả D-pad và bảo đảm dừng, đặt về `0` để đóng CSV; chỉ lúc đó xuất M8-S00.

**Thứ tự:** OFF baseline nếu cần → ON, chỉ các xung ngắn ở vùng giữa hành trình trước → quan sát lực/tiếng/cơ cấu/độ trôi và NC/STOP, không chạy hết hành trình khi chưa biết khoảng hở và dòng → nếu an toàn được kiểm chứng mới so 5 lượt MAX→MIN cùng điều kiện. Nếu bất kỳ phản ứng lạ, cắt nguồn động lực, dừng thử và trả `steer_s147_velocity_pi_enable=0` khi trục đã dừng; không tự tăng Level2 hoặc PWM near SOFT. R1/R2/SELECT và tốc độ bánh phải hồi quy sau mỗi thay đổi firmware có thử trên xe.

## Kiểm thử phần mềm và điều kiện đóng

`tools/test_m8_s147_velocity_pi.c` dùng đúng production helper kiểm tra target, 2 PI, I chỉ cập nhật trên AS5600 fresh, antiwindup, +12/owner cap, không tăng ở <=180RAW, PWM0 không bật lại, invalid dt. `tools/verify_m8_s147_velocity_pi.py` kiểm tra kết nối vào owner, chế độ latch, các rào an toàn và chạy GCC/clang/cc host; Actions CI chạy cùng S14.2/S14.3 và handoff 17 mục. Chỉ ghi **HOST PASS** khi thấy kết quả CI thật; **không được đánh dấu CubeIDE PASS/Flash PASS/Hardware PASS cho tới khi có bằng chứng tương ứng**.

**Điều kiện nghiệm thu:** RIGHT nhanh hơn baseline có ý nghĩa và ổn định ở n lượt, quãng vượt SOFT không xấu đi, NC/STOP, nhiệt/dòng và motor wheels đồng thời đều ổn. Nếu không có khoảng hở cơ khí và current, không tự bật profile có thể gây tăng tốc toàn hành trình. Kết quả firmware đã commit = thử nghiệm chờ nghiệm thu, không phải hoàn tất M8.

## Bản thay thế route M8-S14.8 (2026-10-31)

Tất cả mô tả S14.7 Level1-only và PI thay thế P+D ở trên là **lịch sử**. SOURCE M8-S14.8 đã chuyển Level2 mặc định và nhánh PI opt-in Level1/Level2 lấy MAX positive boost giữa PI/P+D, không cộng boost hay trim làm chậm LEFT. Dừng/NC và bằng chứng hardware vẫn chưa nghiệm thu. Xem `docs/M8_S14_8_LEVEL2_PI_PD_HYBRID.md`.
