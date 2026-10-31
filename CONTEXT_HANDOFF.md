# CONTEXT HANDOFF

> **HỒ SƠ SỐNG 17 MỤC:** Giữ nguyên dữ kiện và quyết định lịch sử A, đồng thời **cập nhật lớp hiện hành** sau thay đổi code/nhận định/lần đo. **Không lấy SHA hoặc trạng thái ở ảnh chụp gốc làm HEAD.** Người tiếp quản phải xác minh trước nhánh `feature/m7-s15-ps2-controls-restoration-staged` và SHA thực tế ở GitHub, rồi đọc mục 1 hiện hành, toàn bộ 0–16 và roadmap cùng SHA. Git commit là nguồn chính, Actions kiểm tra mỗi push; Release chỉ theo milestone/yêu cầu.

---

## 0. Thông tin hồ sơ và phạm vi bao phủ

### Hai lớp nội dung — KHÔNG gộp lịch sử vào trạng thái hiện tại

- **Lịch sử bất biến theo thời điểm:** bảng 2026-10-31 dưới đây, các SHA, log, giả thuyết và đánh giá khi lập hồ sơ A; không xóa hay viết lại để biến thành bằng chứng của commit mới.
- **Cập nhật hiện hành:** xem mục 1 ở đầu (được cập nhật theo các thay đổi có nguồn), các phần bổ sung dưới những mục 0–16 và các mục mới ở cuối `docs/PROJECT_MILESTONES.md`. CI chỉ kiểm tra cấu trúc/update-gate, không tự đọc toàn bộ chat.
- **SHA hiện tại:** không thể tự nhúng SHA của chính commit vào file cùng commit mà không tạo vòng lặp. Trước mỗi lần tiếp quản phải lấy `refs/heads/feature/m7-s15-ps2-controls-restoration-staged` từ GitHub, cố định SHA rồi đọc các file theo SHA đó. Nếu người dùng chỉ định nhánh khác, dùng nhánh mới.
- **Release:** không còn cần trùng HEAD. Một `context-<SHA>` cũ là snapshot tùy chọn, không phải trạng thái hiện hành. `docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md` giữ nguyên 386 dòng và SHA-256 gốc; mọi lần xuất hồ sơ vẫn phải áp dụng đủ 0–16.

**Bản định hướng A lúc đầu, lưu NGUYÊN ý nghĩa lịch sử (SHA không còn là HEAD):**

> **Định hướng nhanh cho hội thoại B.** Dự án **xe tăng cướp bóng**, firmware STM32F767ZI điều khiển **4 motor bánh + 2 motor đánh lái**. Nhánh làm việc hiện hành là `Spring-Mustache/control_motor`, `feature/m7-s15-ps2-controls-restoration-staged`, HEAD đã kiểm tra là **`605b937f8de5328d7b8c74c58accb9387392df90`**. Mục tiêu hiện hành: **đánh lái nhanh hơn khi xe vừa chạy vừa lái, đồng thời giữ hai motor lái bám góc**, không giải quyết đồng bộ bằng cách hãm LEFT liên tục. Firmware **M8-S14.2** mới nhất đã được **commit**, tích hợp **trần LEFT/RIGHT 375/150 hoặc 450/200 CCR và điều phối AS5600 góc–vận tốc: bù motor chậm trước, hãm motor nhanh chỉ khi bên chậm bão hòa**. Chế độ mới **mặc định OFF**, **CHƯA CÓ DỮ LIỆU CubeIDE Clean Build, Flash, hoặc thử thực tế của chính commit mới nhất**. Bước đầu của B: xác minh HEAD/workspace, chạy các verifier và Clean Build; chỉ lên phương án thử trên giá treo sau khi các điều kiện điện/cơ/NC/STOP đã đáp ứng. Người dùng đã **xác nhận lỗi FIRST-STOP kiểu RIGHT còn điện khi CCR=0 đã được fix**; **không coi đó là lỗi đang mở**, dù tài liệu cũ vẫn còn ghi cảnh báo. Lỗi R2 RIGHT `code=9` trước đây là **sự kiện khác**, đã có các lượt sau thành công nhưng chưa có kết luận nguyên nhân.

### Bảng bằng chứng của bản lập hồ sơ A (lịch sử)

| Thuộc tính | Nội dung |
|---|---|
| Dự án | `xe tăng cướp bóng` / STM32F767ZI, firmware `control_motor`, robot 6 motor |
| Hồ sơ | `CONTEXT_HANDOFF.md`, phiên bản bàn giao **1.0** |
| Thời điểm lập hồ sơ | **2026-10-31**, múi giờ `Asia/Ho_Chi_Minh`; **không có giờ tạo xác nhận bằng đồng hồ độc lập** |
| Điểm trạng thái mới nhất đã kiểm tra | HEAD GitHub `605b937f8de5328d7b8c74c58accb9387392df90`, kiểm tra bằng GitHub connector trong phiên lập hồ sơ; chưa đối chiếu ELF được flash trên MCU |
| Lần log phần cứng mới nhất đang có | CSV `M8S11_six_motor_20261030_205211.csv` và snapshot `M8_S00_20261030_205420.txt`, tức **trước** các commit tăng trần/đồng bộ tốc độ cao của M8-S14.2 |
| Nguồn cuộc hội thoại | Nội dung các lượt đang hiển thị trong A; **bản tóm lược cuộc hội thoại cũ** được hệ thống cung cấp; tài liệu/đoạn mã từ GitHub; các tệp được mount trong `/mnt/data` và các log do người dùng gửi |
| Khả năng bao quát | Kiểm tra các lượt gần đây trực tiếp và bản tóm lược các lượt cũ; kiểm tra repo, milestone, log và tài liệu pin. **KHÔNG THỂ KHẲNG ĐỊNH đã xem toàn bộ từng tin nhắn của phần lịch sử cũ bị rút gọn** |
| Giới hạn chính | Không có toàn bộ ảnh Live Expressions lịch sử, build log CubeIDE/ELF/hash flashed, bản ghi xác minh sửa FIRST-STOP hay dữ liệu thử mức 375/150 hoặc 450/200. Không có quyền quan sát xe thực tế từ A/B |

**Cách phân loại bằng chứng trong tài liệu này:**

- **[USER]**: người dùng nói/chấp thuận/báo cáo; là thông tin quan trọng, nhưng không tự đồng nghĩa kiểm thử độc lập.
- **[REPO]**: mã hoặc tài liệu đã đọc từ GitHub tại commit/nhánh nêu rõ; xác nhận nội dung nguồn, không xác nhận firmware đang chạy trên xe.
- **[LOG]**: giá trị kiểm tra trực tiếp từ TXT/CSV người dùng tải lên; phân tích số học phải ghi phương pháp.
- **[HOST]**: kiểm thử C trên máy chủ, không thay thế CubeIDE cross-build.
- **[ASSISTANT-TRƯỚC]**: thông tin assistant trước đã nói nhưng không có bằng chứng tương xứng tại đây; phải xem là báo cáo/chưa kiểm chứng.
- **[SUY LUẬN]**, **[CHƯA KIỂM CHỨNG]**, **[CHƯA CÓ DỮ LIỆU]**: dùng đúng nghĩa, không biến thành kết luận đã xác nhận.

**Tài liệu đã rà soát thực tế:** `docs/PROJECT_MILESTONES.md` (742 dòng, đầu và các phần cuối), các tài liệu M8-S12/S14/S14.1/S14.2 và tài liệu concurrent/FIRST-STOP được truy xuất qua GitHub (phạm vi một số tài liệu là các đoạn liên quan, không phải kiểm toán toàn văn), `Core/Src/main.c` và các header/test C liên quan, 5 CSV+metadata, 4 TXT M8-S00, DOCX pin (đọc văn bản/bảng), tên và phần mở đầu 4 PDF, danh mục của `Mobile_robot.zip`. **Không đọc toàn bộ trang của 4 PDF hoặc mọi CAD chi tiết trong ZIP**; đây là nguồn tham chiếu phụ, không nên trình bày như đã phân tích toàn phần.

**Cảnh báo về tài liệu lịch sử:** `docs/PROJECT_MILESTONES.md` có bảng trạng thái **viết ngày 2026-10-17** ở đầu và rất nhiều phụ lục cập nhật tới 2026-10-31. Bảng đầu **không phải bảng trạng thái hiện hành tuyệt đối**; phải đọc các mục mới hơn theo thứ tự thời gian. Tài liệu FIRST-STOP/concurrent cũng còn cảnh báo trạng thái cũ, đã bị cập nhật bằng xác nhận mới nhất của người dùng.

### B update S14.3, 2026-10-31

B update 2026-10-31: source HEAD c9f3032 confirmed on Windows, user reports CubeIDE build success before this change, but S14.3 firmware/ELF/Flash SHA unverified. Local B logs private, not uploaded public.

## 1. Đọc mục này để tiếp tục ngay

### CURRENT M8-S15 — một thời gian hoàn thành cho hai góc TARGET độc lập (2026-10-31)

[USER — thiết kế cốt lõi milestone] Dù LEFT/RIGHT đánh lái hai góc KHÁC NHAU, chúng phải dùng **cùng một thời gian hoàn thành T chung**, làm nền cho động học Ackermann. Không đồng nhất góc hay tốc độ giữa hai bên. Thay tầng điều khiển đồng bộ S14 PI/P+D/S149 bằng một bộ bám quỹ đạo có hai đầu ra độc lập; không bỏ nền tảng AS5600->±45°, NC/STOP, calibration, vị trí R1/R2, wheel PI/FF và log.

[BRANCH] Tạo nhánh NEW `feature/m8-s15-dual-target-common-time` từ S14.10 HEAD 0bdb4412a1b37c6397391eb5dd9802d921df32ff để giữ lịch sử. Nhánh OLD `feature/m7-s15-ps2-controls-restoration-staged` dự kiến quay về **chính xác commit trước PI M8-S14.7** `bcf6c2a4e122de91ee8fdb864f5fe7cfd10cca5d` (parent của S14.7 first commit 21e3f353...). Khi tiếp quản PHẢI xác minh refs thực tế; không suy cũ là HEAD mới. Lịch sử S14.7–S14.10 vẫn được giữ trên nhánh S15, không xóa tag/release.

[IMPLEMENTED SOURCE PILOT] `Core/Inc/steer_s15_common_time.h`: pure C M8S15_Init(startL,goalL,startR,goalR,requestedT)/M8S15_Step; cùng normalized cubic clock cho L/R nhưng góc vận tốc tham chiếu riêng. Thời gian T do yêu cầu hoặc auto dựa trên 6500 cdeg/s model + 150ms, từ chối T không khả thi theo giả định, clamp 1200-4200ms; đây chỉ là mô hình tĩnh chứ không chứng minh khả năng motor. Từng axis dùng lỗi góc/AS5600 timestamp speed để tính PWM từ owner/feedforward rồi giới hạn cap, +12 rise, **không boost trong 180 RAW cuối**, cho phép giảm tốc trục dẫn; không bật 2 controller đồng thời. `steer_s15_common_time_enable=0` boot và chỉ latch trong D-pad LEFT Level2 tại stroke mới sau neutral. Trường hợp còn lại giữ firmware legacy. S15 session bắt đầu khi BOTH MOVING và sensor speed hợp lệ, dùng hiện trạng góc từ AS5600 hai bên, owner SOFT goals; không tạo quyền chạy motor mới, không override owner NC/STOP. Đường phát lệnh góc bất kỳ từ động học **chưa route trực tiếp tới actuator**, pure planner đã xử lý mục tiêu ±45° độc lập — cần tích hợp owner có stopping target tương ứng trong milestone sau khi pilot an toàn.

[OBSERVATION] M8S00_S15 report enable/last/T/phase/ref/error/candidate/applied. M8S05/M8S06/M8S11 vẫn đo thời điểm kết thúc từng axis. S14.10 shadow không chạy khi S149 OFF trong stroke S15. Test trên HOST C với goals LEFT35 RIGHT30 cùng T và góc trái/phải khác dấu, stale, no motor IO, cap/180 RAW protection; GitHub CI exact SHA kiểm tra sau commit.

[STATUS] New branch firmware pilot is SOURCE-STAGED, not STM32CubeIDE Clean Build, flashed SHA or hardware PASS. No current/coast/clearance/independent supply cutoff verified. S15 **không bảo đảm** về T chung nếu motor bị giới hạn; yêu cầu đánh giá lỗi bám và arrival skew; nếu không đạt T, phải giữ lỗi và cải tiến logic sau bằng chứng. Người dùng chỉ test controller ON, KHÔNG lặp PI OFF.


### CURRENT M8-S14.10 — RIGHT velocity-based terminal taper SHADOW-ONLY (2026-10-31)

**User goal:** "hãy thực hiện" đề xuất cải thiện RIGHT 180RAW cuối theo tốc độ đo AS5600. User chỉ thử controller ON, không lặp OFF. ON 18:13: 3 full MAX→SOFT D-pad LEFT Level2+PI/S149 ON strokes LEFT~1560ms RIGHT~1801ms skew~241ms, RIGHT dẫn~44ms ở 180RAW nhưng vẫn đến SOFT sau LEFT; improvement vs S14.8 ON remains observational only. In S149 logs bins (100,180] frames83 active0, <=100 frames185 active0, final sample after stop; legacy 180RAW exclusion, owner 100RAW taper. No measured coast/current/NC clearance.

**Decision:** preserve exact tested actuator behavior. Stage `Core/Inc/steer_s1410_right_terminal_shadow.h`: pure C hypothetical velocity-adaptive RIGHT terminal taper within 180RAW, using EXISTING `steer_manual_right_axis.speed_filtered_raw_s` and independent valid timestamp, no extra I2C. Target `150+2*remaining RAW/s`; shadow extra <=20CCR and <=original envelope, no hypothesized boost within 40RAW. 50ms travel proxy NOT coast measurement. Only D-pad LEFT RMIN with S149 latch ON, right MOVING; M8S00_S1410 and four bin means (140/100/60 RAW cuts), counts/last values, no UART while energized. **No shadow candidate is applied to PWM/CCR/owner, ever.** Caps450/200, Level2=2 boot, PI/S149 boot OFF, existing 180/100RAW owner, atomic NC/STOP, arming, R1/R2/RX remain unchanged. M8-S14.10 can quantify whether an eventual active taper would be requested; cannot improve arrival without a later gate-based motor change.

**STATE:** SOURCE-STAGED until exact commit CI. Host tests verify pure helper and no actuation. No STM32CubeIDE Clean Build/Flash verified for S14.10, no current/coast/NC clearances/physical power cutoff verified; not HARDWARE PASS. No private CSV on GitHub. Future ON-only log after safe gate, review avgSpeed/avgOwner/avgShadow and actual coast/current before any active change. Documentation: `docs/M8_S14_10_RIGHT_TERMINAL_SHADOW.md`.



### M8-S14.9 telemetry fix after controller-ON trial 18:05 (2026-10-31)

[TEST ON-ONLY] 845 CSV samples, complete no missed/invalid. Five fully qualified MAX→SOFT D-pad LEFT Level2+PI+S149 ON strokes versus four prior S14.8 Level2+PI ON: LEFT mean 1541.7ms vs1562.2ms; RIGHT 1838.3ms vs1917.6ms (79.3ms faster), RIGHT-minus-LEFT skew 296.6ms vs355.4ms (58.9ms lower). RIGHT lag at 180RAW -34.4ms (leads) versus +18.4ms. Values interpolated at ~52-57ms UART cadence; uncontrolled between-session variance remains, no coast/current PASS. S149 counters enable1,last1,frames466,requests359,wins254; no NC/trip/STOP during matched strokes; R2 NC seen in separate position mode, not a S149 manual fault.

[BUG] All M8S00_S149_BIN bins zero because read-only telemetry condition previously required `left_direction<0` and `right_direction<0` simultaneously. Source computes `left_direction=-command`, `right_direction=command`; D-pad LEFT actual signs +1/-1, so impossible. Restore guard `left_direction>0 && right_direction<0`; add exact static regression to S149 verifier. The controller's PI/PD/lead calculation, duty caps, cutoff and atomic safety gates are untouched by this fix. Existing 18:05 diagnostic bin counters cannot be reconstructed from M8S00; use CSV and S149 aggregate counters. New bins require re-build/flash of fix after hardware gate.

[STATUS] Source/host CI must PASS at new SHA, CubeIDE/Flash/Level2 current/clearance/coast still unverified, no Release required. User wants only controller ON trials, no more OFF baseline.


### M8-S14.9 — hiện hành: RIGHT early terminal-lag forecast opt-in, chỉ ON (2026-10-31)

[USER] Chỉ thử bộ điều khiển ON, không bắt lặp PI OFF. Người dùng yêu cầu cập nhật firmware theo báo cáo riêng tư ON Level2 phiên 17:22. Báo cáo cho thấy RIGHT vs LEFT skew tại 180RAW **+18.4ms**, tại SOFT **+355.4ms**, phần xuất hiện trong đoạn cuối **337.1ms**, RIGHT 180→SOFT **782.8ms** và LEFT **445.7ms**. Đây là bốn lượt ON; không suy ra dòng/coast/NC clearance.

[CODE] Giữ `steer_s142_trial_level=2` boot; `steer_s147_velocity_pi_enable=0` boot và `steer_s149_early_lead_enable=0` boot. S14.9 mới latch ở lệnh D-pad LEFT→RMIN Level1/Level2 sau neutral khi PI ON, S14.5 OFF, và người vận hành bật riêng cờ S149. Dự đoán thời gian tới 180RAW từ tốc độ từng AS5600 + offset terminal 337ms (giả thuyết từ log), yêu cầu tối đa 25CCR trước 180 RAW, chọn MAX so với hybrid PI/P+D, giới hạn RIGHT 150/200 Level1/2 và +12CCR, giữ LEFT không giảm. Không đổi cutoff khi một bên <=180RAW, owner taper100RAW, SOFT/NC/STOP/atomic/arming/R1/R2/RX/wheels. M8S00_S149, M8S00_S149_BIN[0..3] snapshot read-only cho target/measured/error/owner/cap/candidates/applied theo khoảng RIGHT RAW.

[STATE] **SOURCE-STAGED** cho tới CI exact SHA; HOST/GCC/CI không phải CubeIDE Clean Build, Flash, đo current/clearance/coast, independent motor power cut, hay HARDWARE PASS. Counterfactual raw-speed replay chỉ cho thấy 84% mẫu yêu cầu boost positive, không chứng minh RIGHT thực sự nhanh hơn; không upload raw CSV/TXT. Xem `docs/M8_S14_9_TERMINAL_LEAD_ON_ONLY.md`. Không tự ép phải thử lại OFF. Cần hardware gate mới vận hành S149; nếu không đủ, tiếp tục host/source analysis.


### M8-S14.8 — cập nhật hiện hành Level2 boot và PI/P+D hybrid (2026-10-31)

[USER] Chọn `steer_s142_trial_level=2` mặc định ngay khi boot cho phiên bản mới; yêu cầu LEFT không bị làm chậm chỉ để RIGHT có skew nhỏ. **Không có phê duyệt nghiệm thu Level2 dưới tải**.

[REPO CODE] S14.8 giữ baseline LEFT405 RIGHT175, cap LEFT450 RIGHT200. PI S14.7 vẫn boot-OFF opt-in, latch ở cả Level1 hoặc Level2 chỉ D-pad LEFT→RIGHT R_MIN nếu S14.5 OFF. Trên cùng owner PWM, S14.2 P+D và S14.7 PI tính hai positive boost candidates; chọn MAX từng trục, không cộng chồng, không dùng P+D trim giảm LEFT. Còn nguyên cutoff 180 RAW, owner taper100 RAW, rise +12CCR, NC/STOP, late EXTI, manual disarmed boot. M8S00_S148 ghi nhánh candidate.

[STATUS] SOURCE-STAGED, CI cần xem theo SHA commit mới; CubeIDE Clean Build, flash chính SHA, nguồn ngắt độc lập, clearances, current/coast, A/B Level2 và GateB mặt đất **CHƯA KIỂM CHỨNG**. Không đưa log CSV riêng tư lên repo public. Tài liệu `docs/M8_S14_8_LEVEL2_PI_PD_HYBRID.md`.


### M8-S14.7 — thử nghiệm hai PI tốc độ đánh lái độc lập, mặc định OFF (2026-10-31)

[USER] Yêu cầu thực hiện bộ điều khiển PID đã từng đề xuất cho động cơ đánh lái nhằm giúp RIGHT bắt kịp. Kiểm tra source tại HEAD `bcf6c2a4e122de91ee8fdb864f5fe7cfd10cca5d` cho thấy PI `Steering_SyncApply` cũ là LEFT follower / RIGHT master và bị bypass trong S14.2 Level1; S14.2 chính dùng cross-coupled P+D, R1 dùng PWM theo lỗi vị trí/fine pulses. **Chưa có PI tốc độ độc lập cho mỗi trục trong chế độ đang chạy**.

