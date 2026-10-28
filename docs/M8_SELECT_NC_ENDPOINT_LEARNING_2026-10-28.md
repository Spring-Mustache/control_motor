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

## Quyết định vận hành 14:07 — tạm hoãn SELECT, giữ các RAW hiện tại

**Nguồn log:** `M8_S00_20261028_140731.txt` do người vận hành gửi lúc 14:07, 2026-10-28.

- `M8S00_CAL,starts=24,complete=0,failed=24,rejected=0,result=9,last_run=9,stageL=1,stageR=1`. Đây là số lượt **trong phiên từ khi khởi động**, không phải 24 công tắc bị lỗi.
- `M8S00_CAL_ABORT,count=24,source=1,mask=0x00000500,ageMs=131,rawL=2264,rawR=245,wpwmTarget=0,wpwmNow=0,rpm10LU=0,rpm10LD=0,rpm10RU=21,rpm10RD=0`. Bit 8 = wheel-stopped gate không đạt; bit 10 = encoder bánh vượt ±2 RPM; bit 9 (lệnh/PWM truyền động) **không bật**. Bánh RU đo +2,1 RPM và không có yêu cầu truyền động bánh. Điều này cùng quan sát cơ khí trước đây củng cố nguyên nhân **bánh xê dịch thụ động khi đánh lái**, gây hủy SELECT mã 9. Không suy diễn nguyên nhân của các 23 lượt trước từ snapshot lần hủy cuối.
- `M8S00_CAL_CONTACT,minL=0,maxL=0,minR=0,maxR=0` và `complete=0`: chưa có 4 giá trị giới hạn nào được học thành công qua SELECT trong phiên này.
- `M8S00_ESCAPE,origin_irq_L=0,origin_irq_R=0,fault_L=0,fault_R=0`, `M8S00_EMPTY,STOP/FAULT`: phiên SELECT này không ghi nhận lỗi vị trí R1/R2; **không đủ để chứng nhận toàn bộ hành trình an toàn**.
- Giới hạn runtime **không đổi**: LEFT MIN=2297, MAX=1374, CENTER=1873; RIGHT MIN=166, MAX=982, CENTER=662. Mục tiêu R2: LEFT=2297, RIGHT=191.
- **Ghi nhận từ quan sát trực tiếp của người vận hành, không phải giá trị SELECT đã xác nhận trong log:** sau khi điều chỉnh cơ khí nhẹ công tắc **R_MIN phải thực sự tác động tại AS5600 RAW khoảng 193**. Con số này khác giới hạn runtime đang lưu `166` là 27 RAW và nằm khoảng 2 RAW *phía trong công tắc* so với setpoint R2 `191` theo chiều RIGHT giảm RAW về MIN. Đây là lý do cần theo dõi khả năng R2 gặp NC trước khi đạt setpoint; không tự ý cập nhật giới hạn an toàn hay mục tiêu R2 khi chưa đo hành trình/trôi an toàn.
- Người vận hành đánh giá các RAW hiện tại nhìn chung đã gần vị trí mong muốn, chỉ lệch nhẹ; **quyết định tạm thời bỏ qua việc thử/sửa SELECT** và tiếp tục công việc điều khiển R1/R2, chạy bánh + đánh lái độc lập. Không yêu cầu làm tiếp 24 lần thử, không tăng ngưỡng RPM và không nới biên thăm dò.

**Phân biệt trạng thái:** lỗi hủy SELECT tại lần cuối đã được chứng minh do điều kiện bánh đứng yên/encoder; nguyên nhân cơ khí được người vận hành quan sát; R_MIN ≈193 là số đo do người vận hành cung cấp; các giới hạn runtime của firmware vẫn là giá trị cũ. **Chưa chỉnh sửa firmware hay ghi giá trị mới vào RAM/Flash.** Khi quay lại SELECT cần quyết định riêng về khoảng dự phòng NC, phương pháp học contact và điều kiện hoạt động an toàn.

## Lịch sử M8 R1/R2 — phương án dừng trước NC (ĐÃ BỊ THAY THẾ)

Từ log cũ, công tắc L_MIN được thấy tác động ở RAW khoảng 2274. Theo quan sát cơ khí mới của người vận hành, R_MIN hiện tác động tại khoảng 193 sau khi chỉnh lại vị trí. Đây là hai điểm NC quan sát, **không phải kết quả học SELECT**. R1 CENTER không đổi: LEFT 1873, RIGHT 662. Các giới hạn NC runtime vẫn LEFT MIN 2297/MAX 1374, RIGHT MIN 166/MAX 982, không được ghi đè.

