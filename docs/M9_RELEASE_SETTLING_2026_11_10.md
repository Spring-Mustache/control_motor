# M9 — Giảm hunting sau khi thả ngoại lực

Ngày10/11/2026, giờ Việt Nam. Phản hồi người dùng: trục có phản lực nhưng sau khi thả tay phải quay qua lại lâu mới tìm được góc đặt. Lần báo này không kèm SHA MCU/trace liên tục; bản đã giao gần nhất là c9fd47fd/profile3. Profile4 PI+D trước đó mới ở workspace, chưa push/flash, nên không quy hiện tượng thực tế cho một bản nháp chưa giao.

## Điều đã xác định từ mã và kiểm tra

Profile3 dùng sàn PWM175/50, giữ bù ma sát đến đích rồi tắt lực, có thời gian nghỉ. Mô hình ma sát/trễ có thể làm cơ cấu vượt mục tiêu rồi đảo hướng nhiều lần. Đây là một cơ chế có căn cứ từ mã; chưa chứng minh là nguyên nhân duy nhất trên xe.

Bản nháp PI+D cũng cần sửa trước bàn giao: kiểm tra cũ chỉ nhìn khả năng ổn định sau thời gian dài, có thể bỏ lọt hunting. Bộ ước lượng cũ bỏ qua dịch chuyển1RAW trong40ms, che mất tốc độ thấp; I giữ tải nhưng nếu xả chậm sau bỏ tải sẽ kéo vượt mục tiêu. Không coi đổi PD thành PID đơn thuần là đã giải quyết.

## Mã sửa và điều kiện không được phá

1. Profile4 giữ lệnh bù I tại mục tiêu, không có timer nghỉ vì trục đang đứng yên. STOP/NC/sensor/new owner vẫn cắt và xóa I.
2. Tốc độ Q8 từ mẫu mới với low-pass60ms, giữ thông tin chuyển động chậm; không đọc thêm I2C trong controller.
3. Ngừng tăng I khi trục đang quay về; xả bù cũ khi đang đẩy quá mục tiêu hoặc sai số ngược chiều I được xác nhận nhiều mẫu. Khi trục đã đứng yên tại đích hoặc chỉ nhiễu±1RAW, không xóa bù giữ tải.
4. Kp LEFT/RIGHT8/4, Ki8/4, Kd2/1; giới hạn PWM300/100 giữ nguyên. Các gain là lựa chọn thiết kế, chưa hiệu chỉnh theo motor thực tế.
5. Giữ200ms PWM0 trước đảo DIR, lease50ms qua SysTick, stale80ms, sampler20ms/trục danh định và tất cả fix Rotate. Không sửa biến trở ACCE/iLIMIT của driver.

## Kiểm tra có tiêu chí quá độ

`tools/m9_servo_release_host_test.c` chạy216 tình huống:3 gain mô hình ×3 quán tính ×3 độ trễ actuator (0/60/120ms) ×2 ma sát ×2 kịch bản ×2 chiều. Các tham số **không phải thông số đã đo của motor/driver**.

- Kịch bản1: ghì lệch60RAW rồi thả.
- Kịch bản2: ngoại lực kéo dài đã được bộ điều khiển bù, sau đó bỏ tải.
- Lấy mẫu một cảm biến mỗi10ms, mỗi bên20ms. Tính lần cuối còn ngoài±3RAW hoặc tốc độ lớn hơn8RAW/s trong15s sau thả, và số lần đổi phía vượt vùng±3RAW. Giữ tiêu chí ổn định<=6s và tối đa3 lần vượt; không chỉ xem vị trí cuối.

| Kết quả trên cùng fixture giả lập | Profile3 đã giao | Profile4 đã sửa |
|---|---:|---:|
| Tình huống vượt tiêu chí | 166/216 | 0/216 |
| Thời gian quá độ lớn nhất quan sát | Chạm hết cửa sổ15s | 5090ms |
| Số lần vượt góc lớn nhất | 22 | 3 |

Các số trên là **kết quả mô phỏng**, không phải cam kết xe sẽ ổn định trong5,09s hoặc không dao động trên mọi tải. Fixture intentionally bao gồm trễ/ramping; tài liệu CC_SHBR12 người dùng cung cấp có chức năng ACCE/DECCE nhưng chưa biết thiết lập/độ trễ thực tế trên xe. Không tự chỉnh biến trở và không coi mô hình120ms là giá trị đã xác nhận.

Ngoài ra,36 mô hình tải giữ/bỏ tải, giữ I ở đúng goal600s giả lập, ghì sai số60s không COOLDOWN, jitter giữ I, xả I ngược chiều và ước lượng chuyển động chậm đều PASS; chạy lại với UBSan. Actual HAL/main kiểm giữ lực ở goal60s, STOP/NC xóa I, watchdog và Rotate handoff. Toàn bộ41 lệnh hồi quy local và CI đúng SHA là bằng chứng phần mềm; ARM compile/link/flash/hardware tách riêng.

## Bàn giao

Dùng nhánh M9 hiện có, cập nhật fast-forward và build/nạp theo [hồ sơ hiện hành](M9_CONTINUOUS_HOLD_CORRECTION_2026_11_10.md). Không giao thêm chuỗi thử mẫu cho người dùng trước khi nhận source. Không gọi lỗi hunting trên xe đã được xác minh hết khi chưa có dữ liệu của firmware mới.
