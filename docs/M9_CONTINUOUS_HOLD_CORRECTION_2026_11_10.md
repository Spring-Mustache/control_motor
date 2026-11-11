**CẬP NHẬT 11/11/2026 — NGƯỜI VẬN HÀNH XÁC NHẬN KẾT QUẢ TRÊN XE.** Trong bối cảnh bản bàn giao `b1c1e9c4`/profile4, người dùng xác nhận bánh tạo phản lực khi bị tác động và không còn quay qua lại nhiều lần để dò góc trong lần thử. Đây là **USER-REPORTED định tính**, chưa kèm log SHA MCU, lực/góc/thời gian/dòng/nhiệt hoặc phạm vi từng trục/tải. CI firmware run42759998485 có ba job kiểm tra/build thành công. Động học tổng thể chưa tích hợp, SELECT tạm hoãn. Bản bàn giao hai phiên và nội dung báo cáo mới nằm tại [hồ sơ 11/11](M9_SESSION_HANDOFF_2026_11_11.md). Các khối có ngày cũ bên dưới giữ trạng thái tại thời điểm đã ghi.

# M9 — Sửa đúng yêu cầu giữ trục chống ngoại lực liên tục

## Yêu cầu và sai sót của bản trước

Mục tiêu xuyên suốt M9 là giữ góc đặt khi có ngoại lực. Bản `c9fd47fd` chỉ triển khai phục hồi theo vị trí: ngừng PWM tại đích và nghỉ3s sau1s không tiến triển. Hai quyết định đó không đáp ứng giữ lực liên tục dưới tải. Gọi bản đó hoàn thiện M9 là kết luận vượt phạm vi thực hiện. Chưa biết sức chịu tải thực của motor không phải lý do để thay yêu cầu bằng thuật toán hồi vị có nghỉ.

**Bản hiện hành dùng profile4: servo vị trí PI + phản hồi vận tốc D.** Giữ hai fix Rotate từ nền đã được xác nhận. Profile3 chuyển thành fixture hồi quy lịch sử, không phải cấu hình firmware mặc định.

## Cập nhật theo phản hồi ghì rồi thả ngày10/11/2026

Người dùng báo trục phản lực được nhưng sau khi thả tay quay lên/xuống nhiều lần mới ổn định. SHA firmware thực tế của lần báo này chưa đi kèm log; đối chiếu mốc cuối đã giao c9fd47fd/profile3, không gọi bản profile4 đang sửa trong workspace là firmware đã được người dùng thử.

Bản đang bàn giao gồm cả servo giữ tải và sửa đáp ứng sau bỏ tải:
- Ước lượng tốc độ Q8 lọc60ms trên từng mẫu mới thay cách bỏ qua1RAW/40ms; D nhận biết chuyển động chậm gần đích.
- Không tích lũy thêm I khi trục đã đang chạy về đích; giữ I khi đứng yên cân bằng tải.
- Giảm phần bù I đang đẩy quá góc khi chuyển động cho thấy sắp qua đích/đã vượt đích. Nếu sai số ngược với I kéo dài>=60ms, ngoài2RAW và chưa quay về, giảm bù cũ để tránh nằm kẹt trong ma sát rồi dò lâu. Không xóa I chỉ vì e=0 hoặc nhiễu±1RAW.
- Kp giảm còn8/4; giữ Kd2/1 và các trần PWM cũ. Không tăng trần lực để chữa hunting.

Xem [kiểm tra bỏ tải và giới hạn](M9_RELEASE_SETTLING_2026_11_10.md).

## Điều khiển chống ngoại lực

`u = Kp*e + I - Kd*v`, trong đó `I` tích lũy sai số theo thời gian để tạo phần lực bù tải. Đơn vị output là CCR timer; không phải số đo mô-men hoặc dòng.

- Khi đang ở goal và chưa có tải, I khởi đầu0 và PWM có thể0. Firmware không tự bịa ra chiều/lực tải chưa quan sát được.
- Khi ngoại lực gây sai lệch đo được, P phản ứng, I tích lũy để chống tải kéo dài. Không chờ ngưỡng8RAW của bản trước; bỏ nhiễu trong±1RAW, hiệu chỉnh từ sai số2RAW.
- Khi trở lại đúng goal, **giữ I đang có**: PWM không bị ép về0 chỉ vì sai số vị trí bằng0. Nhờ đó duy trì lệnh tạo lực cân bằng tải.
- Khi trục đứng yên dưới tải, **không nghỉ định kỳ, không coi không chuyển động là lỗi và không hết phiên theo thời gian**.
- Khi bỏ tải hoặc đổi chiều tải, P/D phản ứng theo vị trí/vận tốc và I giảm/đổi dấu theo sai số mới. Chống tích lũy quá mức khi PWM bão hòa; I không tích lũy thêm trong khoảng chờ đảo chiều.
- Hai trục có I, tốc độ, sai số và output riêng. Mục tiêu không thay đổi theo vị trí bị ngoại lực đẩy tới.
- STOP/NC/lỗi sensor/ra ngoài dải cơ khí/mất điều kiện điều khiển/owner mới cắt output và xóa I. Lệnh mới không kế thừa lực bù của goal cũ.
- Giữ sampler10ms luân phiên (~20ms/trục), tuổi mẫu tối đa80ms và lease PWM50ms qua SysTick. Đây là ngắt khi dữ liệu/vòng điều khiển mất hiệu lực, không phải timer nghỉ khi đang giữ tải hợp lệ.

## Thông số phần mềm

