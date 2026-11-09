# M9 H2-B1 — Phục hồi góc trong toàn hành trình hợp lệ (09/11/2026)

> **2026-11-09 — MCU log181422: giữ goal toàn hành trình + tự retry đã quan sát, phản lực chưa đạt:** 19 dòng hợp lệ trên32520fe0c86facf59756e6c0071985541f7f42d0. Seq2 giữ active1 dù peakError149RAW, tự retry4 lần tại8913/15602/23247/29948ms,LEFT peak30CCR/applied5166ms,cuối30960ms còn lệch−138RAW; không có RANGE5. Seq3 LEFT applied1001ms rồi STOP state4/reason2/active0/lease0 tại9385ms,còn lệch−120RAW. RIGHT peakPWM/applied0 vì sai số không quá4RAW. Xác nhận giới hạn sai số32 đã bỏ, retry và STOP phía phần mềm; không xác nhận lực/mô-men/return hoặc dòng/nhiệt. Không tăng PWM từ log này. Docs-only: giữ MCU32520fe,không cần flash lại hoặc lặp phép thử cùng30/10 chỉ để xác nhận hai lỗi logic đã sửa. Bước tiếp là đánh giá đường tạo lực/hiệu chỉnh actuator; thiếu lực thắng ma sát vẫn là giả thuyết.

## Vấn đề và yêu cầu

Người dùng báo HOLD không tạo phản ứng thấy được và hỏi có cần kích biến Live Expressions. Log riêng M9H2B1_HOLD_20261109_175629.txt có các dòng hoàn chỉnh trên MCUaf6a43dbe28a524dff94dfbfc106b3de2c1efbfd: state3/active1/lease1 rồi state5/reason5/active0 tại2823ms,LEFT goal1886/raw1919/error−33,peakPWM30,appliedLms773,RIGHT error2/output0,retryCount0. Dòng đầu chứa mảnh firmware cũ ghép với M8R1_FINE, không dùng làm một bản ghi HOLD hoàn chỉnh. Không đăng TXT gốc lên repository public.

Không cần kích biến: HOLD đã chạy. Nguyên nhân hủy phiên được xác định trong source: sai số33RAW vượt max_error_raw32. Đây là ngưỡng sai số so với góc đích, không phải chạm giới hạn hành trình vật lý. Người dùng yêu cầu bỏ điều kiện phi thực tế này để còn tiếp tục kéo về khi bị đẩy lệch.

## Thay đổi

- Xóa hẳn max_error_raw khỏi M9B1Config và cấu hình khởi tạo. Không thay32 bằng một trần sai số lớn hơn.
- Bỏ hai nhánh hủy goal theo sai số trong M9B1_Begin và M9B1_Step. Khi caller xác nhận hình học hợp lệ, độ lệch lớn vẫn được đưa vào PD, slew và giới hạn output.
- Giữ kiểm tra cả góc đo và goal bằng Steering_SafetyHardEnvelopeValid trong main adapter, dùng hình học/hành trình hiệu chuẩn hiện có. NC và các điều kiện cũ vẫn có hiệu lực.
- PreOwnerService báo RANGE5 khi thật sự ngoài hành trình; lỗi cảm biến tiếp tục SENSOR4. Trước đó nhánh này gộp cả lỗi hình học vào SENSOR4.
- Giữ nghỉ/retry11/12, STOP/NC/sensor/service/port/new-owner hủy goal không tự hồi; không đổi cap30/10,gain,ngưỡng vào8/ra4/deadband5 hoặc reversal200ms.

90RAW giữa hai mẫu liên tiếp vẫn là kiểm tra bước nhảy dữ liệu cảm biến. Nó khác hoàn toàn với tổng sai số so với goal: đi lệch từ từ100,400RAW vẫn không bị hủy chỉ vì độ lệch đó. Dữ liệu stale hoặc một bước nhảy bất thường vẫn không được dùng để điều khiển motor.

## Kiểm chứng phần mềm

Native-C kiểm33..400RAW cả hai dấu và hai trục, bắt đầu phiên với sai số lớn khi geometry_ok, tiếp tục retry, giữ nguyên goal/seq, hồi về vùng yên theo đầu vào tổng hợp. Main/HAL production fixture tái hiện đúng LEFT1919/goal1886/error−33 từ log, sau đó cả hai trục lệch hơn300RAW trong hành trình fixture và vẫn giữ mục tiêu qua nhiều lần retry. Ra ngoài hành trình fixture thì RANGE5,hai output0 và không tự khởi động khi đưa góc trở lại. Các kiểm STOP/NC/cảm biến/new-owner, giới hạn PWM/ngân sách, formatter UART và native regressions được giữ.

Các mẫu góc trong HOST là tổng hợp, không chứng minh motor tạo lực hay tự trở về. Không có ARM cross-build hoặc flash từ môi trường này. CI đúng commit được kiểm sau push. MCU cuối được log vẫnaf6a43d; bản sửa này chưa có log phần cứng.

## Cập nhật và kiểm tra trên xe

Sau Actions đúng SHA PASS:

```powershell
cd "D:\STM32 task\control_motor"
git status --short
git fetch origin
git merge --ff-only origin/feature/m9-steering-active-position-hold
git rev-parse HEAD
```

Không reset/clean hoặc ghi đè .settings/startup đang sửa. CubeIDE Refresh → Clean Build → Debug/Flash → Resume F8. Không kích biến Live Expressions.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\capture_m9_h2b1_hold.ps1" -TimeoutSeconds 300 -MaxEvents 100 -UntilStopped -OutputDirectory ".\logs"
```

Trên giá đang dùng, mở collector trước CENTER; tác động nhẹ rồi thả trong hành trình, quan sát phản ứng, kết thúc bằng STOP. Không cần lặp H1/H2-B0 hoặc phép thử ngắt nguồn đã bị loại. Log source mới không được FAULT5 chỉ vì sai số vượt32RAW khi hình học/cảm biến vẫn hợp lệ. State6/active1 vẫn là nghỉ có giữ goal;retryCount tăng sau hồi đủ ngân sách. STOP vẫn hủy phiên hoàn toàn.

## Giới hạn còn lại

Sửa này loại bỏ nguyên nhân hủy sai theo độ lệch, không tăng lực giữ. Cap trái3%/phải1% vẫn có thể chưa thắng ma sát; chưa có bằng chứng mô-men,dòng/nhiệt hoặc khả năng tự kéo về của bản mới. Không hứa quay về bắt buộc trong mọi điều kiện điện/cơ hoặc khi đã có lỗi an toàn. Nghiệm thu phần cứng và H3 có tải vẫn chưa hoàn thành.
