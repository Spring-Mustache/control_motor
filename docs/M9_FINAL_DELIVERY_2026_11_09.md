**CẬP NHẬT 11/11/2026 — NGƯỜI VẬN HÀNH XÁC NHẬN KẾT QUẢ TRÊN XE.** Trong bối cảnh bản bàn giao `b1c1e9c4`/profile4, người dùng xác nhận bánh tạo phản lực khi bị tác động và không còn quay qua lại nhiều lần để dò góc trong lần thử. Đây là **USER-REPORTED định tính**, chưa kèm log SHA MCU, lực/góc/thời gian/dòng/nhiệt hoặc phạm vi từng trục/tải. CI firmware run42759998485 có ba job kiểm tra/build thành công. Động học tổng thể chưa tích hợp, SELECT tạm hoãn. Bản bàn giao hai phiên và nội dung báo cáo mới nằm tại [hồ sơ 11/11](M9_SESSION_HANDOFF_2026_11_11.md). Các khối có ngày cũ bên dưới giữ trạng thái tại thời điểm đã ghi.

# M9 — Bản bàn giao phần mềm cuối: giữ góc và tự phục hồi hai đĩa lái

**CẬP NHẬT 10/11/2026 — GHÌ RỒI THẢ BỊ HUNTING.** Người dùng báo phản lực có nhưng dò góc lâu/quay qua lại. Lần báo chưa có SHA MCU; mốc đã giao c9fd47fd, profile4 trước đó chỉ là nháp chưa push. Bản sửa gộp giữ tải liên tục với tốc độ Q8 lọc60ms, ngừng tích lũy I khi đang về đích, xả I đẩy sai chiều khi quá đích, Kp8/4 thay16/8. I vẫn giữ khi đúng goal/đứng yên; không thêm timer nghỉ.216 fixture bỏ tải/trễ có tiêu chí quá độ PASS,36 fixture giữ tải và600s exact-goal I retention PASS, UBSan và41 regression PASS. Đây là HOST, chưa kết luận hardware hunting đã hết. Xem [báo cáo giảm hunting](M9_RELEASE_SETTLING_2026_11_10.md).


**HIỆN HÀNH — M9 GIỮ TRỤC LIÊN TỤC, PROFILE4 (2026-11-10, giờ Việt Nam).** Người dùng nhắc lại đây là bài toán chính của M9. Bản c9fd47fd/profile3 hồi vị có nghỉ không đáp ứng đầy đủ yêu cầu; gọi hoàn thiện là kết luận quá mức. Đã thay cấu hình mặc định bằng PI vị trí + D vận tốc: giữ I/lực bù tại goal, không NO_PROGRESS/cooldown định kỳ khi bị ghì, bỏ ngưỡng8RAW (bỏ nhiễu±1RAW), anti-windup và xóa I khi STOP/NC/owner/fault. Giữ sampler20ms/trục, lease50ms và hai fix Rotate.41 kiểm tra local,36 mô hình tải/ma sát, giữ output tại goal600s giả lập và UBSan PASS; CI/ARM phải kiểm đúng SHA. Logic liên tục đã thực hiện; mô-men/nhiệt và độ lệch thật **CHƯA KIỂM CHỨNG**. [Hồ sơ sửa đúng yêu cầu M9](M9_CONTINUOUS_HOLD_CORRECTION_2026_11_10.md). Các chỉ dẫn profile3/“final hồi vị” bên dưới là lịch sử đã được thay thế.


Ngày chốt mã: 09/11/2026 UTC. Nhánh `feature/m9-steering-active-position-hold`.
Nền trực tiếp: `a944da4f7f66987c96883256ac34914962714562` (M9 + Rotate đã fix).
Commit của bản này được xác định bằng Git HEAD/CI và `BUILD_MANIFEST.json`, không dùng một SHA tự tham chiếu trong chính file.

## 1. Kết quả bàn giao và phạm vi xác nhận

M9 được chốt thành bộ điều khiển giữ mục tiêu hai trục, tự phục hồi khi bị đẩy lệch sau CENTER hoặc lệnh góc Voice 5..40 hoàn thành cả hai bên. Bộ điều khiển không hết hạn theo thời gian phiên, không cần kích biến Live Expressions và không phụ thuộc bật cờ debug để đọc cảm biến.