[IMPLEMENTED SOURCE] Thêm `Core/Inc/steer_s147_velocity_pi.h` hai PI riêng + outer normalized shared progress, pilot RIGHT về R_MIN với targetL560/targetR680 permille/s, phase correction dương lên bên chậm (max120), PI boost max40CCR và rise max12. Lệnh mới được latch chỉ khi `steer_s142_trial_level=1`, `steer_s147_velocity_pi_enable=1`, D-pad LEFT, `steer_s144_rmin_catchup_enable=0`, RIGHT hướng R_MIN; boot `steer_s147_velocity_pi_enable=0`. Không ảnh hưởng D-pad RIGHT/RX/R1/R2/SELECT/Level2. Cho owner và SOFT độc lập quyền quyết định, không boost <=180 RAW một trong hai, không đổi trần375/150 hoặc taper100RAW, không bỏ stop/NC/sensor/late IRQ. Khi S14.7 ON, bỏ qua S14.2 planner chứ không cộng chồng. 2 dòng `M8S00_S147` và `M8S00_S147_PI` ghi tần suất và vòng PI; không nói chỉ số chứng minh arrival.

[TEST] Added exact production C host tests/guard `tools/test_m8_s147_velocity_pi.c`, `tools/verify_m8_s147_velocity_pi.py` and Actions job; `docs/M8_S14_7_DUAL_STEERING_VELOCITY_PI.md` hướng dẫn đầy đủ Clone/Build/Flash/Live Expressions trong cùng lượt. **Đây là firmware source mới**, phải Clean Build+Flash đúng SHA rồi mới đo. GitHub host PASS và STM32 CubeIDE/Flash/Bench A/B được ghi tách biệt và cập nhật khi có kết quả.

[OPEN] S14.5 ON 15:47 không giảm lag so OFF; D-pad LEFT 16:07 S14.5 OFF 3 stroke RIGHT đến sau 547/548/607 ms. S14.7 chưa đo hardware. R1 LEFT code8 của 16:09 được USER xác nhận do vật cản, không phải defect firmware; R1 code9 do RX non-neutral, là event riêng. Cần xác minh khả năng ngắt nguồn, khoảng hở coast và current trước khi thử hết hành trình. Không làm thay đổi verdict M8 GateA/B.


### KẾT LUẬN HIỆN HÀNH — R1 LEFT `code=8` do vật cản, KHÔNG phải lỗi firmware (USER xác nhận 2026-10-31)

- **[USER — quan sát trực tiếp]** Người vận hành xác nhận lượt R1 LEFT không về CENTER được **do cơ cấu bị vướng vật cản**, không phải lỗi hệ thống. Thông tin này giải thích sự kiện `M8_S00_20261031_160926.txt` `FAULT event=5,axis=1,code=8,tick=67140`. Đây là bằng chứng về tác nhân bên ngoài do người vận hành cung cấp, không phải kết luận được suy ra từ UART.
- **[REPO/LOG]** Mã `8` được firmware ghi nhận khi chuyển động bất thường/không tiến triển; chức năng ngắt PWM của trục lỗi được thiết kế sẵn. **Không xếp sự kiện này thành defect firmware đang mở, không khởi xướng patch anti-coast/wrong-way hay yêu cầu lặp lại lỗi này.** Ký hiệu FAULT trong log vẫn giữ nguyên như sự kiện lịch sử, không được xóa hoặc gọi nhầm là không có FAULT.
- **[CẬP NHẬT THAY THẾ NHẬN ĐỊNH CŨ]** Các đoạn phía dưới ghi "R1 LEFT code8 unresolved", "cần tìm nguyên nhân quán tính" là **trạng thái đánh giá trước khi USER xác nhận vật cản**, không còn là việc cần sửa của firmware. Không diễn giải thành "lỗi phần mềm đã được sửa" — không có thay đổi firmware.
- **[PHÂN BIỆT]** R1 `code=9` do RX rời vùng trung tính là lần hủy lệnh độc lập. RIGHT về R_MIN chậm ~0,5–0,6 giây và các điều kiện nghiệm thu dừng/coast, tải mặt đất vẫn là công việc M8 hiện hành.


### LOG M8 16:07–16:09: LEFT R1 FAULT 8 trước CSV; R1 code 9 do RX không trung tính (2026-10-31)

[USER FILES/VERIFIED ANALYSIS] `M8S11_six_motor_20261031_160757.csv` 1031x60, board ms **78246–138813**, 60.567s, metadata complete, rows1031/missed0/invalid0/sequence_missing0/tx_errors0. `M8_S00_20261031_160926.txt` có snapshot lỗi trước CSV: manual STOP event=3 tick66899, rawL1740/rawR510, CCR342/125; FAULT event=5 axis LEFT code8 tick67140, rawL1652/rawR471, R1 active, position PWM300/100, `fault_L=1,fault_R=0`. Code 8 trong `Steering_PositionAxisUpdate()` thuộc hướng sai hoặc sai số không tiến triển/stall; hai snapshot cách241ms và LEFT đã giảm88RAW dù từ 1740 về CENTER1873 cần tăng RAW. **Lỗi đã xảy ra trước lúc bật CSV khoảng11.1s**, không thể xác định riêng nhánh trigger/code8 hoặc dòng điện/driver từ CSV; giả thuyết inertia khi đổi manual→R1 hợp lý nhưng chưa chứng minh.

[CSV R1 DIFFERENT EVENT] R1 MAX→CENTER ở board ms 112083: RIGHT hoàn tất về667; LEFT đang fine ~1864–1868. Board ms114460 `rx=82` (trước đó128), rồi `rx=0`; cùng lúc `position_active` chuyển1→0. M8-S00 history run12 `stop=9`, left reason9, right reason2 phù hợp firmware hủy R1 nếu RX lệch neutral; **đây là hủy do lệnh joystick mới, không phải LEFT code8 tái diễn**. R1 MIN→CENTER lúc 133485 tiếp theo hoàn thành run18: LEFT 1758ms, RIGHT2149ms, skew391ms, reached3/faults0. CSV đầu chứa pos_state_l=4/result8 từ sự kiện FAULT trước khi capture, không phải 39 fault mới trong CSV. No raw NC/trip/stop flags trong 1031 rows.

[STEERING] Ba D-pad LEFT toàn hành trình từ L~2237–2238 / R~1007–1008 tới SOFT L1454/R216: RIGHT chậm LEFT lần lượt **547,548,607 ms**, mean567.3ms. 100RAW cuối LEFT ~305/305/244ms, RIGHT ~547/548/607ms. S14.5 OFF `enable0,biasFrames0,rightPriority0`; S142 level1 S143 enable1 grants14 fastAxis28 fallback0. RAW extrema Lmin1430/Rmin209/Rmax1019, soft 1454/216/999; NC không đồng nghĩa có khoảng hở cơ khí. Bánh mục tiêu khác0 chỉ47/1031 mẫu, không dùng phiên này để nghiệm thu lại +/-100RPM sáu motor.

[DECISION/PENDING] Không tăng PWM near SOFT, không đánh dấu R1 lỗi8 đã sửa. Tách 2 giả thuyết: (a) R1 LEFT code8 trước capture cần xác minh điều kiện động học trước khi đổi hướng; (b) RX manual thay R1 tạo code9 được quan sát, phù hợp guard có chủ ý. Trước phép thử R1 kế tiếp: đảm bảo STOP/NC/ngắt nguồn độc lập, để cơ cấu hết trôi, đưa RX về128/trung tính; bắt đầu CSV **trước** nhấn R1 để giữ những frame gây lỗi và lấy M8-S00 sau dừng. Không tự thử sát NC, giữ S14.5 OFF. Cân nhắc bổ sung gate R1 chống khởi động trên chuyển động còn trôi, **chưa sửa firmware vì cần kiểm chứng nguyên nhân**.

### Phát hiện mới sau A/B S14.5 và hướng hành động (2026-10-31)

[USER LOG — đo định lượng, không phải hardware PASS] OFF `M8S11_six_motor_20261031_153229.csv` 1.356 mẫu, ON `M8S11_six_motor_20261031_154708.csv` 1.596 mẫu; cả hai meta complete/missed0. Hai lượt D-pad LEFT toàn hành trình hoàn tất mỗi phiên: RIGHT chậm LEFT trung bình **511,5ms OFF / 546,5ms ON**; 100 RAW cuối của RIGHT **542ms OFF / 546,5ms ON**. ON M8-S00 15:48:58 có `S144 enable1,biasFrames350,rightPriority80,peakBias200`, tức thuật toán có chạy nhưng **chưa cải thiện** trên tập dữ liệu này. Cỡ mẫu nhỏ, chưa kết luận nguyên nhân toàn diện.

[SOURCE/CSV] Bộ điều khiển manual tự hạ PWM trong 100 RAW cuối và giữ 50 CCR làm sàn RIGHT: khoảng 100 CCR -> 330 RAW/s ở 100–180 RAW, 65 CCR -> ~180–190 RAW/s ở 20–40 RAW, 52–53 CCR -> ~115 RAW/s ở 0–10 RAW. Đây là vận tốc AS5600 suy từ mẫu UART, không phải đo độc lập dòng/lực. LEFT MIN có mẫu thấp tới 1429–1430 so với SOFT1454, RIGHT MIN tới211 so với SOFT216, RIGHT MAX tới1022/1023 so với SOFT999; chưa đủ điều kiện tăng PWM near SOFT.

[CURRENT ACTION] Thêm `tools/analyze_m8_right_low_pwm.py` (tiêu chuẩn thư viện Python, không chạm motor) để lặp lại đối chứng OFF/ON, nhận diện lượt D-pad LEFT đủ điều kiện/nhả sớm, ghi lag và thời gian 100 RAW cuối, bảng tốc độ AS5600 theo PWM và khoảng cách; kiểm tra `.meta.txt` để loại CSV lỗi; self-test trong GitHub Actions. Tài liệu `docs/M8_S14_6_RIGHT_MIN_LOW_PWM_CHARACTERIZATION.md` ghi công thức, số liệu, giới hạn và cổng an toàn. **Không thay đổi `Core/`, S14.5 vẫn boot-OFF**. Bài test script PASS không chứng minh có cách nào làm RIGHT nhanh hơn.

[NEXT HARDWARE GATE] Kiểm tra dừng nguồn độc lập, thực tế còn bao nhiêu khoảng hở khi PWM off ở R_MIN/R_MAX, dòng driver 50–100 CCR cùng nhiệt độ và các lượt lặp an toàn ở giá treo. Nếu không có chứng cứ đó không sửa tầng giảm tốc 100 RAW hoặc nới SOFT. Sau khi có, thiết kế profile tùy trục theo dữ liệu với thử nghiệm boot-OFF, A/B cùng điều kiện và xác minh coast; không ép tăng PWM gần NC.

### TRẠNG THÁI HIỆN HÀNH sau các lần bổ sung (cập nhật 2026-10-31)

**Bắt buộc trước khi dùng:** GitHub ref đúng nhánh → HEAD SHA → đọc source/docs tại SHA đó. **Không** dùng SHA `605b937f` trong bản chụp A, Release mới nhất hay commit SHA viết ở phụ lục để suy ra HEAD.

- **Mục tiêu vận hành [USER/LOG]:** xe sáu motor STM32F767ZI, M8 đang hoàn thiện đánh lái kết hợp chạy bánh. Level1 cùng Fast Start đã có dữ liệu trên giá treo; RIGHT về R_MIN chậm hơn LEFT khoảng 0,46–0,56 s trong các lượt log; quãng trôi gần SOFT và an toàn dưới tải chưa nghiệm thu.
- **Mã gần nhất trước thay đổi quy trình [REPO]:** branch `feature/m7-s15-ps2-controls-restoration-staged` tại `eb5f006e7112e4b85d510467292a94b13098c393` đã bổ sung **M8-S14.5 ưu tiên RIGHT về R_MIN sớm**, biến `steer_s144_rmin_catchup_enable=0` khi boot (thử nghiệm OFF). Trước đó `steer_s142_trial_level=1`, `steer_s143_fast_start_enable=1` đã được chọn mặc định trong code; START arm và NC/STOP vẫn bắt buộc. M8-S14.5 **chưa có thử A/B trên xe hoặc bằng chứng build/flash đúng SHA**; không tự biến commit mới nhất thành hardware PASS.
- **Mục tiêu đang xử lý tại commit chính sách này [USER]:** khôi phục ngữ cảnh từ GitHub không lệ thuộc Release, giữ nguyên byte đặc tả gốc 386 dòng, giữ 17 mục được cập nhật, giữ toàn bộ lịch sử, Actions source checks mỗi push, ZIP chỉ phát hành chủ động theo milestone; **không sửa firmware, PWM hoặc kết luận nghiệm thu**.
- **Đã thực hiện trong source [REPO]:** tách job `verify-handoff` (push và manual, read-only) khỏi `publish-release` (chỉ manual `publish_release=true` sau verify PASS, quyền write riêng), thêm regression kịch bản thiếu docs / direct-repo resume, hướng dẫn người tiếp quản và trạng thái hiện hành rõ hơn, ZIP đọc đúng committed tree nếu yêu cầu. **Kết quả GitHub CI của chính commit này phải kiểm tra riêng sau push**; không tự tuyên bố PASS trước khi thấy Actions.
- **Việc chưa hoàn tất:** Build/Flash STM32 cho firmware đang dùng, A/B S14.5 OFF/ON trên giá treo với dòng điện/quãng trôi/cắt nguồn, kiểm tra R1/R2/STOP/NC hồi quy và Gate B trên mặt đất có tải. Các log CSV riêng tư có thể chỉ nằm trong chat cũ, không mặc định có trong GitHub.
- **Bước đầu của hội thoại tiếp theo:** xác minh HEAD GitHub, đọc mục này + toàn bộ 17 mục, roadmap đầu/cuối, commit diff + Actions đúng SHA, xác định mốc S14.5 chưa kiểm chứng phần cứng; khi có điều kiện an toàn và build đúng SHA mới tiến hành A/B. Không yêu cầu Release vì thiếu ở HEAD, không xóa các thay đổi cục bộ Windows.
- **Tiêu chí kết thúc bước bàn giao:** xác định được SHA, source/host/target/flash/hardware từng mức, yêu cầu còn hiệu lực, lịch sử loại bỏ và đầu vào còn thiếu; không có tài nguyên bắt buộc nào được thay thế bằng giả định.

### LỊCH SỬ: định hướng nguyên gốc khi chốt A (2026-10-31), không dùng làm HEAD hiện hành

1. **Mục tiêu đã được người dùng chốt [USER]:** tăng trần PWM manual LEFT từ **300→450 CCR (+50%)**, RIGHT từ **100→200 CCR (+100%)**, giữ **đồng bộ góc và vận tốc góc**; khi lệch thì ưu tiên **tăng bên chậm trong phần dư trần**, chỉ **giảm bên nhanh nếu bên chậm đã hết headroom**. Không phải chỉ tăng PWM open-loop. Ưu tiên khả năng **vừa chạy bánh vừa đánh lái**, không làm quá chậm bằng giảm LEFT liên tục.
2. **Công việc cuối đã thực hiện [REPO]:** commit **`605b937f`** cho M8-S14.2 cross-coupled high-ceiling, sau commit `9396e416` tốc độ cao nhưng bỏ đồng bộ. Sửa mã có: P+D AS5600, trần động theo khoảng cách SOFT, PWM nền có dự trữ, thống kê `M8S00_S142_SYNC`, unit test C, verifier tĩnh, tài liệu.
3. **Đã kiểm tra [REPO]/[HOST]:** HEAD GitHub trùng SHA; bảng route mã nguồn và NC/STOP vẫn có; các header/test tải về máy thử có `git hash-object` khớp blob SHA GitHub. Biên dịch `gcc -std=c11 -Wall -Wextra -Werror` và chạy hai bài test C đều **PASS**, bài property **17.823.078 input tuples PASS**. Đây chỉ là bằng chứng của **hàm thuần tính toán**, không phải toàn bộ dự án STM32 hoặc động học thực tế.
4. **Chưa thực hiện/chưa có bằng chứng:** CubeIDE **Clean Build**, Flash đúng ELF, thử motor ở 375/150, thử 450/200, log `M8S00_S142_SYNC` từ phần cứng, A/B tốc độ/đồng bộ của firmware mới, kiểm tra hành trình dừng thực ở PWM cao, thử 6 motor cùng lúc. **M8-S14.2 CHƯA HARDWARE PASS**.
5. **Mâu thuẫn cần tránh:** lỗi FIRST-STOP RIGHT vẫn có thể được cấp điện khi CCR=0 **đã được người dùng báo là fix** trong lượt mới nhất. Không ghi nó là “chưa sửa”. **Chưa có dữ liệu về commit sửa/đo cụ thể**, và việc kiểm tra cắt nguồn độc lập khi thay mức PWM vẫn là một **điều kiện kiểm tra chung**, không phải cáo buộc lỗi ấy vẫn hiện hữu.
6. **Hành động đầu tiên của B:** lấy nhánh/commit thực tế từ GitHub và workspace Windows, kiểm tra `git status`, **không ghi đè sửa đổi cục bộ**, chạy `python tools/verify_m8_s142_speed.py` cùng verifier S14, tiến hành **CubeIDE Clean Build**. Ghi SHA+build log và chỉ sang bước nghiệm thu motor khi build pass và điều kiện phần cứng được đáp ứng.
7. **Tiêu chí xong bước đầu:** SHA trong workspace đúng, verifier thực thi có đầu ra PASS (không coi SKIP là PASS), CubeIDE Debug Clean Build không lỗi, đúng `steer_s142_trial_level=0` lúc boot và tệp ELF đã được xác định. Nếu chưa có build log thì trạng thái vẫn **đã commit/chờ cross-build**.
8. **Dữ liệu B cần:** file này trước tiên; truy cập repo qua GitHub hoặc bản clone; log CSV/TXT cũ nếu so A/B; tài liệu pin nếu kiểm tra nguồn/timer. **Không bắt người dùng gửi lại file có thể lấy bằng GitHub**.

### B update S14.3, 2026-10-31

Current goal: user requests near-immediate steering response; Level2 turning speed pleased operator. New M8-S14.3 boot-OFF neutral fast start source-staged. Next Windows sync/build/flash and Level1 flag0/1 A/B with 50ms CSV; no Level2 hardware PASS.

## 2. Bối cảnh, mục tiêu và phạm vi

### 2.1 Hệ thống và lý do

- Robot 6 motor: **4 motor truyền động bánh** (`LU`, `LD`, `RU`, `RD`) và **2 motor đánh lái** (`TURN LEFT`, `TURN RIGHT`), MCU STM32F767ZI/NUCLEO-F767ZI.
- Điều khiển PS2, các lệnh voice qua VC-02, ưu tiên STOP/PS2/Voice, encoder bánh, hai AS5600, bốn công tắc NC MIN/MAX. ST-Link Virtual COM/USART3 cho logger/diagnostic.
- Trước đây khả năng chạy bánh theo PI+FF/S-curve và logger đã được kiểm tra **trên giá treo**. Nhiệm vụ chuyển sang đánh lái, tích hợp chạy+đánh lái, sau đó mới kiểm tra dưới tải mặt đất.
- M8-S12 và S14 tập trung kiểm soát độ lệch của hai motor lái. S14.1 giảm PWM LEFT tốt hơn S14 nhưng làm chậm phản hồi; người dùng yêu cầu ưu tiên tốc độ **và** đồng bộ ở trần lớn hơn.

### 2.2 Mục tiêu cuối cùng và đầu ra

1. Điều khiển PS2 cả 6 motor hoạt động đúng hướng/đúng quyền; manual D-pad và RX đánh lái nhanh nhưng bám quãng góc hiệu chuẩn, kể cả lúc có lệnh chạy bánh LY.
2. Phát hiện/giới hạn góc bằng AS5600 và NC; STOP/disarm, timeout/stall, sai chiều, guard ngắt và kiểm tra nguyên tử trước ghi CCR không bị vô hiệu hóa để tăng tốc.
3. R1 trở về CENTER chính xác ±5 RAW và mượt; R2 chạm MIN đúng NC hai bên; các chế độ L1/L2 spin và SELECT/calibration không bị thay đổi vô ý.
4. Dữ liệu nghiệm thu: commit/build/flash, CSV đồng bộ đã bảo toàn metadata, snapshot `M8S00_*` sau DISARM, thử lặp và quan sát cơ khí/dòng điện, so sánh cùng điều kiện.
5. Gate A: bánh + đánh lái + phối hợp trên giá treo. Gate B: chạy có tải trên mặt đất **chỉ sau Gate A**. Chưa tự coi Gate A/B đã đóng.

**Đang ngoài phạm vi của sửa S14.2:** tự động xử lý SELECT, sửa PI bốn bánh đã tạm PASS, tối ưu R1 RIGHT (milestone tương lai M8-S15), tái thiết kế cơ khí, điều khiển động học phối hợp bánh/tiếp đất, bộ lập kế hoạch DRL theo bài báo rover. Những việc đó chưa tự động trở thành nhiệm vụ đang làm.

**Ràng buộc:** không được tự suy từ PWM CCR sang tốc độ góc/s; không tăng PWM/đổi soft margin vô điều kiện; không Flash hoặc cấp điện nếu chưa đáp ứng điều kiện hardware; không xóa lịch sử dữ liệu, commit/nhánh hoặc sửa cục bộ của người dùng.

