# M9 H2-B1 — Giữ mục tiêu và tự thử lại sau nhiễu (09/11/2026)

## Yêu cầu và phạm vi thực hiện

Người dùng xác nhận đẩy lệch bánh rồi thả tay, bánh không tự quay về; chưa có số đo dòng. Yêu cầu mới là thực hiện luôn để hệ thống tiếp tục cố gắng tạo phản lực khi bị tác động, không chờ đo dòng hoặc đặt giới hạn dòng.

Bản này thực hiện phần giữ mục tiêu và tự thử lại có giới hạn. Không thêm cảm biến/giới hạn dòng vì firmware không có phản hồi dòng; cũng không tuyên bố ngân sách thời gian là giới hạn dòng. Không triển khai lực vô hạn, không tăng PWM tự động, không bỏ STOP/NC/cảm biến/hard envelope. Các giới hạn output hiện hành vẫn giữ. Đây là giới hạn triển khai đã được assistant báo rõ; không ghi thành người dùng đã chấp thuận hoặc nghiệm thu một thông số điện/nhiệt.

## Bằng chứng đầu vào

Firmware621a5ce87385600314404bc8866662c0102f4df4 đã phát output trái đỉnh30CCR, được adapter ghi nhận1007ms rồi FAULT5/reason11/active0; goalL1886/rawL1909/error−23RAW. Người dùng báo không tự trở về sau khi đã thả tay. UART không đo dòng, mô-men hoặc thời điểm thả tay; nguyên nhân duy nhất chưa xác định. Nguyên bản TXT vẫn riêng tư, không đăng vào repository public.

## Hành vi mới

| Sự kiện | Hành vi |
|---|---|
| Sai số kích hoạt HOLD | Tính PD từ mẫu AS5600 mới và áp PWM có slew/cap như trước |
| Không giảm ít nhất2RAW trong1000ms đang có output | COOLDOWN6/reason11/active1; hai PWM0, giữ nguyên goal/seq/source |
| Ngân sách output đạt1500ms | COOLDOWN6/reason12/active1; hai PWM0, giữ nguyên goal/seq/source |
| Cả hai ngân sách đã hồi về0 | Tăng retryCount, tính lại điều khiển theo góc/vận tốc mới; không phát lại PWM cũ |
| Bánh đã trở về vùng yên | Output trở về0; goal tiếp tục tồn tại cho lần bị tác động sau |
| STOP, NC, cảm biến/range/service/port lỗi, owner mới | Cắt output và hủy goal; không tự retry từ goal đã hủy |

HOLD không có thời hạn phiên cố định. Riêng các sự kiện tuning11/12 không còn làm mất goal. State4/5/active0 vẫn là kết thúc thực sự; collector UntilStopped không thoát khi nhận state6/active1.

## Các giới hạn được giữ

- Trần LEFT30CCR=3%, RIGHT10CCR=1%, minPWM0; gain và tốc độ tăng output không đổi. Đây không phải các mức lực đã nghiệm thu.
- Ngưỡng vào8RAW, ra4RAW, deadband5RAW, maxError32RAW; quá phạm vi vẫn hủy goal.
- Ngân sách mỗi trục1500ms output khác0, hồi khi output0 theo tỷ lệ1500/8500. Không xóa lịch sử effort khi chuyển sang nghỉ hoặc retry; hồi hết cả hai mới thử lại.
- Đếm đủ thời gian output của cả hai trục trước khi xử lý một sự kiện nghỉ. Dừng output phụ thuộc nhịp service phần mềm; không phải ngắt dòng/nhiệt phần cứng.
- Giữ khoảng PWM0 khi đổi chiều200ms, kể cả đổi hướng nhiễu trong lúc nghỉ. Tính lại dấu điều khiển từ góc hiện tại.
- Hai trục nghỉ cùng nhau khi một trục cần nghỉ; sau đó output riêng từng trục như trước.

Việc thử lại với cùng cap không chứng minh đã đủ lực thắng ma sát. Không sao chép PWM tinh chỉnh CENTER175/85CCR sang giữ liên tục. Chưa đo được dòng/nhiệt khi đứng giữ và chưa có bằng chứng trở về cơ khí cho bản mới.

## Quan sát log

State6/active1/reason11 hoặc12 là đang nghỉ có giữ mục tiêu, không phải FAULT. Hai PWM0 trong lúc này là hành vi dự kiến. retryCount tăng sau mỗi lần hồi đủ ngân sách và rời nghỉ. faultAxes ở trạng thái6 chỉ trục gây lần nghỉ gần nhất; tên trường được giữ để tương thích log cũ. STOP vẫn state4/reason2/active0; lỗi an toàn vẫn state5/active0.

UART chỉ gửi khi output yên. Vì thế thường nhận thống kê khi nghỉ hoặc sau STOP, không phải một dòng cho mọi bước cấp lực. Kết thúc collector không dừng HOLD.

## Cập nhật và thu log trên máy người dùng

Sau khi Actions của đúng commit PASS, tại Windows PowerShell:

```powershell
cd "D:\STM32 task\control_motor"
git status --short
git fetch origin
git merge --ff-only origin/feature/m9-steering-active-position-hold
git rev-parse HEAD
```

Giữ các thay đổi .settings/startup và dữ liệu đang có; không reset/clean hoặc ghi đè nếu Git báo xung đột. CubeIDE Refresh → Clean Build → Debug/Flash → Resume F8. Chưa có bằng chứng người dùng đã thực hiện các bước này cho source mới.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\capture_m9_h2b1_hold.ps1" -TimeoutSeconds 300 -MaxEvents 100 -UntilStopped -OutputDirectory ".\logs"
```

Trên giá đang dùng, mở collector trước CENTER; không phát lệnh chạy bánh ở lượt đầu. Sau CENTER đã ổn định, tác động nhẹ ở lốp rồi thả, quan sát khả năng kéo lại và rung. Dùng STOP kết thúc; không nắm cơ cấu để ghì motor. Không yêu cầu phép thử ngắt nguồn motor đã bị người dùng loại. Không cần lặp H1/H2-B0/unpowered B1 đã có.

## Kiểm chứng và phần chưa đạt

HOST kiểm tra mã C thực và main/HAL adapter: nhiễu liên tục300s với nhiều lần nghỉ/retry, cap30/10, giữ goal/seq, không reset effort khi nghỉ, hồi đủ cả hai mới retry, tick wrap, đổi chiều trong lúc nghỉ và blanking, trở lại vùng yên rồi nhận nhiễu mới, STOP/NC/owner/sensor/range trong nghỉ không tự hồi. Formatter UART thực có retryCount và kiểm kích thước buffer640bytes. Các hồi quy workflow được kiểm tra riêng; Actions đúng SHA sau commit là bằng chứng CI.

HOST là kích thích tổng hợp, không phải hiệu chuẩn hoặc nghiệm thu motor. CubeIDE target build/flash và log mới CHƯA CÓ. Khả năng kéo lại center, mô-men, dòng/nhiệt và H3 có tải CHƯA KIỂM CHỨNG. MCU cuối được log vẫn621a5ce; không gán kết quả đó cho bản retry mới.