| Tham số | LEFT | RIGHT |
|---|---:|---:|
| PWM tối đa | 300CCR | 100CCR |
| Kp | 8CCR/RAW | 4CCR/RAW |
| Ki | 8CCR/(RAW·s) | 4CCR/(RAW·s) |
| Kd | 2CCR/(RAW/s) | 1CCR/(RAW/s) |
| Tốc độ tăng PWM | 2000CCR/s | 1000CCR/s |
| PWM0 trước đổi DIR | 200ms | 200ms |
| Ước lượng vận tốc | Q8, lọc60ms; sẵn sàng sau40ms | Q8, lọc60ms; sẵn sàng sau40ms |
| Vùng bỏ nhiễu vị trí | ±1RAW | ±1RAW |

Các gain là giá trị thiết kế phần mềm, **chưa được nhận dạng/hiệu chỉnh theo motor trên xe**. PWM cap kế thừa trần vận động M8; không phải giới hạn dòng/nhiệt đã đo. Không áp sàn175/50 vào mọi lệnh servo, vì sàn lực rời rạc sẽ cản cân bằng tải nhỏ; I có thể tăng output tới trần khi sai số còn tồn tại.

`commissioned=0,profile=4` có nghĩa servo có thể cấp lực, nhưng nghiệm thu lực/nhiệt thực tế chưa được ghi nhận. Không cần đổi cờ commissioning trong Live Expressions.

## Bằng chứng kiểm tra

- **41 lệnh hồi quy local PASS**, giữ các kiểm tra M8, Rotate, STOP, ownership, các profile lịch sử và phần mới.
- **36 mô hình động lực học giả lập PASS:** tải giữ liên tục rồi bỏ tải, hai chiều, có/không ma sát, ba mức tốc độ và ba mức quán tính; sử dụng lịch lấy mẫu luân phiên10ms, mỗi trục20ms. Kiểm sai số sau ổn định trong miền mô hình, không chuyển thành số đo chính xác trên xe.
- **Controller C thật:** I tạo lực khi có sai số; giữ PWM khác0 ở đúng goal liên tục600s giả lập; sai số bị ghì60s không có COOLDOWN. Kiểm bão hòa, đổi dấu và xóa I khi STOP. Kiểm thêm không bị kẹt I khi output đi qua0 trong khoảng đảo chiều.
- **Adapter main/HAL thật với mock:** giữ lực bù ở đúng goal60s; bị ghì60s không nghỉ; STOP/NC xóa lực bù; lease50ms, stale80ms và handoff Rotate vẫn đúng.
- **216 tình huống ghì–thả/bỏ tải có độ trễ giả lập PASS** theo tiêu chí vị trí/vận tốc và số lần vượt góc, không chỉ kiểm tra mẫu cuối sau40s. Chi tiết trong hồ sơ bỏ tải.
- **UBSan PASS**, compiler từ chối profile/cap không hợp lệ. CI chạy lại trên đúng SHA, cùng job biên dịch/link toàn bộ firmware ARM và lưu ELF/HEX/BIN/manifest.

Phân biệt dữ liệu:600s/60s và36 mô hình là kiểm tra tự động trên host, không phải đã bật motor giữ tải từng ấy thời gian. Firmware chưa được nạp bởi assistant; không có số liệu lực, dòng hay nhiệt của profile4 trên xe.

## Giới hạn cần diễn đạt chính xác

Logic hiện tại yêu cầu lực bù liên tục và không tự bỏ giữ chỉ vì đứng yên. **Không thể cam kết chống được mọi ngoại lực hoặc góc không dịch dù chỉ một lượng nhỏ**: vòng phản hồi phải quan sát sai số, có độ phân giải/độ trễ và mô-men motor hữu hạn. Phạm vi lực tối đa, độ lệch thực tế và thời gian giữ tải chưa có số đo. MCU không có cảm biến dòng/nhiệt; source không được mô tả là bảo vệ nhiệt kín. SysTick lease cũng không phải watchdog độc lập khi CPU bị halt.

Đây là giới hạn vật lý/bằng chứng, tách biệt với lỗi thuật toán hồi vị có nghỉ đã sửa. Không yêu cầu người dùng thực hiện thêm vòng thử mẫu để nhận code; không tạo HARDWARE PASS giả để hoàn tất báo cáo.

## Cập nhật và sử dụng

Nhánh `feature/m9-steering-active-position-hold`, project `D:\STM32 task\control_motor`. Fast-forward bằng các lệnh trong README; giữ các thay đổi `.settings`, startup và logs. Sau CI đúng SHA PASS: CubeIDE Refresh → Clean Project → Build → Flash/Debug → Resume F8. CENTER/R1 hoặc góc Voice5..40 BOTH-DONE tự bàn giao goal; không cần kích biến debug. Rotate/R2/Voice45 và STOP giữ quy tắc ownership hiện hành.

Thu dữ liệu vận hành thụ động nếu cần lưu báo cáo:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\capture_m9_h2b1_hold.ps1 -TimeoutSeconds 300 -MaxEvents 100 -UntilStopped
```

## Câu dùng trong báo cáo

“M9 triển khai servo giữ góc liên tục cho hai cơ cấu lái bằng điều khiển PI kết hợp phản hồi vận tốc. Thành phần tích phân giữ lệnh bù tải khi sai số đã trở về gần0, giúp bộ điều khiển tiếp tục yêu cầu mô-men cân bằng ngoại lực thay vì ngắt lực tại đích. Chức năng đã được kiểm chứng bằng kiểm tra trên mã C, mô phỏng tải kéo dài/bỏ tải và hồi quy chuyển quyền với PS2/giọng nói. Khả năng chịu tải và độ chính xác thực tế của phiên bản này chưa có số liệu nghiệm thu trên xe.”