R2 được đổi sang mục tiêu **ở phía trong công tắc** để tránh lặp lại việc chạm rồi ESCAPING: LEFT=2240 (34 RAW lùi khỏi điểm NC quan sát), RIGHT=223 (30 RAW lùi khỏi điểm NC quan sát). Đây là khoảng dự phòng khởi đầu **chưa xác minh quãng trôi sau cắt PWM**; tuyệt đối không gọi là khoảng dừng an toàn đã được thử. Giữ nguyên NC EXTI cắt PWM, kiểm tra giới hạn AS5600, xử lý khi motor đã nằm tại NC, và SELECT đang tạm hoãn. Không lấy setpoint R2 làm giá trị MIN thực.

Bài nghiệm thu trên giá treo và cắt nguồn động lực độc lập: trước tiên thử R2 riêng, xác nhận cả hai cơ cấu dừng **trước khi đè công tắc** (đồng thời đo RAW thực tế sau khi đứng yên); chỉ khi đạt mới tiếp tục R1 một lần. R1 PASS khi mỗi motor về CENTER (±5 RAW, ổn định) không cần nhấn lần hai và không có `M8S00_FINE` reason khác 0. Nếu motor còn chạm NC hoặc trôi qua khoảng hở, dừng, không mở rộng thử nghiệm. Tốc độ/độ mượt cần đánh giá thực tế; log STOP/FAULT không chứng minh đã có chạy bánh và lái đồng thời. Không sửa PWM, không tăng số xung, và không đổi logic R1 fine trong commit setpoint này vì `code=10` cũ chưa được giải mã thành `reasonL`.

## Quyết định mới — xoay tại chỗ cần cả hai công tắc MIN

Người vận hành xác nhận rằng hình học của xe chỉ đạt góc cần thiết để **L1/L2 xoay tại chỗ** sau khi cơ cấu lái **trái và phải cùng chạm công tắc MIN**. Do đó hai setpoint tạm thời `2240/223` (lùi trước NC) trong phần lịch sử ở trên **không được sử dụng nữa**.

Firmware M8 chuyển R2 thành lệnh **tìm công tắc MIN vật lý**, với yêu cầu:
1. Mỗi trục chỉ được kết thúc R2 thành công (`result=3`) khi công tắc MIN của chính nó tác động ở mức GPIO hoặc ngắt EXTI. RAW cách đích ±5 **không** được coi là hoàn thành R2.
2. Các RAW `L=2297, R=166` trong mã chỉ là **điểm tham chiếu tiếp cận/giới hạn lỗi khi NC không xuất hiện**, không phải điều kiện hoàn thành. Đặc biệt R_MIN đã được người vận hành chỉnh cơ khí và quan sát tác động khoảng **193 RAW**; đây vẫn là quan sát chứ không phải kết quả SELECT.
3. Nếu không có NC trước giới hạn RAW tiếp cận, quá thời gian hoặc phát hiện AS5600 sai hướng, trục dừng lỗi, **không ép qua chặn cứng để cố lấy NC**.
4. L1/L2 chỉ bắt đầu chế độ quay tại chỗ khi **đồng thời có cả L_MIN và R_MIN đang tác động**, và không có L_MAX/R_MAX. NC nhả khi spin đang chạy => cắt PWM bốn bánh ngay bằng `WheelControl_EmergencyStop()`; đây là dừng khẩn cấp phần mềm, không thay thế ngắt nguồn động lực độc lập. Một yêu cầu L1/L2 chưa đủ NC bị chặn, tăng bộ đếm `ps2_spin_nc_block_frames`.
5. Thêm `M8S00_SPIN_NC` vào báo cáo UART xuất sau khi DISARM: `blocked_frames`, `active_cuts`, `last_mask`. Mã mask L_MIN=4, R_MIN=2; cả hai MIN đúng, không MAX là `0x06`.
6. R1 vẫn phải rời NC và quay về CENTER **trong một lệnh**. Không hạ điều kiện NC, bỏ AS5600, thay tốc độ PWM hay làm lại SELECT ở bước này.