## 3. Yêu cầu hiện hành và cách phối hợp với người dùng

| Loại | Nội dung | Phân loại/trạng thái |
|---|---|---|
| Chức năng | Manual LEFT/RIGHT PS2 D-pad/RX, nhanh hơn nhưng 2 motor lái bám góc nhờ phản hồi AS5600 | **[USER, đã chốt]** |
| Trần mục tiêu | LEFT tối đa 450 CCR; RIGHT tối đa 200 CCR; tăng từ 300/100 | **[USER, đã chốt]**; không chứng minh tăng tốc góc đúng 1,5×/2× |
| Thứ tự điều khiển | Bù motor **chậm** trước; khi nó hết trần mà vẫn lệch, giảm motor **nhanh** theo điều kiện | **[USER, đã chốt qua “đúng vậy, hãy thực hiện”]** |
| Thử tăng dần | Level 0 300/100, level 1 375/150, level 2 450/200; boot OFF | **[ASSISTANT thiết kế và đã code]**, không có phản đối của người dùng; chưa có PASS hardware |
| PWM nền | Level 1 330/125; level 2 405/175, dành headroom | **[ASSISTANT đã code]**, gains thử, chưa tối ưu/USER nghiệm thu |
| Cửa an toàn | NC, STOP, SOFT, encoder/AS5600 freshness, stall, hướng, ESCAPE, R1/R2, L1/L2 và motor bánh không bị thay đổi vô ý | **[Yêu cầu dự án + thiết kế đã code]**; mới kiểm tra nguồn, chưa nghiệm thu PWM cao |
| Lỗi FIRST-STOP | Người dùng nói đã fix hiện tượng RIGHT có điện khi CCR=0 | **[USER báo cáo mới nhất]**, ưu tiên hơn cảnh báo cũ; không yêu cầu họ chứng minh lại trước khi phân tích mã/log |
| R2 RIGHT lỗi 9 | Đã thấy trong một phiên, các phiên sau không báo lỗi | **[LOG]**, nguyên nhân chính xác chưa chứng minh |
| Ngôn ngữ và phong cách | Tiếng Việt, kỹ thuật chính xác, chủ động triển khai, trả lời trực tiếp, không hỏi lại điều đã rõ | **[USER, quy tắc dự án + phản hồi hội thoại]** |
| Quy trình hỏi | Làm phần đủ dữ liệu trước; chỉ hỏi một dữ kiện thiết yếu khi làm thay đổi bước kế tiếp; hỏi bằng thứ người dùng quan sát được | **[USER, quy tắc dự án]** |
| Công cụ | Dùng GitHub connector để đọc/commit cùng nhánh; Windows PowerShell COM4 để ghi log; CubeIDE Live Expressions | **[Thực tế đã dùng/đã hướng dẫn]** |
| Cách báo | Phân biệt “đã commit”, “host test PASS”, “đã flash”, “trên giá treo PASS”, “trên đất PASS”; không báo chung “xong” | **[USER yêu cầu mức bằng chứng rõ]** |

**Lỗi assistant từng bị người dùng sửa:** (a) coi việc bỏ đồng bộ để tăng tốc là hợp lý cho chế độ xe chính thức — **đã bị bác**, vì sẽ không còn căn cứ để mong hai motor bám nhau; (b) tiếp tục nhắc FIRST-STOP chưa fix dù người dùng đã xác nhận sửa; **không lặp lại**. Tránh ngôn ngữ ngụ ý “RIGHT còn chạy khi CCR=0” là lỗi đang hiện hữu. Có thể nói điều kiện đánh giá dừng ở PWM mới mà không hồi sinh cảnh báo đã được sửa.

**Việc được phép trong A:** đọc repo, phân tích tệp, chỉnh và commit mã trên nhánh đã nêu; assistant đã thực hiện và người dùng tiếp tục yêu cầu sửa. Điều này **không phải** bằng chứng người dùng cho phép thao tác phát điện/Flash trực tiếp hay thay đổi hardware. Việc tải tệp/log không cấp quyền mới cho hành động khác.

### B update S14.3, 2026-10-31

USER chose direction: keep S14.2 high-speed cross-coupled control; Level2 subjectively preferred, reduce startup lag. ASSISTANT design keeps Level2 opt-in and safe old coasting wait; FIRST-STOP remains user-confirmed FIX, R2 code9 separate.

## 4. Roadmap và milestone

> Các trạng thái dưới đây được cập nhật theo **bằng chứng mới nhất tìm được**, không bê nguyên bảng snapshot 2026-10-17 ở đầu `PROJECT_MILESTONES.md`. Cột “hạn” là **CHƯA CÓ DỮ LIỆU** nếu chưa thấy cam kết. Mã có thể có nhưng chưa hẳn đã nghiệm thu toàn milestone.

| ID | Milestone | Đầu ra/tiêu chí nghiệm thu | Trạng thái | Bằng chứng tiến độ | Việc còn lại/phụ thuộc | Hạn |
|---|---|---|---|---|---|---|
| M0 | NUCLEO-F767ZI, E5V, GPIO/timer/encoder/pin | Nền phần cứng/firmware khởi tạo | **Nền đã triển khai** | `PROJECT_MILESTONES.md`, DOCX pin | Khi sửa phần cứng cần đối chiếu bản thực tế | Chưa chốt |
| M1 | VC-02 UART voice parser | Nhận sự kiện voice | **Có mã; chưa tái nghiệm thu tích hợp** | Roadmap 2026-10-17 | Test tổng hợp khi đủ Gate A | Chưa chốt |
| M2 | Voice command builder | Movement/turn/angle/time | **Có mã; chưa nghiệm thu đánh lái thực** | Roadmap | Tích hợp voice dưới điều kiện test | Chưa chốt |
| M3 | Arbitration STOP > PS2 > Voice > Idle | Không xung đột quyền motor | **Có mã; chưa full-load PASS** | Roadmap/guards main.c | Regression khi chạy đồng thời | Chưa chốt |
| M4/M4' | Lệnh chạy bánh PS2/Voice, open-loop/ramp | Nền điều khiển bánh | **Đã triển khai** | Roadmap | Kiểm tra tích hợp nếu thay đổi | Chưa chốt |
| M6-S01 | Baseline tốc độ bánh | Lưu baseline so sánh | **Đã lưu** | `baseline/m6-s01-speed-v1` tại `8721bde3` (theo repo milestone) | Không sửa baseline nếu không có lý do | Chưa chốt |
| M6-S02 | PI+FF/S-curve cố định 1s, dấu chiều, ±40RPM | 4 bánh × 4 plateau ±0,5RPM | **PASS trên giá treo theo báo cáo** | 16/16 plateau; 8/8 plateau tiến/lùi ghi trong roadmap | Chưa PASS tải mặt đất | Chưa chốt |
| M6-S03a | RAM log qua RESET, CRC, USER 3s xóa | Xuất CSV nguyên vẹn | **PASS chức năng trên giá treo theo báo cáo** | 1.128 và 1.714 mẫu lịch sử; repo milestone | Không tự suy bền vững sau mất nguồn | Chưa chốt |
| M6-S03b | Tốc độ bánh trên mặt đất có tải | Chạy có tải có log | **Chưa làm/chưa PASS** | Repo milestone | Đợi Gate A | Chưa chốt |
| M7-S01 | Bốn NC limit GPIO/EXTI | Đọc bốn công tắc đúng | **PASS đọc bằng tay trên board [USER]** | Ngày 2026-10-17 roadmap | PASS GPIO không đủ chứng minh dừng motor | Chưa chốt |
| M7-S02 | Xác định chiều motor, low PWM, thoát limit | DIR thực và ngắt limit đúng | **Kiểm tra từng hướng/công tắc có bằng chứng; milestone tổng chưa rõ PASS** | Các bản ghi ngày 2026-10-20/21; RIGHT hai chiều được báo PASS, LEFT DIR0/1 quan sát dừng limit | Xác minh hiệu lực với cấu hình cơ khí đã lắp lại | Chưa chốt |
| M7-S03 | AS5600/encoder và six-point geometry | Mốc MIN/CENTER/MAX đáng tin, hiệu chuẩn | **Có dữ liệu mới; một phần được triển khai, cần nghiệm thu tổng** | M8S00_CAL_RUNTIME hiện tại có 6 RAW; các cờ magnet/quality không trong mọi log | Giữ riêng giá trị cũ trước tháo lắp | Chưa chốt |
| M7-S04/S04B | R1/R2 vị trí, telemetry, hai trục độc lập | Về CENTER/MIN đúng | **Đã có code và log nhiều lượt hoạt động; chưa full PASS** | R1 trong ±5 RAW theo phân tích; R2 các lượt thành công sau lỗi | Độ mượt R1 RIGHT, sự kiện lỗi R2 RIGHT 9 | Chưa chốt |
| M7-S05a/b | D-pad, RX steering, handoff/limit recovery | Điều khiển thủ công 2 bên | **Đã có code, đã có log hoạt động** | CSV các lượt manual 2026-10-30 | Tốc độ/đồng bộ/đồng thời có tải chưa PASS | Chưa chốt |
| M7-S06 | SOFT clearance và back-away | Không đụng limit khi manual | **Có mã; nghiệm thu toàn diện chưa xác nhận** | Main.c hiện giữ SOFT, ESCAPE | Đo coast ở PWM 375/150–450/200 | Chưa chốt |
| M7-S07/A–H | SELECT auto-cal + các sửa NC, noise, CENTER | Hiệu chuẩn tự động an toàn | **Đã có nhiều sửa nguồn; SELECT hiện tạm hoãn** | Các mục M7-S07/B/C/D/E/F/G/H trong roadmap | Không đưa SELECT thành blocker của S14.2 nếu user không yêu cầu | Chưa chốt |
| M7-S08 | R1 fine ±5 RAW mỗi bên | Đúng tâm, không giật | **Đạt vị trí ở các log, chưa PASS độ mượt** | M8S00 R1 nhiều lượt, RIGHT tinh có xung cuối | M8-S15 xử lý mượt RIGHT | Chưa chốt |
| M7-S09 | Kinematics D-pad/RX hai phía ngược chiều | Đúng cơ học | **Có mã và log manual** | 5 CSV thể hiện RAW hai bên đổi ngược dấu | Tích hợp tải | Chưa chốt |
| M7-S10 | SOFT LEFT + RIGHT settle/coast | Kiểm soát lệnh đảo chiều | **Có mã, chưa nghiệm thu toàn diện** | NC/settle guards `main.c` | Thử có tải và gần limit | Chưa chốt |
| M7-S11 | Incident quarantine/safety audit | Cách ly và log trạng thái an toàn | **Lịch sử đã code, không coi lỗi FIRST-STOP còn hiện hữu** | Repo ghi incident 2026-10-22/23; user sau xác nhận FIRST-STOP đã fix | Gỡ cảnh báo tài liệu cũ khi có chứng cứ fix | Chưa chốt |
| M7-S12/S13/S14 | PS2 full input, pin quarantine, thụ động log incident | Quan sát/chẩn đoán đủ | **Các giai đoạn lịch sử, thay bằng source restore sau đó** | Roadmap | Không dùng các quarantine cũ làm cấu hình hoạt động hiện tại | Chưa chốt |
| M7-S15 | Khôi phục PS2 đầy đủ trên cùng nhánh | Đầy đủ input điều khiển | **Source-restored; mức nghiệm thu toàn chức năng chưa đủ dữ liệu** | `feature/m7-s15-ps2-controls-restoration-staged`, các commit lịch sử | Regression với 6 motor/STOP/voice | Chưa chốt |
| M7-S16 | RIGHT-master PI AS5600, kiểm tra sau lắp lại | Đồng bộ tốc độ raw/s | **Code có, nhưng đã bị waypoint/S14/S14.2 thay ở nhánh manual active** | Roadmap 2026-10-24, main.c | Tránh kích hoạt đồng thời bộ PI cũ với S14.2 | Chưa chốt |
| M8-S00 | Snapshot first stop/fault + đồng thời 6 motor | Xuất an toàn và chẩn đoán | **M8-S00 xuất TXT được và log kiểm tra; đồng thời 6 motor được user báo PASS trước đó** | 4 TXT, tài liệu concurrent ghi user 2026-10-28 | Chưa có test đồng thời với bản PWM cao mới | Chưa chốt |
| M8-S11 | CSV sáu motor | Dòng 60 cột sạch, liên tục | **Đã kiểm chứng truyền file** | 5 CSV+meta, mới nhất 1.711 dòng và không mất mẫu | CSV chưa có cờ S14.2 per-row; cần M8-S00 bổ trợ | Chưa chốt |
| M8-S12 | Baseline R1 RIGHT | R1 gần tâm ổn | **Source và log baseline; chưa PASS êm hoàn toàn** | `f33ca72...` theo tóm lược; R1 RIGHT có xung tinh, độ mượt chưa đạt | S15 sau ưu tiên đồng bộ | Chưa chốt |
| M8-S13 | Thử phương án shared-goal/phase | Không làm sai lệch nặng hơn | **Thử rồi REVERT / bị thay thế** | Tóm lược: mean gap xấu hơn khoảng 12%, không giữ trên HEAD | Không tái đưa lại nếu không có bằng chứng mới | Chưa chốt |
| M8-S14 | Cross-coupled giảm PWM bên nhanh | Góc bám, bảo vệ nguyên vẹn | **Đã code + log thử; KHÔNG HARDWARE PASS** | S14 `20189e36`; CSV 20:14, M8S00_S14 frames=328 | LEFT ở 100-CCR RIGHT bão hòa và lệch tăng | Chưa chốt |
| M8-S14.1 | Tăng giảm LEFT khi RIGHT gần cap 100 | LEFT gap giảm | **Đã code + log bench thử; chưa PASS chính thức, không phải mục tiêu tối ưu hiện hành** | `ff1531fe`, M8S00_S141 extraFrames=123 và 533 | Chậm phản hồi; dùng làm đối chứng level0 | Chưa chốt |
| **M8-S14.2** | **Tăng trần có đồng bộ góc/vận tốc** | **LEFT 450 / RIGHT 200, bù bên chậm trước; vận hành an toàn** | **ĐÃ COMMIT, HOST TEST PASS, CHỜ MCU BUILD/FLASH/HARDWARE** | **HEAD `605b937f`; kiểm tra C/marker trong A** | Clean Build → level1 giá treo → level2 có điều kiện → A/B + đồng thời LY | Chưa chốt |
| M8-S15 *(dự kiến, khác M7-S15)* | Giảm giật R1 RIGHT | R1 đúng tâm và êm | **ĐỀ XUẤT / TẠM HOÃN** | Log R1 cuối skew 403ms, các phiên 459/525ms | Chỉ tập trung sau xác nhận S14.2 hoặc khi lỗi an toàn R1 buộc ưu tiên | Chưa chốt |
| Gate A | Bánh+đánh lái và phối hợp trên giá treo | PASS toàn kết hợp | **CHƯA ĐÓNG** | PASS các phần riêng nhưng chưa đủ acceptance hiện hành | S14.2, R1, STOP/NC/concurrent regression | Chưa chốt |
| Gate B/M8 tải đất | Chạy + lái mặt đất có tải | Kiểm tra tải thực | **CHƯA LÀM/CHƯA PASS** | Không có log nghiệm thu tải | Gate A | Chưa chốt |

**M7-S07 submilestones lưu riêng để không bỏ mất lịch sử:** S07 SELECT auto-cal; S07B independent NC và status retention; S07C motion result 7; S07D RIGHT wrong-way gần R_MAX; S07E chẩn đoán LEFT limit; S07F CENTER overshoot/R_MAX AS5600 spike; S07G RIGHT MAX noise-hold deadlock; S07H CENTER capture/R1 deadband. Các mục trong repo dùng nhãn khác nhau `CODED`, `SOURCE REVIEWED`, `NOT HARDWARE PASS`. Không nâng chúng thành PASS chỉ từ HEAD còn chứa code.

**M8 công việc ngoài phạm vi bảng:** SELECT được người vận hành **tạm hoãn** từ 2026-10-28; L1/L2 spin được user báo PASS trên giá treo thời điểm đó; các snapshot M8-S02/S04/S06/S11 đóng vai trò dữ liệu đo, không đủ dữ kiện để tự tạo milestone nghiệm thu độc lập cho tất cả mã.

### B update S14.3, 2026-10-31

New milestone S14.3: use prior neutral sensor stillness to shorten only safe starts, no stop/NC/SOFT override; SOURCE-STAGED, target acceptance pending.

## 5. Công việc đã thực hiện và kết quả hiện tại

### 5.1 Repository, nhánh và chuỗi commit

- Repo: <https://github.com/Spring-Mustache/control_motor>
- Nhánh: `feature/m7-s15-ps2-controls-restoration-staged` (không tự tạo workspace/worktree khác khi chỉ chỉnh tham số nhỏ).
- Các mốc trực tiếp liên quan (SHA rút gọn phải đối chiếu bằng GitHub trước khi checkout):
  - `f33ca72...`: khôi phục baseline M8-S12 theo bản tóm lược hội thoại cũ; chưa truy xuất full SHA trong hồ sơ này.
  - `20189e36b1489bbb8b330a30105b2f7c6afc5c75`: M8-S14 source-staged, mặc định OFF; sau đó thu được M8S00_S14 frames >0 trên firmware thử.
  - `ff1531fe5caf53c24ee93aaa6b6ad5d95ec0859b`: S14.1 LEFT saturation-aware; added `steer_s141_left_enable`, `M8S00_S141`.
  - `c29a8b2aced5a87c1bd490d76bdab54bef780c72`: S14.2 speed-only draft 375/150 và 450/200, **đã bị sửa/thay thế**, không dùng làm bản cuối.
  - `9396e4168006be4679989629a11d76fb0fe6caf3`: sửa ESCAPE NC của S14.2 để vẫn chặn 300/100; cũng là **bản chưa có đồng bộ**, không dùng làm bản cuối.
  - `bb2efc3276576d7c835bf5625605eda7fea2df07`: bổ sung AS5600 angle/velocity cross-coupled, dự trữ PWM; có bản vá tiếp theo.
  - **`605b937f8de5328d7b8c74c58accb9387392df90`**: hiện hành, điều chỉnh áp dụng PWM bù **liên tục theo service**, không nhấp nhô giữa các lần cập nhật AS5600; **HEAD GitHub đã đọc và xác nhận**.
- M8-S14.2 hiện hành sửa/thêm các file:
  - `Core/Src/main.c`
  - `Core/Inc/steer_s142_speed.h`
  - `Core/Inc/steer_s142_sync.h`
  - `tools/test_m8_s142_speed.c`
  - `tools/test_m8_s142_sync.c`
  - `tools/verify_m8_s142_speed.py`
  - `docs/M8_S14_2_SPEED_CEILINGS.md`
  - `docs/PROJECT_MILESTONES.md`
- File `docs/PROJECT_MILESTONES.md` còn chứa đoạn **speed-only draft** trước đoạn **cross-coupled update**; đọc đoạn sau và code HEAD làm trạng thái hiện hành. Các cảnh báo FIRST-STOP trong docs lịch sử chưa được chỉnh theo user update.

### 5.2 S12/S14/S14.1: từ log phần cứng thực tế

**Tính toán từ CSV [LOG]**: chỉ lấy các đoạn `manual_active=1`, `ccr_l>0`, `ccr_r>0`; bắt đầu lại quãng đường ở **mẫu đầu** cả hai motor cùng có CCR; đổi RAW→góc bằng hiệu chuẩn piecewise hiện hành (xem mục 14); lấy trung bình/đỉnh của `abs(travel_deg_left − travel_deg_right)`. Phép đo là **chênh quãng góc AS5600 của motor lái**, không phải tốc độ chuyển hướng xe, không biết tải thực tế. D-pad/RX đầu vào và vị trí khởi đầu khác nhau giữa phiên, nên chỉ so sánh tham khảo.

| CSV | Trạng thái có cơ sở | D-pad LEFT TB/đỉnh | D-pad RIGHT TB/đỉnh | RX LEFT TB/đỉnh | RX RIGHT TB/đỉnh |
|---|---|---|---|---|---|
| `182136` | 8 lượt, 2 mỗi hướng; **không xác nhận chính xác mode S14 của từng lượt** | lượt 1: 0,492°/0,811°; lượt 2: 1,959°/3,274° | 1,954°/2,565°; 1,135°/1,775° | 0,932°/1,404°; 1,858°/3,533° | 1,703°/2,515°; 0,595°/1,325° |
| `200251` | **S14 chưa bật** theo M8S00 `boot=0,last=0,frames=0`; dùng làm đối chứng cũ S12 | 0,644°/1,500° | 2,237°/3,130° | 0,885°/1,354° | 1,433°/2,114° |
| `201427` | **S14 chạy** theo M8S00 `boot=1,last=1,frames=328` | 1,031°/2,524° | 0,974°/2,706° | 1,418°/3,038° | 0,252°/0,916° |
| `204553` | **S14.1 chạy ít nhất vài khung** `extraFrames=123` | 0,585°/1,619° | 0,944°/2,522° | 0,779°/1,795° | 0,611°/2,058° |
| `205211` | 4 D-pad LEFT, 4 D-pad RIGHT, 3 RX LEFT, 3 RX RIGHT; **S14.1 được kích hoạt ít nhất một số lượt**, `extraFrames=533` | TB theo các lượt ≈0,812°, đỉnh toàn bộ 2,253° | ≈0,664°, 2,171° | ≈0,924°, 2,530° | ≈0,317°, 1,724° |

