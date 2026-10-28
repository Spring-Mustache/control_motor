# M8 — hoàn thiện R1/R2 và đo điều khiển bánh + lái đồng thời

Ngày 2026-10-28. Trạng thái: **đã sửa mã nguồn, CHƯA Build STM32/CubeIDE hoặc thử trên xe**.

## Yêu cầu được giữ

- Bốn bánh LU/LD/RU/RD và hai cơ cấu lái L/R có thể nhận lệnh PS2 đồng thời. LY/Up/Down không được khóa D-pad/RX hay R1/R2. L1/L2 spin và SELECT là chế độ riêng.
- Một lần R1 phải tự tiếp tục thoát MIN nếu đang chạm NC, rồi tiến về CENTER; không mặc định đòi nhấn R1 lần hai.
- Không nới chặn NC/AS5600/hard-envelope, timeout, quá hành trình, dừng khẩn; không tăng PWM để ép cơ cấu vượt qua vùng kẹt.
- SELECT tạm hoãn theo yêu cầu người vận hành. Không sửa thuật toán hoặc các giá trị hiệu chuẩn MIN/MAX/CENTER hiện đang lưu.

## Bằng chứng có trước bản sửa

- M8-S00 tại 13:38: LEFT R1 gần CENTER nhưng kết thúc `code=10` khi `rawL=1895`, CENTER LEFT=1873, lệch 22 RAW. RIGHT RAW=661, CENTER RIGHT=662, đã DONE. `code=10` là **lỗi chỉnh tinh**, nhưng snapshot đó không có mã phụ; không kết luận được chắc chắn hết tám xung hay PWM quá yếu.
- Sau khi chỉnh cơ khí, người vận hành quan sát R_MIN phải thực sự chạm NC khoảng RAW=193. Số này là **quan sát cơ khí**, không phải SELECT đã xác nhận.
- SELECT log 14:07: `mask=0x00000500`, RU RPM=+2.1 trong khi target/PWM bánh=0. Đây là bằng chứng trực tiếp nguyên nhân SELECT lần hủy cuối do RPM thụ động; SELECT được tạm hoãn.

## Thay đổi thực hiện ở lượt này

1. **LEFT coarse→fine handoff**: trước đây cả hai trục vào điều khiển chỉnh tinh khi còn sai số <=35 RAW. LEFT nay giữ PWM coarse đã có (không tăng biên) cho tới <=15 RAW; RIGHT vẫn giữ 35 RAW. Mục đích là giảm nhu cầu nhiều xung ngắn ở motor trái vốn còn lệch 22 RAW khi lỗi. Thời gian R1 tổng tối đa 9s, vùng thành công ±3..5 RAW, tối đa tám xung fine, biên PWM trái/phải, xung tối đa 60ms, quãng thoát NC 1s và kiểm tra NC trước CCR không đổi. Đây là **giả thuyết cải thiện cần nghiệm thu**, không phải cam kết chắc chắn đã chữa lỗi code10.
2. **Đo PWM chạy–lái đồng thời**: `M8S00_ObserveConcurrent()` chạy sau `WheelControl_Update()` và cả ba owner lái. Đếm frame có PWM phần mềm khác 0 ở cả 4 bánh, sau đó có ít nhất 1 motor lái PWM khác 0 và riêng số frame cả 2 motor lái cùng có PWM. Chỉ quan sát; không cấp PWM hoặc bỏ gate. Bộ đếm từ lúc boot: `fourWheel`, `oneSteer`, `bothSteer`.
3. **UART**: `M8S00_CONCURRENT,scope=since_boot,request=...,fourWheel=...,oneSteer=...,bothSteer=...,ccrL=...,ccrR=...`. Chỉ in khi DISARM/đứng yên qua cổng xuất log M8-S00. Cho phép đọc kể cả khi chỉ có thử chạy–lái đồng thời và chưa có STOP/FAULT.
4. Thêm `tools/verify_m8_r1_center_concurrent.py`: kiểm tra tĩnh 13 điều kiện trọng tâm. Chưa chạy firmware trên phần cứng.

## Phân biệt quan trọng về mục tiêu R2 hiện trên branch

Tại thời điểm thực hiện sửa R1 trong lượt này, branch **đã có** các mục tiêu R2 lùi vào trong công tắc từ một thay đổi trước đó: LEFT=2240 RAW, RIGHT=223 RAW. Đó là **setpoint R2**, không phải MIN runtime. Các giá trị MIN runtime vẫn LEFT=2297, RIGHT=166; CENTER LEFT=1873, RIGHT=662.

Theo quan sát contact (LEFT khoảng 2274 từ log, RIGHT khoảng 193 do người vận hành đo), setpoint mới lùi vào tương ứng 34 và 30 RAW. Đây là **khoảng hình học trên AS5600, không phải khoảng dừng đã xác nhận an toàn**; chưa có dữ liệu quãng trôi thực. Không nên hiểu đây là hiệu chuẩn SELECT hoặc điểm tiếp xúc NC mới. Nếu nhận thấy setpoint khác mong muốn hoặc motor đi gần công tắc hơn dự kiến, phải dừng và rà cấu hình trước khi tiếp tục.

## Quy trình nghiệm thu

1. Trên giá treo chắc chắn, có ngắt nguồn động lực độc lập đang hoạt động, và đã kiểm tra NC + chiều DIR khi nguồn động lực **ngắt**. Không đặt xe chạy thử trên nền khi chưa có dữ liệu mới.
2. Build Debug từ branch mới, không Generate Code từ `.ioc` có thể khác phần mã đã kiểm tra. Chạy các script tĩnh trước flash:
   `python tools/verify_m8_r1_center_concurrent.py`,
   `python tools/verify_m8_r1_escape_nc.py`,
   `python tools/verify_m8_concurrent_drive_steer.py`,
   `python tools/verify_m8_s00_capture.py`.
3. Test từng trục/biên an toàn với lệnh thấp trước, rồi một chu trình R2 → thả → R1 một lần. Đặt tay tránh cơ cấu và chuẩn bị sẵn cắt nguồn động lực. R1 PASS chỉ khi **cả hai motor** dừng trong sai số CENTER ±5 RAW và không có lỗi, không cần nhấn lại. Nếu motor trái tiếp tục code10, lấy `M8S00_FINE,FAULT,reasonL,pulsesL,errorL` thay vì tăng PWM hoặc bấm lặp.
4. Chỉ sau bài thử R1/R2 riêng hoàn tất, mới thử LY thấp + D-pad/RX trong thời gian ngắn trên giá treo, bảo đảm không va chạm và hai tín hiệu PWM đánh lái không bị mất khi bánh chạy. `oneSteer>0` là bằng chứng **ít nhất một trục lái** được command đồng thời cùng bốn bánh, `bothSteer>0` là bằng chứng **cả hai CCR** khác 0 ở cùng frame; không chứng minh motor vật lý quay hay bộ nguồn hoạt động.
5. Thả lệnh, DISARM START, chờ tất cả dừng. Chạy `powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s00.ps1`, bấm USER ngắn sau khi script sẵn sàng, gửi TXT. Không reset firmware trước khi xuất vì bộ đếm này ở RAM.

**Không xác nhận M8 PASS khi chưa có Build/flash và quan sát thực tế.**