Bản này thay profile thử PD30/10 + xung300/200ms và85/12ms bằng profile3 **phục hồi vị trí theo phản hồi**. Chỉ tạo lực khi có sai lệch cần sửa. Khi đến mục tiêu, PWM về0; mục tiêu tiếp tục được giám sát để lần tác động sau lại được hiệu chỉnh. Không cam kết khóa cơ khí tuyệt đối hoặc giữ mô-men bằng0 sai số.

**Trạng thái:** phần mềm đã triển khai;40 lệnh hồi quy local PASS, bao gồm các bài trên mã C thật,162 mô hình động lực học giả lập và UBSan. GitHub Actions phải hoàn tất ở đúng commit trước khi bàn giao; workflow bổ sung biên dịch/link toàn bộ STM32F767ZI và lưu ELF/HEX/BIN kèm manifest. Kết quả build không đồng nghĩa đã nạp/chạy trên xe. **CHƯA CÓ DỮ LIỆU HARDWARE cho profile3**, chưa có số đo lực giữ, thời gian ổn định, dòng hay nhiệt của bản này. Không bắt người dùng chạy thêm vòng thử mẫu trước khi nhận mã final.

## 2. Những vấn đề đã xử lý trong mã

| Vấn đề trước | Hành vi của bản chốt |
|---|---|
| PWM nền30/10CCR thấp; xung rời rạc và nghỉ làm cơ cấu dễ rít/nhích | Dùng nền vận động LEFT175..300, RIGHT50..100CCR từ M8; tăng bù ma sát trong chính trần này khi phản hồi không tiến triển |
| Bù khởi động mất đi ngay khi góc nhích nhẹ | Giữ phần bù đã tăng trong suốt lần phục hồi; đặt lại khi đạt mục tiêu |
| Lệch lớn từng làm mất goal | Không có ngưỡng lỗi32RAW hủy HOLD; kiểm tra dải cơ khí và độ hợp lệ cảm biến riêng |
| Lấy mẫu idle mỗi trục khoảng100ms | HOLD đọc một cảm biến mỗi10ms, luân phiên; danh định20ms/trục; đọc có timeout, không thêm vòng I2C trong controller |
| Một trục không tiến triển làm cả hai nghỉ | Bộ đếm phục hồi/nghỉ riêng từng trục; bên còn lại tiếp tục sửa góc |
| Không có deadline cho mọi mức PWM | Mọi PWM của profile3 cần lease50ms; SysTick cắt cả hai nếu hết lease hoặc tuổi mẫu đạt80ms |
| HOLD có thể tranh quyền với Rotate vừa đủ lệnh | Thu hồi HOLD trước arbitration khi Rotate LEFT/RIGHT đã đủ lệnh; xóa deadline cũ trước khi R2 ghi PWM |

Các LED RUN/ERR/iOVER và tiếng rít do người dùng báo trước đây không chứng minh nguyên nhân điện/cơ duy nhất. Bản này sửa các hạn chế nhìn thấy được trong thuật toán; chưa xác nhận sức kéo trên phần cứng.

## 3. Thuật toán và thông số

Với mỗi trục, `e=wrap4096(goal-raw)`; tốc độ được tính từ thay đổi góc trên cửa sổ40ms. Bắt đầu hiệu chỉnh ở `|e|>=8RAW`, ngừng hiệu chỉnh vị trí ở `|e|<=4RAW`. RAW là đơn vị AS5600, không gọi là độ bánh xe khi chưa hiệu chỉnh động học.

Lệnh cơ sở: `u = minPWM + Kp*|e| + bù_ma_sát - Kd*v_hướng_về_đích`, sau đó giới hạn PWM và tốc độ tăng. Khi dự đoán tới vùng đích trong một cửa sổ tốc độ, ngừng cấp lực để trôi; không phát một cú đảo chiều chỉ vì thành phần D. Nếu đã vượt mục tiêu và cần đổi chiều, có200ms PWM0 trước đổi DIR. Giảm lực và cắt tại mục tiêu có ưu tiên hơn tăng lực.