**Quan trọng:** `M8S00_S141,last=0` ở cuối các log S14.1 **không phủ nhận** `extraFrames>0`; lệnh manual cuối có thể là RIGHT nên S14.1 không latched cho lệnh cuối. M8-S11 CSV **không ghi cờ S14.1/S14.2 tại từng row**; không được tùy tiện gán nhãn A/B giữa các lượt trong cùng một file.

**Dữ liệu UART đã tự kiểm tra**: 
- `M8S11_six_motor_20261030_182136.csv`: **1.629×60**.
- `..._200251.csv`: **795×60**; meta `missed=0,busy=0,tx_errors=0`, rows 795.
- `..._201427.csv`: **762×60**, meta sạch.
- `..._204553.csv`: **687×60**, meta sạch.
- `..._205211.csv`: **1.711×60**, meta `missed=0,busy=0,tx_errors=0,invalid_lines=0,sequence_missing=0,complete=True,COM4,115200`. Trong các CSV này bốn `pwm_lu/ld/ru/rd` đều bằng 0, **không kiểm chứng chạy bánh đồng thời trong chính các bản ghi này**.

### 5.3 Snapshot M8-S00 và R1/R2

| TXT | S14/S14.1 | R1 và R2 | Fault |
|---|---|---|---|
| `M8_S00_20261030_200825.txt` | S14 `boot=0,last=0,frames=0` | R1 kind1 6 lượt từ boot; R2 kind2 1 lượt, `finishValid=0` | **FAULT RIGHT `event=5,axis=2,code=9` tại tick 42138, `nc=6`, `fault_R=1`** |
| `M8_S00_20261030_201535.txt` | S14 `boot=1,last=1,frames=328,trimLframes=156,trimRframes=67` | R1 5 lượt, lượt cuối `finishSkewMs=525`; R2 1 lượt `finishValid=1, faults=0`, skew 232ms | Không có fault được ghi trong snapshot này |
| `M8_S00_20261030_204728.txt` | S14 frames=372; S14.1 `enable=1,last=0,extraFrames=123` | R1 lượt cuối skew 459ms; R2 skew 247ms, success | Không có fault được ghi trong snapshot này |
| `M8_S00_20261030_205420.txt` | S14 frames=1250; S14.1 `enable=1,last=0,extraFrames=533` | R1 kind1 15 lượt, lượt cuối skew 403ms; R2 1 lượt, skew **313ms**, `faults=0` | Không có fault được ghi trong snapshot này |

`M8S00_R2ENC` trong lần đầu `valid=0`; các lần sau `valid=1`. `M8S00_TIMING` cho R1/R2 là thông tin tổng hợp/lượt cuối, không mặc định trùng mọi đoạn của CSV; counter `scope=since_boot` có thể bao gồm nhiều lệnh trước lúc bắt đầu capture.

**Sự kiện RIGHT code 9**: tại RAW LEFT 2279, RIGHT 199, `nc=6` (hai MIN đang active), CCR=0 tại ảnh lỗi. `M8S00_ESCAPE` báo RIGHT fault 1. Chưa xác định nhánh nào tạo `code=9`, có thể liên quan logic IRQ/pending/NC, nhưng **không được ghi nguyên nhân là đã xác định**. Các lần R2 sau hoàn thành bình thường; không đủ để chứng minh sự kiện không tái diễn.

**R1 độ mượt**: RIGHT thường về CENTER muộn hơn LEFT. Các log cuối lần lượt ghi skew 525, 459, 403ms. Trong S12 cấu hình RIGHT fine `fine_enter_raw=10`, PWM85, `pulse_ms=10` theo người dùng; firmware clamp hiệu dụng `enter>=15`, pulse>=12ms. M8-S12 lưu baseline **15 RAW / PWM85 / 12ms**, dung sai CENTER ±5 RAW. Xung tinh ít, nhưng vẫn hơi giật; **M8-S15 chưa thực hiện**.

### 5.4 Kiểm tra mã M8-S14.2 thật sự đã thực hiện trong lúc lập hồ sơ

- GitHub connector đọc HEAD và các file tại commit `605b937f`, xác nhận dấu hiệu `steer_s142_trial_level=0`, latch chỉ ở đầu lệnh, `Steering_S142SyncApply`, `M8S142_SyncPlan`, động trần theo `dist_l/dist_r`, `M8S00_S142_SYNC`, các kiểm tra NC atomic, escape 300/100, R1/R2 cũ.
- `/mnt/data/s142_verify/Core/Inc/steer_s142_speed.h`: `git hash-object=408cea8bb05ff58fe87e113067a0e49edc225efd`, **khớp** SHA blob GitHub file đó.
- `/mnt/data/s142_verify/Core/Inc/steer_s142_sync.h`: `4e69ad2dccb27336b8fe6aab611f431b749dfa5f`, **khớp** GitHub.
- `/mnt/data/s142_verify/tools/test_m8_s142_speed.c`: `2ba0f985e33eaabf9520874c4ddf363f9306172e`, **khớp** GitHub.
- `/mnt/data/s142_verify/tools/test_m8_s142_sync.c`: `b3a440055a88674a16e36b44b13ef47725b6fd8b`, **khớp** GitHub.
- **[HOST tự chạy lại]** với GCC Debian 14.2.0, `-std=c11 -Wall -Wextra -Werror`: `PASS S14.2 speed caps and SOFT envelope`; `PASS S14.2 cross-coupled: boost first, saturation taper, coast and limits`.
- **[HOST tự biên dịch/chạy lại]** `tools/test_s142_property.c` là bài property **được lưu cục bộ** trong thư mục kiểm tra, không xác nhận nó đã commit; sử dụng hai production header khớp SHA GitHub. Đầu ra: `PASS 17823078 input tuples for PWM caps, zero preservation, boost-first, taper-on-saturation`. Bài này **không** tương đương stress test thời gian thực/hardware closed loop.
- **[CHƯA KIỂM CHỨNG]** chạy script verifier trực tiếp trong một bản clone Windows của người dùng, full STM32 cross-compile link, driver vật lý, tiếp xúc NC ở PWM cao, đo độ trễ giữa sensor update và PWM, và tất cả kịch bản fault/recovery.

### 5.5 Những nhận định của assistant trước cần hạ mức tin

- Assistant từng báo “PASS C” và “17,8 triệu tổ hợp” trước khi lập hồ sơ. Nay đã **tự chạy lại** host helper/property ở trên nên hai nhận định đó có bằng chứng host, **không được mở rộng** thành firmware STM32 PASS.
- Assistant trước từng nói các kết quả A/B chứng minh cải thiện ~20–45%. Chỉ xác nhận **sai lệch đo trong mẫu đã ghi**, không chứng minh thay đổi do riêng controller khi đầu vào RX, vị trí bắt đầu và hành trình khác nhau.
- Assistant trước từng nói lỗi FIRST-STOP còn tồn tại. **ĐÃ BỊ USER ĐÍNH CHÍNH**, không giữ thông tin này ở trạng thái hiện hành.

### B update S14.3, 2026-10-31

B data: Level1 M8S00 sync frames230; M8S11 12:51/13:14/13:21 meta rows603/1124/1461, 0 UART errors. Combined test four wheels user reports good, steering lag ~0.5s. GCC helper neutral/stale/coast/wrap checks succeeded, target code unbuilt.

## 6. Công việc đang dang dở và điểm dừng chính xác

**Ưu tiên P0 — chốt M8-S14.2 nguồn và build.**

- Mục đích: bản high-ceiling **không open-loop**; hai cấp có phần dư để tăng motor chậm trước khi hãm motor nhanh.
- Bước đã xong: commit `605b937f`; source static checks và pure-C host tests pass; docs và field diagnostics đã cập nhật; README/tài liệu FIRST-STOP **chưa dọn cảnh báo cũ**.
- Điểm dừng: **sau commit và host test, trước CubeIDE Clean Build và flash**. Không có log nào ghi hoạt động `M8S00_S142_SYNC` trên MCU. Chưa có bằng chứng thực xe có đúng mã mới.
- File trọng tâm: `Core/Src/main.c`, `Core/Inc/steer_s142_speed.h`, `Core/Inc/steer_s142_sync.h`, `tools/verify_m8_s142_speed.py`, hai unit C, `docs/M8_S14_2_SPEED_CEILINGS.md`.
- Bước kế: xác nhận `git status` của người dùng trước fast-forward; chạy verifier; Clean Build/Flash; lập protocol chỉ level 1 giá treo trước. Nếu compile lỗi, sửa code **theo lỗi thực tế**, không đoán rồi thay phần safety.
- Vướng mắc: không có quyền truy cập GUI CubeIDE/USB/VCP của máy người dùng trong A; không có báo cáo driver/motor current/soft coast ở mức PWM cao; phiên đang có chỉ là nguồn và logs cũ.
- Tiêu chí hoàn thành P0: bản build sạch xác định SHA ELF; không có compiler warnings nguy hiểm; default OFF; bộ giám sát GPIO/NC/STOP và branch owner còn nguyên qua xem source và, khi có thể, bài thử không cấp động lực.

**Ưu tiên P1 — hardware Gate A nhỏ cho mức 1, sau đó cân nhắc mức 2.**

- Người dùng phải thao tác trên phần cứng; assistant có thể viết lệnh/script, phân tích log và điều chỉnh thuật toán theo bằng chứng.
- Mọi PWM mới phải được xem là **yêu cầu**, không bảo đảm công suất/tốc độ cơ khí. Hỗ trợ stop/cutoff phần cứng và chassis treo an toàn theo điều kiện test.
- Quan sát `M8S00_S142,last=1/2`, `M8S00_S142_SYNC,frames>0`, `boostL/R`, `trimL/R`, `satL/R`, `gap/pred`, CCR thực trong CSV; kiểm tra current, coast, CLEARANCE, timeout/stall.
- Không nâng level 2 hoặc chạy LY đồng thời chỉ từ test host hoặc một lượt ngắn không lỗi.

**Ưu tiên P2 — so A/B và quyết định controller.**

- S14.2 level 0 có thể giữ S14 hoặc S14.1 nếu bật flags tương ứng; level 1/2 là controller mới. So thời gian đi cùng khoảng góc, mean/peak gap chuẩn hoá, đổi chiều, response lúc đang chạy bánh, lặp lại với cùng RX start/raw nếu khả thi. Không coi một hiệu suất tốt trong CSV cũ là mục tiêu đạt tốc độ cao.

**Ưu tiên P3 — M8-S15 R1 RIGHT và Gate B**, theo sau P0–P2 trừ khi phát hiện lỗi an toàn cần ưu tiên.

### B update S14.3, 2026-10-31

Code and docs S14.3 staged, not proven built/flashed. Next: keep dirty CubeIDE project settings/startup/logs, sync, run verifier, Clean Build/Flash, test Level1 A/B response and coast before considering Level2.

## 7. Tệp, tài nguyên và phiên bản cần chuyển sang B

> Đường dẫn `/mnt/data` chỉ là nơi mount của A. **B không mặc nhiên có cùng đường dẫn**, kể cả nếu tên file giống nhau. Nếu B nằm cùng Project thì thử Files/Project trước; không tự tạo link sandbox dựa vào tên.

| Tài nguyên | Vai trò | Tên/đường dẫn | Phiên bản/trạng thái | B cần cung cấp lại? | Ghi chú |
|---|---|---|---|---|---|
| **Hồ sơ này** | Điểm bắt đầu | `CONTEXT_HANDOFF.md` | 1.0, 2026-10-31 | **Có, đính kèm hoặc dán trong B** | B đọc trước khi hỏi |
| Repo nguồn | Firmware chính | <https://github.com/Spring-Mustache/control_motor> | HEAD nêu dưới | **Không nếu B có GitHub connector**, khác thì clone/zip | Quyền truy cập có thể đổi |
| Nhánh hoạt động | Liên tục lịch sử | `feature/m7-s15-ps2-controls-restoration-staged` | HEAD **`605b937f8de5328d7b8c74c58accb9387392df90`** | Nêu SHA trong B | Không force/reset để “khớp” |
| Source main | Motor owner/NC/PS2/UI/diag | `Core/Src/main.c` | Trong HEAD | GitHub lấy được | Mọi thay đổi phải kiểm tra branch owner |
| Header speed | Caps/envelope/base/rise | `Core/Inc/steer_s142_speed.h` | SHA blob `408cea8...` | Có trong repo | 300/100, 375/150, 450/200 |
| Header sync | Pure cross-coupled controller | `Core/Inc/steer_s142_sync.h` | SHA blob `4e69ad2...` | Có trong repo | Predictor + boost/fallback trim |
| Verifiers | Regression source/C | `tools/verify_m8_s142_speed.py`, `tools/test_m8_s142_speed.c`, `tools/test_m8_s142_sync.c`; `tools/verify_m8_s14_sync.py` | Source-staged; host C tested | Có trong repo | Verifier có thể SKIP nếu không có cc |
| Tài liệu hiện hành | S14.2 + milestone | `docs/M8_S14_2_SPEED_CEILINGS.md`, `docs/PROJECT_MILESTONES.md` | HEAD, một số đoạn lịch sử cũ vẫn chưa chỉnh | Có trong repo | Đọc đoạn follow-up mới nhất |
| Tài liệu đối chứng | S12/S14/S14.1 | `docs/M8_S12_STEERING_BASELINE_AND_NEXT_CONTROL.md`, `docs/M8_S14_CROSS_COUPLED_MANUAL_STEER.md`, `docs/M8_S14_1_LEFT_SATURATION_TAPER.md` | Baseline/đối chứng | Có trong repo | Đừng trộn chế độ |
| Kết hợp 6 motor | Tài liệu concurrent | `docs/M8_CONCURRENT_DRIVE_STEERING_2026-10-28.md` | Test lịch sử; cảnh báo FIRST-STOP cũ | Có trong repo | User đã cập nhật fix |
| Snapshot M8-S00 | Chẩn đoán | `docs/M8_S00_FIRST_STOP_DIAGNOSTIC_2026-10-28.md` | Mô tả exporter; first-stop cảnh báo lịch sử | Có trong repo | Reset làm mất dữ liệu RAM |
| CSV+metadata (5 cặp) | So sánh góc/S14/S14.1 | `/mnt/data/M8S11_six_motor_20261030_{182136,200251,201427,204553,205211}.csv` và `.meta.txt` | 2026-10-30 | **Cần đính kèm nếu B không có Files/Project** | S14.2 high-ceiling chưa có CSV |
| TXT M8-S00 (4 file) | Chế độ/counter/R1/R2 | `/mnt/data/M8_S00_20261030_{200825,201535,204728,205420}.txt` | 2026-10-30 | Như trên | `code=9` RIGHT ở file đầu |
| DOCX pin | Mapping phần cứng | `Bao_cao_cau_hinh_chan_STM32F767ZI_xe_tang_cuop_bong.docx` | Ngày lập 24/08/2026, tài liệu v1.0 | Chỉ cần đính kèm nếu kiểm tra pin/PCB | Đã đọc văn bản/bảng; cần đối chiếu phần cứng và source mới |
| Sơ đồ/hình pin | Tham chiếu layout | `f767zi-cau hinh chan.png` | Không xác nhận revision | Cần lại khi truy vết pin thực tế | Có trong A, chưa phân tích từng net bằng hình |
| PDF ST Nucleo | Tài liệu hãng | `UM1727(1).pdf`, `UM1974(1).pdf` | Tài liệu tham chiếu, không xác nhận revision hiện đại | Không bắt buộc trong P0 | Đã nhận dạng title, số trang và trang mở đầu |
| PDF driver H-bridge | Điện/driver | `User's manual CC_SHBR12(1).pdf` (trang đầu ghi `CCS_SHB12 Smart H-bridge`) | Manual Rev1.0, 13 trang | **Có ích nếu kiểm tra dòng/STOP/driver** | Chưa phân tích toàn bộ thông số điện |
| PDF bài báo rover | Bối cảnh nghiên cứu | `aerospace-10-00219-v2.pdf` | *Deep Reinforcement Learning-Based Failure-Safe Motion Planning for a 4-Wheeled 2-Steering Lunar Rover*, 35 trang | Không cần cho M8-S14.2 | Không tự triển khai thuật toán DRL từ bài báo |
| CAD cơ khí | Geometry/kiểm tra clearance | `Mobile_robot.zip` | Chứa các `SLDASM/SLDPRT` và STL | Chỉ khi cần đánh giá cơ khí | Đã xem danh mục ZIP, chưa phân tích CAD; 38,9MB |
| Kiểm thử host cục bộ | Bằng chứng công thức | `/mnt/data/s142_verify/` | 2 header/2 test chính SHA khớp GitHub; có thêm property test | **Có ích, có thể tái chạy từ repo** | File `test_s142_property.c` ở A chưa xác nhận đã commit |
| Lệnh PowerShell | Ghi log | `tools/capture_m8_s11.ps1`, `tools/capture_m8_s00.ps1` | Trong repo | Không cần nếu clone | COM4 được log chứng minh, khác máy có thể đổi |
| Yêu cầu bàn giao | Đặc tả file này | `Văn bản đã dán (1).txt` | Văn bản do user gửi trong lượt cuối | File này đã hiện thực hóa | B không cần nếu đã có hồ sơ |

**Khi cần tái hiện phần kỹ thuật quan trọng:** ưu tiên copy `CONTEXT_HANDOFF.md`, cung cấp repo link + SHA, và chuyển **toàn bộ log M8S11/M8S00** nếu định phân tích A/B. Không tự giả định sandbox link của A dùng được trong B.

## 8. Lịch sử quyết định và các phương án đã cân nhắc

| Quyết định | Trạng thái hiện hành | Chọn/lý do | Đánh đổi | Loại/thay thế | Điều kiện mở lại | Nguồn |
|---|---|---|---|---|---|---|
| Ưu tiên nghiệm thu bánh giá treo trước tải đất | **Giữ** | 4 bánh PI+FF đạt plateau; cần đánh lái và phối hợp | Gate A còn mở | Không chạy đất trước khi đủ kiểm tra | Khi test đủ an toàn/tải | Repo roadmap 2026-10-17 |
| Đánh lái dùng 2 AS5600 và NC | **Giữ** | Góc thực/guard | Phụ thuộc hiệu chuẩn và thời gian lấy mẫu | Không dựa chỉ PWM open-loop | Nếu sensor đo lỗi thực tế | Source/log |
| M8-S13 shared goal | **Hủy/revert** | Thử trước đó làm mean gap xấu hơn khoảng 12% theo tóm lược | Ít phức tạp hơn nhưng hiệu năng xấu | Đã bỏ | Có bằng chứng mới đáng tin | Tóm lược lịch sử cũ |
| M8-S14 giảm motor dẫn trước, không boost | **Còn làm đối chứng; không phải hướng chính** | Giữ 300/100 caps cũ | Chậm/tăng lệch LEFT trong log | S14.2 ưu tiên bù follower | Khi cần đối chứng level0 | CSV/S14 docs |
| M8-S14.1 tăng trim LEFT khi RIGHT=~100 | **Còn là tùy chọn cũ** | Cải thiện LEFT so với S14 | Giảm tốc toàn xe ở lái LEFT; người dùng không muốn dùng như giải pháp cuối | S14.2 có headroom | Nếu test PWM cao không đạt an toàn | USER phản hồi 2026-10-31 |
| Tăng trần chỉ ở hai biến 300/100 | **Không đủ** | Source còn hard cap 400 và các clamp 300/100 | Thay biến đơn lẻ không hiệu lực | Thực hiện gating manual riêng | Nếu cấu trúc code thay | GitHub `main.c` |
| M8-S14.2 *speed-only* bỏ đồng bộ | **ĐÃ BỊ USER BÁC, ĐÃ THAY THẾ** | Assistant thử tách phép đo tốc độ | Không còn cơ chế bám góc | Commit `c29a8b2`/`9396e41` được follow-up sửa | Chỉ như test biệt lập có kiểm soát, không chế độ xe chính | USER hỏi “bỏ hãm thì mong hai motor bám nhau bằng gì?” |
| M8-S14.2 trần cao **và** đồng bộ | **QUYẾT ĐỊNH HIỆN HÀNH [USER]** | Boost bên chậm trong headroom, trim bên nhanh chỉ khi follower saturation | Tối ưu tốc độ vs độ chính xác; gains chưa đo | Không dùng speed-only | Nếu bench chứng minh không an toàn/không hiệu quả | USER “đúng vậy, hãy thực hiện” + HEAD `605b937f` |
| Level 1 trước Level 2 | **ASSISTANT thiết kế chờ thử** | Tăng 375/150 trước 450/200 | Thêm bước test | Không nhảy trực tiếp lên peak chỉ từ host PASS | Chỉ khi có bằng chứng hardware | Docs M8-S14.2 |
| FIRST-STOP RIGHT CCR=0 | **USER xác nhận đã fix** | Người dùng cập nhật trạng thái thực tế | Chưa rõ SHA/sơ đồ/cách thử sửa | Bỏ nhận định “lỗi còn tồn tại” | Chỉ mở lại nếu có sự cố mới/bằng chứng mâu thuẫn | Lượt người dùng mới nhất trước bàn giao |
| SELECT | **Tạm hoãn** | Không phải điều kiện kết thúc M8-S00 trong báo cáo 2026-10-28 | Chưa nghiệm thu SELECT | Không ép vào S14.2 | Khi user ưu tiên lại | Docs concurrent |
| S15 R1 RIGHT mượt | **Hướng tiếp theo, chưa chốt thực hiện ngay** | Skew R1 còn 403ms | Phụ thuộc đồng bộ cao & safety | Không sửa cùng lúc S14.2 | Sau M8-S14.2 hoặc lỗi R1 đòi ưu tiên | Hội thoại A |

