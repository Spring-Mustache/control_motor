# M8 SELECT — học điểm MIN/MAX từ NC thật (2026-10-28)

## Mục tiêu

Khi nhấn SELECT ở chế độ hiệu chuẩn, MIN và MAX của từng trục được đo tại thời điểm công tắc NC **tương ứng thực sự tác động**, không đặt là một số RAW cố định. LEFT và RIGHT có hai chiều RAW khác nhau; không so sánh số RAW của hai trục để quyết định đã đến hành trình.

## Trạng thái thực tế của firmware

Luồng SELECT hiện tại **đã học theo NC**: `STEER_CAL_SEEK_MIN` đọc AS5600, chỉ gán `axis->min_raw=raw` khi `limit==min_bit`; `STEER_CAL_SEEK_MAX` làm tương tự với `max_bit`. Sau khi rời từng công tắc và trở lại CENTER, cả bốn số đo chỉ được ghi vào `steer_cal_*_runtime` nếu **cả hai** trục về `STEER_CAL_DONE`, thứ tự RAW không cuộn 4095→0 và hành trình đo có hình học hợp lệ. Lần hiệu chuẩn thất bại **không thay một nửa** giới hạn đang sử dụng.

Một lớp bảo vệ riêng vẫn giới hạn phép thăm dò tối đa 64 RAW bên ngoài các điểm dừng đã học trước đó. Đây là chặn phòng trường hợp NC hỏng/mất dây, **không phải quy tắc định nghĩa MIN/MAX**. Để thay nó bằng ngân sách di chuyển độc lập khỏi RAW cũ, cần chứng minh quãng quay vật lý tối đa, độ trôi khi cắt PWM và trạng thái NC ở cả bốn đầu. Không bỏ chặn này chỉ bằng cách tăng hằng số hoặc cho motor chạy vô thời hạn.

CENTER hiện vẫn là RAW cố định đã đo: LEFT=1873, RIGHT=662. R1 dùng CENTER này. Mục tiêu R2 vẫn tách biệt và cố định LEFT=2297, RIGHT=191 (và chỉ bị kẹp nếu vượt qua NC runtime mới). SELECT học giới hạn **chưa tự tính lại R2**; cần đo quãng trôi rồi chọn một vị trí R2 **lùi vào phía trong** NC để chạy bình thường không đè công tắc.

## Bổ sung M8-S00 (không sửa PWM, giới hạn hay trình tự SELECT)

Nhấn USER ngắn sau khi DISARM có thể xuất log cả trong một phiên **chỉ chạy SELECT**, không cần R1/R2 trước đó. File UART có ba dòng mới:

- `M8S00_CAL`: số lần SELECT, số lần PASS/FAIL, mã từ chối, trạng thái và lỗi mỗi trục.
- `M8S00_CAL_CONTACT`: 4 RAW được đọc khi công tắc MIN/MAX thực sự tác động và thời gian chạm mỗi công tắc.
- `M8S00_CAL_RUNTIME`: bốn RAW đã được chấp nhận làm giới hạn runtime, CENTER cố định, mục tiêu R2 hiện hành và lý do lỗi chính.

Nếu `complete=0`, các RAW `*_CONTACT` chỉ là **kết quả quan sát**, không được coi là đã ghi thành giới hạn runtime. Nếu NC phải không tác động, `minR/maxR` sẽ còn bằng 0 theo phía chưa chạm; không được lấy vị trí motor đang đứng làm MIN/MAX.

## Bài thử an toàn

1. Đặt xe trên giá treo chắc chắn, motor bánh không chạm mặt đất; có công tắc **cắt nguồn động lực độc lập** đã được kiểm tra. Kiểm tra NC của cả hai trục khi **ngắt nguồn động lực** trước khi chạy SELECT.
2. Build/flash commit mới; không Generate Code từ `.ioc` đang khác cấu hình I2C main.c. Mở PS2 analog, các nút về neutral. Hold START ~1,5 s để ARM, thả hoàn toàn.
3. Hold SELECT khoảng 0,6 s rồi thả. **Không giữ thêm các nút khác hoặc lái LY/D-pad**. Quan sát SELECT tuần tự tìm MIN, rời MIN, tìm MAX, rời MAX và đưa về CENTER. Nếu motor chạy sai hướng, không ngắt được, vượt phạm vi cơ khí, NC không chuyển trạng thái hoặc có dấu hiệu kẹt: cắt nguồn động lực.
4. Sau khi dừng hoàn toàn, disarm bằng START mới, không reset MCU; chạy `powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s00.ps1` trong PowerShell rồi nhấn USER ngắn. Gửi nguyên file TXT.
5. Nếu SELECT báo lỗi do giới hạn thăm dò 64 RAW, **không tăng trực tiếp giới hạn này** trước khi đánh giá khoảng quay cơ khí an toàn và giá trị tại công tắc từ log.

Trạng thái: mã xuất báo cáo đã commit, nhưng **chưa Build/flash/test phần cứng**. Mục tiêu cuối cùng `SELECT → 4 NC thật → 4 RAW runtime → CENTER` chưa được xác minh hoàn tất trên xe.


## Lượt SELECT 14:00 — hủy sớm mã 9

Log `M8_S00_20261028_140031.txt`: `starts=11,failed=11,last_run=9,stageL=stageR=1`; cả bốn `*_CONTACT=0`. Không thể quy lỗi cho công tắc NC hoặc biên 64 RAW vì chưa có bất kỳ lần chạm nào được ghi. Mã 9 đi từ **hai** nơi: trong `Steering_CalUpdate()` (control/STOP/PS2 neutral/owner/test/wheel gate), hoặc lớp bảo vệ chung sau cùng khi permit/trip bị thay đổi.

Giả thuyết đáng ưu tiên: `Steering_PositionWheelsStopped()` kiểm tra encoder ±2 RPM liên tục khi SELECT đang SEEK; trên mặt đất đánh lái gây dịch chuyển bánh thụ động (xe có tâm quay cơ cấu lái lệch tâm bánh). Điều này phù hợp tiền sử M8 nhưng **chưa được chứng minh trong lượt 14:00** vì file chưa lưu điều kiện gây mã 9.

Bản chẩn đoán bổ sung (commit sau 14:00) xuất `M8S00_CAL_ABORT` với `count`, `source`, `mask`, `ageMs`, RAW và cả bốn RPM tại lúc abort. `source=1` là điều kiện hủy trong SELECT, `source=2` là lớp bảo vệ chung. Mã `mask` là bitset: 0 control mode khác PS2; 1 control STOP; 2 cancel; 3 spin/RF; 4 PS2 không neutral; 5 cal_arm bất ngờ; 6 owner lái khác; 7 test/debug/voice; **8 check bốn bánh dừng thất bại**; **9 motor bánh đang có lệnh hoặc PWM**; **10 encoder bánh vượt ±2 RPM**; 11 mất permit; 12 safety trip; 13 mất PS2/digital. Cần xét cả bit 8, 9, 10: bit 8 + 10 nhưng **không có bit 9** là bằng chứng phù hợp bánh dịch chuyển thụ động, vẫn cần đối chiếu quan sát cơ khí.

**Không sửa hoặc bỏ check bánh dừng, NC, anti-stall hay biên thăm dò 64 RAW trong lần bổ sung này.** Bài thử tiếp theo là một SELECT duy nhất trên giá treo và cắt nguồn độc lập; sau khi disarm xuất TXT và xem `M8S00_CAL_ABORT`. Chưa PASS và chưa target Build/flash trên STM32.