| Đại lượng | LEFT | RIGHT |
|---|---:|---:|
| Nền PWM lúc đang hiệu chỉnh | 175CCR | 50CCR |
| Trần PWM | 300CCR | 100CCR |
| Kp (CCR/RAW) | 0,5 | 0,25 |
| Kd (CCR/(RAW/s)) | 0,125 | 0,046875 |
| Tốc độ tăng tối đa | 2000CCR/s | 1000CCR/s |
| Khoảng PWM0 trước đổi chiều | 200ms | 200ms |
| Không tiến triển khi thực sự cấp PWM | 1000ms | 1000ms |
| Thời gian cấp PWM tối đa mỗi đợt phục hồi | 5000ms | 5000ms |
| Nghỉ khi kẹt/hết đợt rồi tự phục hồi | 3000ms | 3000ms |

Nền và trần PWM, Kp lấy từ biểu thức vị trí M8 `min+|e|/2` và `min+|e|/4`; tốc độ tăng, damping, dự đoán và bù ma sát là quyết định thiết kế của bản này, chưa hiệu chỉnh trên xe. Phần bù bắt đầu tăng sau200ms thời gian cấp lực mà chưa tiến thêm2RAW, tăng trong300ms và luôn nằm trong trần M8. Hết đợt không xóa goal. Đây là giới hạn phản hồi/thời gian của phần mềm, **không phải giới hạn dòng ampere hoặc bảo vệ nhiệt**. Không có dữ liệu dòng đưa về MCU.

## 4. Quyền điều khiển và các điều kiện kết thúc

- Boot/reset: không goal, không lực HOLD. Có thể nhận mục tiêu khi CENTER/R1 hoặc Voice5..40 có BOTH-DONE hợp lệ.
- Lệnh chạy bánh tiến/lùi thông thường giữ HOLD độc lập. Manual steering, R1/R2 mới, SELECT, Rotate và spin thu hồi quyền trước owner mới.
- R2/Voice45 kết thúc tại NC không kích HOLD tại công tắc giới hạn.
- STOP, NC, sensor lỗi/quá tuổi, ra ngoài dải cơ khí, mất điều kiện PS2/safety hoặc quá hạn servo lease cắt và thu hồi. Điều kiện trở lại bình thường không tự phục hồi goal đã bị hủy; cần một lệnh vị trí mới hoàn thành.
- Trường hợp chỉ bị kẹt/hết thời gian phục hồi giữ goal, nghỉ riêng trục rồi tự tính lại từ góc mới.
- GPIO NC, pending EXTI và epoch NC được kiểm tra lại trong vùng ghi PWM nguyên tử. Mapping DIR giữ như M8: LEFT RAW+ DIR0; RIGHT RAW+ DIR1.
- SysTick watchdog cần SysTick còn chạy; không phải watchdog phần cứng độc lập và không chứng minh bảo vệ khi CPU bị debugger halt hoặc toàn bộ interrupt bị khóa.

## 5. Rotate được giữ nguyên

Giữ hai sửa lỗi ở nền `b382354e` và sửa chuyển quyền ở `a944da4f`:
1. R2 đã bắt đầu không bị hủy chỉ vì bánh bị quay thụ động; điều kiện không có lệnh/PWM kéo bánh vẫn được kiểm tra.
2. Spin chỉ bắt đầu đếm giây sau khi wheel API thực sự chấp nhận lệnh; có thời hạn chờ nhận lệnh, STOP và NC hiện hành.
3. HOLD nhường quyền ngay khi lệnh Rotate đã hoàn chỉnh, kể cả trước lúc state Rotate chuyển sang R2.

Bằng chứng phần cứng trước đây thuộc `b382354e`: Rotate RIGHT yêu cầu5000ms, lệnh thực5006ms; LEFT yêu cầu10000ms, lệnh thực10002ms; người dùng xác nhận hoạt động. Các số đó không phải số đo của firmware M9 final.

## 6. Kiểm chứng có thể tái lập