### B update S14.3, 2026-10-31

Rejected blindly deleting 100/160ms +100ms quiet or increasing PWM: prior tests show coast near SOFT. Chose boot-OFF fast path requiring BOTH AS5600 neutral quiet observed previously and fresh check after command; old gate on doubt.

## 9. Vấn đề, lỗi và bài học từ những lần thử trước

### 9.1 RIGHT FIRST-STOP / CCR0 (lịch sử, **user báo đã fix**)

- **Lịch sử:** trước kia khi LY-forward traction, RIGHT có thể biểu hiện còn lực/motion dù phần mềm đọc CCR=0. Docs 2026-10-22/28 và một số câu trả lời A ghi cảnh báo đó.
- **Cập nhật gần nhất [USER]:** “bất thường RIGHT vẫn có thể được cấp điện khi CCR bằng 0 đã được fix rồi mà sao còn đề cập?”. Assistant đã rút cảnh báo và nhận lỗi về việc giữ ghi chú cũ.
- **Kết luận bàn giao:** **không coi đây là blocker còn mở**; trạng thái là **user-reported FIXED**, chi tiết commit/đo kiểm sau sửa **CHƯA CÓ DỮ LIỆU**. Khi làm test điện ở PWM cao vẫn tuân thủ SOP chuẩn, không vì vậy nói lỗi cũ còn tồn tại. Nếu sửa docs, đánh dấu rõ “historical; operator says fixed” và tham chiếu bản fix nếu tìm được, không xóa bằng chứng lịch sử.

### 9.2 RIGHT R2 code=9 (sự kiện khác)

- `M8_S00_20261030_200825.txt`: `M8S00_META,FAULT,valid=1,event=5,axis=2,code=9,tick=42138`, `rawL=2279,rawR=199,nc=6,irqPosition=6`, `M8S00_ESCAPE fault_R=1,last_code_R=9`. CSV cùng đợt ghi R2 RIGHT result=9, không phải DONE3.
- Sau đó các file 201535/204728/205420 có `M8S00_EMPTY,FAULT`, `M8S00_R2ENC valid=1`, R2 DONE cả hai; skew 232/247/313ms. **Không kết luận nguyên nhân**, không nói đã sửa dứt điểm.
- Nếu tái diễn, cần snapshot ngay sau lỗi trước reset và xem xử lý pending EXTI/NC trong `Steering_PositionUpdate`; không thử lặp nguy hiểm chỉ để lấy log.

### 9.3 S14/S14.1 không đạt tốc độ mục tiêu

- S14 cải thiện RIGHT nhưng làm mean gap LEFT tệ hơn S12 (`1.031° vs 0.644° Dpad LEFT`, `1.418° vs 0.885° RX LEFT`), trong S14 RIGHT ở CCR100 phần lớn sample LEFT.
- S14.1 tăng giảm LEFT trong lúc RIGHT near cap: có 123 rồi 533 khung điều chỉnh nhiều hơn S14 và giảm mean gap LEFT, nhưng **vẫn dựa vào giảm tốc LEFT**; user chủ động đổi mục tiêu.
- Bài học: đồng bộ không được tối ưu chỉ bằng giảm bên nhanh; khi có headroom phải tăng follower trước, và không giả định duty multiplier là speed multiplier.

### 9.4 R1 RIGHT chưa mượt

- R1 nhiều lần về CENTER đủ dung sai; các lượt cuối có skew hoàn thành hai trục khoảng 525→459→403 ms qua các phiên (không phải đường xu hướng đã kiểm soát nhân quả).
- S12 RIGHT fine default hiệu dụng 15 RAW / PWM85 / 12ms; còn xung chỉnh tinh, đôi lúc giật. Chưa thử S15. Không tự đổi logic R1 khi chỉ đang sửa manual high-speed.

### 9.5 Lỗi ghi log, tỷ lệ mẫu và thông tin thiếu

- M6-S03 từng sửa dòng metadata quá dài; CSV mới không mất row/sequence theo meta. M8-S11 period=50ms nhưng timestamp thực mỗi mẫu biến thiên; tính tốc độ dựa `ms`, không giả định chính xác 50ms.
- M8-S00 lưu snapshot/counters theo RAM, không bảo đảm sau reset/mất nguồn; xu hướng dữ liệu `since_boot` có thể qua nhiều lượt.
- CSV 60 cột không có cờ S14.1/S14.2 theo từng row. Không tự kết luận các lượt A/B trong một file nếu thay level midcapture.
- PowerShell COM4 dùng cho các file cũ; COM có thể đổi khi cắm lại thiết bị. Hai script cùng port không chạy đồng thời.

### B update S14.3, 2026-10-31

Software before-CCR wait vs motor after-CCR lag distinguished; latter may persist after fast change. R2 RIGHT code9 not conflated with user-confirmed FIRST-STOP FIX.

## 10. Rủi ro, giả định và phụ thuộc

> Nhãn **định tính** dưới đây là đánh giá mới khi bàn giao, không phải tần suất xác suất đã đo. Rủi ro phát sinh theo bối cảnh PWM cao hơn; không hàm ý lỗi FIRST-STOP cũ chưa fix.

| Rủi ro | Căn cứ | Khả năng | Ảnh hưởng | Dấu hiệu | Giảm thiểu/dự phòng | Quyết định cần có |
|---|---|---|---|---|---|---|
| Quãng trôi/overshoot tăng khi lên 375/150 hoặc 450/200 | CCR tăng; chưa có đo dòng/quán tính | **Chưa định lượng** | Cao tới cơ khí/NC | RAW tiếp tục di chuyển sau CCR0, chạm NC ngoài ý muốn | Test treo và xa SOFT, đo coast; giữ envelope về 300/100 từ 180 RAW; dừng thử nếu không đủ clearance | Có đủ clearance đo được để lên level2? |
| Tăng PWM không tăng tốc như giả định | Đặc tính driver/motor/tải không tuyến tính | Chưa định lượng | Trung bình/cao (mục tiêu không đạt) | cdeg/s không tăng, current tăng, nóng | So cdeg/s vs CCR/current, giảm cấp hoặc chỉnh base | Có cần tune PWM nền? |
| Dao động PWM/vận tốc do loop samples không đồng thời | AS5600 hai kênh riêng, P+D gains chưa tune | Chưa định lượng | Trung bình/cao | PWM dao động/giật, gap sign đổi liên tục | Phân tích log CCR và timestamps, cập nhật/giảm gains theo bằng chứng, không dùng stale | Cần điều chỉnh bộ lọc/slew không? |
| Bù motor chậm không đủ khi ở trần | LEFT/RIGHT khác cơ khí và cap | Có căn cứ bão hòa cũ | Trung bình | `satLf/satRf` tăng, gap vẫn lớn | Chỉ trim leader sau saturation; không nâng trần vượt mức user đã chốt | Chấp nhận gap/tốc độ nào? |
| R2 RIGHT lỗi NC code 9 tái diễn | Có 1 fault ghi nhận, 3 báo cáo sau không fault | **Chưa xác định** | Cao | `M8S00_META,FAULT code=9,axis=2` | Giữ NC/IRQ, capture fault trước reset, không thử R2 quá nhiều chỉ để tạo lỗi | Phân tích khi có dấu hiệu mới |
| Trạng thái FIRST-STOP trong docs lỗi thời | User báo fix nhưng tài liệu cũ giữ cảnh báo | Chắc có mâu thuẫn tài liệu | Trung bình do làm sai quyết định | B tự coi lỗi “unresolved” | Cập nhật docs là lịch sử đã fix theo user, tìm commit nếu cần | Không hỏi user lặp lại |
| Chưa biết firmware chạy trên MCU là SHA nào | Không có ELF/build/flash log mới | Chắc còn thiếu | Cao cho kết luận S14.2 | M8S00 không có `S142_SYNC` | Ghi SHA, build, flash; kiểm tra diagnostics | Ai thực hiện CubeIDE/flash |
| 6-motor concurrent tại mức high PWM | Các CSV gần đây bốn bánh PWM đều 0; có báo cáo PASS 6 motor ở bản cũ | Chưa định lượng | Cao (động học/tải/điện) | stall, mất nguồn, nhiễu AS5600, kéo cơ khí | Steering-alone trước; combo LY nhỏ treo sau khi pass | Khi nào thử phối hợp |
| Git push/merge ghi đè local changes | Windows workspace có thể có `.cproject`, `.settings`/local changes | Chưa biết hiện trạng | Cao (mất công việc) | `git status --short` không sạch, `merge --ff-only` fail | Không `reset --hard`, force, clean; đối chiếu từng file | Giữ/thỏa thuận nội dung local |
| Timer/UI assumption | Timer ARR=999 tại HEAD và DOCX, nhưng không đo xung thực | Có dữ kiện code; vật lý chưa kiểm chứng | Trung bình | Duty đo khác 45%/20% | Oscilloscope/driver telemetry nếu cần | Có đủ nguồn đo không |

**Giả định chưa xác nhận:** người dùng đã flash S14.1 khi cung cấp CSV 20:45/20:52 (M8S00 field chứng minh tính năng S14.1 có chạy); nhưng **không chứng minh đã flash S14.2 mới**. Không có dữ kiện đáp ứng thời gian đồng thời LY và lái với cap cao. Không có ngưỡng sai lệch góc/tốc độ thời gian được **user chốt** để PASS; đề xuất tiêu chí khi thiết kế test nhưng không đặt thay user.

**Phụ thuộc:** máy Windows có STM32CubeIDE, ST-Link/COM, branch GitHub, phương tiện dừng/cấp nguồn phù hợp, giá treo an toàn, AS5600 và NC đấu đúng, log phải xuất khi an toàn. Trong B có thể không còn quyền GitHub connector hoặc file attachments của A; nên bắt đầu bằng xác minh sẵn có.

### B update S14.3, 2026-10-31

Safety: no fast start while moving/coasting, NC active, disarmed, sensors stale, input not neutral, escape/R1/R2/Select. Physical independent cutoff still needed; no automatic Level2 selection.

## 11. Kế hoạch tiếp tục có thể thực hiện

### 11.1 Kế hoạch ưu tiên

| Ưu tiên | Hành động cụ thể | Mục đích | Đầu vào | Đầu ra | Tiêu chí hoàn thành | Phụ thuộc |
|---|---|---|---|---|---|---|
| **P0 — đầu tiên trong B** | Đọc hồ sơ, xác minh GitHub HEAD SHA và `git status --short` của workspace; bảo toàn local changes | Không lệch phiên bản hoặc ghi đè | Repo/branch/HEAD | Checklist nguồn đồng nhất | SHA đúng, không làm mất local changes | Quyền repo / máy người dùng |
| **P0** | Chạy `python tools/verify_m8_s142_speed.py`, `python tools/verify_m8_s14_sync.py`, GCC host test (nếu cần), Clean Build Debug | Compile firmware thực và giữ route guards | Commit hiện hành, CubeIDE | Build log + ELF/firmware được xác định | Không lỗi; SKIP không coi PASS | P0 nguồn |
| **P0** | Kiểm tra runtime default OFF và nguồn thông tin phiên bản flashed; chưa cấp nguồn motor nếu chưa phù hợp an toàn | Không nhầm code chạy | Live Expressions, flash log | Bằng chứng phần mềm nạp đúng | `steer_s142_trial_level=0` sau reset và build đúng SHA | Build/flash |
| **P1** | Rà soát NC/STOP/per-axis SOFT, phản ứng cắt PWM và cơ cấu dừng/nguồn trên giá treo với phương thức đã được kiểm tra | Nghiệm thu bảo vệ trước cap mới | Hardware, protocol an toàn | Phiếu test bảo vệ | Hệ thống STOP/NC hoạt động đúng, không overrun | Người dùng/hardware |
| **P1** | Thử **level1=375/150** ở đoạn cách xa SOFT, xung ngắn, đo CCR/AS5600/tốc độ góc/coast/current, file CSV+TXT | Xác định giới hạn thật | MCU nạp, M8S11/M8S00 | Dataset level1 sạch, metrics | Bù/trim hoạt động; SOFT/STOP/NC không sai; đủ clearance | P0, hardware gate |
| **P1 có điều kiện** | Nếu level1 ổn định và clearance đủ, thử **level2=450/200** theo cùng quy trình | Xác định ưu thế tốc độ trần mục tiêu | Level1 đạt; hardware | Dataset level2 | Tốc độ cải thiện, gap và dừng trong phạm vi nghiệm thu được chốt | P1 level1 |
| **P2** | So A/B 0/1/2, ít nhất 5 lượt mỗi lệnh D-pad LEFT/RIGHT RX LEFT/RIGHT, điều kiện gần giống nhau | Quyết định gain, trần, headroom | CSV riêng từng mode + M8S00 | Báo cáo mean/peak cdeg gap, cdeg/s, travel time, saturation, safety | Có căn cứ định lượng và repeatability, không khẳng định nhân quả khi unmatched | P1 logs |
| **P2 có điều kiện** | Tối ưu P+D/filter/rise/trim trong giới hạn hoặc rollback cấp nếu không đạt | Cải thiện điều khiển | Phân tích logs | Commit hiệu chỉnh + test | Không tăng lỗi/giật, đạt mục tiêu tốc độ/gap | P2 số liệu |
| **P2/P3** | Thử LY nhỏ + steering trên giá treo, R1/R2/L1/L2/STOP regression | Gate A tích hợp | Steering-alone PASS, giá treo | M8S00 concurrent/CSV chứng minh | Kết hợp an toàn, không mâu thuẫn owner | P1/P2 |
| **P3** | M8-S15 R1 RIGHT giảm giật, sau đó Gate B mặt đất có tải | Hoàn thành nhiệm vụ còn lại | Gate A và R1 log | Nghiệm thu riêng | R1 đúng CENTER và mượt trên xe; ground-load an toàn | Gate A |

**Tình trạng chấp thuận:** user **đã chấp thuận** hướng S14.2 “trần cao + đồng bộ boost-first” và đã yêu cầu assistant triển khai. **Lộ trình stepwise level1 trước level2 và việc thử trên giá treo là biện pháp thử nghiệm do assistant đề xuất**, chưa phải kết quả phần cứng PASS; không tự gán đó là các bài thử đã diễn ra.

### 11.2 Lệnh Windows/PowerShell chuẩn

```powershell
cd "D:\STM32 task\control_motor"
git status --short
git fetch origin
git merge --ff-only origin/feature/m7-s15-ps2-controls-restoration-staged

python tools/verify_m8_s142_speed.py
python tools/verify_m8_s14_sync.py
```

**Lưu ý:** nếu `git status` báo sửa cục bộ/`merge --ff-only` fail, **dừng và bảo toàn local diff**. Không `git reset --hard`, `git clean`, `git push --force`, hoặc tự merge xung đột bằng cách ghi đè dữ liệu chưa được đối chiếu.

**Ghi M8-S11 CSV (lệnh đã dùng cho các phiên trước):**

```powershell
cd "D:\STM32 task\control_motor"
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s11.ps1 -Port COM4 -TimeoutSeconds 240 -OutputDirectory .\logs
```

Sau khi PowerShell mở COM và khi mọi motor dừng, dùng Live Expressions thiết lập (khi cần): `m8_s11_stream_period_ms=50`, `m8_s11_stream_enable=1`; làm bài thử theo điều kiện an toàn, sau khi dừng đặt `m8_s11_stream_enable=0`, đợi `#M8S11_END`. Bắt đầu capture **trước** bài thử. Mỗi chế độ tạo một CSV riêng. Không chạy song song M8-S00 và M8-S11 trên cùng COM.

**Sau DISARM, ghi M8-S00:**

```powershell
cd "D:\STM32 task\control_motor"
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s00.ps1 -Port COM4 -TimeoutSeconds 180 -MaxCaptureLines 128 -OutputDirectory .\logs
```

Đợi toàn bộ motor dừng và PS2 neutral, **DISARM bằng START**, nhấn–nhả USER **ngắn dưới 3 giây**, đợi `#M8S00_END` và kiểm tra có đủ dòng mới `M8S00_S142`/`M8S00_S142_SYNC`. Không reset trước khi xuất. `COM4` dựa trên log cũ; xác minh nếu cổng đã đổi.

### B update S14.3, 2026-10-31

Windows: git status, git fetch, merge --ff-only on same branch, python tools/verify_m8_s143_faststart.py (no compiler = SKIP), CubeIDE Clean Build/flash; Level1 A/B five+ trials every direction and measure request->CCR/AS5600, current/coast; Level2 only after physical gate.

## 12. Câu hỏi còn mở và thông tin cần bổ sung

| Câu hỏi/dữ liệu thiếu | Vì sao cần | Nếu thiếu | Có thể tiếp tục gì? | Nguồn | Ưu tiên |
|---|---|---|---|---|---|
| Workspace Windows đã đồng bộ SHA `605b937f` chưa? có local diff không? | Tránh ghi đè/flash sai | Không xác định bản build | Đọc GitHub/source, host tests | `git status`, `git rev-parse HEAD` | **Ngay khi làm việc với máy** |
| Clean Build CubeIDE/ELF đã thành công cho SHA đó chưa? | Không thể claim firmware ready | Chưa thể test có căn cứ | Static test, xem source | Log build/ELF/user | **P0** |
| Driver/cơ khí đủ clearance và dừng ở level1/2 không? | Cap PWM cao hơn | Không thể quyết định level2 hoặc LY concurrent | Thiết kế test không cấp nguồn/host | Quan sát thực tế, current, coast | **P1 trước powered trial** |
| Giới hạn motor điện/dòng và nguồn phù hợp PWM cao? | CCR cao có thể tăng tải | Không chứng minh driver/motor phù hợp | Tham chiếu `CCS_SHB12` manual và thông số thật | Datasheet/đo thực | P1 |
| Kết quả M8S00_S142_SYNC trên MCU? | Chứng minh coordinator chạy thực, duty boost/taper | Không nghiệm thu S14.2 | Build, lập protocol | TXT mới | P1 |
| Thời gian/góc/độ lệch nào được user coi là PASS? | Chọn chuẩn nghiệm thu cuối | Có thể phân tích tương đối nhưng chưa chốt PASS | Đề xuất tiêu chí từ baseline sau test | USER khi đến ngưỡng quyết định | P2 |
| FIRST-STOP fix nằm ở commit/sơ đồ nào? | Dọn docs, truy vết khi cần | Không ảnh hưởng việc đã nhận user nói fix; mất traceability | Tiếp tục S14.2 trong phạm vi an toàn | Git history/user khi thật sự cần | Trung bình, **không hỏi ngay** |
| Nguyên nhân RIGHT R2 `code=9`? | Chống tái diễn | Không thể tuyên bố fix chắc chắn | Giữ guards và test normal | M8-S00 fault mới/source audit | Chỉ ưu tiên cao nếu tái diễn |
| Có tải khi vừa LY vừa lái trong mode S14.2 ra sao? | Mục tiêu thực tế | Không thể gọi Gate A/B PASS | Steering-only bench | Log concurrent tương lai | P2 |
| B có quyền GitHub/Files giống A? | Truy cập nguồn | Phải cung cấp lại file liên quan | Đọc hồ sơ này | B tự kiểm tra connector/Project | Khi mở B |
| Có deadline/chi phí chốt cho M8-S14.2/M8-S15 không? | Lập kế hoạch thời gian | Không được bịa thời hạn | Làm kỹ thuật theo ưu tiên | User nếu phát sinh ràng buộc | Thấp |

**Đã biết, không hỏi lại:** repo/nhánh/HEAD cuối, mục tiêu boost-first, trần 450/200, FIRST-STOP user đã báo fix, COM4 trong các log cũ, tên file CSV/TXT, các giá trị hiệu chuẩn hiện hành, kỹ thuật R1 ±5 RAW.

### B update S14.3, 2026-10-31

Missing: new STM32 build log/flashed ELF, actual current and measured coast at Level2, new A/B latency CSV, scope signal-to-power evidence. Not required to implement guarded source patch.

## 13. Dòng thời gian và diễn biến quan trọng