**Bài nghiệm thu theo thứ tự:** (a) nguồn động lực có công tắc ngắt độc lập, xe trên giá treo chắc chắn; (b) R2, xác minh **cả hai NC tác động** và hai trục đều báo result=3, không có fault/trip; (c) nhấn R1 một lần, xác nhận mỗi trục rời NC rồi về CENTER; (d) chỉ thử L1/L2 với bánh treo tự do sau khi R2 đã đạt hai NC và tất cả điều kiện dừng vẫn an toàn. Không thử xoay tại chỗ chịu tải trên mặt đất trước khi vượt qua các bài kiểm tra giá treo. Không đánh đồng tiếp xúc NC với chứng nhận quãng trôi an toàn/hình học hoàn chỉnh.

Trạng thái: **đã sửa mã nguồn, chưa kiểm chứng STM32CubeIDE Build, chưa flash và chưa nghiệm thu phần cứng**.

## M8-S00 14:49 — kết quả kiểm tra R1 và cờ công tắc hai trục

**Log gốc:** `M8_S00_20261028_144914.txt`, người vận hành cung cấp ngày 2026-10-28.

- Sự kiện STOP đầu tiên: `event=4,axis=2,code=2`, tức motor **phải** được báo DONE bình thường trong bộ vị trí R1/R2 (không phải lỗi). R1 CENTER phải = 662; snapshot tại `rawR=660`, sai lệch -2 RAW. `fine_phase_R=3`, `fine_pulses_R=1`, `reasonR=0`.
- Tại **cùng thời điểm ghi sự kiện của motor phải**, motor trái ở `rawL=1879`, CENTER=1873, sai lệch +6 RAW; `fine_phase_L=2`, `fine_pulses_L=1`, `fine_error_L=-6`, `reasonL=0`. Snapshot này **không ghi nhận motor trái đã hoàn tất** sau đó. `plState=1/prState=1` có thể phản ánh thứ tự capture trước khi hàm `Steering_PositionAxisDone` cập nhật state.
- `irqPosition=6` tương ứng cờ L_MIN=4 và R_MIN=2 đã xuất hiện từ lúc bắt đầu tác vụ, nhưng tại snapshot về CENTER `nc=0`. Không được nhầm cờ IRQ lịch sử là hai NC đang đồng thời tác động tại lúc xuất log.
- `M8S00_ESCAPE,origin_irq_L=3,origin_irq_R=3,fault_L=0,fault_R=0`: cả hai bên đều từng gặp ngắt NC của công tắc xuất phát trong quá trình ESCAPING, tổng 3 mỗi bên **kể từ boot**. Các bộ đếm không xác nhận số lần R2 thành công hoặc tình trạng hình học khi xoay tại chỗ.
- `M8S00_EMPTY,FAULT`, `trip=0`: không ghi nhận fault vị trí hoặc safety trip trong snapshot/phiên quan sát, **không phải chứng nhận mọi chức năng PASS**.
- `M8S00_SPIN_NC,blocked_frames=0,active_cuts=0,last_mask=0x00`: không có lượt spin bị NC chặn/cắt được ghi; **không chứng minh** L1/L2 đã thử hoặc đang hoạt động đúng.
- `M8S00_CONCURRENT,request=0,fourWheel=0,oneSteer=0,bothSteer=0`: không có bằng chứng bánh và lái thực sự vận hành đồng thời trong phiên.
- SELECT không dùng (`starts=0`), giữ MIN runtime L=2297/R=166, CENTER L=1873/R=662; R2 đích tiếp cận L=2297/R=166. R_MIN thực tế đã chỉnh cơ khí chạm khoảng RAW 193 theo quan sát người vận hành.

**Quyết định:** ghi nhận tiến triển R1 phải CENTER ±2 RAW; trái đang cách CENTER 6 RAW khi phải DONE, nhưng chưa xác nhận trái DONE trong cùng lần R1. Giữ nguyên code PWM, R1 fine và interlock NC-spint tại bước đọc log; không tuyên bố R2 đạt đủ hai NC hay L1/L2 PASS. Lần xác minh kế tiếp chỉ cần đối chiếu kết quả **cả hai motor R2 thật sự đè NC, sau đó R1 một lần về CENTER**, ưu tiên quan sát rõ khi LEFT kết thúc hoặc một log trạng thái cuối; thử trên giá treo chắc chắn có cắt nguồn động lực độc lập.