- `python tools/verify_m9_h2b1_hold.py`: mã controller và adapter HAL/main thật, fixture lịch sử tách riêng; profile3 thực kiểm quiet60s, PWM300/100, cắt ngay khi về goal, poll10ms/50ms, lease50ms, stale80ms, STOP/NC/owner/link/range, wrap và chuyển sang Rotate.
- `python tools/verify_m9_final.py`: compiler từ chối cấu hình sai;162 kịch bản vòng kín giả lập (3 mức ma sát ×3 tốc độ ×3 quán tính ×2 hướng ×3 độ lệch),60s/kịch bản; kiểm giữ yên, nghỉ/tự phục hồi, hai trục độc lập, đảo chiều, wrap; chạy lại với undefined-behavior sanitizer.
- Toàn bộ40 lệnh hồi quy local M8/M9 PASS; gồm parser/executor/Rotate API thật, STOP và quyền điều khiển.
- `python tools/build_firmware_ci.py`: biên dịch các C/assembly của Core, Services, HAL; link với script FLASH thật cho Cortex-M7 hard-float, đóng dấu SHA đầy đủ. ELF/HEX/BIN/map và `BUILD_MANIFEST.json` nằm trong artifact Actions `M9-final-<full SHA>` sau job thành công.
- CI kiểm thêm PowerShell5.1 trên Windows, yêu cầu386 dòng gốc và bàn giao17 mục.

Mô hình giả lập không dùng thông số motor đã đo. PASS mô phỏng không xác định sai số/thời gian đáp ứng trên xe. Build ARM từ CI cũng không phải xác nhận CubeIDE Windows hoặc flash phần cứng.

## 7. Cập nhật một lần để dùng bản bàn giao

Project chính: `D:\STM32 task\control_motor`. Giữ thay đổi local `.settings`, startup và logs; không reset/clean/stash/force-checkout. `Clean Project` trong CubeIDE là bước build riêng, không phải `git clean`.

```powershell
cd 'D:\STM32 task\control_motor'
git fetch origin
if ($LASTEXITCODE -ne 0) { throw 'Fetch failed' }
git switch feature/m9-steering-active-position-hold
if ($LASTEXITCODE -ne 0) { throw 'Giu nguyen thay doi local; chua chuyen duoc nhanh' }
git merge --ff-only origin/feature/m9-steering-active-position-hold
if ($LASTEXITCODE -ne 0) { throw 'Giu nguyen thay doi local; chua cap nhat duoc' }
git log -1 --oneline
```

Sau khi Actions đúng SHA thành công: CubeIDE **Refresh → Clean Project → Build → Flash/Debug → Resume F8**. Chạy CENTER/R1 hoặc lệnh góc hợp lệ như thao tác bình thường; HOLD tự nhận goal khi hai bên hoàn tất. Không phải kích `POWER_COMMISSIONED`, `enable` hoặc bất kỳ biến Live Expressions nào. `commissioned=0,profile=3` có nghĩa profile phục hồi có thể cấp lực, còn nghiệm thu vật lý chưa được ghi nhận.

Lệnh ghi log thụ động sẵn có để lưu dữ liệu vận hành nếu cần, không phải một vòng thử mẫu bắt buộc trước khi nhận final:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\capture_m9_h2b1_hold.ps1 -TimeoutSeconds 300 -MaxEvents 100 -UntilStopped
```

Collector chỉ nghe. Kết thúc collector không dừng motor; STOP vận hành vẫn là đường kết thúc điều khiển.

## 8. Đoạn có thể dùng trong báo cáo dự án

“Milestone M9 triển khai chức năng duy trì mục tiêu góc của hai cơ cấu lái sau khi lệnh vị trí hoàn tất. Bộ điều khiển sử dụng phản hồi AS5600, hiệu chỉnh độc lập từng trục, bù ma sát có giới hạn, giảm lực khi tiến gần mục tiêu và chuyển quyền điều khiển với PS2/giọng nói. Mục tiêu được giữ sau khi ngoại lực làm sai lệch; khi một trục không tiến triển, trục đó tạm nghỉ và tự phục hồi mà không làm mất mục tiêu của trục còn lại. Phần mềm đã được kiểm chứng bằng hồi quy trên mã C thật và các mô hình mô phỏng. Chưa có số liệu thực nghiệm của phiên bản cuối để kết luận định lượng về lực giữ, độ chính xác, thời gian ổn định và nhiệt độ.”

Có thể bổ sung “đã biên dịch/link toàn bộ STM32F767ZI” sau khi job ARM của commit cuối PASS. Không ghi “hoàn tất nghiệm thu M9 trên xe”, “chống lệch tuyệt đối”, “không bao giờ dừng” hoặc các số phần trăm chưa đo. Động học tổng thể/SELECT đang hoãn thuộc phạm vi riêng, không được tự nâng thành hoàn tất bởi bản M9 này.