| Thời điểm | Diễn biến | Mức chứng cứ |
|---|---|---|
| 2026-08-24 | DOCX v1.0 cấu hình pin STM32F767ZI được lập | [DOCX metadata trong file] |
| 2026-10-17 | Chốt Gate A giá treo trước Gate B mặt đất; M6-S02/S03a báo PASS giá treo, M7-S01 bốn NC báo PASS GPIO | [REPO/USER qua milestone] |
| 2026-10-20–21 | Kiểm tra động cơ lái/DIR/limit; thu bốn mốc RAW ban đầu, RIGHT hai chiều báo PASS; user cung cấp center cũ 1857/3398 | [REPO ghi log ảnh USER, ảnh gốc không còn trong hồ sơ này] |
| 2026-10-22–23 | M7-S04…S14, SELECT, manual, incident/quarantine, chỉ thị safety, LY-forward RIGHT anomaly | [REPO roadmap, lịch sử] |
| 2026-10-23–24 | M7-S15 phục hồi PS2 trên cùng nhánh; M7-S16 RIGHT-master PI | [REPO] |
| 2026-10-26 | Lắp lại cơ cấu, user cung cấp CENTER **1873/662**; các mốc cũ không dùng lẫn | [REPO] |
| 2026-10-28 | M8-S00 exporter/6-motor concurrent; operator báo R2 MIN, L1/L2 spin, D-pad với 6 motor PASS trong phạm vi test lúc đó; SELECT hoãn; R1 còn giật; sync chưa đạt | [REPO lưu user report] |
| 2026-10-30 | M8-S12 tinh RIGHT R1 mặc định 15 RAW/PWM85/12ms; M8-S14 cross-coupled manual giảm leader, không boost | [REPO] |
| 2026-10-30, file 20:02/20:08 | CSV 795 dòng và M8S00 `S14 frames=0`, RIGHT R2 code9 xuất hiện | [LOG] |
| 2026-10-30, file 20:14/20:15 | S14 chạy `frames=328`, lệch LEFT lớn hơn S12; R2 lần sau DONE | [LOG] |
| 2026-10-30, trước file 20:45 | M8-S14.1 commit `ff1531fe`; tăng giảm LEFT khi RIGHT saturating | [REPO] |
| 2026-10-30, file 20:45/20:47 | S14.1 `extraFrames=123`, LEFT gap giảm so S14 một lượt | [LOG] |
| 2026-10-30, file 20:52/20:54 | CSV 1.711 dòng, S14.1 `extraFrames=533`; 4+4+3+3 lượt manual, R1 skew 403ms, R2 skew313ms DONE | [LOG] |
| 2026-10-31, sau đó | USER nói cần tăng trần vì hãm LEFT làm vừa chạy vừa lái chậm: LEFT 450, RIGHT 200 | [USER quyết định] |
| Sau yêu cầu | Assistant commit S14.2 speed-only `c29a8b2` rồi fix NC ESCAPE `9396e416`; **chưa phải giải pháp cuối** | [REPO] |
| Ngay sau đó | USER phản đối việc bỏ đồng bộ; yêu cầu tốc độ cao **kèm** bám góc. Assistant đồng ý và được yêu cầu thực hiện | [USER] |
| 2026-10-31 | Commit `bb2efc32`, rồi `605b937f` bổ sung/điều chỉnh cross-coupled boost-first/taper-on-saturation | [REPO] |
| Trong phiên lập hồ sơ | Đã xác minh GitHub HEAD `605b937f`, biên dịch và chạy lại host C tests + 17.823.078 property tuples, không có MCU build/flash | [REPO/HOST] |
| Tin nhắn ngay trước yêu cầu bàn giao | USER khẳng định FIRST-STOP RIGHT CCR0 **đã fix**, yêu cầu assistant đừng cứ nhắc là còn lỗi; assistant rút lời | [USER, ưu tiên mới nhất] |
| Hiện tại | USER yêu cầu xuất một hồ sơ bàn giao đầy đủ cho B | [USER] |

Các mốc phút của file (`yyyyMMdd_HHmmss`) là **tên file capture**, không nhất thiết là thời điểm bắt đầu/sự kiện bên trong MCU; thời gian trong CSV là `ms` theo board và không cộng/đối chiếu tuyệt đối giữa các lần khởi động.

### B update S14.3, 2026-10-31

13:21 2026-10-31 combined M8S11 1461 rows, user sees wheel response good but steering 0.5s lag. ~13:37 user requests fix and reports Level2 speed satisfactory. New S14.3 source follows.

## 14. Phụ lục nội dung cần giữ chính xác

### 14.1 Bảng pin quan trọng (nguồn: DOCX cấu hình 24/08/2026; cần đối chiếu nếu schematic/source đổi)

| Phần | PWM/ngoại vi | DIR/tín hiệu | Ghi chú |
|---|---|---|---|
| TURN LEFT | `PB15`, `TIM12_CH2` | `PD14` | Timer PSC=5, ARR=999 |
| TURN RIGHT | `PF9`, `TIM14_CH1` | `PF10` | **Không phải PD15**, timer PSC=5 ARR=999 |
| LEFT UP | PE5/TIM9_CH1 | PE3 | Bánh |
| LEFT DOWN | PF6/TIM10_CH1 | PF13 | Bánh |
| RIGHT UP | PF7/TIM11_CH1 | PF12 | Bánh |
| RIGHT DOWN | PE6/TIM9_CH2 | PE4 | Bánh |
| AS5600 LEFT | `I2C2 PB10/PB11` | AS5600 absolute RAW | Hai module hai bus vì địa chỉ cố định |
| AS5600 RIGHT | `I2C1 PB8/PB9` | AS5600 absolute RAW | Đọc góc cơ cấu đã calibrate |
| LEFT MIN/MAX | `PD10` / `PD11` | EXTI10/11; NC→GPIO, COM→GND | Pull-up; HIGH khi tác động/đứt NC |
| RIGHT MIN/MAX | `PF8` / `PF5` | EXTI8/5; NC→GPIO, COM→GND | Pull-up |
| PS2 | `PA4` CS, PA5 SCK, PA6 MISO, `PD7` MOSI | SPI1 | PS2 mode valid 115 trong logs |
| UART log | USART3 / ST-Link VCP | 115200, 8N1 | COM4 ở các log cũ |

**PWM duty nếu ARR=999:** LEFT 450 CCR ≈45%; RIGHT 200 CCR≈20%; mức cũ 300≈30% và 100≈10%. Đây là **duty PWM timer**, không phải công suất/dòng/tốc độ motor. DOCX và HEAD `main.c` đều có TIM12/TIM14 `Period=999`; pin/timer nên xác nhận theo board thực nếu đổi hardware.

### 14.2 Hiệu chuẩn góc đang dùng trong log 2026-10-30

Từ `M8S00_CAL_RUNTIME` trong các TXT mới: 

| Mốc | LEFT RAW | RIGHT RAW |
|---|---:|---:|
| MIN | **2297** | **166** |
| CENTER | **1873** | **662** |
| MAX | **1374** | **1029** |
| R2 tiếp xúc MIN thực trong log | LEFT thường 2268–2269 | RIGHT 199 |

**Các giá trị cũ trước tháo/lắp** `L_CENTER=1857,R_CENTER=3398`, `L_MIN=2314,L_MAX=1429,R_MIN=2929,R_MAX=3694` là **lịch sử**, không phải hiệu chuẩn đang chạy trong log 2026-10-30. Ngoài ra các giá trị `M8S00_CAL_RUNTIME` là runtime từ bản firmware đã flash khi ghi log; không tự suy firmware mới `605b937f` đang nạp trên MCU.

**Hàm quy đổi RAW→góc** dùng trong phân tích CSV:

```text
LEFT (cdeg chuyển sang độ):
  L_RAW >=1873: L_deg = -(L_RAW-1873)*45/(2297-1873)
  L_RAW < 1873: L_deg = +(1873-L_RAW)*45/(1873-1374)
RIGHT:
  R_RAW <=662:  R_deg = -(662-R_RAW)*45/(662-166)
  R_RAW > 662:  R_deg = +(R_RAW-662)*45/(1029-662)

Hướng LEFT:  travelL= +(Ldeg-Ldeg_start);
             travelR= -(Rdeg-Rdeg_start).
Hướng RIGHT: travelL= -(Ldeg-Ldeg_start);
             travelR= +(Rdeg-Rdeg_start).
Gap mỗi mẫu = abs(travelL-travelR) (độ),
start là mẫu ĐẦU cả ccr_l,ccr_r >0 trong cùng một lượt.
```

**Không nhầm** `M8S00_TRACK meanGap_permille` là cùng phép đo độ nêu ở bảng; một thứ là normalized progress per-mille của firmware, một thứ là gap calibrated travelled degrees từ CSV.

### 14.3 Các cấu hình/final formula S14.2 (nguồn: HEAD `605b937f`)

**Mode latch:** `volatile uint8_t steer_s142_trial_level = 0U` tại boot; khi nhận manual D-pad/RX **lệnh mới sau neutral**: `steer_s142_latched = Steering_S142Level(steer_s142_trial_level)`; chỉ 0,1,2 hợp lệ. Khi `steer_s142_latched>0`, S14/S14.1 không sở hữu cùng phiên manual, không dùng PI cũ để re-clamp LEFT 300.

| `steer_s142_trial_level` | Trần LEFT/RIGHT (`M8S142_Cap`) | Nền D-pad LEFT/RIGHT (`M8S142_Base`) | Ghi chú |
|---|---|---|---|
| `0` | 300 / 100 CCR | 300 / 100 | Chế độ cũ, có thể dùng S14/S14.1 theo flags riêng |
| `1` | **375 / 150 CCR** | **330 / 125 CCR** | Cấp thử đầu tiên |
| `2` | **450 / 200 CCR** | **405 / 175 CCR** | Đúng đỉnh người dùng yêu cầu |

- RX chế độ thử: điều chế tuyến tính từ các PWM minimum cũ (LEFT 175 / RIGHT 50 theo default nguồn) tới `M8S142_Base`, dùng `magnitude` 0..117. Giá trị đầu vào PS2 có thể không chạy tới full magnitude trong mỗi lượt.
- **SOFT cap envelope:** nếu khoảng cách mỗi motor tới **SOFT của chính nó** `remaining<=180 RAW`, cap quay về 300/100; nếu `>=320 RAW` thì bằng trần level; ở giữa nội suy tuyến tính `old_cap + (high-old_cap)*(remaining-180)/140`. Trong 100 RAW cuối, taper cũ vẫn áp dụng.
- **Slew tăng:** `M8S142_Rise(target, previous, old_cap)` không cho tăng trên `max(previous,old_cap)+12` CCR **mỗi lượt xử lý**; giảm cấp không bị trì hoãn. Đây **không phải** giới hạn dốc PWM theo giây đã được đo.
- **P+D dự đoán:** `prediction_cdeg = travelL_cdeg - travelR_cdeg + (speedL_cdeg_s-speedR_cdeg_s)*120/1000`, clamp ±9000 cdeg.
- **Deadband:** |prediction| ≤40 cdeg (0,40°) thì không boost/trim.
- **Lagger boost:** khi LEFT dẫn trước, RIGHT nhận `need = min(75, (prediction-40)*30/100) CCR` (đổi ngược bên khi RIGHT dẫn). Boost thực tế `min(need, cap_lagger-duty_lagger)`.
- **Leader trim có điều kiện:** chỉ khi lagger bị bão hòa `need>room`, |prediction|>150 cdeg (1,50°) **và** lagger chưa bắt kịp đủ nhanh theo điều kiện tốc độ; `trim = min(48, (abs(prediction)-150)*12/100)` CCR, không hạ duty bên dẫn dưới floor của owner.
- **Ràng buộc đầu ra:** không cấp PWM cho axis đã có 0, không đảo chiều, không vượt per-axis cap động, không chạy đồng bộ khi một bên ≤180 RAW tới SOFT hoặc AS5600 stale/invalid; giữ NC/EXTI/STOP, trạng thái settling/escape, R1/R2.
- **Điều kiện test C:** boost follower đúng dấu, trim chỉ khi saturation, không vượt PWM cap, bảo toàn zero, vùng gần SOFT không tăng chỉnh, cả hai bên đều có logic; đã chạy host helper tests. Chưa chạy MCU physical loop.

**Các trường M8-S00 mới:**

```text
M8S00_S142,level=<0|1|2>,last=<0|1|2>,higherL=<count>,
 higherR=<count>,soft=<count>,peakL=<CCR>,peakR=<CCR>
M8S00_S142_SYNC,frames=<count>,skip=<count>,
 boostLf=<count>,boostRf=<count>,trimLf=<count>,trimRf=<count>,
 satLf=<count>,satRf=<count>,gap=<cdeg>,pred=<cdeg>,
 boostL=<CCR>,boostR=<CCR>,trimL=<CCR>,trimR=<CCR>
```

**Lưu ý:** các chuỗi trên là **định dạng diễn giải**; giá trị trong `<>` là placeholder, không phải log có sẵn. Chưa có file M8S00 thật chứa dòng `S142`/`S142_SYNC`. Các counter thống kê là yêu cầu PWM/bộ điều khiển, **không chứng minh tốc độ cơ khí hoặc motor current**. Chế độ cũ có thêm:

```text
M8S00_S14,boot=...,last=...,frames=...,trimLframes=...,trimRframes=...,skipped=...,
 gap_cdeg=...,predict_cdeg=...,spdL_cdeg_s=...,spdR_cdeg_s=...,trimL=...,trimR=...
M8S00_S141,enable=...,last=...,rightNearCap=...,
 extraFrames=...,baseTrimL=...,planTrimL=...
```

### 14.4 Ngữ nghĩa điều khiển PS2 và an toàn

- D-pad/RX direct manual: hướng LEFT/RIGHT theo biến command, đảo chiều khi trung tính/nhận lệnh mới; không tự đảo đột ngột giữa lúc đang chạy.
- `R1`: về CENTER ±5 RAW; `R2`: tới công tắc MIN vật lý; **không tự điều chỉnh R1/R2 bằng tuner S14.2 manual**.
- `L1/L2`: spin tại chỗ, quyền riêng; `SELECT`: calibration (hiện tạm hoãn); `LY/Up/Down`: chạy bánh; một số chế độ steering được phép cùng traction theo bản concurrent source, nhưng S14.2 high-ceiling **chưa thử kết hợp**.
- STOP và NC được xét trước actuator CCR; `Steering_ManualAxisUpdate` xử lý từng trục; cuối pass còn `__disable_irq` + `Steering_TestRawLimitMask()` + pending IRQ + ghi CCR có guard. Giữ nguồn điện/cơ khí theo protocol test phù hợp.
- `M8-S00` là snapshot post-DISARM để tránh UART blocking lúc motor chạy; nhấn USER ngắn dưới 3s và không reset.

### 14.5 Kết quả host test lặp lại (không thay cho CubeIDE)

```text
$ gcc -std=c11 -Wall -Wextra -Werror -I Core/Inc tools/test_m8_s142_speed.c -o test_speed
$ ./test_speed
PASS S14.2 speed caps and SOFT envelope

$ gcc -std=c11 -Wall -Wextra -Werror -I Core/Inc tools/test_m8_s142_sync.c -o test_sync
$ ./test_sync
PASS S14.2 cross-coupled: boost first, saturation taper, coast and limits

$ gcc -std=c11 -O2 -Wall -Wextra -Werror -I Core/Inc tools/test_s142_property.c -o test_property
$ ./test_property
PASS 17823078 input tuples for PWM caps, zero preservation, boost-first, taper-on-saturation
```

`tools/test_s142_property.c` là tệp ở **thư mục verify cục bộ của A**, không được xác nhận có trong repo; không hướng dẫn B gọi nó từ root repo nếu chưa có file. Hai unit chính có trong repo tại commit mới nhất.

### 14.6 Checklist nghiệm thu mẫu (đề xuất để thực thi, **chưa được user chốt làm tiêu chí cuối**)

- [ ] SHA repo/ELF/flash bản mới xác định, Clean Build PASS.
- [ ] Sau reset, `steer_s142_trial_level=0` và chưa tự cấp motor.
- [ ] Từng NC, STOP/disarm, PS2 loss/owner guard, chiều motor và phản hồi AS5600 đúng trong test an toàn phù hợp; không bỏ qua khi tăng PWM.
- [ ] Level 1: D-pad LEFT/RIGHT, RX LEFT/RIGHT ở vị trí cách SOFT phù hợp; không stall/overshoot nguy hiểm, khoảng trôi đủ clearance.
- [ ] `M8S00_S142,last=1` và `M8S00_S142_SYNC,frames>0`, các boost/trim có dấu đúng và không vượt cap.
- [ ] Level 2: chỉ thử khi level 1 được chấp nhận về dừng/clearance/current; đo angular cdeg/s và travel time, không chỉ CCR.
- [ ] A/B lặp đủ, mean/peak travel gap và tốc độ không hồi quy đáng kể; số liệu có mẫu so sánh cùng start/magnitude RX.
- [ ] Thử LY cùng lái trên giá treo sau khi steering-alone đạt; R1/R2 và L1/L2/STOP không bị regress.
- [ ] Gate A đã đạt mới xét mặt đất có tải; không gộp hai gate.

### B update S14.3, 2026-10-31

S14.3 parameters: quiet 100ms, >=3 distinct valid samples, cumulative <=2 RAW, <=1 RAW per step, age65ms, gap80ms; next command fresh >=10ms sample to enter MOVING with first cycle PWM0; retains 850ms fail-closed old settle.

## 15. Chỉ dẫn để hội thoại B tiếp quản

**Gửi trực tiếp cho trợ lý B:**

> Đây là hồ sơ từ hội thoại A. Đọc **mục 1 và 6 trước**, sau đó dùng mục 7 để kiểm tra tài nguyên, mục 14 để giữ thông số và công thức chính xác. Mục tiêu hoạt động là **M8-S14.2 tăng trần đánh lái 450/200 nhưng vẫn đồng bộ angle/velocity**, không phải speed-only. HEAD GitHub đã kiểm tra `605b937f8de5328d7b8c74c58accb9387392df90`, code và helper C đã commit/host-tested; **chưa có CubeIDE Clean Build/Flash hoặc log hardware của bản mới**. Hành động đầu tiên là xác minh HEAD/working tree và build, không tự kết luận firmware đã nạp. Giữ nguyên NC/STOP/SOFT/R1/R2 và dữ liệu người dùng. Người dùng **đã báo FIX lỗi FIRST-STOP RIGHT CCR0**; đừng nhắc nó là lỗi hiện hữu. RIGHT R2 `code=9` là sự kiện riêng chưa có root cause. Chỉ hỏi dữ kiện thiếu khi thật sự làm thay đổi bước tiếp theo; làm phần đã đủ nguồn trước. Không tự sửa những subsystem đã tạm đạt và không gọi bản source-staged là hardware PASS. Khi nhận log/đề bài mới, cập nhật trạng thái theo bằng chứng, không chỉ đọc câu assistant cũ.

**Cách phối hợp:** dùng tiếng Việt; thẳng và chính xác; không ép người dùng điền checklist dài trước khi được hỗ trợ; nếu cần log thì đưa **một lệnh chạy được và rõ thời điểm thao tác**. Không hỏi lại mục tiêu 450/200, thứ tự boost-first, hoặc FIRST-STOP đã fix. Sau mỗi commit báo link/SHA và trạng thái kiểm chứng thực. Không tự phát động công việc chưa được đề nghị ngoài chuỗi S14.2, trừ khi phát hiện vấn đề an toàn cụ thể cần bảo vệ.

### B update S14.3, 2026-10-31

Next assistant must inspect new HEAD/Release; require evidence clean build and Flash; collect M8S00_S143 plus CSV, diagnose actual CCR and AS5600 lag. Do not ask repeated COM4 or D:\STM32 task\control_motor unless changed.

## 16. Kiểm tra chất lượng và khoảng trống bàn giao

### 16.1 Tự rà soát tính nhất quán

| Câu kiểm tra | Kết quả |
|---|---|
| Trạng thái milestone S14.2 có khớp HEAD? | **Có**: `605b937f` source-staged + host test; chưa MCU/hardware PASS |
| Đã phân biệt speed-only draft và bản mới? | **Có**: speed-only `c29a8b2`/`9396e41` bị thay, bản hiện hành boost-first |
| FIRST-STOP có còn bị ghi “chưa fix” như lỗi hiện tại? | **Không**: user báo fixed; các docs cũ bị đánh dấu lịch sử |
| Đã giữ R2 RIGHT code9 riêng? | **Có**, root cause chưa xác minh |
| Đã có bước đầu và tiêu chí hoàn thành cụ thể? | **Có**: SHA/workspace, verifier, CubeIDE Clean Build, sau đó hardware gates |
| Đã chỉ tệp cần mang sang B? | **Có**, bảng mục 7; không giả định A sandbox tự chuyển |
| Có nhầm nguồn host test với STM32 build? | **Không** |
| Có nhầm mốc hiệu chuẩn cũ với runtime mới? | **Không** |
| Có tự thêm deadline hoặc phần trăm tiến độ? | **Không** |

### 16.2 Khoảng trống còn lại và ảnh hưởng

1. **Lịch sử hội thoại cũ bị rút gọn:** không kiểm tra được toàn văn một số lượt M0–M8, ảnh Live Expressions ban đầu và nội dung trao đổi cụ thể của M8-S13. Phần đó chỉ được tổng hợp từ saved summary và repo milestone, không tái tạo chi tiết thiếu.
2. **Chưa có CubeIDE build log/flash artifact SHA của `605b937f`:** không thể xác nhận firmware chạy trên board là code mới; không tự đưa test level1/2 vào mục “đã thử”.
3. **Chưa có CSV/M8S00 sau sửa S14.2:** không có kết quả thực về speed-up, đồng bộ, dòng tải, quãng trôi, NC/STOP tại PWM mới.
4. **FIRST-STOP fix:** chỉ có thông báo rõ ràng của user, không có commit/dữ liệu phần cứng của bản sửa trong lượt bàn giao. Trạng thái giao tiếp vẫn là **user-confirmed fixed**, không mở lại lỗi trừ khi có bằng chứng mới; docs cần chỉnh để tránh hiểu sai.
5. **R2 RIGHT `code=9`:** biết sự kiện và vài lượt sau thành công nhưng không xác nhận root cause.
6. **Tài liệu nền PDF/CAD:** mới đối chiếu tiêu đề, metadata, trang mở đầu và danh mục, chưa kiểm toán nguyên lý driver/cơ khí đầy đủ; không thể xác nhận công suất hay khả năng chịu tải bằng chúng.
7. **Bản ghi milestone hiện tại:** có một số dòng lịch sử và cảnh báo cũ; nếu cần phát hành báo cáo chuẩn cuối, phải đối chiếu/tách “history” khỏi “current”.
8. **Tiêu chí nghiệm thu tốc độ/độ lệch chính thức chưa do user định lượng:** có thể phân tích tương đối, nhưng không tự áp chuẩn số độ hoặc phần trăm chưa được chốt.
9. **Không biết B có cùng công cụ/Project attachments:** cần chuyển file này và kiểm tra quyền GitHub/Files; chỉ gửi lại log/phụ lục khi B thực sự không lấy được nguồn.

### 16.3 Không áp dụng và trạng thái tài liệu

- Không có cam kết tiến độ theo giờ/ngày; **không áp dụng** ETA/hạn chót.
- Không có PR cần review/merge được xác nhận; **không tự nói có PR**. GitHub branch HEAD đã cập nhật trực tiếp ở A.
- Không có hành động điều khiển phần cứng/Flash do assistant từ xa; **không tự tuyên bố assistant đã vận hành robot**.
- Chưa đưa nội dung các PDF/CAD vào báo cáo điều khiển vì chưa đủ cơ sở đo điện/cơ khí; đây là giới hạn được công khai, không phải kết luận tài liệu vô dụng.

**Trạng thái hồ sơ:** **HOÀN CHỈNH TRONG PHẠM VI NGUỒN HIỆN CÓ**; các phần chưa thể kiểm chứng đã được đánh dấu, nhất là firmware STM32/hardware S14.2, provenance FIRST-STOP fix, lịch sử cũ rút gọn và ngưỡng PASS định lượng.

### B S14.3 completeness check

17 section headings retained. New source is staged only, user logs private, no MCU build or hardware PASS. Prior A snapshot retained, not silently rewritten.

### B CI regression fix (2026-10-31)

First S14.3 push `81800747` failed context-release due to verifier regex `.*` with DOTALL incorrectly consuming subsequent sections, reporting all 17 nonempty sections empty. A new source/documentation commit corrects heading matching to `[^\\n]`-style exclusion of line feeds (`[^\\n]*`) and preserves all original 0–16 content. This is CI-only, not new firmware revision or MCU/hardware PASS.

### B — cập nhật 2026-10-31: sửa false-negative verifier S14.3

Người vận hành chạy `python tools/verify_m8_s143_faststart.py` trên Windows với phiên bản `4c461753`, bảy kiểm tra khác PASS, duy nhất `legacy fallback` FAIL. Đã so `Core/Src/main.c`: macro RIGHT min=160U, LEFT min=100U, quiet=100U, timeout=850U và nhánh `fast_now || (settle_ms >= min_ms && quiet_ms >= 100)` còn nguyên. Nguyên nhân là verifier tìm chuỗi đúng 3 khoảng trắng trong khi source dùng 2. Sửa verifier kiểm tra macro bằng regex linh hoạt khoảng trắng và kiểm tra các dấu hiệu logic fallback; **không đổi firmware**. Chưa có bằng chứng Clean Build, Flash hay kiểm thử fast start trên MCU của bản mới. Hành động tiếp theo: đồng bộ commit, chạy verifier rồi Clean Build/Flash và ghi CSV A/B mức 1.

### Hội thoại B 2026-10-31 — sửa S14.3 neutral polling mismatch

[USER IMAGE] `steer_s143_fast_start_enable=1`, `steer_s142_trial_level=2`, `steer_s142_last=2`, `steer_s143_fast_grants=0`, `steer_s143_fast_starts=0`, `steer_s143_fallback_starts=41`. Không có lượt Fast Start được cấp phép; không được coi delay đã cải thiện. `debug_as5600_*_status=39` biểu diễn MD hợp lệ nếu I2C comm_ok, không tự chứng minh lỗi nam châm.

[REPO] `STEER_POS_IDLE_POLL_MS=50`, hàm IdleTelemetry chỉ đọc 1 cảm biến mỗi lượt rồi luân phiên -> thời gian cập nhật mỗi sensor >=100ms. Cấu hình S14.3 cũ `MAX_GAP=80`, `MAX_AGE=65` khiến observer xóa lịch sử yên tĩnh khi bắt gặp mẫu mới bình thường. Đây là lỗi software đã tìm được.

[CODE-STAGED] Sửa `Core/Inc/steer_s143_faststart.h` MAX_AGE=140, MAX_GAP=180; chuyển observer sau IdleTelemetry (không thêm I2C). Unit test thêm mô phỏng 50ms interleaved, verifier kiểm tra thời gian và thứ tự. Giữ hoàn toàn old settle fallback và các bảo vệ độc lập, Level2 không auto-boot. Không có CubeIDE Clean Build/Flash/motor-current/coast proof cho bản sửa.

[NEXT] Sau GitHub sync/verify và Clean Build/Flash: thử lại trên giá treo với Level1, Fast OFF/ON, đo `grants`, `fast_starts`, `fallback_starts`, M8S11 50ms và M8S00; >=5 lần/chiều, đo trôi gần SOFT và STOP/cutoff. Chỉ cân nhắc Level2 dưới tải khi có chứng cứ dừng an toàn.

### Hội thoại B 2026-10-31 — lỗi thứ hai S14.3: dấu thời gian trước I2C

[USER] Ảnh Live Expressions vẫn `fast_grants=0`, `fast_starts=0`, lần này `fallback=3`. M8_S00_20261031_142133.txt có `enable=1`, `grants=0`, `fastAxis=0`, `fallback=10`, `quietFrames=4877`, cho thấy observer neutral tích lũy thành công nhưng khởi động nhanh vẫn không được cấp.

[SOURCE] `Steering_ManualUpdate` ghi `uint32_t now=HAL_GetTick()` trước đọc hai AS5600 blocking, trong khi `Steering_PositionReadOne()` ghi `steer_pos_*_last_sample_ms=HAL_GetTick()` mới; `M8S143_Ready` tính `uint32_t(now-sample_ms)` và bị wrap khi now cũ hơn sample. Tương tự khi `Steering_ManualAxisUpdate` tính tuổi mẫu sau một đọc I2C mới, làm fast_starts luôn dễ fail. Đây là lỗi firmware đã xác định từ source, nhưng log trước không chứa mask từ chối từng lượt nên không thể khẳng định mọi lượt đều chỉ do nguyên nhân này.

[PATCH STAGED] Dùng HAL tick sau I2C tại cả grant và axis fast_now, thêm read-only `steer_s143_last_gate_missing_mask` và `M8S00_S143_GATE` để biết chính xác điểm fail tiếp theo. Mask bits 2 LEFT state,3 RIGHT state,4 raw NC,5 active NC,6 LEFT observer,7 RIGHT observer. Giữ boot-OFF, mức Level2 là opt-in, NC/STOP/soft, drift guard, old settling fallback, 850ms fail closed. Host regression kiểm tra pre-I2C timestamp inversion. Chưa chứng minh target Build/Flash/actual latency.

[NEXT] Sau cập nhật SHA mới Windows, chạy verifier và CubeIDE Clean Build/Flash; treo xe, ở Level1, Fast=1, neutral >=300ms, vài xung ngắn; nếu `grants` vẫn 0, gửi missing mask + S00. Không thử Level2 dưới đất trước khi STOP/coast/current đạt.

### B 2026-10-31: False-negative NC/soft verifier and CI guard

User ran python tools/verify_m8_s143_faststart.py from commit 7cae7b13 and got FAIL: NC/soft checks retained. Source inspection shows S14.3 changed grant check to if (Steering_TestRawLimitMask() != 0U), recording missing bit 0x10, plus if (debug_limit_active_mask != 0U), bit 0x20. The old verifier expected an equality-to-zero expression which was removed during diagnostic refactor. SOFT protection still calls Steering_ManualAxisDone(left, STEER_MANUAL_RESULT_SOFT) when at_soft is nonzero. This failure is a verifier false negative, not evidence of missing firmware safety gates.

Changes in this follow-up: update only tools/verify_m8_s143_faststart.py, plus GitHub Actions workflow to execute the verifier including native GCC C helper tests before releasing a new context handoff; preserve all motor source and PWM constants unchanged. Update both 17-section handoff and milestone log in same commit. GitHub Actions success means source checks and GCC unit tests passed, NOT CubeIDE Build, Flash or physical acceptance.

Next Windows step: git fetch origin; merge --ff-only branch retaining dirty IDE files; verify new HEAD; rerun Python verifier; CubeIDE Clean Build/Flash. First physical test remains Level1 on suspended stand, no ground/Level2 until current, coast and independent STOP validation. Monitor fast_grants, fast_starts and steer_s143_last_gate_missing_mask from fresh firmware, without guessing safety from counter values.

### B 2026-10-31 — M8-S14.4 defaults and RIGHT soft creep

[USER] After Level1 Fast Start M8S11 14:57:37 and S00 14:59:03, requests successful settings be persisted ON; reports RIGHT slows markedly as it reaches R_MIN while LEFT reaches earlier. Logs evidence: 950 valid CSV rows/zero lost UART; S143 enable1 grants11 fastAxis22 fallback0 quietFrames3657; Level1 375/150 caps selected. This is functional evidence, not proven physical cutoff or Level2 under-load acceptance.

[DECISION AND PATCH] Boot defaults only: `steer_s142_trial_level=1` and `steer_s143_fast_start_enable=1`; wheel_pid_enable already1 and UART stream period already50, stream enable stays0. Manual steering enable, PS2 armed and safety motion permit remain0 at powerup; START+neutral required before actuation. Level2 remains deliberate user selection; do not assume subjective Level2 speed means safety PASS. Update both S14.2/S14.3 verifiers so they don't falsely require old OFF defaults; run in GitHub CI and maintain context release.

[CAUSE FROM CSV AND CODE] D-pad toward R_MIN, RIGHT PWM decreases from ~100 at 180 RAW remaining to ~90/67/58 near 80/40/17 RAW. Code S14.2 high cap fades fully back to original <=100 at <=180 RAW, manual final approach additionally tapers toward RIGHT min50 within100 RAW. LEFT at CCR0 while RIGHT still moves at 50–60. Right raw min211 vs SOFT216 and left min1432 vs SOFT1454 observed; actual coast/current/physical clearance not measured. This commit DOES NOT raise PWM, shift soft threshold, remove taper or alter rate loop. Separate engineer/bench gate needed before faster final approach.

[NEXT] Synchronize Windows while protecting dirty project settings, run S14.2+S14.3 verifiers, CubeIDE Clean Build+Flash exact HEAD. Suspended bench with independent motor supply cutoff and checked NC/STOP: inspect boot defaults, confirm disarmed until START, short pulse & stop/coast checks Level1, no automatic UART. R_MIN tuning only after evidence of physical clearance. Level2 boot or ground test not authorized by current data.

### B update 2026-10-31 — M8-S14.5 RIGHT early catch-up

[USER] Asked to improve RIGHT's ability to catch up at R_MIN after new M8S11 15:11 (1722 rows, 0 missing/tx errors) and M8-S00 15:14. Existing Level1 Fast Start functional; S143 grants24 fastAxis46 fallback0, S142 last1. LEFT often reaches SOFT ~0.5s before RIGHT. CSV RIGHT PWM ~125 CCR across much early travel and ~50–100 in last 100 RAW. M8-S14.2 equal instantaneous progress does not anticipate RIGHT's deliberately slower last 100 RAW. Observed coast over SOFT (L_MIN to~1430 vs1454; R_MIN~209 vs216; R_MAX~1020 vs999). Do not turn off terminal taper or move SOFT without physical clearance/current evidence.

[DESIGN/CODE staged] New Level1 D-pad LEFT/R_MIN *opt-in* `steer_s144_rmin_catchup_enable=0` boot. Helper adds 0–200 calibrated cdeg of desired early RIGHT lead to existing S14.2 P+D predictor only if RIGHT not >250 cdeg ahead and BOTH remaining >320 RAW; fades to zero 380→320 RAW. Original NC/EXTI/STOP, per-axis SOFT, 12CCR rise, Level1 caps375/150, R1/R2, RX, D-pad RIGHT and wheel PID unchanged. No effect on verified boot defaults S142 Level1/S143 FastStart ON, motor starts disarmed, UART stream boot OFF. Source tests pure GCC, CI and docs added.

[NEXT] Preserve dirty Windows project; sync SHA, verify S142 and S143, CubeIDE Clean Build+Flash exact commit, bench only with independently verified motor isolation and mechanical clearance. Compare baseline S144=0 and trial=1 in matching LEFT toward R_MIN strokes (>=5 each) using CSV and M8S00_S144; monitor arrival lag and coast. No default ON of untested S144 or ground Level2 trial. If bias/priority counters rise but arrival still lags, examine mechanics and motor driver rather than increasing near SOFT blindly.

### B update — quy trình GitHub không phụ thuộc Release (2026-10-31)

[USER] Yêu cầu xem xét lợi ích của Release-per-push so với mô hình 3 lớp, ưu tiên: bảo toàn ngữ cảnh chính xác > hội thoại mới tự tiếp quản > tin cậy > bớt lặp. Yêu cầu kiểm tra 9 file cụ thể tại HEAD, không chỉnh đặc tả gốc 386 dòng hoặc kết luận hardware, không xóa release/tag cũ, không sửa firmware, mô phỏng push code/docs và tiếp quản khi không có Release. Không yêu cầu loại bỏ hoàn toàn ZIP/Release.

[REPO BASE] HEAD được xác minh trước thay đổi tại `eb5f006e7112e4b85d510467292a94b13098c393` trên nhánh `feature/m7-s15-ps2-controls-restoration-staged`. Workflow cũ có một job `publish-handoff`, quyền write, mỗi push tạo ZIP/RELEASE_STATUS + prerelease. Verifier đã bảo vệ SHA256 nguyên văn 386 dòng và 17 section nhưng `workflow_dispatch` không có previous SHA nên bỏ qua update gate, checker của chính nó không được coi là code update. `build_context_release.py` đọc working tree, dễ lẫn file chưa commit trong gói manual. Prompt/policy/README còn ưu tiên release làm nguồn đọc và handoff đầu còn SHA `605b937f`, roadmap bảng 2026-10-17 dù có phụ lục mới. Các nguồn này đã đọc trực tiếp tại SHA nêu trên, không dựa vào mã nhớ.

[DECISION/IMPLEMENTATION] Giữ nguyên đặc tả gốc 386 dòng và SHA256 `13d8976ec0d74057af94bf84c8445bddbb48715b308964e22eb6684711f708a4`. Tách Actions verify độc lập trên mỗi push và manual, Release job được cấp write chỉ khi `workflow_dispatch` `publish_release=true` sau verify PASS. Verifier bắt code cả tool và workflow thay đổi phải có handoff + roadmap cùng range, manual check parent. Test synthetic commit chỉ-code FAIL, code+docs PASS, gốc thay byte FAIL, resume đọc Git tree không có Release PASS; kiểm chứng CI thật sau push. ZIP vẫn tạo bằng `build_context_release.py` khi cần milestone, nhưng đọc từng blob từ đúng SHA (`git show`) không phụ thuộc working copy dirty. Các context Release/tag lịch sử nguyên vẹn, không tự tạo nữa; README, prompt và policy chỉ cần nguồn repository.

[GIẢI QUYẾT MÂU THUẪN] `CONTEXT_HANDOFF.md` vừa là ảnh chụp A và 17 mục sống: dữ kiện lịch sử không bị viết đè; mục 1 và phần bổ sung được duy trì theo quyết định mới, người tiếp quản kiểm tra SHA thực từ GitHub. Roadmap cũng giữ bảng gốc ngày 17-10 nhưng có hướng dẫn trạng thái mới ở trên. Trạng thái lỗi FIRST-STOP [USER đã fix] giữ nguyên, S14.5 source-stage và user hardware A/B chưa PASS. Không biến CI/Python thành chứng minh CubeIDE/Flash/xe.

[TRẠNG THÁI] Tool/policy/docs-only thay đổi, không có `Core/`, `Drivers/`, `.ioc`, `.ld` hay giới hạn motor thay đổi. Thất bại của release thủ công do quyền không ảnh hưởng CI verify. CI mới phải được xem ở SHA commit sau push; mô phỏng chat thật ở một phiên ChatGPT độc lập không thể chứng minh bằng test Python nên chỉ xác nhận thao tác đọc theo SHA ở repo và hướng dẫn cho phiên mới.

[NEXT] Sau push xem Actions đúng SHA PASS và xác nhận không có release/tag `context-<SHA>`; truy xuất trực tiếp README/HANDOFF/roadmap/policy/commit-diff cùng SHA. Nếu cần ZIP, chọn workflow Run workflow và `publish_release=true` trên nhánh đã xác minh; chỉ khi verify PASS mới publish. Sau đó quay lại thử nghiệm firmware M8-S14.5 (A/B suspended bench, kiểm soát STOP/NC/current/coast) khi có bằng chứng an toàn cơ khí.

### B update — M8-S14.6 đo đặc tính RIGHT ở PWM thấp (2026-10-31)

[REQUEST] Người dùng yêu cầu thực hiện bước tiếp theo sau S14.5 ON không giúp RIGHT bắt kịp. Đã sử dụng log riêng tư OFF 15:32 và ON 15:47 từ chat, không công khai CSV/TXT thô. Kết quả đo như phần mục 1 và `docs/M8_S14_6_RIGHT_MIN_LOW_PWM_CHARACTERIZATION.md`.

[WHY NO POWER CHANGE] Cả hai log hiện hữu cho biết tốc độ AS5600 thực ở 50–100 CCR; riêng 0–10 RAW còn ~115 RAW/s. S14.5 hiệu lực phần mềm nhưng arrival gap không cải thiện. Tự tăng gần SOFT khi chưa đo current/physical stop và NC clearance có nguy cơ va chạm. Code firmware nguồn ở commit trước vẫn giữ Level1/FastStart boot-ON, S14.5 opt-in boot-OFF, SOFT/NC/STOP nguyên vẹn. Đây là quyết định thiết kế có giới hạn dữ liệu, không phải từ bỏ mục tiêu tăng tốc.

[IMPLEMENTED] Thêm script `tools/analyze_m8_right_low_pwm.py` đọc 60-col CSV và meta COM4, thống kê mỗi D-pad LEFT khởi phát từ MAX: số lượt hoàn tất, lag L/R và 100 RAW cuối, báo chưa hoàn tất riêng, zone PWM/RAW/s, mức vượt SOFT quan sát; tùy chọn JSON. GitHub Actions tự kiểm tra synthetic test. Đổi ở `tools/`, `.github/workflows/`, `docs/`, `CONTEXT_HANDOFF.md`, không có firmware hoặc tệp an toàn motor. Script test không thay CubeIDE/Flash/hardware PASS.

[OPERATION] Dùng `python tools/analyze_m8_right_low_pwm.py --off .\\logs\\M8S11_six_motor_20261031_153229.csv --on .\\logs\\M8S11_six_motor_20261031_154708.csv`; meta được đọc tự động nếu có. GitHub Actions PASS cần kiểm tra thực tế sau push. Chờ dữ liệu cắt nguồn/clearance/current rồi mới cho phép profile hãm thay thế và nghiệm thu; nếu chưa có, chỉ phân tích với log đã thu.


### B update — R1 LEFT code8 thực sự trong M8-S00 16:09, trước CSV 16:07 (2026-10-31)

[USER LOG] 1031 mẫu/60.567s CSV complete, M8-S00 FAULT LEFT code8 tick67140 trước sample CSV đầu tick78246. Manual STOP tick66899 tại L1740/R510 (CCR342/125); snapshot R1 FAULT L1652/R471 sau241ms dù LEFT cần tăng RAW về CENTER1873. Source code8 gồm wrong-direction/stall/abs-error progression (không phân biệt được nhánh từ snapshot). R1 chuyển khi còn coast là giả thuyết cần bảo đảm không tái diễn, không phải kết luận đã xác nhận.

[OTHER R1] R1 từ MAX tại112083 bị kết thúc result9 sau RX từ128→82→0 lúc114460; M8S00 history run12 left reason9/right reason2 xác nhận hủy theo guard input không trung tính, không phải lỗi code8. R1 từ MIN sau đó run18 PASS LEFT1758ms RIGHT2149ms skew391ms. CSV đầu pos result LEFT8 là trạng thái còn lưu của fault xảy ra trước capture.

[MANUAL] 3 D-pad LEFT toàn hành trình full range RIGHT lag547/548/607ms (mean567.3ms), S144 OFF, S143 grants14/fastAxis28/fallback0. 1031 CSV row no NC/STOP/trip, RAW Lmin1430 Rmin209 Rmax1019 vượt SOFT vẫn đòi clearance. Wheels chỉ47/1031 row có target khác0: không đủ nghiệm thu tốc độ cao.

[STATUS] Chỉ ghi lại phân tích trong docs/roadmap/handoff; không thay firmware/PWM/safety, không có biến mới, không cần CubeIDE/Flash. R1 LEFT code8 là unresolved. Lần kiểm tra tiếp theo nếu có điều kiện an toàn: ghi CSV trước R1, RX=128, không chồng lệnh manual và chờ thực tế hết trôi. Điều tra gate anti-coast trước khi đề xuất firmware behavior; không suy diễn sự cố đã được fix.

### B clarification — xác nhận vật cản, sửa phân loại R1 LEFT `code=8` (2026-10-31)

[USER] Người vận hành nói rõ: "bị lỗi không về được do bị dính vật cản là nha, cái này không tính là lỗi của hệ thống". Áp dụng cho sự kiện LEFT R1 `code=8` trong M8-S00 16:09 đã thảo luận ngay trước đó. Xếp là **cản trở cơ khí ngoài ý muốn**, không phải lỗi thuật toán/firmware; không cần dựng patch R1 chống trôi hay điều tra nguyên nhân chuyển hướng của sự kiện đã được giải thích. Mã FAULT=8 là dữ liệu chẩn đoán thật, không được xóa; việc firmware phát hiện sai lệch và ngắt PWM là đường phản ứng bảo vệ theo source. Mức test chức năng toàn bộ R1 vẫn dựa trên những lượt kiểm chứng độc lập, không được tự tuyên bố HARDWARE PASS chỉ vì sự kiện này đã có nguyên nhân.

[CORRECTION TO HISTORY] Những bản ghi phía trên gọi code8 unresolved/inertia suspect phản ánh thời điểm chưa biết có vật cản; **đã được thông tin USER mới thay thế về nguyên nhân sự kiện**. Không biến hiện tượng vật cản thành "đã sửa lỗi firmware". R1 code9 do RX nonneutral là sự kiện riêng; RIGHT R_MIN ~0.5–0.6s lag, quãng vượt SOFT, test loaded Gate B vẫn chờ. Không thay `Core/`, NC/STOP, PWM, limits hoặc boot flags; cập nhật chỉ docs, không cần Build/Flash hoặc Live Expressions mới.

### B update — M8-S14.7 dual AS5600 velocity PI code staging (2026-10-31)

[USER REQUEST] Firmware PI per motor: thay hoạt động đồng bộ S14.2 P+D bằng một bộ điều khiển tốc độ riêng cho LEFT và RIGHT trong pilot D-pad LEFT→R_MIN, outer speed reference dùng normalized progress. Mục tiêu RIGHT bám tốt hơn ~567ms lag mà không tăng PWM gần SOFT, không đổi bảo vệ.

[REPO DECISION] Code mới thêm `steer_s147_velocity_pi_enable=0` boot, `steer_s147_latched` latch một lần khi manual new stroke Level1 D-pad LEFT và S14.5 OFF. Dual PI samples AS5600 per-axis actual timestamps, mục tiêu riêng cùng quỹ đạo, boost-only <=40CCR và chỉ ngoài 180 RAW cả hai trục. Giữ baseline owner level1 RIGHT125 cap150 LEFT330 cap375, ramp12, terminal 100RAW taper không đổi, NC/STOP/late EXTI. S14.2 or S14.7, không chạy cả hai; R1/R2/SELECT/RX/Dpad RIGHT giữ cũ. Host test, docs, Actions cập nhật; git diff phải xác minh không thay bất kỳ hệ thống bảo vệ cơ bản nào.

[VALIDATION LIMIT] Host/static verify có thể kiểm tra đường code/guard và helper, không có CubeIDE target build, flashed MCU hoặc đo phần cứng. Chưa có dữ liệu S14.7 ON. Không tuyên bố PI giải quyết delay, không đổi đánh giá R1 LEFT obstruction do USER xác nhận. Tài liệu hướng dẫn đầy đủ `docs/M8_S14_7_DUAL_STEERING_VELOCITY_PI.md` ghi bảng Live Expressions phải ON và cách capture.


### M8-S14.7 — kiểm tra CI lần 1 và chỉnh script kiểm tra (2026-10-31)

[CI FIRST RUN] Commit `21e3f353d724a5d3f8544a942a8789c0c6775b09`: handoff nguyên văn/17 mục, S14.2/S14.3/S14.5 test đều PASS; S14.7 verifier FAIL trước khi chạy host C do `ValueError: substring not found` — script đòi 11 dấu cách, C source thực có 10. Đây là lỗi anchor của static verifier, không phải chứng cứ firmware PID C lỗi hoặc đúng.

[FIX] Chỉnh `tools/verify_m8_s147_velocity_pi.py` so với đúng lời gọi trong main.c, không sửa firmware, gains, mức PWM, NC/SOFT. CI cần chạy lại toàn bộ và có bằng chứng PASS mới bàn giao để CubeIDE Clean Build/Flash.


### M8-S14.7 — lần kiểm tra C, sửa sign-compare (2026-10-31)

[HOST RUN2] SHA `0d8f35037dce78cd140cae57504bea6eb235fce4`: sau khi sửa anchor, 14 static route/safety checks PASS, nhưng GCC `-Wall -Wextra -Werror` dừng với 2 cảnh báo ở helper PI `request < MAX_EXTRA_CCR` và `request > MAX_EXTRA_CCR` (int32_t so unsigned macro). Đây là lỗi kiểu C, **không** phải phép thử vật lý hay CubeIDE target build.

[FIX] Ép `(int32_t)M8S147_MAX_EXTRA_CCR` trong hai phép so; giữ nguyên giá trị MAX40 và logic, không đổi limit, duty hay bảo vệ. Cần xác minh CI tiếp theo có biên dịch và chạy toàn bộ C tests thành công.

### M8-S14.8 — cập nhật 17 mục theo đặc tả 386 dòng (2026-10-31)

0. **Nguồn/phạm vi:** source/docs đúng nhánh/SHA; log A/B từ chat là riêng tư, không tự công khai.
1. **Tình trạng:** M8-S14.8 source-staged Level2 default, PI/P+D hybrid opt-in; xem mục 1 cập nhật.
2. **Mục tiêu:** làm RIGHT đến đích nhanh hơn và duy trì tốc độ LEFT, không chỉ giảm skew bằng cách hãm.
3. **Yêu cầu:** USER chủ động chọn Level2 boot; không tự cho phép Flash/cấp động lực; NC/STOP/arming bắt buộc.
4. **Milestones:** M8-S14.8 sau S14.7; GateA/ground loaded GateB chưa đóng.
5. **Đã làm:** main route/default, hybrid pure C helper, tests/verifier, CI workflow, M8S00_S148 và docs.
6. **Dở dang:** Clean Build, kiểm hash ELF/Flash, dòng điện, clearance, coast, ngắt nguồn độc lập, A/B.
7. **Tệp:** `Core/Src/main.c`, `Core/Inc/steer_s148_level2_hybrid.h`, test/verify S148, doc S148, roadmap/workflow.
8. **Chọn/loại:** chọn MAX(PI,P+D boost) không cộng, bỏ P+D trim trong nhánh hybrid; không tăng caps 450/200.
9. **Lỗi:** S14.7 ON chưa chứng minh RIGHT arrival nhanh; R1 LEFT code8 do vật cản USER xác nhận, không phải firmware defect.
10. **Rủi ro:** Level2 boot giúp có thể gọi 450/200 sau START; chưa có chứng cứ nguồn/current/STOP clearance ở Level2.
11. **Kế hoạch:** xem Actions chính SHA; đồng bộ Windows không ghi đè dirty; Clean Build/Flash; gate cắt nguồn; xung ngắn vùng giữa; A/B giá treo.
12. **Thiếu:** flashed ELF, Level2 coast/clearance/current, RIGHT absolute arrival improvement, khả năng ngắt nguồn động lực.
13. **Thời gian:** A/B S14.7 Level1 ON 16:36, OFF 16:48; user yêu cầu phiên bản mới Level2 default 2026-10-31.
14. **Thông số:** Level1 330/125 baseline 375/150 cap; Level2 405/175 baseline 450/200 cap; near SOFT 180/100 RAW, rise12 CCR.
15. **Tiếp quản:** kiểm HEAD mới, CI/commit diff; tài liệu S148 supersedes S147 route; Release không bắt buộc.
16. **Giới hạn:** CI chỉ source/host, chưa target Build/Flash/hardware/loaded; raw logs riêng tư; bảo tồn lịch sử/386-line spec.

### M8-S14.9 — hồ sơ bàn giao bổ sung đầy đủ 17 mục (2026-10-31)

0. **Nguồn:** HEAD branch phải xác minh mỗi phiên; báo cáo ON 17:22 riêng tư; GitHub source docs commit của hiện hành.
1. **Hiện trạng:** S14.9 SOURCE-STAGED RIGHT early terminal forecast opt-in, Level2 boot default, không xem là H/W PASS.
2. **Mục tiêu:** RIGHT đến SOFT nhanh hơn mà không hãm LEFT; giữ nguyên vùng STOP/NC và 180RAW cutoff.
3. **Yêu cầu user:** sửa firmware dựa tài liệu chẩn đoán S14.8 ON; chỉ thử controller ON, không thử OFF lại; Level2 boot=2 duy trì.
4. **Milestone:** S14.9 sau S14.8, CI host xong mới chuyển Clean Build, Flash và GateA kiểm điện/cơ; loaded GateB chưa.
5. **Đã làm:** predictor C pure, main opt-in latch, max-based RIGHT candidate, 4-bin M8S00 snapshots, C host tests/verifier, workflow và docs.
6. **Đang dở:** chứng minh hiệu quả chuyển động bằng ON-only data, hiệu lực tại MCU, coast/clearance/current, independent power cut.
7. **Files:** `Core/Inc/steer_s149_terminal_lead.h`, `Core/Src/main.c`, `tools/test_m8_s149_terminal_lead.c`, `tools/verify_m8_s149_terminal_lead.py`, `docs/M8_S14_9_TERMINAL_LEAD_ON_ONLY.md`, CI, handoff và roadmap.
8. **Quyết định:** lead forecast tự chọn opt-in OFF boot; MAX chứ không SUM; Level2 max450/200, +12 rise, RIGHT candidate max+25; không sửa owner<=180RAW.
9. **Kết quả/lỗi:** log ON S14.8 gần đồng thời tại 180RAW nhưng RIGHT thêm ~337ms trễ cuối; không kết luận motor lỗi. R1 code9 cùng RX mất trung tính là sự kiện riêng, R1 code8 vật cản lịch sử.
10. **Rủi ro:** tăng năng lượng động lực trước 180RAW có thể làm coast tăng dù cap không đổi; không hứa khả năng ngừng an toàn.
11. **Bước tiếp:** CI exact SHA, Windows git status/fetch/ff-only, CubeIDE Clean Build/Flash chứng cứ, điện/cơ gate rồi ON-only short-mid pulses + D-pad LEFT on stand.
12. **Thiếu:** flashed ELF SHA, current, coast distance, NC physical clearance, bộ ngắt nguồn động lực độc lập, SOFT arrival ON-only mới.
13. **Chronology:** 2026-10-31 17:22 M8S14.8 ON test; phân tích sâu; người dùng chủ động yêu cầu sửa theo báo cáo.
14. **Thông số:** S149 offset337ms/deadband50ms, request<=25CCR, actual rise<=12CCR, Level2 max450/200, fast envelope 320→180, owner taper100RAW.
15. **Bàn giao:** kiểm HEAD và doc S149 trước khi trả lời; không dùng snapshot SHA cũ làm HEAD, không ép GitHub Release.
16. **Hạn chế:** CI static/GCC không bằng CubeIDE/Flash/Stand/Ground PASS; dữ liệu log tư nhân không commit; đặc tả nguyên văn 386 dòng giữ nguyên.


[HOST DIAGNOSTIC] First CI of SHA 0b355fc failed only the static string-match 'new fresh stroke latch': source has no spaces around ==/!=, but verifier demanded spaces. Fix the verifier literal without changing firmware; strict C host test still needs to run in next CI. This is not a hardware failure.


[HOST TEST FIX] CI SHA bc633d33: tất cả 17 source/static checks PASS; GCC production helper chạy nhưng test sai kỳ vọng RIGHT=200 khi request theo công thức thực chỉ là 23 CCR trên owner175 -> kết quả 198 CCR. Sửa EXPECTED trong host test, firmware/giới hạn motor không đổi; cần CI SHA mới.


[HOST TEST FIX2] SHA 1def13e: M8S149 pure GCC C host expectation for Level1 used cap150 instead of calculated 125+23=148; adjust test to expect 148 (cap still150), no firmware/limits modification. Need rerun exact-SHA CI.

### M8-S14.9 read-only telemetry bugfix — 17-section audit note

0. Source evidence: ON-only CSV 18:05 + M8S00 18:07 private; prior ON 17:22 comparison.
1. Current task: fix impossible sign condition for S149_BIN, no motor behavior change.
2. Goal: retain measured RIGHT arrival improvement and restore diagnosability of four RAW bins.
3. Requirement: no further OFF tests; Level2 default and PI/S149 opt-in unchanged.
4. Milestone: S149 diagnostics correction, new host CI; target Build/Flash pending.
5. Done in source: passive bin condition + static regression.
6. Pending: CI PASS at fix SHA, target build and refreshed M8S00 on safe stand.
7. Files: main.c, verify_m8_s149_terminal_lead.py, handoff, roadmap, S149 doc.
8. Decision: change direction guard only; do not touch PI/PD/lead gain or 180RAW taper.
9. Error: bins all zero although S149 frames466, requests359, wins254.
10. Risk: old M8S00 bins do not provide per-bin readings; no inferred correction for missing evidence.
11. Next: CI exact SHA, Windows safe fast-forward, CubeIDE Clean Build; only after current/coast/NC gate ON-only trial.
12. Missing: source-to-flash proof, Level2 current/clearance/coast, next post-fix 4-bin diagnostic.
13. Chronology: 2026-10-31 18:05 ON trial after S149 build, same-date passive fix.
14. Metrics: ON S149 R1838.3ms L1541.7ms skew296.6ms, prior S148 ON R1917.6ms L1562.2ms skew355.4ms.
15. Resumption: inspect latest HEAD and Actions, no automatic Release, no private CSV push.
16. Validation: CI static/GCC not MCU build/flash or hardware acceptance.

### M8-S14.10 — bổ sung bàn giao 17 mục (2026-10-31)

0. **Nguồn:** repo HEAD đúng branch phải đọc lại, log S14.9 ON 18:13 riêng tư, S14.10 docs.
1. **Điểm dừng:** S14.10 pure shadow no-actuation, chỉ đề xuất PWM trên dữ liệu cũ, chưa active control.
2. **Mục tiêu:** định lượng tốc độ terminal RIGHT và đánh giá taper có khả năng thay đổi timing an toàn hay không.
3. **Ý người dùng:** thực hiện hướng S14.10, chỉ test controller ON, không yêu cầu PI OFF.
4. **Milestone:** S14.9 ON đã cải thiện 241ms skew tại 18:13 vs S14.8 355ms; S14.10 thu nhận velocity shadow trước kiểm tra điện/cơ.
5. **Làm xong trong code:** header pure shadow, 4-bin summary, UART S1410, host C/unit/static, CI, docs.
6. **Còn thiếu:** CI exact SHA, MCU Clean Build, Flash ELF SHA, đo quãng trôi/dòng/clearance/ngắt nguồn cơ khí.
7. **Files:** Core/Inc/steer_s1410_right_terminal_shadow.h, Core/Src/main.c, tools/test_m8_s1410_right_terminal_shadow.c, tools/verify_m8_s1410_right_terminal_shadow.py, docs/M8_S14_10_RIGHT_TERMINAL_SHADOW.md, workflow, handoff, milestones.
8. **Quyết định:** KHÔNG đưa candidate của S14.10 vào CCR; target giả định 150+2*RAW/s, extra <=20CCR, zero window <=40RAW, stale guard100ms.
9. **Bằng chứng:** S149 bin 100-180 inactive, RIGHT đến SOFT chậm 241ms dù dẫn 44ms tại 180RAW; không có bằng chứng vật lý cho tăng PWM gần SOFT.
10. **Rủi ro:** Tăng PWM vùng 180RAW có thể vượt quãng trôi NC dù cap thấp; proxy50 chỉ đại lượng tốc độ*50ms.
11. **Bước tiếp:** verify CI exact new SHA; git fetch/ff-only; CubeIDE Build/Flash với gate; ON-only log S1410 4 bins và kiểm đo vật lý rồi quyết định.
12. **Thiếu:** khả năng ngắt nguồn động lực độc lập, dòng, nhiệt, quãng trôi thực sau PWM=0, clearance NC, Flash/Build proof.
13. **Thời gian:** 2026-10-31 17:22 S148 ON, 18:05 và 18:13 S149 ON; 18:13 chẩn đoán terminal.
14. **Giới hạn:** Level2 boot2, cap L450 R200, PI/S149 boot OFF, boost cutoff180RAW, owner taper100RAW, shadow 180RAW and <=40RAW no raise.
15. **Tiếp quản:** đọc doc S1410 và diff mới, không chọn release cũ làm HEAD; raw logs riêng tư.
16. **Xác minh:** SOURCE/HOST/CI chỉ mô phỏng logic; không có hardware safety PASS hoặc active S14.10 speed improvement.

### M8-S15 — 17 mục bàn giao riêng về thiết kế bám quỹ đạo chung

0. Repo/current HEAD: verify independent NEW S15 and OLD rolled-back branches, never assume one SHA for both.
1. Goal: independent LEFT/RIGHT angle targets sharing ONE completion time, future Ackermann support.
2. User intent: S15 NEW branch; OLD branch pre M8-S14.7 PI exactly; only controller ON tests.
3. Inputs: two angles in calibrated centidegrees ±4500, two independent AS5600 feedback/timestamps; common T.
4. Done: pure one-clock cubic trajectory, per-axis position/velocity law, source opt-in pilot, docs/host CI.
5. Preserved: M0-M6 wheel controller, M7 NC/limit/state, AS5600 ±45 mapping, calibration, R1/R2, arbitration.
6. Replaced only on S15 pilot: S14.2/S147/S148/S149 coordination is bypassed, not combined.
7. Hardware pilot: Dpad LEFT Level2 SOFT owner targets, not arbitrary external kinematics command.
8. Future: expose independent target ownership with correct stop for internal angles and validated dynamic trajectories.
9. Safety: no increase inside 180RAW; cap Level2 L450/R200, +12 rise, owner SOFT/NC/STOP and atomic CCR remain.
10. Test: C unit covers 35°/30° common T, opposite-signed goals, reject infeasible T, missing sensor, PWM0, terminal no boost.
11. Risks: fixed-angle-trajectory model does not guarantee real-time physical arrival; motor/slew/stall/clearance constrained.
12. Missing: CubeIDE Clean Build and flash exact SHA, electrical current, coast, NC clearance, independent cutoff.
13. Timing evidence: S14.10 ON 18:37 L1540ms R1795ms skew255ms; not S15 outcome.
14. Telemetry: M8S00_S15 plus M8S05/M8S06/M8S11; compare SOFT arrival, angle and phase error, NC/trip.
15. Recovery: OLD branch target bcf6c2a4...; S15 branch includes original history so nothing discarded; lease-safe Git actions.
16. Acceptance: one T completion for two independent targets, measured tolerance required, not claimed achieved by CI.


### M8-S15 CI gate maintenance (2026-10-31)

M8-S15 has exclusive control ownership on fresh D-pad LEFT Level2 pilot. The S14.7 and S14.8 historical static verifiers originally required PI latch start with an exact pre-S15 substring and were misleadingly FAIL after adding the deliberate `steer_s15_latched==0U` guard. Update both verifiers to require this mutual exclusion plus original Level1/2 and other safety checks. **No motor-code change in this gate-maintenance commit**. CI exact SHA must be rechecked. Branch S15 separate from legacy pre-PI rollback.

END OF CONTEXT HANDOFF
