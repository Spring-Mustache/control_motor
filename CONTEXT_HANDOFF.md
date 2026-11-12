**CẬP NHẬT M10 13/11/2026 — WINDOWS IPV4 /24 VERIFIED (USER-REPORTED).** PowerShell xác nhận `Ethernet 2`: `192.168.10.10/24` `Preferred`; PHY link đầu laptop `Up/100 Mbps`, gateway trống. **Giữ nguyên IP Windows**; chọn static IP mục tiêu STM32 `192.168.10.20/24`, chưa cấu hình/chưa bảo đảm không trùng. Hoàn tất kiểm tra host Windows đến mức USER-REPORTED; chưa có ARP/ICMP/UDP/FLASHED. Source audit phát hiện HAL_ETH_Init có sẵn nhưng chưa có HAL_ETH_Start/IP service; linker chưa đặt ETH DMA sections tường minh. Việc tiếp theo là triển khai network trong M10 và CI exact SHA, không sửa M9, không kích hoạt motor; chi tiết xem `docs/M10_ETHERNET_PLAN_2026_11_13.md`.


**CẬP NHẬT 13/11/2026 — M10 WINDOWS RJ45 LINK UP 100 Mbps (USER-REPORTED).** PowerShell của người vận hành cho thấy adapter Windows `Ethernet 2` (Intel I219-LM) báo `Up`, `100 Mbps`; Wi-Fi vẫn Up. Đây chỉ là dấu hiệu link Ethernet ở đầu laptop theo sơ đồ RJ45 onboard, **không** là ping/UDP hay hardware end-to-end PASS. Chưa thay IP/flash. Bước tới là quan sát IPv4 tại adapter Ethernet 2, không chỉnh Wi-Fi. Xem `docs/M10_ETHERNET_PLAN_2026_11_13.md`. M9 giữ nguyên.

**CẬP NHẬT M10 13/11/2026 — RJ45 ONBOARD CONFIRMED.** Người vận hành xác nhận M10 dùng cổng RJ45 tích hợp NUCLEO-F767ZI, không dùng module PHY ngoài. Theo UM1974: CN14/RMII/PHY LAN8742A U9, JP6 và JP7 ON. Đây là quyết định lựa chọn phần cứng, **không phải ETH LINK, ping hay UDP PASS**. Tiếp tục Windows: kiểm tra cáp/Windows NIC read-only trước khi cài IP/firmware; rồi Ubuntu dùng cùng giao thức. M9 SHA `7fd8ab8` và source Profile 4 `b1c1e9c4` bảo toàn, chưa chỉnh motor/flash. Chi tiết: `docs/M10_ETHERNET_PLAN_2026_11_13.md`.

**CẬP NHẬT 13/11/2026 — CHUYỂN SANG M10 ETHERNET, ĐÓNG BĂNG M9.**
Người vận hành yêu cầu tách nhiệm vụ kết nối Ethernet laptop ↔ STM32F767ZI thành milestone riêng **M10**, thử **Windows trước, Ubuntu sau**, không làm ảnh hưởng các chức năng đã đạt ở M9.
Nhánh M10 `feature/m10-ethernet-windows-ubuntu` được tạo từ HEAD M9 đã xác minh `7fd8ab82552efb59de12d12e6960eaeda8149d16`; mốc firmware Profile4 vẫn là `b1c1e9c4d8f7d191b21123a328ee1dea550595da`. Không thay đổi code motor M9, không merge và không flash khi mới tách nhánh.
Khảo sát source: `control_motor.ioc` đã chọn ETH RMII LAN8742A (PHY0); `main.c` có HAL_ETH_Init, nhưng chưa có bằng chứng đã triển khai giao thức IP/TCP/UDP; không đồng nhất PHY với ping thành công.
Trạng thái hiện tại: **M10-S00 DOCUMENTATION/SOURCE BASELINE ONLY**. M10-S01 Windows link/ARP/ICMP/UDP echo, M10-S02 Ubuntu cùng giao thức, M10-S03 kiểm thử hồi quy và bàn giao. Tất cả build mới/FLASHED/kiểm tra Ethernet thực tế **CHƯA KIỂM CHỨNG**.
Đọc trước `docs/M10_ETHERNET_PLAN_2026_11_13.md`, `docs/M10_SESSION_HANDOFF_2026_11_13.md`, `docs/PROJECT_CURRENT.md` trên main và xác minh HEAD mới. 17 mục (0–16) lịch sử bên dưới được giữ nguyên, các nhãn “hiện hành” có ngày cũ là snapshot lịch sử và không được ghi đè lựa chọn M10.
**Ranh giới:** M10 code mạng chỉ chạy riêng trên nhánh M10; chưa thêm lệnh điều khiển motor qua Ethernet; khi nạp M10 lên cùng MCU sẽ thay firmware đang chạy, nên phải bảo toàn đường rollback M9 và kiểm điều kiện an toàn trước flash.

# CONTEXT HANDOFF

**CẬP NHẬT 11/11/2026 — NGƯỜI VẬN HÀNH XÁC NHẬN KẾT QUẢ TRÊN XE.** Trong bối cảnh bản bàn giao `b1c1e9c4`/profile4, người dùng xác nhận bánh tạo phản lực khi bị tác động và không còn quay qua lại nhiều lần để dò góc trong lần thử. Đây là **USER-REPORTED định tính**, chưa kèm log SHA MCU, lực/góc/thời gian/dòng/nhiệt hoặc phạm vi từng trục/tải. CI firmware run42759998485 có ba job kiểm tra/build thành công. Động học tổng thể chưa tích hợp, SELECT tạm hoãn. Bản bàn giao hai phiên và nội dung báo cáo mới nằm tại [hồ sơ 11/11](docs/M9_SESSION_HANDOFF_2026_11_11.md). Các khối có ngày cũ bên dưới giữ trạng thái tại thời điểm đã ghi.

**CẬP NHẬT 10/11/2026 — GHÌ RỒI THẢ BỊ HUNTING.** Người dùng báo phản lực có nhưng dò góc lâu/quay qua lại. Lần báo chưa có SHA MCU; mốc đã giao c9fd47fd, profile4 trước đó chỉ là nháp chưa push. Bản sửa gộp giữ tải liên tục với tốc độ Q8 lọc60ms, ngừng tích lũy I khi đang về đích, xả I đẩy sai chiều khi quá đích, Kp8/4 thay16/8. I vẫn giữ khi đúng goal/đứng yên; không thêm timer nghỉ.216 fixture bỏ tải/trễ có tiêu chí quá độ PASS,36 fixture giữ tải và600s exact-goal I retention PASS, UBSan và41 regression PASS. Đây là HOST, chưa kết luận hardware hunting đã hết. Xem [báo cáo giảm hunting](docs/M9_RELEASE_SETTLING_2026_11_10.md).


**HIỆN HÀNH — M9 GIỮ TRỤC LIÊN TỤC, PROFILE4 (2026-11-10, giờ Việt Nam).** Người dùng nhắc lại đây là bài toán chính của M9. Bản c9fd47fd/profile3 hồi vị có nghỉ không đáp ứng đầy đủ yêu cầu; gọi hoàn thiện là kết luận quá mức. Đã thay cấu hình mặc định bằng PI vị trí + D vận tốc: giữ I/lực bù tại goal, không NO_PROGRESS/cooldown định kỳ khi bị ghì, bỏ ngưỡng8RAW (bỏ nhiễu±1RAW), anti-windup và xóa I khi STOP/NC/owner/fault. Giữ sampler20ms/trục, lease50ms và hai fix Rotate.41 kiểm tra local,36 mô hình tải/ma sát, giữ output tại goal600s giả lập và UBSan PASS; CI/ARM phải kiểm đúng SHA. Logic liên tục đã thực hiện; mô-men/nhiệt và độ lệch thật **CHƯA KIỂM CHỨNG**. [Hồ sơ sửa đúng yêu cầu M9](docs/M9_CONTINUOUS_HOLD_CORRECTION_2026_11_10.md). Các chỉ dẫn profile3/“final hồi vị” bên dưới là lịch sử đã được thay thế.


**BẢN BÀN GIAO M9 FINAL — 2026-11-09 UTC.** Người dùng yêu cầu chốt trong một lượt, không thêm vòng thử mẫu. Bản hiện hành thay profile xung thử bằng **profile3 phục hồi vị trí hai trục**: dải M8 LEFT175..300/RIGHT50..100CCR, bù ma sát giữ qua tiến triển, dừng theo phản hồi góc; poll10ms luân phiên, lease50ms, sensor80ms; kẹt/hết đợt nghỉ riêng từng trục rồi tự phục hồi. STOP/NC/sensor/owner vẫn thu hồi; không có ngưỡng lệch32RAW. Hai fix Rotate và handoff HOLD→Rotate từ nền `a944da4f` được giữ.40 lệnh hồi quy local,162 mô hình giả lập và UBSan PASS; CI/ARM phải được kiểm ở đúng SHA. **Phần mềm đã triển khai; hardware profile3 CHƯA KIỂM CHỨNG**, commissioned vẫn0 (không khóa profile3). Không yêu cầu Live Expressions hoặc vòng thử trung gian để nhận mã final. Hồ sơ hiện hành: [M9_FINAL_DELIVERY_2026_11_09.md](docs/M9_FINAL_DELIVERY_2026_11_09.md). Các khối “hiện hành/pending/next” cũ bên dưới là lịch sử, không thay thế chỉ thị final này.


**HIỆN HÀNH 2026-11-09 — TIẾP TỤC M9 TRÊN NỀN ROTATE ĐÃ FIX.**
Theo chỉ thị mới, M9 đã được tiếp tục trên `feature/m9-steering-active-position-hold`.
Nền firmware là `b382354ec1592589843f4b579643e5b1abbbecf0`, đã có xác nhận Rotate
RIGHT5s/LEFT10s; hồ sơ nền là `22e7eb5cd18c2441e89af1170831efe39fbc38a8`.
Tích hợp toàn bộ M9 mới nhất từ `3df43edde8f53b5e84561704189a83ac8286a56b`.
Commit tích hợp dùng22e7eb5 làm parent thứ nhất và3df43ed làm parent thứ hai,
giữ cả lịch sử, không force-push và không đổi nhánh M8 đã thử thành công.
Các ghi chú M9 tạm dừng bên dưới là lịch sử đã được chỉ thị này thay thế.
Cả hai fix Rotate được giữ: R2 tiếp tục khi chỉ có RPM bánh thụ động; timer spin
chỉ tính từ lúc wheel API nhận lệnh. M9 nhường HOLD ngay khi Rotate hoàn chỉnh
đang chờ arbitration, cắt lease/deadline cũ trước khi R2 ghi PWM đầu tiên.
Giữ candidate M9 LEFT300CCR/<=200ms, RIGHT85CCR/<=12ms và PD30/10 như3df43ed;
chưa có log hardware xác nhận candidate HOLD này. Rotate PASS của b382354e
không tự chuyển thành hardware PASS của bản M9 tích hợp. Xem [báo cáo tích hợp](docs/M9_ROTATE_BASE_INTEGRATION_2026_11_09.md).


**XÁC NHẬN HIỆN HÀNH 2026-11-09 — ROTATE ĐÃ HOẠT ĐỘNG TRONG HAI LẦN THỬ MỚI.**
Người vận hành xác nhận đã sửa được lỗi. Log MCU `b382354ec1592589843f4b579643e5b1abbbecf0`
ghi RIGHT yêu cầu5s, thời gian lệnh xoay5006ms; LEFT yêu cầu10s, thời gian10002ms.
Cả hai result1/started1, hai MIN NC hợp lệ, trip0, không hủy. Đây là PASS cho hai
lần thử được ghi và xác nhận chuyển động của người vận hành. R2 chỉ12–13ms và
passiveFrames0, nên log này không kiểm chứng riêng toàn hành trình R2 từ CENTER
hoặc chứng minh RPM thụ động là nguyên nhân duy nhất của log lỗi cũ.
Giữ `b382354e` làm mốc firmware Rotate đã thử thành công; M9 vẫn tạm dừng.
Không tiếp tục chỉnh Rotate hoặc tự quay lại M9 khi chưa có chỉ thị mới.


**CẬP NHẬT HIỆN HÀNH 2026-11-09 — SỬA HỦY R2 DO RPM BÁNH THỤ ĐỘNG.**
Log MCU `d4894029` xác nhận 4/4 Rotate bị hủy trước spin: R2 chỉ chạy 170–350ms,
`pos=9`, `result=4`, `started=0`, `nc=0`, `trip=0`. Log cũ chưa có mask của điều kiện
hủy. HOST đã tái hiện đúng chuỗi lỗi này khi bánh quay thụ động trong lúc đánh lái.
Sửa gate Voice R2/45: đứng yên trước khi nhận lệnh; khi đã đánh lái chỉ yêu cầu
không có lệnh/PWM kéo bánh. RPM thụ động không tự hủy owner. Spin vẫn chờ RPM
về mức hợp lệ qua wheel API gốc, thời gian xoay chỉ bắt đầu khi API nhận lệnh.
UART thêm `passiveFrames`, `abortSrc`, `abortMask`, `permit`. M9 vẫn tạm dừng;
nhánh hiện hành `feature/m8-r1-voice-center-right-late-fine-10raw`.
Chưa có kiểm chứng trên xe của bản sửa mới. Xem [báo cáo sửa R2](docs/M8_VC02_ROTATE_R2_CONTINUATION_2026_11_09.md).


**HIỆN HÀNH 2026-11-09 — TẠM DỪNG M9, SỬA VOICE ROTATE TRÊN NỀN M8.**
Theo chỉ thị mới của người vận hành, nhánh đang làm là `feature/m8-r1-voice-center-right-late-fine-10raw`,
phát triển trực tiếp từ mốc tiền đề M9 `d367618d4d9c4c382a9a7208f9b878dfefa21ace`.
Xác minh HEAD của nhánh này; các chỉ dẫn M8-S15 cũ bên dưới là lịch sử.
M9 HOLD tạm dừng, được giữ riêng tại `feature/m9-steering-active-position-hold`
(HEAD đã kiểm tra `3df43edde8f53b5e84561704189a83ac8286a56b`);
MCU cuối cùng có log M9 xác nhận là `b89b4d5c45c6173f5d726b9b1d9f7f755af05fe1`,
chưa được coi là đã flash 3df43ed. Không merge M9 vào nhánh sửa M8.
Mục tiêu hiện tại: Rotate LEFT = R2 → L2, RIGHT = R2 → L1, giữ xoay đủ số giây
sau khi bộ điều khiển bánh chấp nhận khởi động; giữ nguyên NC/STOP/PS2/PID.
Chi tiết, giới hạn bằng chứng và cách cập nhật/test: [M8_VC02_ROTATE_STARTUP_2026_11_09.md](docs/M8_VC02_ROTATE_STARTUP_2026_11_09.md).


> **HỒ SƠ SỐNG 17 MỤC:** Giữ nguyên dữ kiện và quyết định lịch sử A, đồng thời **cập nhật lớp hiện hành** sau thay đổi code/nhận định/lần đo. **Không lấy SHA hoặc trạng thái ở ảnh chụp gốc làm HEAD.** Người tiếp quản phải xác minh trước nhánh đang làm `feature/m8-s15-dual-target-common-time` và SHA thực tế ở GitHub, rồi đọc mục 1 hiện hành, toàn bộ 0–16 và roadmap cùng SHA. Nhánh M7-S15 cũ là mốc rollback trước PI, không phải nơi tiếp tục M8-S15. Git commit là nguồn chính, Actions kiểm tra mỗi push; Release chỉ theo milestone/yêu cầu.
> **2026-11-09 — LEFT recovery300CCR/<=200ms, SOURCE/HOST chờ MCU:** USER báo ERR và iOVER đều tắt,RUN nhấp nháy,motor có tiếng rít nhưng đĩa lái đứng yên. LED/RUN không đo mô-men; chưa xác định nguyên nhân duy nhất. Ghi chúM8 đã có ca175CCR/30–45ms không dịch cảm biến,assistant đã bỏ sót khi chọn burst đầu. Bản mới dùng LEFT300CCR (trần chạy CENTER hiện có),cửa sổ tối đa200ms,cắt sớm theo goal/damping/STOP/NC/cảm biến và deadlineSysTick. Đây là cấu hình thử do assistant chọn,không phải giới hạn dòng/nhiệt đã nghiệm thu. RIGHT vẫn85CCR/12ms do chưa có powered test. PD nền30/10,4cửa sổ/vòng,cooldown>=3s+budget hồi,không max_error_raw. Host300/200,goal cut sớm,mainblocked225ms và sanitizer kiểm riêng; CI exactSHA trước flash. MCU cuối vẫnb89b4d5. Xem docs/M9_H2B1_LEFT_RECOVERY_WINDOW_2026_11_09.md.

> **2026-11-09 — MCU log184953: trợ lực LEFT đã phát, chưa về center:** 13dòng hợp lệ trênb89b4d5c45c6173f5d726b9b1d9f7f755af05fe1. Seq3 LEFT kick0→4→8→12,peak175CCR,sai số tại các nhóm−89→−83→−78→−78RAW;cuối23053ms state6/reason13/active1,retry3,appliedLms3916. Xác nhận đường software/adapter burst hoạt động và budget4đợt nghỉ có giữ goal; không chứng minh mô-men,thời lượng điện hoặc tự về center. RIGHT sai số<=2RAW,output/kick0,chưa thử lực. Seq4 là CENTER mới,gần0sai số rồi state4/reason1=NEW_OWNER;không dùng làm bằng chứng HOLD tự hồi hoặc STOP2. Cần quan sát cơ khí bên trái nhích/giật theo burst hay đứng yên trước chỉnh tiếp. Docs-only:giữ MCUb89b4d5,không flash/lặp phép thu chỉ để chứng minh kick đã kích;không tự nâng lực từ file.

> **2026-11-09 — HOLD trợ lực thắng ma sát, SOURCE/HOST chờ MCU:** USER yêu cầu thực hiện bước tạo phản lực sau log181422. PD nền vẫn30/10CCR; thêm burst LEFT175CCR/30ms,RIGHT85CCR/12ms từ giá trị tinh chỉnh CENTER hiện có. Chỉ trợ lực khi lệch>=8RAW,PD kéo đúng về goal,vận tốc<=20RAW/s ổn định200ms và có mẫu mới; sau burst PWM0>=200ms/mẫu mới. Tối đa4burst mỗi trục mỗi vòng; reason13 nghỉ hai trục,giữ goal,nghỉ>=3000ms và hồi đủ budget rồi retry. SysTick chỉ cắt burst hết hạn,không gia hạn bởi vòng main; adapter trả số PWM thực đã ghi,kickL/R đếm lần trợ lực. No-progress dùng thời gian output tích lũy qua khoảng0. Giữ STOP/NC/sensor/hành trình và bỏ32RAW; không bật commissioning2. Mức mới là lựa chọn thử nghiệm,không phải giới hạn dòng/nhiệt hoặc motor PASS. Xem docs/M9_H2B1_BREAKAWAY_ASSIST_2026_11_09.md.

> **2026-11-09 — MCU log181422: giữ goal toàn hành trình + tự retry đã quan sát, phản lực chưa đạt:** 19 dòng hợp lệ trên32520fe0c86facf59756e6c0071985541f7f42d0. Seq2 giữ active1 dù peakError149RAW, tự retry4 lần tại8913/15602/23247/29948ms,LEFT peak30CCR/applied5166ms,cuối30960ms còn lệch−138RAW; không có RANGE5. Seq3 LEFT applied1001ms rồi STOP state4/reason2/active0/lease0 tại9385ms,còn lệch−120RAW. RIGHT peakPWM/applied0 vì sai số không quá4RAW. Xác nhận giới hạn sai số32 đã bỏ, retry và STOP phía phần mềm; không xác nhận lực/mô-men/return hoặc dòng/nhiệt. Không tăng PWM từ log này. Docs-only: giữ MCU32520fe,không cần flash lại hoặc lặp phép thử cùng30/10 chỉ để xác nhận hai lỗi logic đã sửa. Bước tiếp là đánh giá đường tạo lực/hiệu chỉnh actuator; thiếu lực thắng ma sát vẫn là giả thuyết.

> **2026-11-09 — Bỏ giới hạn sai số HOLD32RAW sau log175629 (SOURCE/HOST, chưa flash):** USER yêu cầu bánh bị đẩy lệch trong hành trình hợp lệ phải tiếp tục được kéo về. Đã xóa max_error_raw và hai nhánh hủy goal theo sai số trong Begin/Step, không thay32 bằng một trần lệch khác. Main vẫn kiểm tra góc đo và goal theo hành trình hiệu chuẩn; STOP/NC, sensor stale/jump, service/port và owner mới vẫn hủy goal. Profile30/10CCR và cơ chế nghỉ/retry11/12 không đổi. HOST tái hiện chính LEFT1919/goal1886/error−33 và sai số33..400RAW cả hai dấu/trục, vẫn giữ goal; lỗi ngoài hành trình vẫn ngắt. MCU cuối xác nhận làaf6a43d, lực kéo về thật/dòng/nhiệt chưa đạt. Xem docs/M9_H2B1_FULL_TRAVEL_RECOVERY_2026_11_09.md. Cập nhật này thay thế mọi ghi chú giữ maxError32RAW trước đó.

> **2026-11-09 — HOLD tự thử lại có giới hạn (SOURCE/HOST, CHƯA THỬ TRÊN XE):** USER xác nhận đẩy lệch rồi thả tay nhưng bánh không quay lại, chưa có số đo dòng; yêu cầu luôn cố gắng tạo lại phản lực và không muốn chờ đo dòng. Đã đổi riêng NO_PROGRESS11/EFFORT_BUDGET12 thành COOLDOWN6/active1: giữ nguyên hai goal, đưa cả hai PWM về0, hồi đủ ngân sách output rồi tính lại điều khiển từ góc mới và tự thử tiếp. Không tăng trần30/10CCR, không thêm hoặc tuyên bố có giới hạn dòng. STOP/NC/sensor/range/owner vẫn hủy goal, không tự khởi động lại. Đây là triển khai có giới hạn của yêu cầu; không phải cấp lực vô hạn hoặc đã sửa được phản ứng cơ khí. Xem docs/M9_H2B1_PERSISTENT_RETRY_2026_11_09.md.

> **2026-11-09 — First MCU powered-tuning register-output/cutoff evidence (NOT HOLD response PASS):** private UART M9H2B1_HOLD_20261109_172834.txt has3 complete rows, SHA621a5ce87385600314404bc8866662c0102f4df4 seq1/source2 CENTER/profile1. WAIT_DATA at37ms -> HOLDING at109ms -> FAULT reason11(NO_PROGRESS) at1868ms, active0/lease0/faultAxes1. LEFT peakCCR30, acknowledged nonzero output1007ms, final/peak error23RAW (goal1886,raw1909); RIGHT peakCCR/output-time0, peak error3RAW. Firmware cut both on its response guard, not a captured Voice STOP. Driver torque, release timing, current and thermal result are not established. No automatic PWM increase; next required observation is whether LEFT was released promptly or still externally held during the no-progress interval. Docs-only update: keep MCU621a5ce, do not reflash or repeat old static logs for this record.

> **2026-11-09 — USER-authorized powered H2-B1 tuning:** USER requests actual continuous two-axis HOLD using existing STOP/NC, without a prerequisite motor-supply disconnect test. New profile1=TUNING permits PWM after fresh eligible BOTH-DONE; POWER_COMMISSIONED remains0 and means current/thermal acceptance is pending, not power locked in profile1. Initial engineering caps LEFT30/RIGHT10CCR (3%/1%), independent PD/slew,200ms reversal blank; no-progress1s and recoverable output-time budget cut BOTH axes on tuning fault, with no auto-resume. No fixed HOLD session expiry. Boot has no target/output. Source/HOST verified locally; new CubeIDE build/flash/physical test still pending. See docs/M9_H2B1_POWERED_TUNING_2026_11_09.md. Older closed-power/pending notes below are dated history; do not demand the rejected disconnect pretest or repeat validated H1/H2-B0/unpowered B1 logs.

> **2026-11-08 — H2-B1 MCU uncommissioned integration observed:** private UART firmware `4d004757f48f67c1b1b5192b8020c3e8d196ba9a`, four complete lines across seq1/source2 CENTER and seq2/source3 Voice angle. Both retain two goals beyond5s (5002/5001ms), active1/state2/reason7, commissioned0/lease0, all PWM/peakPWM/appliedMs/everApplied0. This is LIMITED MCU target-retention/telemetry PASS, not powered HOLD, current/thermal/load or fault-injection acceptance. Latest DOCX identifies DC775 geared motors: wheel110rpm x4, TURN LEFT30rpm, TURN RIGHT32rpm; manufacturer/winding/gearbox model and safe stationary current are not specified. See docs/M9_H2B1_FULL_HOLD_2026_11_08.md. Earlier pending/819f635 notes below are dated history.

> **H2-B1 SOURCE/HOST CI VERIFIED (2026-11-08):** source `ac4fc082167bb13b299cba089bc8998e4d1bd2da`, GitHub Actions [42467841303](https://github.com/Spring-Mustache/control_motor/actions/runs/42467841303) completed SUCCESS. Linux full persistent two-axis controller + production main hooks and all legacy regressions PASS; Windows native PowerShell5.1 parser PASS. Physical commissioning remains0; CubeIDE/MCU/HOLD torque/thermal/load validation pending. A following docs-only HEAD is not a new actuator implementation.

> **2026-11-08 — USER clarified FULL H2-B1 (not H1):** delivery is persistent TWO-axis position HOLD, no3s expiry and no single-axis/short-session substitute. Full C controller + atomic HAL owner route + main idle/PS2/Voice/NC handoff are now SOURCE-STAGED with local strict GCC tests120s(controller)/60s(main hooks) PASS. Physical commissioning remains CLOSED0; PWM caps/gains/reversal timings are unverified, so no powered HOLD is enabled or claimed. GitHub CI on the new source and CubeIDE/MCU validation remain separate. See docs/M9_H2B1_FULL_HOLD_2026_11_08.md.

> **2026-11-08 — USER-REPORTED motor-lead current baseline:** User measured on wires FROM driver TO motor (not driver input). During CENTER travel LEFT0.8–1A and RIGHT≈2A; after stopping meter reads0A on both. Earlier LEFT5A/RIGHT10A were estimates from discussed motor powers80/200W, NOT verified continuous/stall/HOLD ratings. These readings are a running-current baseline, not safe HOLD limits; meter type/bandwidth and thermal/standstill-current envelope remain unverified. MCU firmware stays819f635; H2-B1 powered output has not been implemented.

> **2026-11-08 — H2-B0 MCU STATIC INTEGRATION/TELEMETRY PASS (1 healthy Voice CENTER run):** Private user log `M9H2_GUARD_20261108_151606.txt`, MCU firmware `819f635aafbe9dde3ee0e4b9c5c8e6cbc282e0c2`, seq1/source2. H2-A state6/reason1 and H2-B0 bState3/bReason1 completed 3003ms, 268 guard updates; freshL/R267/268; sensorErr/deniedNC/deniedOwner/blockBits/dropped0; normal VOICE1->IDLE0 accepted. bPeakL/R0 and bEligibleL/R0; bCcrL/R=dutyL/R=0, powered0. This validates one quiet static integration window, NOT nonzero-candidate/disturbance/STOP-NC fault-injection or powered HOLD. H2-B1/H3 remain pending; no firmware changes in this documentation update.

> **USER clarification (2026-11-08):** Động học chưa hoàn thành để tích hợp/nạp vào firmware; SELECT tạm hoãn. Không tự đồng nhất hai trạng thái hoặc coi nội suy RAW của Voice là động học bánh xe hoàn chỉnh.

> **M9 H2-B0 SOURCE CI PASS 2026-11-08:** Firmware commit 9e4f4533b9c7de820295f9c36ceab28541048dc0, GitHub Actions 42460572191 completed SUCCESS: Linux GCC exact-C H2-B0 candidate + hard-power-lock verifier and H0/H1/H2-A regressions; Windows PowerShell 5.1 syntax PASS. Target CubeIDE Build/Flash/H2B0 MCU log remains PENDING. H2-B1 powered hold not implemented (unknown motor stall current/temp, driver PWM0 coast/brake and independent cutoff). Docs-only HEAD after this report is not new firmware.

> **LATEST M9 H2-B0 SOURCE 2026-11-08:** User wants H2-B started. Bounded candidate HOLD near target added as pure-C policy after H2-A guard. It can compute scores but M9H2B_POWER_STAGE_ENABLED=0, verified PWM0/time0, plus H2-A power lock0; no physical PWM/DIR/I2C/owner writes. CCS_SHB12 manual driver 12A continuous does not verify motor stall/temperature or PWM0 brake/coast. H2-B1 powered action still pending real electrical evidence. Review docs/M9_H2B_POWER_GATE_2026_11_08.md. H2-A earlier MCU 3s PASS, do not ask user to repeat old H1. Verify exact Actions SHA after commit.

> **LATEST 2026-11-08 M9-H2-A ON-MCU 3000ms PASS:** User delivered PRIVATE `M9H2_GUARD_20261108_143827.txt`, MCU SHA `5dd1502ac606c5e254ddda3484baffbf2a70a534`, one complete Voice CENTER `seq=2`, H2 `state=6,reason=1,durationMs=3000,updates=294,freshL=294,freshR=293,blockBits=0,deniedOwner=0,deniedNC=0,sensorErr=0,modeStart=1,modeNow=0,modeOK=1,voiceBusy=0,dutyL=0,dutyR=0,powered=0`. H0/H1 same 3s complete, drift <=1 RAW, no bad/stale/NC. **H2-A POST-DONE QUIET MCU PASS for one run**, not active holding or nonzero dynamic hold response. H2-B motor PWM hard locked 0; unknown stall current, heat and CCS_SHB12 PWM0 coast/brake still must be resolved. Raw UART private, only aggregate in docs. Docs-only commit does not require reflash.

> **2026-11-08 SOURCE CI VERIFIED — H2-A owner handoff fix:** Source commit `977533f79194e2ac3a6b97768675b6647348dbd8`, GitHub Actions run `42459137638` **completed SUCCESS**: Linux exact-C GCC host covers normal Voice->IDLE after DONE, PS2 release and reject takeover/STOP/new Voice/PS2 cancel, H2 hard duty0; older M8/H0/H1 regressions and full context contract PASS; Windows PowerShell5.1 capture parser PASS. Exact MCU build/flash and new 3-second H2 log **not yet available**. No H2-B powered output or HOLD load PASS. Any later docs-only commit will have a different SHA; distinguish it from source flashed SHA.


> **LATEST H2-A 2026-11-08:** User uploaded 2 private UART files at MCU `8214610`. Valid `M9H2_GUARD` record aborted in **13ms** with `reason3 / blockBits256`, `deniedOwner1`, `sensorErr0`, `deniedNC0`, `dutyL/R0,powered0`. Reconstructed from source: ordinary `VOICE->IDLE` after M8 DONE wrongly looked like new owner, not proven hardware fault. New source fix whitelists **only clean normal Voice/PS2 post-DONE->IDLE**, all STOP/NC/other-owner/busy continue to abort; added modeStart/modeNow/modeOK/voiceBusy telemetry. Second TXT has malformed concatenated H0+M8R1 line and **no H2**; cannot infer UART cause or count as H0 pass. H2 still POWER HARD OFF; exact CI after new commit, target Flash and 3s MCU H2 log need verification. Read `docs/M9_H2_GUARD_SOURCE_2026_11_07.md`. Never publish raw TXT.

> **M9-H2-A EXACT CI VERIFIED 2026-11-07:** C source guard `55db22f67c81517ada48106f5a506b7b367b7e4c`, GitHub Actions `42370515934` **COMPLETED/SUCCESS**: strict Linux GCC actual-C H2-A monitor/fault and hard power-off, full M8/H0/H1 context regression PASS, Windows PowerShell5.1 H2 capture script syntax PASS. Initial `b9b75a` Windows parser fail due `$MaxEvents:`, fix only PS interpolation, motor source unchanged. Chưa có CubeIDE Clean Build/Flash hoặc `M9H2_GUARD` MCU log. **H2-B ACTIVE not coded/tested, 0 PWM always.** Sau commit docs HEAD mới cần CI riêng.

> **NEWEST M9-H2-A 2026-11-07:** Theo yêu cầu USER triển khai H2, đã viết **bộ giám sát H2-A tách riêng**, chỉ nhận goal sau cả hai trục M8 DONE2, tính near-goal PD shadow và đánh giá STOP/NC/owner/wheel/stale/timeout. Thêm `Core/Src/m9_h2_hold_guard.c`/`.h`, `M9H2_GUARD` UART3 sau idle và `tools/capture_m9_h2_guard.ps1`. Bắt buộc `M9H2_POWER_PERMITTED=0U`, mọi `dutyL=dutyR=0`, **KHÔNG có đường ghi PWM/DIR trong H2-A**. Cần xác minh Actions SHA mới, CubeIDE Build/Flash và MCU guard status; **H2-B ACTIVE CHƯA TRIỂN KHAI** do chưa kiểm chứng current/thermal, PWM0 coast/brake, independent motor E-stop. H0/H1 đã có 130 real motion frames trên SHA `3dbdfaf`. Đọc `docs/M9_H2_GUARD_SOURCE_2026_11_07.md` và các hồ sơ cũ. Không được gọi H2 powered PASS.

> **M9-H1 REAL measured 2026-11-07:** Người dùng gửi `M9H1_REAL_20261107_183513.txt` với **4/4 M8 position DONE**, 130 REAL frames AS5600/CCR (19+37+36+38) hoàn chỉnh ở MCU SHA `3dbdfaf678b700f03f3517cf0e70664c5557f6ba`; tuổi mẫu 0..1ms, no bad/stale/overflow/drop. `u_shadow` **khác0** và thường bão hòa ±40; có trường hợp PD `u` đề xuất **hãm ngược lỗi khi M8 vẫn chạy, còn cách đích 150–180 RAW**, nên gain `Kd/Kp=0.5s` hiện không được đưa sang PWM thật. M8 sai số cuối ≤5RAW; H0 seq3 `reason=3` sau 1180ms (đã abort observer, nguyên nhân chi tiết chưa xác định). **H1 REAL DYNAMIC DATA CAPTURE PASS 4/4**, KHÔNG có HOLD external-force/thermal/current PASS, H2 chưa cấp điện giữ. Đọc `docs/M9_H1_REAL_DRIVE_CAPTURE_2026_11_07.md`; raw TXT vẫn private, docs-only commit không cần nạp lại firmware.

> **M9-H1 REAL capture HOST CI PASS 2026-11-07:** Source commit `93551551422559914052ec8b8671a521715714b4`, Actions `42368389841` completed SUCCESS: Linux GCC exact-C test for REAL trace + wiring/forbidden motor writes, old H0/H1/M8 regressions and full context contract PASS; native PowerShell 5.1 parser PASS. Đây **không phải CubeIDE Build/Flash hay log chuyển động trên xe**. HEAD sau commit chỉ sửa docs cần verify CI riêng; H2 ACTIVE vẫn chưa có.

> **M9-H1 REAL DATA instrumentation, 2026-11-07:** Người dùng **không muốn mô phỏng**, muốn ghi dữ liệu chuyển động thật của xe **trong một lần thử** để cải tiến PD. Đã chuẩn bị bộ ghi RAM-only 80 frame (80ms giữa frame) trên các lệnh M8 đánh lái thật R1 CENTER/Voice5–40; mỗi frame chứa RAW, error, velocity PD filtered, candidate PD SHADOW, **CCR thực tế M8 chỉ đọc** và timestamp/NC/state. H0/H1 static 3s vẫn giữ nguyên; chỉ xuất `M9H1_REAL_BEGIN/FRAME/END` qua UART3 IRQ sau khi cả hai motor đã dừng, và sau các log M8/H0/H1. Công cụ `tools/capture_m9_h1_real.ps1` một lần nghe **4 lượt thực** và gom tất cả file TXT local, không dùng Live Expressions hay tác động tay/hack sensor. **Chưa có CubeIDE build/flash/log REAL và KHÔNG có đo ngoại lực tháo tải**; không suy H2 ACTIVE PASS. Xem `docs/M9_H1_REAL_DRIVE_CAPTURE_2026_11_07.md`. Phải kiểm tra exact GitHub CI sau commit.

> **M9-H1 NEWEST 2026-11-07 — ON-MCU STATIC UART:** Người dùng gửi **4/4 cặp M9H0_DRIFT + M9H1_SHADOW**, MCU `sha=00ccaeace99a6948880ecf85061dea5d2f77514e`, mỗi cửa sổ 3000–3010 ms, mỗi trục 294–296 mẫu, H0 peak drift≤1RAW và H1 `peakVel=0`, `peakCmd=0` cả 8 trục-cửa sổ; 28 cửa sổ biến thiên 1RAW bị loại, không có lỗi sensor/stale/drop/đảo dấu. **H1 ON-MCU UART DATA CAPTURE PASS giới hạn tĩnh**, **chưa chứng minh hồi vị/giảm chấn khi ngoại lực** vì |error|≤6RAW dưới ngưỡng kích hoạt8RAW. H1 SOURCE/HOST CI đã PASS, H2 ACTIVE chưa triển khai, không cấp lực HOLD. GitHub branch M9; sau cập nhật tài liệu phải kiểm tra HEAD mới và CI. Đọc `docs/M9_H1_PD_SHADOW_2026_11_06.md`; log gốc vẫn private.

> **M9-H1 CHECKPOINT 2026-11-06:** SOURCE + exact-SHA **HOST CI PASS** tại commit `7d8c98e14a0fd0cc35fc95c1a45d51988d464c2b`, GitHub Actions run `42237917589`: Linux `verify_m9_h1_shadow.py` biên dịch C thật + mô phỏng load-release PASS, hồi quy H0/M8 và workflow handoff PASS; Windows PowerShell syntax PASS. **Chưa có CubeIDE Clean Build/Flash hoặc log MCU H1**, càng không có mô-men HOLD ACTIVE. Nhánh hiện hành `feature/m9-steering-active-position-hold` cần kiểm tra HEAD sau commit docs. Lệnh và evidence: `docs/M9_H1_PD_SHADOW_2026_11_06.md`.

> **HIỆN HÀNH — 2026-11-06 M9-H1:** Người dùng yêu cầu **triển khai M9-H1 PD SHADOW**. Nhánh `feature/m9-steering-active-position-hold` lấy nền `d367618d`; H0 đã có 4/4 cửa sổ private MCU 3 giây **DATA CAPTURE PASS giới hạn** ở SHA `8c2b3fe7` (trôi đỉnh ≤1 RAW). **H1 SOURCE-STAGED, cần kiểm tra exact-SHA HOST CI rồi CubeIDE Build/Flash/MCU log riêng**; tính PD tham khảo trong bộ nhớ, không ghi PWM, không có HOLD_ACTIVE. Bảo toàn STOP, NC, PS2, VC-02, M8 đã nghiệm thu. Xem `docs/M9_H1_PD_SHADOW_2026_11_06.md`; đừng nhầm các dòng M8 “LATEST” lịch sử bên dưới với HEAD.

> **TRẠNG THÁI HIỆN HÀNH 2026-11-06 — M9:** **H0 SOURCE + exact-SHA HOST CI PASS** tại `8c2b3fe7d57154db7d11f8abca40351c4753d362`; **người dùng đã cung cấp 4 dòng UART MCU H0**, mỗi cửa sổ 3 giây hoàn tất, **DATA CAPTURE PASS trong phạm vi quan sát tĩnh**, drift đỉnh ≤1 AS5600 RAW/trục, không có bad/jump/drop; chưa thử ngoại lực, không có mô-men HOLD. CubeIDE build/flash log độc lập chưa được cung cấp. Các cập nhật M8 cũ vẫn là lịch sử, H1/H2/H3 chưa viết.  Người dùng đã chọn phát triển **bộ điều khiển phần mềm giữ góc lái Active Position Hold**, **không đầu tư thêm phần cứng**. Nhánh công việc hiện tại: `feature/m9-steering-active-position-hold`, tách từ đúng `d367618d4d9c4c382a9a7208f9b878dfefa21ace`. M9-H0 đã triển khai mã **read-only**, H1/H2/H3 vẫn PLANNED; chưa có mã HOLD ACTIVE hay powered-hold PASS. H0 passive drift → H1 PD shadow → H2 opt-in bounded active → H3 integration; đọc `docs/M9_STEERING_ACTIVE_POSITION_HOLD_PLAN_2026_11_06.md`. **Không nhầm nhánh thử M8 R1 10RAW chưa có physical PASS với M9 đã nghiệm thu.** Các ghi chú “CURRENT/LATEST” M8 bên dưới là lịch sử theo thời điểm ghi, không phải HEAD M9.

> **HỒ SƠ SỐNG 17 MỤC:** Giữ nguyên dữ kiện và quyết định lịch sử A, đồng thời **cập nhật lớp hiện hành** sau thay đổi code/nhận định/lần đo. **Không lấy SHA hoặc trạng thái ở ảnh chụp gốc làm HEAD.** Người tiếp quản phải xác minh nhánh người dùng đang làm (hiện tại `feature/m9-steering-active-position-hold`) và HEAD SHA thực tế ở GitHub, rồi đọc mục 1 hiện hành, toàn bộ 0–16 và roadmap cùng SHA. Nhánh M7-S15 cũ là mốc rollback trước PI, không phải nơi tiếp tục M8-S15. Git commit là nguồn chính, Actions kiểm tra mỗi push; Release chỉ theo milestone/yêu cầu.

---

**CURRENT 2026-11-02:** PowerShell receiver now uses SHA reported by STM32 (no local Git lookup) and accepts complete frames even when CubeIDE marks firmware dirty=1 due to .prefs changes. Optional manual label is explicitly unverified. Operator had 4/4 frames at 16:06 but old script reported invalid4; TXT saved locally. Firmware itself unchanged; S15.3 OFF, no new flash required. See final addendum.

**MỚI NHẤT 2026-11-03:** S15.1 CENTER L1873/R665 hợp lệ, bản mới Live Expressions có dấu hiệu lệch byte: giá trị gốc PWM85 hiển thị 21760, entry15 thành3840. Đây mới là giả thuyết debugger/ELF, chưa chứng minh bằng log MCU. Bổ sung snapshot RAW 32-bit và M8S00_AS5600 chỉ sau DISARM; giữ hàm I2C và motor gốc. CENTER LEFT theo yêu cầu đổi 1886, RIGHT giữ662. Cần xác nhận target Build/Flash và so sánh MCU độc lập. Xem bổ sung cuối hồ sơ.

**2026-11-03 LATEST:** Trial theo yêu cầu RIGHT R_MIN last100RAW tăng floor PWM50->60, chỉ opt-in boot OFF và latch new manual stroke; không đổi STOP/NC/SOFT, R1/R2/S15.3 vẫn OFF. M8S15_TAIL60 chỉ ghi candidate, actual PWM M8S152_B. Chưa cho phép thử cấp nguồn khi NC margin ~3mm, chưa có coast/current/cutoff. Xem cuối tài liệu.

**LATEST CORRECTION 2026-11-03:** Theo yêu cầu người vận hành, RIGHT R_MIN last100RAW floor60 giờ **default ON (enable=1U)** trên bản nguồn mới, thay cho OFF ở commit 9418d4a. S15.3 vẫn OFF và STOP/NC/SOFT/R1/R2/LEFT giữ nguyên. Không chạy full-stroke điện vì chưa đo coast/current và có cảnh báo khoảng hở NC ~3mm. Xem phần bổ sung cuối hồ sơ.

**HIỆN HÀNH — 2026-11-03:** Đã loại hết thử nghiệm PWM60 (không còn flag, candidate, M8S15_TAIL60), khôi phục mã 50CCR a85438e và giữ CENTER LEFT=1886, RIGHT=662, RAW32. Riêng S15.3 có thử nghiệm dự báo sớm RIGHT→R_MIN khi hai trục 320<remaining<=650 RAW, +80pm ảo, thêm tối đa +12CCR, S15.3 boot OFF. Thêm đếm earlyFrames và avoidedReductionR trong UART khi DISARM; không sửa LEFT, NC, SOFT hay cuối hành trình. Mọi ghi chú PWM60 cũ chỉ để lưu lịch sử; chưa có bằng chứng cải thiện trên STM32/xe thật.

**HIỆN HÀNH 2026-11-03 19:09:** Log faf628b S15.3 OFF, DpadLEFT RIGHT không hoàn tất RAW225/SOFT216, RX LEFT trễ541ms. Nguồn cập nhật bật S15.3 mặc định ON chỉ khi RIGHT->R_MIN ở lệnh Level2 mới, không đổi terminal50CCR, LEFT, NC, STOP, SOFT. S15.3 preview giữa hành trình chỉ >320 đến <=650RAW cả hai, tối đa +12CCR. Build/Flash/test thật chưa kiểm chứng. Luôn gửi lệnh ghi log cùng cập nhật firmware.

**HIỆN HÀNH 2026-11-03 RIGHT R_MIN:** NC RIGHT thực đo 195..197 RAW; người vận hành chọn mốc195. Đã thay boot runtime R_MIN166→195 và R2 reference166→195; margin50 nên SOFT R_MIN245 thay216. Giữ CENTER LEFT1886 RIGHT662, taper50CCR, S15.3 ON riêng RIGHT->R_MIN, LEFT/STOP/NC/R2 NC-only. Cần xác nhận M8S00_CAL_RUNTIME minR195 r2R195, M8S00_STEER softR245 sau Flash; SELECT scan có thể ghi đè runtime. Mỗi cập nhật firmware phải kèm lệnh ghi log và dùng thư mục chính. Không có bằng chứng hoạt động trên motor mới ở thời điểm commit.

**HIỆN HÀNH 2026-11-03 S15.3 ETA:** Log ef3f328 19:33 minR195/SOFT245, 4/4 tới đích nhưng S15.3 ON vẫn request/applied0 bởi ePred; đã đổi sang ETA theo velocity q‰/s quy đổi RAW/s và hai mốc thời gian đoạn cuối L484/R886ms. Chỉ bù RIGHT->RMIN khi cả hai còn (320..650]RAW, tối đa+12CCR, **không tăng PWM <=320RAW kể cả reactive trước đây**, giữ LEFT/50CCR/STOP/NC. M8S153 thêm etaFrames/peakEtaLagMs; chờ target Build/Flash và đo thực. Người dùng chỉ test thư mục chính và phải gửi lệnh cập nhật kèm lệnh ghi log.

**HIỆN HÀNH 2026-11-03 S15.3D (CHƯA PASS):** A/B f790f48 cùng RMIN195 SOFT245: OFF DpadLEFT RIGHT+385ms, RXLEFT RIGHT+660ms nhưng RXRIGHT LEFT STALL13 sát SOFT; ON DpadLEFT RIGHT STALL13 ở RAW257 cách SOFT245 12RAW, RXLEFT dừng stopCode5 trước SOFT dù ETA đã bù 31/38 lần. Bản firmware tiếp theo S15.3 boot OFF; chỉ thêm chẩn đoán thụ động M8S153_FAULT hai trục và M8S153_STOP5 trước khi xóa owner/cắt CCR, UART sau DISARM. Giữ LEFT, S14.2, floor50, RMIN195/SOFT245, NC/STOP/STALL650; chưa có MCU Clean Build/Flash hoặc xác minh điện/cơ thực tế của commit mới. HOST CI, firmware được Flash, và an toàn thực là ba cổng tách biệt. Luôn gửi lệnh cập nhật firmware và lệnh ghi log từ thư mục chính cho người dùng.

**HIỆN HÀNH 2026-11-03 NC manual:** User D-pad/RX needs second press leaving NC, R1 normal. Source patch manual ESCAPE→MOVING after 120ms release without SETTLING; own origin NC edge holds PWM0 full pass. No CubeIDE/Flash/hardware PASS. S15.3D STALL remains BLOCKED. Details docs/M8_MANUAL_NC_DIRECTIONAL_RELEASE_2026_11_03.md.

**HIỆN HÀNH 03/11 S15.3:** Chế độ B cô lập RIGHT trim S14.2 boot ON trên Level2 RIGHT->RMIN mới, ETA OFF; UART M8S153_MODE; log 20:36 ETA OFF 2/4, RIGHT STALL RAW257/255 CCR56/55 NC0. Chưa kết luận PASS; xem docs/M8_S153_TRIM_ISOLATION_2026_11_03.md.

## 0. Thông tin hồ sơ và phạm vi bao phủ

**Cập nhật bàn giao 11/11/2026.** Ngày 11/11/2026 bổ sung lời người vận hành sau khi tiếp nhận bản b1c1e9c4 và hồ sơ hai phiên chat. Đã rà chat gốc trước lúc xoá. Bản chép nguyên văn đầy đủ và các log UART riêng tư không nằm trong bản chuyển chat. Xem docs/M9_SESSION_HANDOFF_2026_11_11.md.


[FINAL snapshot] Nguồn rà soát: yêu cầu hiện tại, ngữ cảnh đã rút gọn, source tại a944da4f, kết quả40 regression local và các tài liệu hiện có. Không tuyên bố còn toàn bộ hội thoại nguyên gốc. Bản final chỉ có bằng chứng phần mềm; CI/ARM và hardware là các mức khác nhau.


**Hiện hành — LEFT300/200 sau báo đứng yên:** Nguồn: log184953,USER xác nhận motor đứng yên/có tiếng rít,ERR/iOVER tắt;code branchb284c597. Các quan sát điện/cơ do USER báo,không biến thành phép đo mô-men hoặc xác nhận ACCE đang bật/tắt.

**Kết quả MCU184953:** Dữ liệu mới13dòng thực tế của TXT184953,2seq,cùngfirmwareb89b4d5;đã kiểm số học goal−raw. Không đăng log gốc riêng tư.

**Hiện hành — trợ lực sau181422:** Nguồn cập nhật: branch4887bc09a60fe24207703d7117f9f7dc01857bbe,log MCU32520fe và chỉ thị 'hãy thực hiện'. Cấu hình175/85,30/12 dựa sourceCENTER đang có; không dựng phép đo dòng/nhiệt.

**Kết quả MCU181422:** Cập nhật từ19 dòng TXT181422 thực tế trên MCU32520fe; log được đọc/kiểm tra số học. Không đăng nguyên bản riêng tư.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Phạm vi mới: yêu cầu hiện tại và log riêng175629 trên MCUaf6a43d. Nguồn gốc386dòng không đổi. Kiểm thử tổng hợp không phải số đo phản lực.

**Hiện hành — retry 09/11:** Cập nhật từ HEAD194266126f0a0dd760a09bd0858e619e0a46e8c1 và hội thoại hiện tại; chỉ xác nhận nguồn/kiểm thử thực sự đọc/chạy. Người dùng đã trả lời về việc thả tay; chưa có dữ liệu dòng/nhiệt mới. Không tuyên bố bao quát những hội thoại không truy cập được.

### Hai lớp nội dung — KHÔNG gộp lịch sử vào trạng thái hiện tại

- **Lịch sử bất biến theo thời điểm:** bảng 2026-10-31 dưới đây, các SHA, log, giả thuyết và đánh giá khi lập hồ sơ A; không xóa hay viết lại để biến thành bằng chứng của commit mới.
- **Cập nhật hiện hành:** xem mục 1 ở đầu (được cập nhật theo các thay đổi có nguồn), các phần bổ sung dưới những mục 0–16 và các mục mới ở cuối `docs/PROJECT_MILESTONES.md`. CI chỉ kiểm tra cấu trúc/update-gate, không tự đọc toàn bộ chat.
- **SHA hiện tại:** không thể tự nhúng SHA của chính commit vào file cùng commit mà không tạo vòng lặp. Trước mỗi lần tiếp quản M8-S15 phải lấy `refs/heads/feature/m8-s15-dual-target-common-time` từ GitHub, cố định SHA rồi đọc các file theo SHA đó. Nhánh M7-S15 cũ được giữ ở mốc trước PI; nếu người dùng chỉ định nhánh khác, dùng nhánh mới.
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

**Cập nhật bàn giao 11/11/2026.** Mốc source b1c1e9c4/profile4. Người vận hành xác nhận khi tác động bánh đã có phản lực và không còn quay qua lại nhiều lần để dò góc trong lần thử. Tiếp tục từ kết quả USER-REPORTED này, đối chiếu HEAD/CI tài liệu rồi phục vụ báo cáo và chuyển chat. Động học chưa tích hợp, SELECT tạm hoãn.


[RELEASE SETTLING 2026-11-10] Hợp nhất yêu cầu giữ trục với phản hồi mới hunting sau bỏ tải; không giao bản PI+D sơ bộ. Kiểm exact CI/ARM rồi cập nhật một lần. Không nhầm profile4 workspace thành firmware người dùng đã thử.


[CONTINUOUS HOLD CORRECTION] Hiện tại phải bàn giao profile4 servo liên tục; không quay lại recovery có nghỉ của c9fd47fd. Sau xác minh CI, đưa mã cập nhật/build/nạp, không giao vòng thử mẫu mới. Chỉ ghi khả năng chịu lực thực tế khi có dữ liệu.


[HÀNH ĐỘNG HIỆN HÀNH] Bàn giao M9 final trên nhánh hiện có, xác minh CI đúng SHA và đưa một lệnh cập nhật/build/nạp. Không mở thêm chuỗi thử mẫu hoặc xin lại quyền phát triển. Không nói người dùng đã nạp firmware khi chỉ mới push GitHub. Profile3 tự cấp quyền sau BOTH-DONE; commissioned0 không phải khóa lực.


**HIỆN HÀNH 2026-11-09 — TIẾP TỤC M9 TRÊN NỀN ROTATE ĐÃ FIX.**
Theo chỉ thị mới, M9 đã được tiếp tục trên `feature/m9-steering-active-position-hold`.
Nền firmware là `b382354ec1592589843f4b579643e5b1abbbecf0`, đã có xác nhận Rotate
RIGHT5s/LEFT10s; hồ sơ nền là `22e7eb5cd18c2441e89af1170831efe39fbc38a8`.
Tích hợp toàn bộ M9 mới nhất từ `3df43edde8f53b5e84561704189a83ac8286a56b`.
Commit tích hợp dùng22e7eb5 làm parent thứ nhất và3df43ed làm parent thứ hai,
giữ cả lịch sử, không force-push và không đổi nhánh M8 đã thử thành công.
Các ghi chú M9 tạm dừng bên dưới là lịch sử đã được chỉ thị này thay thế.
Cả hai fix Rotate được giữ: R2 tiếp tục khi chỉ có RPM bánh thụ động; timer spin
chỉ tính từ lúc wheel API nhận lệnh. M9 nhường HOLD ngay khi Rotate hoàn chỉnh
đang chờ arbitration, cắt lease/deadline cũ trước khi R2 ghi PWM đầu tiên.
Giữ candidate M9 LEFT300CCR/<=200ms, RIGHT85CCR/<=12ms và PD30/10 như3df43ed;
chưa có log hardware xác nhận candidate HOLD này. Rotate PASS của b382354e
không tự chuyển thành hardware PASS của bản M9 tích hợp. Xem [báo cáo tích hợp](docs/M9_ROTATE_BASE_INTEGRATION_2026_11_09.md).


**XÁC NHẬN HIỆN HÀNH 2026-11-09 — ROTATE ĐÃ HOẠT ĐỘNG TRONG HAI LẦN THỬ MỚI.**
Người vận hành xác nhận đã sửa được lỗi. Log MCU `b382354ec1592589843f4b579643e5b1abbbecf0`
ghi RIGHT yêu cầu5s, thời gian lệnh xoay5006ms; LEFT yêu cầu10s, thời gian10002ms.
Cả hai result1/started1, hai MIN NC hợp lệ, trip0, không hủy. Đây là PASS cho hai
lần thử được ghi và xác nhận chuyển động của người vận hành. R2 chỉ12–13ms và
passiveFrames0, nên log này không kiểm chứng riêng toàn hành trình R2 từ CENTER
hoặc chứng minh RPM thụ động là nguyên nhân duy nhất của log lỗi cũ.
Giữ `b382354e` làm mốc firmware Rotate đã thử thành công; M9 vẫn tạm dừng.
Không tiếp tục chỉnh Rotate hoặc tự quay lại M9 khi chưa có chỉ thị mới.


**CẬP NHẬT HIỆN HÀNH 2026-11-09 — SỬA HỦY R2 DO RPM BÁNH THỤ ĐỘNG.**
Log MCU `d4894029` xác nhận 4/4 Rotate bị hủy trước spin: R2 chỉ chạy 170–350ms,
`pos=9`, `result=4`, `started=0`, `nc=0`, `trip=0`. Log cũ chưa có mask của điều kiện
hủy. HOST đã tái hiện đúng chuỗi lỗi này khi bánh quay thụ động trong lúc đánh lái.
Sửa gate Voice R2/45: đứng yên trước khi nhận lệnh; khi đã đánh lái chỉ yêu cầu
không có lệnh/PWM kéo bánh. RPM thụ động không tự hủy owner. Spin vẫn chờ RPM
về mức hợp lệ qua wheel API gốc, thời gian xoay chỉ bắt đầu khi API nhận lệnh.
UART thêm `passiveFrames`, `abortSrc`, `abortMask`, `permit`. M9 vẫn tạm dừng;
nhánh hiện hành `feature/m8-r1-voice-center-right-late-fine-10raw`.
Chưa có kiểm chứng trên xe của bản sửa mới. Xem [báo cáo sửa R2](docs/M8_VC02_ROTATE_R2_CONTINUATION_2026_11_09.md).


**HIỆN HÀNH 2026-11-09 — TẠM DỪNG M9, SỬA VOICE ROTATE TRÊN NỀN M8.**
Theo chỉ thị mới của người vận hành, nhánh đang làm là `feature/m8-r1-voice-center-right-late-fine-10raw`,
phát triển trực tiếp từ mốc tiền đề M9 `d367618d4d9c4c382a9a7208f9b878dfefa21ace`.
Xác minh HEAD của nhánh này; các chỉ dẫn M8-S15 cũ bên dưới là lịch sử.
M9 HOLD tạm dừng, được giữ riêng tại `feature/m9-steering-active-position-hold`
(HEAD đã kiểm tra `3df43edde8f53b5e84561704189a83ac8286a56b`);
MCU cuối cùng có log M9 xác nhận là `b89b4d5c45c6173f5d726b9b1d9f7f755af05fe1`,
chưa được coi là đã flash 3df43ed. Không merge M9 vào nhánh sửa M8.
Mục tiêu hiện tại: Rotate LEFT = R2 → L2, RIGHT = R2 → L1, giữ xoay đủ số giây
sau khi bộ điều khiển bánh chấp nhận khởi động; giữ nguyên NC/STOP/PS2/PID.
Chi tiết, giới hạn bằng chứng và cách cập nhật/test: [M8_VC02_ROTATE_STARTUP_2026_11_09.md](docs/M8_VC02_ROTATE_STARTUP_2026_11_09.md).

**Hiện hành — LEFT300/200 sau báo đứng yên:** **2026-11-09 — LEFT recovery300CCR/<=200ms, SOURCE/HOST chờ MCU:** USER báo ERR và iOVER đều tắt,RUN nhấp nháy,motor có tiếng rít nhưng đĩa lái đứng yên. LED/RUN không đo mô-men; chưa xác định nguyên nhân duy nhất. Ghi chúM8 đã có ca175CCR/30–45ms không dịch cảm biến,assistant đã bỏ sót khi chọn burst đầu. Bản mới dùng LEFT300CCR (trần chạy CENTER hiện có),cửa sổ tối đa200ms,cắt sớm theo goal/damping/STOP/NC/cảm biến và deadlineSysTick. Đây là cấu hình thử do assistant chọn,không phải giới hạn dòng/nhiệt đã nghiệm thu. RIGHT vẫn85CCR/12ms do chưa có powered test. PD nền30/10,4cửa sổ/vòng,cooldown>=3s+budget hồi,không max_error_raw. Host300/200,goal cut sớm,mainblocked225ms và sanitizer kiểm riêng; CI exactSHA trước flash. MCU cuối vẫnb89b4d5. Xem docs/M9_H2B1_LEFT_RECOVERY_WINDOW_2026_11_09.md. Bước tiếp:CI đúng SHA ->Clean Build/flash/F8 ->CENTER,đẩy nhẹ rồi thả trên giá ->STOP/TXT và quan sát chuyển động. Không kích Live Expressions hoặc đổi biến trở driver.

**Kết quả MCU184953:** **2026-11-09 — MCU log184953: trợ lực LEFT đã phát, chưa về center:** 13dòng hợp lệ trênb89b4d5c45c6173f5d726b9b1d9f7f755af05fe1. Seq3 LEFT kick0→4→8→12,peak175CCR,sai số tại các nhóm−89→−83→−78→−78RAW;cuối23053ms state6/reason13/active1,retry3,appliedLms3916. Xác nhận đường software/adapter burst hoạt động và budget4đợt nghỉ có giữ goal; không chứng minh mô-men,thời lượng điện hoặc tự về center. RIGHT sai số<=2RAW,output/kick0,chưa thử lực. Seq4 là CENTER mới,gần0sai số rồi state4/reason1=NEW_OWNER;không dùng làm bằng chứng HOLD tự hồi hoặc STOP2. Cần quan sát cơ khí bên trái nhích/giật theo burst hay đứng yên trước chỉnh tiếp. Docs-only:giữ MCUb89b4d5,không flash/lặp phép thu chỉ để chứng minh kick đã kích;không tự nâng lực từ file.

**Hiện hành — trợ lực sau181422:** **2026-11-09 — HOLD trợ lực thắng ma sát, SOURCE/HOST chờ MCU:** USER yêu cầu thực hiện bước tạo phản lực sau log181422. PD nền vẫn30/10CCR; thêm burst LEFT175CCR/30ms,RIGHT85CCR/12ms từ giá trị tinh chỉnh CENTER hiện có. Chỉ trợ lực khi lệch>=8RAW,PD kéo đúng về goal,vận tốc<=20RAW/s ổn định200ms và có mẫu mới; sau burst PWM0>=200ms/mẫu mới. Tối đa4burst mỗi trục mỗi vòng; reason13 nghỉ hai trục,giữ goal,nghỉ>=3000ms và hồi đủ budget rồi retry. SysTick chỉ cắt burst hết hạn,không gia hạn bởi vòng main; adapter trả số PWM thực đã ghi,kickL/R đếm lần trợ lực. No-progress dùng thời gian output tích lũy qua khoảng0. Giữ STOP/NC/sensor/hành trình và bỏ32RAW; không bật commissioning2. Mức mới là lựa chọn thử nghiệm,không phải giới hạn dòng/nhiệt hoặc motor PASS. Xem docs/M9_H2B1_BREAKAWAY_ASSIST_2026_11_09.md. Bước tiếp là Actions đúng SHA mới,CubeIDE Clean Build/flash/F8,collector UntilStopped và kiểm kickL/R,peakPWM cùng phản ứng cơ khí. Không cần bật Live Expressions.

**Kết quả MCU181422:** **2026-11-09 — MCU log181422: giữ goal toàn hành trình + tự retry đã quan sát, phản lực chưa đạt:** 19 dòng hợp lệ trên32520fe0c86facf59756e6c0071985541f7f42d0. Seq2 giữ active1 dù peakError149RAW, tự retry4 lần tại8913/15602/23247/29948ms,LEFT peak30CCR/applied5166ms,cuối30960ms còn lệch−138RAW; không có RANGE5. Seq3 LEFT applied1001ms rồi STOP state4/reason2/active0/lease0 tại9385ms,còn lệch−120RAW. RIGHT peakPWM/applied0 vì sai số không quá4RAW. Xác nhận giới hạn sai số32 đã bỏ, retry và STOP phía phần mềm; không xác nhận lực/mô-men/return hoặc dòng/nhiệt. Không tăng PWM từ log này. Docs-only: giữ MCU32520fe,không cần flash lại hoặc lặp phép thử cùng30/10 chỉ để xác nhận hai lỗi logic đã sửa. Bước tiếp là đánh giá đường tạo lực/hiệu chỉnh actuator; thiếu lực thắng ma sát vẫn là giả thuyết.

**Hiện hành — bỏ ngưỡng lệch sau175629:** **2026-11-09 — Bỏ giới hạn sai số HOLD32RAW sau log175629 (SOURCE/HOST, chưa flash):** USER yêu cầu bánh bị đẩy lệch trong hành trình hợp lệ phải tiếp tục được kéo về. Đã xóa max_error_raw và hai nhánh hủy goal theo sai số trong Begin/Step, không thay32 bằng một trần lệch khác. Main vẫn kiểm tra góc đo và goal theo hành trình hiệu chuẩn; STOP/NC, sensor stale/jump, service/port và owner mới vẫn hủy goal. Profile30/10CCR và cơ chế nghỉ/retry11/12 không đổi. HOST tái hiện chính LEFT1919/goal1886/error−33 và sai số33..400RAW cả hai dấu/trục, vẫn giữ goal; lỗi ngoài hành trình vẫn ngắt. MCU cuối xác nhận làaf6a43d, lực kéo về thật/dòng/nhiệt chưa đạt. Xem docs/M9_H2B1_FULL_TRAVEL_RECOVERY_2026_11_09.md. Cập nhật này thay thế mọi ghi chú giữ maxError32RAW trước đó. Bước tiếp: kiểm tra CI đúng commit mới, cập nhật Windows bằng fast-forward, Clean Build/flash/F8 rồi thu log mới. Không cần kích Live Expressions.

**Hiện hành — retry 09/11:** **2026-11-09 — HOLD tự thử lại có giới hạn (SOURCE/HOST, CHƯA THỬ TRÊN XE):** USER xác nhận đẩy lệch rồi thả tay nhưng bánh không quay lại, chưa có số đo dòng; yêu cầu luôn cố gắng tạo lại phản lực và không muốn chờ đo dòng. Đã đổi riêng NO_PROGRESS11/EFFORT_BUDGET12 thành COOLDOWN6/active1: giữ nguyên hai goal, đưa cả hai PWM về0, hồi đủ ngân sách output rồi tính lại điều khiển từ góc mới và tự thử tiếp. Không tăng trần30/10CCR, không thêm hoặc tuyên bố có giới hạn dòng. STOP/NC/sensor/range/owner vẫn hủy goal, không tự khởi động lại. Đây là triển khai có giới hạn của yêu cầu; không phải cấp lực vô hạn hoặc đã sửa được phản ứng cơ khí. Xem docs/M9_H2B1_PERSISTENT_RETRY_2026_11_09.md. Bước tiếp: kiểm tra Actions đúng SHA mới, cập nhật project Windows bằng fast-forward, CubeIDE Clean Build/flash/Resume và thu log UntilStopped. Không lặp các phép thử H1/B0 đã đạt.

**2026-11-09 — First MCU powered-tuning register-output/cutoff evidence (NOT HOLD response PASS):** private UART M9H2B1_HOLD_20261109_172834.txt has3 complete rows, SHA621a5ce87385600314404bc8866662c0102f4df4 seq1/source2 CENTER/profile1. WAIT_DATA at37ms -> HOLDING at109ms -> FAULT reason11(NO_PROGRESS) at1868ms, active0/lease0/faultAxes1. LEFT peakCCR30, acknowledged nonzero output1007ms, final/peak error23RAW (goal1886,raw1909); RIGHT peakCCR/output-time0, peak error3RAW. Firmware cut both on its response guard, not a captured Voice STOP. Driver torque, release timing, current and thermal result are not established. No automatic PWM increase; next required observation is whether LEFT was released promptly or still externally held during the no-progress interval. Docs-only update: keep MCU621a5ce, do not reflash or repeat old static logs for this record.

**2026-11-09 — USER-authorized powered H2-B1 tuning:** USER requests actual continuous two-axis HOLD using existing STOP/NC, without a prerequisite motor-supply disconnect test. New profile1=TUNING permits PWM after fresh eligible BOTH-DONE; POWER_COMMISSIONED remains0 and means current/thermal acceptance is pending, not power locked in profile1. Initial engineering caps LEFT30/RIGHT10CCR (3%/1%), independent PD/slew,200ms reversal blank; no-progress1s and recoverable output-time budget cut BOTH axes on tuning fault, with no auto-resume. No fixed HOLD session expiry. Boot has no target/output. Source/HOST verified locally; new CubeIDE build/flash/physical test still pending. See docs/M9_H2B1_POWERED_TUNING_2026_11_09.md. Older closed-power/pending notes below are dated history; do not demand the rejected disconnect pretest or repeat validated H1/H2-B0/unpowered B1 logs.

**2026-11-08 — H2-B1 MCU uncommissioned integration observed:** private UART firmware `4d004757f48f67c1b1b5192b8020c3e8d196ba9a`, four complete lines across seq1/source2 CENTER and seq2/source3 Voice angle. Both retain two goals beyond5s (5002/5001ms), active1/state2/reason7, commissioned0/lease0, all PWM/peakPWM/appliedMs/everApplied0. This is LIMITED MCU target-retention/telemetry PASS, not powered HOLD, current/thermal/load or fault-injection acceptance. Latest DOCX identifies DC775 geared motors: wheel110rpm x4, TURN LEFT30rpm, TURN RIGHT32rpm; manufacturer/winding/gearbox model and safe stationary current are not specified. See docs/M9_H2B1_FULL_HOLD_2026_11_08.md. Earlier pending/819f635 notes below are dated history.

**2026-11-08 — USER clarified FULL H2-B1 (not H1):** delivery is persistent TWO-axis position HOLD, no3s expiry and no single-axis/short-session substitute. Full C controller + atomic HAL owner route + main idle/PS2/Voice/NC handoff are now SOURCE-STAGED with local strict GCC tests120s(controller)/60s(main hooks) PASS. Physical commissioning remains CLOSED0; PWM caps/gains/reversal timings are unverified, so no powered HOLD is enabled or claimed. GitHub CI on the new source and CubeIDE/MCU validation remain separate. See docs/M9_H2B1_FULL_HOLD_2026_11_08.md.

**2026-11-08 — USER-REPORTED motor-lead current baseline:** User measured on wires FROM driver TO motor (not driver input). During CENTER travel LEFT0.8–1A and RIGHT≈2A; after stopping meter reads0A on both. Earlier LEFT5A/RIGHT10A were estimates from discussed motor powers80/200W, NOT verified continuous/stall/HOLD ratings. These readings are a running-current baseline, not safe HOLD limits; meter type/bandwidth and thermal/standstill-current envelope remain unverified. MCU firmware stays819f635; H2-B1 powered output has not been implemented.

**2026-11-08 — H2-B0 MCU STATIC INTEGRATION/TELEMETRY PASS (1 healthy Voice CENTER run):** Private user log `M9H2_GUARD_20261108_151606.txt`, MCU firmware `819f635aafbe9dde3ee0e4b9c5c8e6cbc282e0c2`, seq1/source2. H2-A state6/reason1 and H2-B0 bState3/bReason1 completed 3003ms, 268 guard updates; freshL/R267/268; sensorErr/deniedNC/deniedOwner/blockBits/dropped0; normal VOICE1->IDLE0 accepted. bPeakL/R0 and bEligibleL/R0; bCcrL/R=dutyL/R=0, powered0. This validates one quiet static integration window, NOT nonzero-candidate/disturbance/STOP-NC fault-injection or powered HOLD. H2-B1/H3 remain pending; no firmware changes in this documentation update.

**USER clarification (2026-11-08):** Động học chưa hoàn thành để tích hợp/nạp vào firmware; SELECT tạm hoãn. Không tự đồng nhất hai trạng thái hoặc coi nội suy RAW của Voice là động học bánh xe hoàn chỉnh.

**Điểm dừng hiện hành:** đã nhận và kiểm tra log H2-B0 từ STM32; giữ firmware819f635. Đường cấp lực H2-B1 và tích hợp H3 chưa hoàn thành. Không lặp lượt tĩnh vừa đạt.

### 2026-11-07 — M9-H1 thực hiện yêu cầu USER: đo đường cong M8 thật trên xe

- [CI] M9-H1 REAL source `93551551422559914052ec8b8671a521715714b4` exact Actions `42368389841` **SUCCESS**; gồm Linux GCC actual-C trace host regression + verification guard cấm ghi motor, Windows PowerShell5.1 parser và context contract. Không thay thế CubeIDE/Flash/on-MCU real log.

- [USER] Không muốn bài mô phỏng hay phải đo/chỉnh biến Live Expressions, muốn lấy **dữ liệu phần cứng thật** để hiệu chỉnh bộ PD. Không được thay mục tiêu sang H2 active hoặc ép người dùng đẩy bánh cơ học.
- [IMPLEMENTATION] Thêm `Core/Src/m9_h1_real_trace.c`/`Core/Inc/m9_h1_real_trace.h`, hook trong `Core/Src/main.c` khi bắt đầu/rời owner M8 R1/Voice5–40 và superloop sau cập nhật actual CCR + cached AS5600, không I2C/motor write. Mẫu t=ms, raw L/R, err L/R, filtered vel L/R, candidate u L/R (shadow units), CCR L/R (actual M8 PWM), age, NC/state. 80 frame at minimum80ms. Frame buffer vào RAM, UART3 chỉ sau motor idle.
- [LOGGING] `M9H1_REAL_BEGIN`, `M9H1_REAL_FRAME`, `M9H1_REAL_END` + H0_DRIFT/H1_SHADOW cũ trong một `tools/capture_m9_h1_real.ps1` chạy cho 4 lệnh thực từ Voice/R1. Không thêm lệnh gửi đi/không tự khởi động trục. Chờ REAL_END trước lệnh sau. On-target firmware build/flash/log pending; CI HOST GCC + wiring/no-actuation and PS parser phải xác minh.
- [SCOPE] Dữ liệu khi **M8 đang chạy motor thật**, shadow PD không can thiệp. Đây là dynamic tracking thực nhưng **không chứng minh phản ứng khi motor ngắt và ngoại lực biến mất**. H2 ACTIVE vẫn chưa code/bật.
- [PRIVATE] Chỉ commit mô tả và kết quả tổng hợp; TXT từ xe lưu `logs` local, không đưa lên repository public.

### Cập nhật kiểm chứng thực tế 2026-11-07 — H1 MCU UART 4 cặp, static-only

- Bằng chứng người dùng cấp: 4 M9H0_DRIFT + 4 M9H1_SHADOW có SHA 00ccaeace99a6948880ecf85061dea5d2f77514e; seq1..4 matched, source3 ×3/source2 ×1, duration3000..3010ms, 294–296 mẫu/trục, no invalid/stale/jump/drop.
- H1 noise filter loại 28 lần biến thiên 1RAW; H0 peakVel100 RAW/s trong một số lượt nhưng H1 peakVel0; tất cả peakCmd0 vì |err|≤6 RAW < enter8. Chỉ static capture PASS, **chưa có kiểm chứng PD đáp ứng tải ngoài hoặc điều khiển PWM**.
- Cần HOST dynamic stimulus hoặc controlled passive bench excursion để quan sát ứng xử khi |err|≥8, không tự chuyển H2. Xem đầu file và M9_H1_PD_SHADOW doc. Sau cập nhật MD HEAD docs thay đổi, nhưng MCU log thuộc SHA trước đó.

### TIẾN ĐỘ 2026-11-06 — M9-H1 PD SHADOW (SOURCE + HOST CI PASS; chưa cấp điện giữ góc)

- [CI CONFIRMED] Sau sửa signedness, exact SHA `7d8c98e14a0fd0cc35fc95c1a45d51988d464c2b` GitHub Actions `42237917589` **SUCCESS**. Linux GCC pure-C load-release tests và wiring/no-actuator-write checks PASS, Windows PowerShell 5.1 parser PASS, context contract + M8/H0 regressions PASS; không chứng minh CubeIDE target, flash hay motor.

- [USER REQUEST] Triển khai bước H1 sau khi đã nhận và giải thích 4 log H0 thực (độ trôi đỉnh ≤1RAW, peakVel90RAW/s có thể do 1RAW/11ms).
- [SOURCE] Thêm `Core/Inc/m9_h1_shadow.h` + `Core/Src/m9_h1_shadow.c`, nối vào đúng cửa sổ H0 3s chỉ khi cả hai trục DONE2 của R1/Voice5..40, dùng cache AS5600 hiện hữu, không thêm I2C.
- [CONTROL SHADOW] Sai số RAW 12-bit shortest signed, hysteresis vị trí enter8/exit4RAW; deadband5RAW; tốc độ theo chênh lệch trong ≥100ms và loại thay đổi ≤1RAW; stale>250ms và sample jump>90RAW bị loại, unsigned wrap tick được xét. Mô phỏng PD `Kp_q8=256`, `Kd_q8=128`, cap±40 **SHADOW UNITS** (không phải CCR, không phải mô-men đo, không đưa sang driver).
- [SAFE OUTPUT] `M9H1_SHADOW` xuất một dòng tóm tắt mỗi cửa sổ qua UART3 chỉ khi motor/wheel/serial idle; H0/M8 logs có ưu tiên. `tools/capture_m9_h1_shadow.ps1` chỉ nghe cả H0+H1. H0 history vẫn nguyên, không ghi log riêng tư người dùng lên GitHub.
- [HOST] `tools/verify_m9_h1_shadow.py` biên dịch GCC module C thật và kiểm tra wiring/safety; `tools/m9_h1_host_test.c` giả lập tải đẩy trục lệch, thả tải làm trục lao về, yêu cầu tín hiệu PD shadow **đổi sang hãm trước khi qua góc đích**, kiểm tra jitter/stale/jump/wrap/cap; CI exact SHA/Windows PowerShell phải xác minh sau push.
- [TRẠNG THÁI] H1 code chưa Build/Flash/test thật. H2 HOLD_ACTIVE/H3 vẫn chưa triển khai; **không** tự động chuyển shadow proposal thành PWM. Đọc chi tiết `docs/M9_H1_PD_SHADOW_2026_11_06.md`.

### KẾT QUẢ HIỆN HÀNH — M9-H0 private on-MCU UART 4 cửa sổ (2026-11-06)

- **[USER-PROVIDED MCU UART]** Bốn records M9H0_DRIFT liên tiếp seq1..4, MCU stamp SHA **8c2b3fe7d57154db7d11f8abca40351c4753d362** (trùng SHA GitHub Actions PASS). Cả 4 `reason=1`, duration 3002..3003ms, mỗi trục 268..269 mẫu/cửa sổ, age 1ms, zero bad/jump/zeroDt/drop, wheelsMoved=0. Chỉ lưu **tóm tắt** trong repo, không công khai TXT người dùng.
- `source=3` Voice position ×3 và `source=2` Voice standalone CENTER ×1; **chưa có sample source=1 PS2 R1**. Peak drift mỗi trục 0..1 RAW, max peakErr 5 RAW; đó là ổn định **đọc AS5600 khi không có tải thử được mô tả**, không phải chứng cứ lực giữ hoặc phanh.
- `peakVel` tới 90 RAW/s xảy ra khi 1 RAW thay đổi trong khoảng 11ms; là tốc độ vi phân đỉnh nhạy nhiễu/lượng tử. **Không được dùng trực tiếp cho `Kd` H1**. Phải có lọc + deadband/đánh giá sai số và dt.
- Phân tích SOURCE tại đúng SHA: nhánh kiểm tra an toàn trong main loop gọi `Steering_PositionReadOne(1U)` và `(0U)` mỗi pass khi motion permit1/trip0, ngoài idle poll 50ms xen kẽ. Do vậy ~89 mẫu/s/trục trong 4 records là khả dĩ mà **không cần thêm I2C từ H0**. Không coi đây là bảo đảm control-loop period cố định.
- **Kết luận bằng chứng:** H0 HOST CI PASS + MCU UART DATA CAPTURE PASS (4 cửa sổ tĩnh giới hạn). Chưa có bằng chứng tác động ngoại lực, đo dòng/nhiệt hoặc HOLD/STOP/NC dưới tải; CubeIDE Clean Build/Flash độc lập không xác nhận chỉ từ tự công bố SHA. **NEXT:** H1 shadow PD với velocity filter, không cấp PWM, và regression owner/safety; H2 ACTIVE vẫn chặn.

### TIẾN ĐỘ M9-H0 — source read-only drift monitor (2026-11-06)

- Implemented `Core/Inc/m9_h0_drift.h`, `Core/Src/m9_h0_drift.c` và wiring thụ động trong `Core/Src/main.c`; `tools/verify_m9_h0_drift.py` + GCC host test + PowerShell collector `tools/capture_m9_h0_drift.ps1`.
- Khi cả hai Position axis DONE2 và toàn lệnh result2, chỉ R1 CENTER (PS2/Voice B0) hoặc Voice5..40 position-mode4, source snapshots goals/raw khi cả hai PWM đã OFF; quan sát **3 giây** sau DONE bằng dữ liệu AS5600 cache từ polling idle gốc, không thêm I2C/đổi CCR/DIR.
- Kết thúc cửa sổ (reason1), lệnh mới (reason2), STOP/fault (reason3). Theo dõi drift/peak error/max raw/s, samples/invalid/jump/zero dt, wheel activity và dropped; UART3 IRQ chỉ lúc motor+serial idle. Không giữ khi NC endpoint R2/Voice45.
- Trạng thái H0 tại source commit: **SOURCE-STAGED**, thử GCC unit có thể PASS, CI exact SHA phải kiểm tra GitHub; **CubeIDE Build/Flash/MCU/UART/độ trôi thật CHƯA xác minh**. H1/H2/H3 chưa viết. Xem `docs/M9_H0_PASSIVE_DRIFT_2026_11_06.md`.
- Các mẫu M8 Voice15/30/40 USER PASS trên SHA 9db12228 khác M9 H0. Chưa phát lực giữ/servo PD.

### CHUYỂN MILESTONE 2026-11-06 — M9 Steering Active Position Hold (USER xác nhận, docs-only)

- Branch: `feature/m9-steering-active-position-hold`, fork **exact** `d367618d4d9c4c382a9a7208f9b878dfefa21ace` (M8 RIGHT CENTER trial), branch tạo trước cập nhật docs; M9 commit mới chỉ cập nhật hồ sơ và kế hoạch.
- Người vận hành muốn **chương trình giữ góc bằng mô-men motor DC có sẵn**, không thêm phần cứng. Không phải khóa trục cơ khi mất điện. Tránh bật ngược khi ngoại lực biến mất: dự kiến PD phản hồi vị trí/tốc độ, giới hạn mô-men gián tiếp qua PWM và thời gian, các cổng NC/STOP/sensor/motor-owner.
- Lộ trình `H0 MONITOR (PWM0)` → `H1 SHADOW PD (PWM0)` → `H2 ACTIVE opt-in boot OFF, thử trên gá` → `H3 phối hợp wheel/PS2/Voice`. Hiện **chưa triển khai H0 hoặc HOLD firmware**.
- M8 6/6 Voice15/30/40 của 9db12228 **USER PASS**; trial R1/Voice B0 center10 RAW tại base d367 vẫn **chưa có physical PASS**. Không sửa lịch sử M8 hay nhận lầm kết quả.
- Đọc chi tiết `docs/M9_STEERING_ACTIVE_POSITION_HOLD_PLAN_2026_11_06.md`, `docs/PROJECT_MILESTONES.md` cập nhật 2026-11-06 và toàn bộ hồ sơ 0–16. Hành động tiếp theo: rà soát idle sensor/motor owner, thiết kế H0 read-only, chỉ thay mã khi có nhiệm vụ triển khai. Tách HOST CI, CubeIDE, Flash và hardware.

### S15.3 proposed RIGHT-only ACTIVE pilot, default OFF (2026-11-02)

[USER DATA, S15.2 PASS observation] Additional D-pad LEFT run 1,957ms LEFT vs 2,606ms RIGHT => 649ms later. Other complete runs: D-pad RIGHT 25ms, RX LEFT 625ms, RX RIGHT 133ms. RIGHT 51RAW to SOFT when LEFT stops; S14.2 planner had 89 LEFT-boost, 44 RIGHT-trim, 0 RIGHT-boost requests, not proof of actual PWM modulation. [SOURCE] Staged S15.3 opt-in RIGHT only, +<=12CCR, both distances >180RAW, >=12RAW moved each, S152 sensor validity, no new I2C, preserves LEVEL2 450/200, STOP/NC/soft, S15 mutually exclusive, records M8S153 via S00/PowerShell. [RELEASE] source/HOST checks first; no target CubeIDE clean build/flash/hardware gate proof yet. Do NOT default ON, do NOT activate near RIGHT MIN without current/coast/clearance. [DOC] docs/M8_S153_RIGHT_ONLY_STAGE_2026_11_02.md.


### CHỐT QUY TRÌNH TEST S15.2 — DỮ LIỆU CHỈ QUA POWERSHELL (2026-11-01)

[USER, binding requirement] Người vận hành **KHÔNG dùng app terminal/COM và KHÔNG dùng Live Expressions** để gửi log. Assistant đã sai khi hướng dẫn terminal và Live Expressions; không lặp lại. [DONE SOURCE] `tools/capture_m8_s152.ps1` là PowerShell native, dùng `System.IO.Ports.SerialPort` ST-LINK VCP, lưu 4 bản `M8S152` trong 4 M8S00 frames vào 1 TXT, kiểm `kind/dirL` và final=1/PWM0; không cần app hay thay đổi MCU firmware 762534d. [TEST OPERATOR] One PowerShell command trước 4 lượt; mỗi lượt PS2 START arm, test, START disarm và **USER short press** để MCU xuất UART (gate full disarm); PowerShell auto ghi, không chạy S11 song song. [LIMIT] Chỉ RAM last stroke; xuất trước khi bắt đầu lượt sau. [VALIDATION] Github source/static and Windows parser; physical COM/CubeIDE/HW chưa nghiệm thu. Chi tiết `docs/M8_S152_POWERSHELL_CAPTURE.md`.


### Cập nhật SOURCE S15.2 (2026-11-01, chờ exact SHA CI)

[USER] Máy local đang 17ff3d7; muốn cập nhật một lần sau khi có firmware S15.2, không pull riêng các docs trước. [SOURCE] S15.2 thêm pure helper `steer_s152_manual_relation.h` và post-atomic RAM observer `M8S152_Record` cho D-pad LEFT/RIGHT và RX; 96 rows latest stroke, 50ms, signed calibrated relation proxy, angle/speed prediction from independent AS5600 timestamps, post-disarm M8S152 UART. Không áp PWM shadow hay thêm I2C/active UART. Level2/Fast Start defaults, LEFT owner, RIGHT actual200/proposed300, STOP/NC/SOFT còn nguyên. [TEST] New HOST/static CI script, CubeIDE/Flash/hardware not verified. [NEXT] Check CI exact SHA, then local safe ff-only from 17ff3d7, Clean Build/Flash, independent electrical/NC checks before powered log. S15.3 not started; S15.4 then VC-02; S15.5 deferred.



### CẬP NHẬT PHẠM VI MỚI NHẤT 2026-11-01 — S15 kết thúc chặng tại S15.4; chuyển sang VC-02

[USER] Chỉ triển khai chuỗi S15.2 SHADOW → S15.3 RIGHT-only → **S15.4 R1/R2 route** trong hiện tại. Không chờ Ackermann hoàn chỉnh. **S15.5 TẠM HOÃN** vì động học chưa hoàn chỉnh, **S15.6 tạm hoãn dưới nhãn S15**; regression sáu motor và Gate A/B chuyển sang giai đoạn VC-02 integration, không bỏ nghiệm thu. Bước sau S15.4 là tích hợp lệnh VC-02 với các owner bánh/lái được bảo vệ, không tự suy từ event parser rằng TURN/ANGLE được thực thi.

[SOURCE CHECK] `Voice_FinalizeCommand` và `VC02_M2_ProcessEvent` đã parse/build nhiều loại sự kiện/chuỗi, nhưng `Voice_IsOpenLoopCommandSupported` **chỉ thực thi FORWARD/BACKWARD không góc**, còn `Voice_ExecuteMotorControl` gọi `Turn_Motors_Stop()`. Mốc voice tiếp theo phải phân biệt parser với executor, giữ `STOP > PS2 > Voice > Idle`. ID dự kiến VC02-I1 parser/route audit & HOST, VC02-I2 opt-in executor, VC02-I3 regression/Gate A với Gate B riêng; không có SHA firmware hay acceptance mới. RIGHT proposed 300 vẫn inactive. Tài liệu chuẩn: `docs/M8_S15_MILESTONE_PLAN_2026_11_01.md`.

### CẬP NHẬT ĐỊNH HƯỚNG 2026-11-01 — milestone S15 (ưu tiên nguồn hiện hành)

[USER] Đã làm rõ mục tiêu: manual D-pad/joystick RX cần **giữ quan hệ góc** giữa hai bánh trong thời gian điều khiển; "đồng tốc" trước đó là diễn giải không đúng vì cung đánh lái khác nhau. Giữ **dự đoán sai lệch góc** (ý tưởng P+D S14), **Fast Start mặc định ON**, **Level2 mặc định 2**, **giữ LEFT** không hãm để đồng bộ; tập trung tăng khả năng bám của RIGHT. R1 target CENTER có thể dùng common-T nếu feasible; R2 **chỉ xét common-T khi cả LEFT/RIGHT xuất phát CENTER hợp lệ**; không từ CENTER thì owner vị trí độc lập tới MIN vật lý/NC. Lệnh góc mục tiêu/Ackermann tương lai mới dùng common-T có điều kiện. Chế độ manual không tự quay CENTER khi nhả joystick. [THAY THẾ] Không dùng "hai trục manual phải đồng tốc góc" hay "S15 T chung cho mọi lệnh D-pad" như mục tiêu hiện hành.

[PLAN] Chi tiết trong `docs/M8_S15_MILESTONE_PLAN_2026_11_01.md`: S15.1 telemetry HOST/CI PASS nhưng chưa CubeIDE/Flash/HARDWARE; S15.2 SHADOW bám quan hệ góc cho D-pad/RX là việc kế tiếp; S15.3 RIGHT-only controller; S15.4 routing R1/R2 theo xuất phát; S15.5 target góc bất kỳ/Ackermann; S15.6 regression Gate A, Gate B riêng. Không có gain `tau`, mapping góc thực tế hoặc dung sai sai lệch được nghiệm thu. RIGHT 300CCR proposed ONLY, active 200; giữ cap LEFT450, +12/180/100RAW, STOP/NC/SOFT. Chưa chỉnh firmware ở commit roadmap này.



### HIỆN HÀNH — kết quả thử M8-S15 19:34–19:35 ngày 2026-10-31

- **[GITHUB]** SHA source tại lúc nhận log là `cee4d8bce562d6c77c36dbe0991caab28b4d8d2b`, Actions run `41200962485` PASS; commit hồ sơ sau phân tích `2cd12a2f753e6df4a0a9e4a4d28d95ab628c09fa` có Actions run `41204441709` PASS, gồm contract 386 dòng/17 mục và static/GCC host regression S14.2–S15, Release job SKIP theo chính sách. Nhánh M7-S15 cũ ở `bcf6c2a4e122de91ee8fdb864f5fe7cfd10cca5d` là mốc trước S14.7. Đây là các mốc đã kiểm, không tự nhận là HEAD tương lai; phải đọc ref GitHub lại khi tiếp quản.
- **[USER FILE/LOG]** `M8S11_six_motor_20261031_193425.csv` có 1.071 dòng × 60 cột, board ms 192384–250923 (58,539 s); meta ghi `complete=True`, rows/captured_rows=1071, missed/busy/tx_errors/invalid_lines/sequence_missing đều 0. `M8_S00_20261031_193556.txt` ghi `M8S00_S15,enable=1,started=7,frames=844,skip=7,rejectedT=0,T=1200`; `last=0` do một lệnh khác sau pilot, không phủ nhận các frame S15 trước đó. Đây là bằng chứng **firmware có S15 đã chạy trên MCU**, không truy được ELF hoặc SHA đã Flash chính xác.
- **[PHÂN TÍCH CSV]** Có 7 đoạn D-pad LEFT, 6 đoạn đến SOFT ở cả hai trục (`manual_result_l/r=11`), 1 đoạn nhả sớm. **D-pad LEFT đưa LEFT về L_MAX (SOFT 1454 RAW), RIGHT về R_MIN (SOFT 216 RAW), không đưa LEFT về L_MIN.** Hai đoạn đủ hành trình từ RAW khoảng L2232–2237/R1006–1007: thời gian từ mẫu đầu lệnh tới kết quả SOFT theo CSV lần lượt LEFT/RIGHT **1995/2443 ms** (skew **448 ms**) và **1938/2607 ms** (skew **669 ms**). M8-S06 riêng ghi một lượt cùng hướng `id=11` LEFT **1960 ms**, RIGHT **2588 ms**, skew **628 ms**; chênh vài chục ms với CSV do lấy mẫu khoảng 50–57 ms. Lượt CENTER→MIN `id=15` ghi LEFT **1117 ms**, RIGHT **1766 ms**, skew **649 ms**. Không gộp các hành trình khác chiều/điểm đầu thành A/B có kiểm soát.
- **[KẾT LUẬN]** Bộ lập kế hoạch chung đã chạy, nhưng **hai trục chưa hoàn thành đồng thời** trong những lượt trên. Ở hai lượt D-pad LEFT đủ hành trình, RIGHT chỉ sau LEFT **114/57 ms** tại mốc còn 180 RAW, rồi **448/669 ms** tại SOFT; đoạn 180→SOFT của LEFT **570/513 ms**, RIGHT **904/1125 ms** theo mẫu CSV. 100–180 RAW, RIGHT CCR đúng 100; 0–40 RAW, trung bình khoảng 58/56,6 CCR. S15 cố ý không boost trong 180 RAW cuối, nên đây là điểm tập trung chẩn đoán, **không phải quyền tăng PWM gần NC**. `T=1200` là bản ghi session S15 cuối, không phải T riêng của mọi lượt; CSV không có `phase/ref/error` S15 từng hàng. Không chứng minh hai góc target bất kỳ (ví dụ 35°/30°) đã được route tới actuator; phần đó mới có HOST test. NC=4/6 chỉ xuất hiện trong R2, không có `trip` hay `stop` trong CSV; M8-S00 `FAULT` rỗng. **[USER-REPORTED, bổ sung sau phân tích]** khoảng hở R_MIN→NC còn **3 mm** trong bài thử được báo cáo; không có bằng chứng va NC trong các lượt manual này. Chưa có đo dòng, quãng trôi sau CCR0, nguồn ngắt độc lập, CubeIDE Clean Build hoặc SHA flash cho bản này. Bốn bánh không chạy đồng thời trong 7 đoạn D-pad LEFT, Gate A/B vẫn mở.
- **[ĐIỂM DỪNG]** Giữ firmware và các giới hạn hiện tại; phân tích nguyên nhân chênh thời điểm bằng `M8S00_S15` thiếu lịch sử T/ref theo từng stroke và tốc độ/CCR/RAW đã có. Không yêu cầu thử thêm motor hoặc thêm lượt chạy từ thông tin này. Chỉ cần đối chiếu bằng chứng Build/Flash hay điều kiện điện/cơ khi bước thao tác thực sự phụ thuộc; không yêu cầu lặp PI OFF. Cập nhật `docs/M8_S15_DUAL_TARGET_COMMON_TIME.md` và roadmap, không đăng log thô.
- **[ĐỀ XUẤT 2026-11-01; CHƯA CODE]** Người dùng muốn giữ đáp ứng LEFT và xem xét hai encoder motor lái làm phản hồi tốc độ. Source S15 đã dùng cùng mốc 180/100 RAW cho hai bên và tốc độ AS5600; encoder TIM4/LPTIM1 hiện chỉ ghi chẩn đoán >=50 ms, `cpr=0` không ngăn dùng counts/s nhưng chưa có tỷ số counts/góc vùng cuối. Hai lượt D-pad LEFT có khoảng 75%/91% skew phát sinh sau 180 RAW, nơi RIGHT owner <=100 CCR rồi giảm về 50, LEFT <=300 rồi về 175. Hướng đề xuất: ghi T/ref/góc/tốc độ AS5600 + encoder có dấu/owner/cap/applied theo stroke, xác nhận tỷ số và độ mới; giữ LEFT, dùng tốc độ encoder hỗ trợ RIGHT bắt kịp trong trần hiện tại trước 180 RAW. T dài hơn một mình không làm RIGHT nhanh hơn. Nếu vẫn không đạt, báo không khả thi dưới envelope hiện hành và chỉ bàn profile terminal RIGHT như quyết định riêng. Không đổi firmware, không coi bản đề xuất là Hardware PASS.

### M8-S15 — một thời gian hoàn thành cho hai góc TARGET độc lập (source pilot 2026-10-31)

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

**Cập nhật bàn giao 11/11/2026.** Mục tiêu vẫn giữ góc hai trục chống ngoại lực và ổn định khi bỏ tải. Nhiệm vụ hiện tại là ghi kết quả, báo cáo và chuyển ngữ cảnh hai phiên lên GitHub/Markdown. Chưa có yêu cầu mới sửa thuật toán hoặc mở lại động học/SELECT.


**Hiện hành — LEFT300/200 sau báo đứng yên:** Mục tiêu vẫn giữ hai goal liên tục;chỉnh LEFT là phản hồi bằng chứng thử hiện có,không đổi sản phẩm thành chương trình một trục. Sai số lớn hợp lệ vẫn yêu cầu kéo về.

**Hiện hành — trợ lực sau181422:** Giữ hai goal liên tục sau BOTH-DONE; trợ lực ngắn nằm trong owner HOLD hiện hữu,không thay bằng chương trình open-loop một trục hoặc phiên tự hết hạn.

**Hiện hành — bỏ ngưỡng lệch sau175629:** HOLD hai trục giữ góc đích khi nhiễu lớn trong hành trình hợp lệ. Sai số vị trí là đầu vào hiệu chỉnh, không tự biến thành lỗi hủy mục tiêu.

**Hiện hành — retry 09/11:** Mục tiêu vẫn là HOLD hai trục liên tục sau BOTH-DONE. Sau nhiễu, bộ điều khiển tiếp tục cố gắng trong phong bì thử hiện có; khả năng tự quay về thật của cơ cấu chưa xác minh.

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

**Cập nhật bàn giao 11/11/2026.** Người dùng cho phép cập nhật cải tiến, kết quả và nội dung báo cáo lên GitHub, xuất MD đầy đủ để xoá phiên Công việc và chuyển dự án sang Chỉ dự án. Giữ mốc firmware, mức chứng cứ và lịch sử, tận dụng tài nguyên có sẵn trước khi hỏi lại.


[CONTINUOUS HOLD CORRECTION] Yêu cầu xuyên suốt là giữ trục chống ngoại lực. Không được thay yêu cầu bằng hồi vị sau lệch rồi nghỉ. Chưa có lực/nhiệt thực nghiệm không phải lý do chọn sai chức năng; giữ ranh giới logic và khả năng vật lý.


[CHỈ THỊ MỚI NHẤT] Người dùng cần một bản final để hoàn thành dự án/báo cáo, không còn thời gian thử mẫu. Thực hiện mọi phần code/kiểm tra tự động/tài liệu trong một lượt. Không đổi yêu cầu này thành bắt đo dòng/nhiệt trước khi giao; đồng thời không tự bịa kết quả nghiệm thu thực tế.


**Hiện hành — LEFT300/200 sau báo đứng yên:** Tiếp tục yêu cầu sửa lực đã có của USER,không xin lại quyền cùng phạm vi. Giới hạn300/200 là lựa chọn thử nghiệm của assistant,không ghi USER đã chọn hoặc nghiệm thu các số này. Không hỏi lại dòng đo chưa có.

**Hiện hành — trợ lực sau181422:** USER yêu cầu triển khai bước tạo phản lực,không muốn chờ số đo dòng. Assistant chọn cấu hình burst và giới hạn thử; không ghi các con số này thành thông số đã được USER nghiệm thu. STOP/NC/sensor/hành trình vẫn giữ.

**Hiện hành — bỏ ngưỡng lệch sau175629:** USER đã chỉ rõ ngưỡng32RAW không thực tế và yêu cầu sửa. Được phép xóa ngưỡng lệch quanh goal. Giữ STOP/NC, hành trình thực, kiểm tra cảm biến và các giới hạn output đang có; không yêu cầu đo dòng để thực hiện sửa logic này.

**Hiện hành — retry 09/11:** USER yêu cầu thực hiện luôn, chưa có số đo, không cần giới hạn dòng. Phần được thực hiện là giữ goal và tự retry có nghỉ; assistant không triển khai lực vô hạn hoặc bỏ STOP/NC/sensor/range. Trần PWM và ngân sách output vẫn giữ; đây là giới hạn triển khai được báo rõ, không ghi thành yêu cầu bỏ giới hạn đã được nghiệm thu.

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

**Cập nhật bàn giao 11/11/2026.** M9 H2-B1/H3 đã có source/HOST/ARM CI tại b1c1e9c4. Ngày 11/11 bổ sung USER-REPORTED trên xe về chống lực và hết dò qua lại nhiều lần. Phạm vi mọi góc/tải, lực/nhiệt/độ chính xác định lượng và nghiệm thu tổng thể vẫn chưa có dữ liệu đầy đủ.


[CONTINUOUS HOLD CORRECTION] M9 profile4 thực hiện giữ tải liên tục bằng PI+D; H2-B1/H3 source+HOST có bằng chứng, HARDWARE chưa nghiệm thu. Bản profile3 trước bị xác định thiếu chức năng chủ chốt.


[M9 FINAL] H2-B1 controller và H3 quyền/Rotate tích hợp đã triển khai profile3;40 kiểm tra local PASS. Giữ cột SOURCE/HOST riêng với ARM CI và HARDWARE. Không đánh dấu toàn bộ dự án hoặc động học/SELECT đã hoàn tất.


**Hiện hành — LEFT300/200 sau báo đứng yên:** M9 H2-B1 LEFT recovery window SOURCE/HOST,exactSHA CI chờpush;motor return/dòng/nhiệt chưa nghiệm thu. RIGHT powered,H3 tải vẫn chưa thử.

**Kết quả MCU184953:** M9 H2-B1 MCU burst/adapter LEFT đã quan sát12lần và4burst/vòng;goal retention/cooldown tiếp tục. Return cơ khí chưa đạt,RIGHT powered response chưa thử,dòng/nhiệt/H3 chưa nghiệm thu.

**Hiện hành — trợ lực sau181422:** M9 H2-B1 stiction assist SOURCE/HOST đang kiểm CI; STM32 build/flash/return thật,dòng/nhiệt và H3 tải chưa xác minh. Log32520fe vẫn chứng minh giữ goal/retry/STOP của bản cũ,không chứng minh burst mới.

**Kết quả MCU181422:** M9 H2-B1 MCU goal-retention vượt32RAW và auto-retry đã quan sát trongseq2; STOP revoke quan sátseq3. Giữ góc cơ khí/return chưa đạt; RIGHT chưa được kích lực,không nghiệm thu hai trục/H3.

**Hiện hành — bỏ ngưỡng lệch sau175629:** M9 H2-B1 full-travel goal retention: SOURCE/HOST đã thực hiện, CI đúng SHA xác minh sau push. CubeIDE/flash/HARDWARE phản lực/return và H3 có tải chưa xác minh.

**Hiện hành — retry 09/11:** M9 H2-B1 persistent retry: SOURCE-STAGED + HOST native-C; exact-SHA CI kiểm tra sau push. CubeIDE/FLASH/khả năng kéo lại center/dòng/nhiệt/H3 phối hợp có tải CHƯA KIỂM CHỨNG.

**Bổ sung hiện hành 2026-11-01 (không thay thế lịch sử bên dưới):** S15.1 source+HOST CI pass/chưa có CubeIDE Clean Build, flash SHA, hardware pass; S15.2 manual relation SHADOW chưa bắt đầu (ưu tiên); S15.3 right-only tracking chưa bắt đầu; S15.4 R1/R2 mode routing chưa bắt đầu; S15.5 arbitrary-angle/Ackermann chưa bắt đầu; S15.6 Gate A hồi quy + Gate B tách biệt chưa bắt đầu. Tiêu chí/điều kiện chuyển mốc và quyết định [USER] xem `docs/M8_S15_MILESTONE_PLAN_2026_11_01.md`. Không có hạn/chỉ tiêu sai số được chốt.



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

**Cập nhật bàn giao 11/11/2026.** Đã kiểm tra GitHub HEAD b1c1e9c4 và run42759998485 có ba job kiểm tra/build thành công. Kết quả người vận hành được bổ sung vào hồ sơ, báo cáo profile4 và bản chuyển chat. Không phát sinh kết quả đo số mới từ xác nhận này.


[RELEASE SETTLING 2026-11-10] Thêm216 dynamic-release fixtures có lag/ma sát, kiểm transient settling và crossing, baseline profile3 thất bại166/216 so với0/216 bản sửa. Thêm slow-motion Q8, jitter và wrong-I-unwind tests; giữ exact-goal I600s.


[CONTINUOUS HOLD CORRECTION] Đã thêm I giới hạn/anti-windup, giữ I khi e=0, không timer nghỉ/no-motion trong profile4; STOP/NC/new owner xóa I;36 plant case và600s exact-goal C test, actual HAL60s. Giữ41 hồi quy.


[DELIVERED CODE] Thêm recovery_axis và cấu hình final; giữ bù ma sát xuyên qua tiến triển; nghỉ/stall riêng; sample20ms/trục danh định; deadline trên mọi PWM qua SysTick. Thêm162 mô phỏng C/UBSan, test actual main/port/SysTick/poll và workflow ARM full compile/link; giữ toàn bộ hồi quy cũ bằng fixture legacy riêng.


**Hiện hành — LEFT300/200 sau báo đứng yên:** Chỉ đổi cap/cửa sổ trợ lực LEFT175/30 ->300/200 và cập nhật kiểm thử. Cùng controller/cutter hiện có. Test xác nhận end sớm bằng mẫu goal mới,deadline1ms,blocked main225ms và bác bỏ301CCR/201ms.

**Kết quả MCU184953:** Seq3 peak175,kick12,applied3916ms tổngoutput,peakerror104,cuốierror−78. Nhóm đầu4kicks error−83,8kicks−78,12kicks vẫn−78. Không suy áp dụng3916ms đều là output175.

**Hiện hành — trợ lực sau181422:** Đã thêm state assist từng trục,cửa sổ đứng yên,mẫu mới,pulse/settle4đợt và cooldown13. SysTick handler thật gọi cutter chỉPWM0; HAL port trả output thực,không tự rearm khi deadline đã hết. Thêm kickL/R và tests core/main/ISR.

**Kết quả MCU181422:** Seq2 peak149RAW,active1 suốt15dòng,4retry,LEFT5166msoutput;seq3 STOP2 sau1001msoutput. Không có RANGE5. Mức PWM30CCR lặp lại vẫn không có hồi về mục tiêu trong các dòng cuối.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Đã xóa field max_error_raw, cập nhật cấu hình/fixtures, bỏ kiểm sai số trong Begin và Step. PreOwnerService phân biệt ngoài hành trình=RANGE5 với lỗi cảm biến=SENSOR4. Native-C/main/HAL và sanitizer kiểm riêng.

**Hiện hành — retry 09/11:** Đã thêm COOLDOWN6, giữ goal/seq/source, hồi đầy hai ngân sách rồi retry từ AS5600 mới, thống kê retryCount và collector hiểu trạng thái nghỉ. HOST gồm 300s nhiễu không đổi, hồi ngân sách/tick-wrap, đổi chiều trong nghỉ, STOP/NC/owner/sensor/range và main/HAL thật; không phải số đo cơ khí.

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

**Cập nhật bàn giao 11/11/2026.** Điểm dừng chuyển sang hoàn thiện hồ sơ/báo cáo và chuyển chat. Khả năng chống lực, giảm hunting đã được người dùng xác nhận định tính trong lần thử. Lần báo chưa có SHA UART, góc thử, dữ liệu riêng từng trục hoặc đo thời gian/lực/nhiệt. Commit tài liệu không đòi flash lại.


[RELEASE SETTLING 2026-11-10] Đang hoàn thiện source+hồ sơ theo phản hồi hunting; việc khắc phục trên xe chưa được xác minh. Không có SHA/log đi kèm phản hồi mới, không tự gán phần cứng đang dùng profile4.


[CONTINUOUS HOLD CORRECTION] Điểm dừng: hoàn thiện source servo liên tục và kiểm CI/ARM đúng commit. Phần còn chưa biết là phạm vi ngoại lực/mô-men/nhiệt trên xe, không phải một kế hoạch test mẫu do người dùng phải thực hiện trước khi nhận source.


[ĐIỂM DỪNG FINAL] Không có vòng thử mẫu kế tiếp được giao cho người dùng. Sau CI hoàn tất, giao đúng commit và hồ sơ final. Dữ liệu cơ khí của profile3 chưa có, không chặn việc giao phần mềm nhưng chặn kết luận hardware PASS.


**Hiện hành — LEFT300/200 sau báo đứng yên:** Điểm dừng:chuẩn bị build/flash candidate300/200 sauCI. MCU cuốib89b4d5 là bản175/30;không gán log/quan sát cũ cho config mới. Không kết luận motor đủ lực trước test.

**Kết quả MCU184953:** Điểm dừng sau flashb89b4d5: assist thực sự được adapter ghi nhận nhưng chưa kéo về. Giữ source;cần một quan sát cơ khí nhích/giật hay đứng yên để chọn kiểm actuator hoặc hiệu chỉnh xung;không hỏi lại số đo chưa có.

**Hiện hành — trợ lực sau181422:** Chờ CI source mới và người dùng flash. Trợ lực không phát lúc boot hoặc trong vùng yên. Log phần cứng cuối vẫn32520fe,không gán burst mới cho file181422. Cần quan sát góc thật sau burst mới.

**Kết quả MCU181422:** Điểm dừng mới: đã flash và chạy32520fe. Không sửa tiếp ngưỡng lệch hoặc yêu cầu bật Live Expressions. Vấn đề còn lại là tạo phản lực/cơ khí return; cap30/10 giữ nguyên,chưa chọn mức mới.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Sau sửa32RAW: chờ CI commit mới rồi target build/flash. MCUaf6a43d vẫn là bản cũ có ngưỡng32, không coi log175629 là thử phần mềm sửa mới. Không hứa lực30CCR đã đủ.

**Hiện hành — retry 09/11:** Điểm tiếp theo là xác minh CI của commit retry rồi build/flash source mới. MCU gần nhất được log vẫn621a5ce; code mới chưa có UART. 30CCR bên trái từng không sửa được sai số; retry không tự làm tăng mô-men, không hứa motor sẽ kéo về ở cùng cap.

**Điểm dừng sau log172834:** MCU621a5ce đã chạy profile1, LEFT output1007ms peak30CCR rồi FAULT11/active0/lease0; RIGHT chưa phát lực. Không phải STOP terminal hoặc HOLD response PASS. Cần biết có nhả tay ngay hay tiếp tục giữ LEFT trong khoảng1s trước khi quyết định đổi cap/gain/guard. Không tự nâng PWM; không reflash cho docs-only record này.

**Điểm dừng mới nhất (2026-11-09):** đã triển khai profile1=TUNING có PWM sau BOTH-DONE, trần30/10CCR và guard không tiến triển/ngân sách output; POWER_COMMISSIONED0 không phải khóa power ở mode1. Local actual-C/production main/formatter/sanitizer và các hồi quy PASS; xác minh GitHub exact-SHA CI trước CubeIDE Clean Build/flash/test thật. USER đã yêu cầu bỏ phép thử ngắt nguồn làm điều kiện tiên quyết; dùng STOP/NC hiện có, không đổi điều đó thành bằng chứng phần cứng. Chưa có log powered của source mới. Không lặp các log đã đạt.

**Điểm dừng mới nhất (log165103):** H2-B1 SOURCE/HOST CI PASS và MCU giữ mục tiêu/telemetry chưa commissioning đã quan sát hai phiên trên5s tại4d004757. UART chứng minh firmware này đang thực thi; chưa có transcript Clean Build/flash riêng. Lực HOLD vẫn khóa0, commissioning/gain/PWM/đảo chiều/nhiệt và H3 có tải chưa nghiệm thu. DC77530/32rpm mới xác định họ motor/tốc độ, chưa xác định dòng đứng giữ. Không lặp H1/H2-B0 hoặc hai lượt tĩnh vừa xác nhận.

**Cập nhật ưu tiên 2026-11-08:** H2-B0 đã có một cửa sổ MCU tĩnh 3003ms đạt tiêu chí; build/flash thực tế được suy từ firmware chạy và UART SHA, chưa có transcript CubeIDE riêng. H2-B1 powered và H3 integration là phần còn dở; các ưu tiên S14 phía dưới là lịch sử. Không thay firmware hoặc yêu cầu flash lại cho bản ghi tài liệu này.

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

**Cập nhật bàn giao 11/11/2026.** Tệp tiếp quản hai phiên là docs/M9_SESSION_HANDOFF_2026_11_11.md, gồm17 mục và nội dung báo cáo. Giữ link CI42759998485 và mốc source b1c1e9c4. Chat gốc đã xoá, bản Markdown cùng GitHub là nguồn độc lập. Raw UART cũ chỉ được dẫn theo số tổng hợp.


[TỆP FINAL] docs/M9_FINAL_DELIVERY_2026_11_09.md là báo cáo/bảng thông số/hướng dẫn hiện hành. tools/build_firmware_ci.py tạo ELF/HEX/BIN/map và BUILD_MANIFEST.json ở Actions artifact khớp SHA, không tạo GitHub Release tự động.


**Hiện hành — LEFT300/200 sau báo đứng yên:** Core/Inc/m9_h2b1_hold.h và commissioning.h;tests/verifierH2B1;report LEFT_RECOVERY_WINDOW. Không sửa wiring/driver pot,không publish TXT gốc.

**Kết quả MCU184953:** Thêm TXT riêng tư M9H2B1_HOLD_20261109_184953.txt;chỉ tổng hợp trong báo cáo BREAKAWAY_ASSIST/handoff. Không công bốraw.

**Hiện hành — trợ lực sau181422:** Tệp chính: m9_h2b1_hold.h/.c,commissioning.h,main.inc,main.c,main.h,stm32f7xx_it.c;fixtures/verifierH2B1; báo cáo BREAKAWAY_ASSIST. Raw logs giữ riêng tư.

**Kết quả MCU181422:** Thêm tài nguyên riêng tư M9H2B1_HOLD_20261109_181422.txt,19dòng/2seq; chỉ aggregate trong báo cáo full-travel,không upload TXT lên publicrepo.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Source chính Core/Inc/m9_h2b1_hold.h, m9_h2b1_commissioning.h, Core/Src/m9_h2b1_hold.c và m9_h2b1_main.inc; tests/verifier H2B1; báo cáo docs/M9_H2B1_FULL_TRAVEL_RECOVERY_2026_11_09.md. TXT175629 vẫn riêng tư.

**Hiện hành — retry 09/11:** Tệp chính: Core/Inc/m9_h2b1_hold.h, Core/Inc/m9_h2b1_commissioning.h, Core/Src/m9_h2b1_hold.c, Core/Src/main.c, tools/m9_h2b1_*host_test.c, tools/verify_m9_h2b1_hold.py, tools/capture_m9_h2b1_hold.ps1 và docs/M9_H2B1_PERSISTENT_RETRY_2026_11_09.md. Raw TXT vẫn riêng tư, không đăng public.

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

**Cập nhật bàn giao 11/11/2026.** Chốt dùng b1c1e9c4/profile4 làm mốc source sau sửa hunting, giữ hai fix Rotate. Cập nhật USER-REPORTED11/11 thay thế trạng thái chưa có phản hồi xe trong phần lịch sử10/11. Kết quả này chưa xác định miền tải và nhiệt. Mục tiêu bộ nhớ Chỉ dự án chưa hoàn tất.


[QUYẾT ĐỊNH FINAL] Bỏ profile thử30/10 + xung làm mặc định; chọn dải vận động M8 làm cơ sở phục hồi, bù ma sát trong cùng trần, không tăng vô hạn. Đây là quyết định kỹ thuật trong phạm vi yêu cầu final; không gọi thông số là rating dòng/nhiệt đã đo.


**Hiện hành — LEFT300/200 sau báo đứng yên:** Bỏ cách lặp xung175/30 sau khi USER báo đứng yên. Dựa trần chạy CENTER trái300,thêm cửa sổ thử200ms có phản hồi;không áp300 liên tục,không bỏ guard. Assistant thừa nhận đã bỏ sót ghi chúM8 175/30–45 yếu.

**Hiện hành — trợ lực sau181422:** Thay quyết định không nâng output trong các bản chẩn đoán bằng hai mức burst hữu hạn do USER yêu cầu triển khai tạo lực. Không đổi continuousPD thành floor175/85 và không bỏ giới hạn output. Phong bì mới chưa nghiệm thu vật lý.

**Kết quả MCU181422:** Không tự coi log PWM là bằng chứng lực. Các thay đổi32RAW/retry được xác nhận ở MCU phạm vi này; không tự tăng cap hoặc bỏ cooldown từ file gửi lên.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Quyết định mới thay thế việc giới hạn sai số quanh goal ở32RAW. Không nâng trần thành64/128 hoặc một giá trị khác. Điều kiện biên cơ khí và chất lượng cảm biến độc lập vẫn giữ.

**Hiện hành — retry 09/11:** Thay thế quyết định hủy goal vĩnh viễn chỉ cho hai sự kiện tuning11/12: giờ nghỉ/retry. Giữ nguyên hủy goal không tự hồi phục cho STOP/NC/sensor/range/owner/service/port. Không tăng cap/gain/minPWM từ yêu cầu chưa có đo dòng.

**[USER, quyết định mới 2026-11-01]** Giữ thuật toán dự báo sai lệch góc, S14.3 Fast Start và Level2 boot. **Đính chính yêu cầu cũ:** không ép manual LEFT/RIGHT cùng số độ/giây; cung khác nhau có thể tạo góc thực tế lệch lớn. Manual cần giữ quan hệ góc mong muốn; thuật toán phải hỗ trợ RIGHT bám, không hãm LEFT. T chung giữ cho góc target biết trước, R2 chỉ xét từ CENTER của cả hai; ngoài CENTER không ép T. **[ĐỀ XUẤT, chưa nghiệm thu]** SHADOW `e=theta_R_ref-theta_R`, `e_pred=e+tau*(omega_R_ref-omega_R)`, mapping `f` giữa hai trục/geometry và `tau` chưa hiệu chuẩn. Xem milestone plan.



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

**Cập nhật bàn giao 11/11/2026.** Các lỗi32RAW hủy goal, PWM/xung yếu, R2 hủy do gate và profile3 có nghỉ được giữ trong dòng thời gian. Hiện tượng dò qua lại nhiều lần được người dùng báo đã hết trong lần thử mới. Tách điều quan sát khỏi giả thuyết nguyên nhân và số mô hình.


[RELEASE SETTLING 2026-11-10] Sai sót kiểm thử trước: xét steady-state sau chờ lâu bỏ lọt hunting; tốc độ bỏ qua1RAW/40ms che chuyển động chậm. Sửa tiêu chí kiểm tra theo thời gian quá độ và số lần vượt góc.


[CONTINUOUS HOLD CORRECTION] Sai sót đã thừa nhận: c9fd47fd chỉ là recovery có nghỉ; lời chốt hoàn thiện M9 quá mức. Zero velocity dưới tải có thể là HOLD thành công; không dùng no-progress để ngắt lực servo. Test mới kiểm đúng điều này.


[BÀI HỌC] Bù ma sát bị đặt lại sau mỗi nhích2RAW tái tạo stop/start; mô phỏng đã phát hiện, bản chốt giữ bù tới khi đạt goal. Poll100ms/trục của idle không phù hợp dùng nguyên trạng cho điều khiển có lực nên HOLD có lịch10ms luân phiên riêng.


**Hiện hành — LEFT300/200 sau báo đứng yên:** RUN nhấp nháy không chứng minh duty/đủ lực,ERR/iOVER tắt chỉ là trạng thái quan sát. Motor rít phù hợp có đáp ứng điện nhưng chưa đủ cơ lực theo USER;ACCE/ramp hoặc stiction vẫn chưa xác nhận riêng.

**Kết quả MCU184953:** Các mẫu góc đo tiến gần goal11RAW qua hai nhóm,nhóm ba không cải thiện. Chưa biết tác động tay/cơ khí trong từng nhóm;không gán toàn bộ dịch chuyển cho motor hoặc chốt nguyên nhân thiếu lực duy nhất.

**Hiện hành — trợ lực sau181422:** Log181422 cho thấy cố gắng30CCR nhiều lần nhưng lỗi−138RAW vẫn còn. Stiction là giả thuyết; burst mới thử khắc phục cơ cấu chưa nhúc nhích,không tuyên bố đây là nguyên nhân duy nhất hoặc đã giải quyết.

**Kết quả MCU181422:** Các lần retry1,3,4 có sai số đầu/cuối lần lượt−86/−86,−138/−138,−138/−138RAW. Phần mềm cố gắng nhưng chưa sửa được góc; không gán mọi thay đổi RAW trong phiên cho motor vì lực ngoài/thời điểm nhả không được ghi.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Logaf6a43d: HOLD3/active1,LEFT peak30/output773ms,goal1886/raw1919/e−33 rồi FAULT5/reason5. Đã xác định nhánh lỗi phần mềm max_error_raw gây hủy; chưa xác định nguyên nhân motor thiếu phản lực.

**Hiện hành — retry 09/11:** USER đã xác nhận đẩy lệch rồi thả tay; không hỏi lại câu này. Log621a5ce cho thấy peak30CCR,1007ms output,NO_PROGRESS11. Thiếu lực thắng ma sát vẫn là giả thuyết; chưa có điện áp/dòng/mô-men hoặc trace dày chứng minh nguyên nhân duy nhất.

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

**Cập nhật bàn giao 11/11/2026.** Thiếu log SHA mới và điều kiện thử11/11 nên nhận xét gắn với bối cảnh bản bàn giao b1c1e9c4. Chưa có force/current/thermal/settling thực nghiệm. Không dùng5090ms hoặc3lần vượt của fixture làm số đo xe. Các file đính kèm sau xoá chat có thể cần tìm lại.


[RELEASE SETTLING 2026-11-10] Driver có ACCE/DECCE theo manual nhưng thiết lập thực chưa biết; lag0/60/120ms trong fixture là giả lập. Không tự tăng PWM hoặc xoay biến trở; không chuyển số5090ms mô hình thành thời gian đáp ứng thực.


[CONTINUOUS HOLD CORRECTION] Servo liên tục có thể duy trì PWM ở trạng thái đứng yên. Không có phản hồi dòng/nhiệt; chưa xác nhận tải giữ được hoặc mức nhiệt. Không cam kết khóa cứng tuyệt đối hoặc mọi ngoại lực.


[GIỚI HẠN FINAL] Chưa có torque/current/thermal hoặc thời gian đáp ứng trên xe của profile3. SysTick lease không hoạt động nếu SysTick/CPU bị halt. Không suy mô phỏng hoặc ARM build thành nghiệm thu vật lý. Giữ nguyên STOP/NC/sensor/owner và dải cơ khí.


**Hiện hành — LEFT300/200 sau báo đứng yên:** 300CCR/200ms không phải giới hạn motor an toàn đã đo. SysTick cắt lệnhPWM khi IRQ/tick hoạt động,không xác nhận driver ngừng dòng ngay (ACCE/DECCE chưa biết). Giữ STOP/NC/hành trình,không haltCPU lúc cấp lực.

**Hiện hành — trợ lực sau181422:** SysTick cắt khi nhịp tick/IRQ còn chạy; không phải ngắt nguồn hoặc bảo vệ dòng/nhiệt độc lập và không bảo đảm khi halt CPU/mask IRQ kéo dài. Giữ hướng dẫn không Pause/step lúc cấp lực.

**Kết quả MCU181422:** Dòng/nhiệt/mô-men vẫn chưa đo. repeated30CCR không chứng minh giới hạn điện/cơ an toàn hoặc đủ lực. Không tự nâng PWM liên tục từ thông số CENTER.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Bỏ ngưỡng sai số không được bỏ hành trình: cached raw và goal vẫn qua Steering_SafetyHardEnvelopeValid, NC có ưu tiên. PWM/gain không đổi; dòng/nhiệt chưa biết và ngân sách thời gian không phải giới hạn dòng.

**Hiện hành — retry 09/11:** Thử lại định kỳ vẫn có nguy cơ nóng/kẹt cơ khí; ngân sách thời gian không phải dòng/I2t/nhiệt. Không tuyên bố việc giữ cap làm phần cứng an toàn đã nghiệm thu. STOP/NC/cảm biến/hard envelope vẫn có ưu tiên cao hơn retry.

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

**Cập nhật bàn giao 11/11/2026.** Kiểm HEAD/CI của commit tài liệu, nhận file chuyển chat, dùng phần báo cáo đã soạn. Khi chuyển Chỉ dự án, ảnh UI cho thấy chat Công việc là điều kiện chặn. Người dùng chọn bảo toàn ngữ cảnh trước khi xoá phiên này. Chưa giao vòng thử mẫu hoặc công việc firmware kế tiếp.


[RELEASE SETTLING 2026-11-10] Sau CI/ARM PASS giao code M9 trên nhánh hiện tại, tài liệu continuous-hold và release-settling. Không yêu cầu thêm vòng thử mẫu trước khi bàn giao.


[CONTINUOUS HOLD CORRECTION] Giữ quyền STOP/NC/sensor và Rotate. Xác minh CI/ARM, giao bản servo4 và báo cáo hiện hành. Không yêu cầu thêm vòng thử mẫu, không tuyên bố đã flash Windows.


[TIẾP TỤC] Xác minh ref/CI commit final, đưa hướng dẫn fast-forward trên project Windows chính rồi CubeIDE Refresh/Clean/Build/Flash/F8. Không reset, stash hoặc tạo worktree Windows. Không triển khai vòng thử mẫu mới trái yêu cầu; dữ liệu vận hành chỉ thu nếu người dùng muốn bổ sung.


**Hiện hành — LEFT300/200 sau báo đứng yên:** CI đúng commit ->git fetch/merge --ff-only,D:\STM32 task\control_motor ->Clean Build/flash/F8 ->collector UntilStopped ->CENTER,đẩy nhẹ/thả LEFT trên giá,quan sát dịch chuyển/rung ->STOP ->TXT. Không lặp H1/B0/kiểm kích hoạtLive.

**Kết quả MCU184953:** Không cần nạp lại cho cập nhật tài liệu. Trước hiệu chỉnh tiếp,lấy quan sát người dùng về nhích/giật theo đợt hoặc đứng yên hoàn toàn;không mặc định tăng PWM/thời lượng chỉ vì có kick.

**Hiện hành — trợ lực sau181422:** CI exactSHA ->git fetch/merge --ff-only tại D:\STM32 task\control_motor,giữ dirtysettings ->Clean Build/flash/F8 ->collector ->CENTER ->đẩy nhẹ/thả trên giá ->quan sát phản ứng/rung ->STOP ->TXT. Không yêu cầu disconnect pretest hoặc lặp H1/B0.

**Kết quả MCU181422:** Giữ firmware32520fe; bước tiếp đánh giá actuator và ngưỡng tạo lực. Không cần flash docs-only hoặc lặp cùng30/10 để kiểm lại threshold/retry đã quan sát. Không tuyên bố sửa cơ khí xong.

**Hiện hành — bỏ ngưỡng lệch sau175629:** CI đúng SHA -> git fetch/merge --ff-only -> CubeIDE Clean Build/flash/F8 -> mở capture_m9_h2b1_hold.ps1 UntilStopped -> CENTER, tác động nhẹ/thả trong hành trình -> STOP và TXT. Kiểm goal còn hiệu lực khi lệch>32, không suy cơ khí PASS từ synthetic test.

**Hiện hành — retry 09/11:** Kiểm tra CI đúng SHA -> git fetch/merge --ff-only ở D:\STM32 task\control_motor, giữ các thay đổi .settings/startup -> CubeIDE Clean Build/flash/F8 -> collector -UntilStopped -MaxEvents100 -TimeoutSeconds300 -> CENTER, đẩy nhẹ/thả, quan sát rồi STOP. Thu state6/active1, retryCount tăng và terminal STOP state4/reason2/active0; cơ khí phải quan sát riêng.

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

**Cập nhật bàn giao 11/11/2026.** Còn thiếu dữ liệu định lượng, phạm vi từng trục/góc/tải và xác minh bộ nhớ đã chuyển. Các dữ kiện này không chặn bàn giao/báo cáo định tính. Chỉ thu thêm khi nhiệm vụ tiếp theo cần, không hỏi lại kết quả chống lực người dùng đã xác nhận.


[KHÔNG HỎI LẠI ĐỂ GIAO] Phần mềm final không phụ thuộc một câu trả lời mới. Thực nghiệm lực giữ/độ chính xác/nhiệt CHƯA CÓ DỮ LIỆU; ghi giới hạn trong báo cáo, không đòi người dùng tự chẩn đoán hoặc làm bảng hỏi.


**Hiện hành — LEFT300/200 sau báo đứng yên:** Thiếu log config300/200,góc thật có trở về,dòng/nhiệt và setting/ramp driver. Có thể triển khai candidate giới hạn để thử,không gọi đã xác định nguyên nhân hoặc đã sửa phần cứng.

**Kết quả MCU184953:** Thiếu quan sát cơ khí mới,thời lượng điện thật,mô-men/dòng/nhiệt và phản hồi RIGHT. Không cần kiểm lại Live Expressions,SHA hoặc kick activation đã xác nhận ở log này.

**Hiện hành — trợ lực sau181422:** Còn thiếu log source trợ lực mới,góc có trở về thật không,mô-men/dòng/nhiệt. Không hỏi lại số đo chưa có hoặc thời điểm thả tay đã trả lời. Không coi kickCount hay HOST là lực/motor PASS.

**Kết quả MCU181422:** Thông tin cần bổ sung cho nghiệm thu lực vẫn là phản ứng cơ khí,đường output thật và dữ liệu điện/nhiệt phù hợp. Không hỏi lại việc thả tay/đo dòng đã trả lời; source/core gates không còn là phần chưa xác nhận.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Không hỏi lại việc thả tay, Live Expressions hay số đo đã được trả lời. Thiếu log của source sửa mới, phản lực/return thật và dòng/nhiệt. Có thể tiếp tục source/CI, chưa thể kết luận đủ lực.

**Hiện hành — retry 09/11:** Đã có câu trả lời thả tay và chưa có số đo; không hỏi lặp lại. Dòng/nhiệt/mô-men thật và PWM đủ thắng ma sát chưa xác định; không chặn triển khai source giữ goal/retry, nhưng chưa có căn cứ nâng lực hoặc kết luận hardware PASS.

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

**Cập nhật bàn giao 11/11/2026.** 11/11/2026 đọc link chia sẻ rồi rà chat gốc từ H2-B0 tới b1c1e9c4; người dùng xác nhận chức năng chống lực và hết dò nhiều lần; xuất bản ngắn; chat gốc bị xoá; người dùng gặp mục Chỉ dự án bị khoá bởi Công việc; yêu cầu cập nhật GitHub và MD để chuyển phiên.


[2026-11-09 UTC — FINAL REQUEST] Sau a944da4f M9+Rotate và CI42625676055 PASS, người dùng yêu cầu chốt M9 trong một lần xử lý; triển khai profile3,40 lệnh hồi quy và162 mô hình giả lập PASS; bổ sung ARM CI.


**Hiện hành — LEFT300/200 sau báo đứng yên:** USER báo đĩa đứng yên,kế tiếp ERR/iOVER tắt và tiếngrít. Sau rà ghi chúM8,candidate chuyển LEFT từ fine pulse yếu sang recovery window300/200,trong cùng goal owner.

**Kết quả MCU184953:** Bản trợ lựcb89 đã chạyMCU;log184953 quan sát12burstLEFT. Vấn đề từ chưa có trợ lực chuyển sang trợ lực chưa đưa góc về mục tiêu.

**Hiện hành — trợ lực sau181422:** Sau xác nhận lỗi32RAW/retry được sửa nhưng lực30CCR chưa kéo về,USER yêu cầu thực hiện hiệu chỉnh actuator. Bản mới thêm burst có feedback/settle và cutofftick,không tăng lực liên tục.

**Kết quả MCU181422:** Sau flash bản bỏ32RAW,log181422 xác nhậnseq2 lệch tới149RAW vẫn giữ goal và retry4lần;seq3 kết thúc bằng STOP. Vướng mắc hiện tại chuyển sang hiệu quả tạo lực.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Sau retryaf6a43d, log175629 cho thấy một nhánh hủy khác: sai số33 vượt32. USER yêu cầu sửa ngay; bản mới loại bỏ nhánh sai số, giữ nhánh hành trình và cảm biến.

**Hiện hành — retry 09/11:** Sau log FAULT11 và xác nhận thả tay, USER yêu cầu không bỏ cuộc khi bị tác động. Bản mới giữ goal qua tuning events và thử lại sau hồi ngân sách, thay cho cần CENTER mới mỗi lần; các lỗi an toàn vẫn không tự hồi.

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

**Cập nhật bàn giao 11/11/2026.** Câu người dùng ngày11/11: “xác nhận khi tác động vào bánh đã có thể chống lại lực tác động đồng thời không có tình trạng quay đi quay lại nhiều lần để dò vị trí chính xác”. Báo cáo dùng USER-REPORTED định tính, chưa có logSHA và số đo mới. Tham số profile4 theo M9_CONTINUOUS_HOLD_CORRECTION và M9_RELEASE_SETTLING.


**Hiện hành — LEFT300/200 sau báo đứng yên:** LEFT assist300CCR=30%,tối đa200ms;RIGHT85CCR/12ms. PD nền30/10,still200ms,|v|<=20,settle>=200ms,freshsample,<=4attempts,rest>=3000ms+2budgets về0. Có thể nghỉ sớm do11/12;không cam kết luôn đủ4x200ms.

**Kết quả MCU184953:** Seq3 thời điểm10006/kick4/e−83,16497/kick8/e−78,23053/kick12/e−78;goal1886,raw1964 cuối. Seq4 goal1886/raw1886,e0/kick0,rút owner1 tại2416ms. Không có STOP2 trong file.

**Hiện hành — trợ lực sau181422:** Nền30/10CCR;assist175/85CCR,duration30/12ms,still200ms,|v|<=20RAW/s,off200ms,4pulse/vòng,rest>=3000ms+budget recovery. Reason13=assist round budget,kickL/R=acknowledged burst starts. Giữ8/4/deadband5,không max_error_raw.

**Kết quả MCU181422:** Seq2 cuối30960ms goalL1886/rawL2024/e−138,peak149,peakPWM30,applied5166,retry4. Seq3 cuối9385ms goalL1886/rawL2006/e−120,peak133,applied1001,STOP2/active0. RIGHT goal662/raw659,e+3,applied0.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Không còn max_error_raw. 8RAW vào hiệu chỉnh/4RAW ra/deadband5 giữ nguyên. Max jump90RAW là thay đổi GIỮA HAI MẪU, không phải sai số tổng so với goal; stale250ms,cap30/10,reversal200ms và cooldown/retry không đổi.

**Hiện hành — retry 09/11:** State6=COOLDOWN,active1,reason11 hoặc12,hai PWM0. Tuning capL30/capR10CCR; budget1500ms,window10000ms,no-progress1000ms; effort hồi theo1500/8500,không reset khi retry; retryCount đếm lần rời cooldown. Ngưỡng8/4/deadband5/maxError32RAW không đổi.

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

**Cập nhật bàn giao 11/11/2026.** Bắt đầu bằng docs/M9_SESSION_HANDOFF_2026_11_11.md rồi đọc HEAD nhánhM9 và hồ sơ17 mục. Giữ kết quả11/11, source b1c1e9c4, động học chưa tích hợp và SELECT hoãn. Các câu yêu cầu quay lại thử burst/đợi flash trong khối cũ thuộc thời điểm lịch sử.


[CHỈ DẪN B] Đọc báo cáo final trước các thử nghiệm cũ. Không quay lại bản xung thử hoặc lộ trình test mẫu khi chưa có chỉ thị mới. Giữ hai fix Rotate. Luôn phân biệt final phần mềm với hardware đã được xác nhận.


**Hiện hành — LEFT300/200 sau báo đứng yên:** Tiếp quản report LEFT_RECOVERY_WINDOW trước bản175/30. Không nhầm user LEDoff với chứng nhận driver không giới hạn dòng hoặc ACCEdisabled. Giữ workingtreeWindows,khôngreset/clean.

**Kết quả MCU184953:** Ưu tiên log184953 trước ghi chú source chưaflash. Đừng lấy CENTER mới seq4 làm nghiệm thu HOLD return;đừng coi reason13/active1 là hủy mục tiêu hoặc lỗi cảm biến.

**Hiện hành — trợ lực sau181422:** Đọc báo cáo assist mới nhất trước các ghi chú chỉ30/10. Build lại cả main/IRQ/header vì port callback và cấu trúc config đã đổi. Không chỉnh Live Expressions hoặc gọi source/CI là flashPASS.

**Kết quả MCU181422:** Tiếp quản kết quả MCU181422 trước các ghi chú chưaflash bản32520fe. Không coi lần CENTER mới seq3 trở về1889 là HOLD tự kéo lại; đó là phiên CENTER mới.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Ưu tiên bản full-travel sau log175629, đọc ghi chú32RAW trước đó như lịch sử. Xác minh branch HEAD/CI, giữ dirty .settings/startup; không reset hoặc reflash bảnaf rồi kỳ vọng hết ngưỡng32.

**Hiện hành — retry 09/11:** Tiếp quản bản ghi retry mới nhất trước các mục lịch sử hủy goal11/12. Không đồng nhất log cũ621a5ce với commit mới, HOST với motor PASS, hoặc user muốn không giới hạn với việc đã bỏ bảo vệ. Không reset/clean Git của người dùng, không yêu cầu disconnect test đã bị loại.

**Gửi trực tiếp cho trợ lý B:**

> Đây là hồ sơ từ hội thoại A. Đọc **mục 1 và 6 trước**, sau đó dùng mục 7 để kiểm tra tài nguyên, mục 14 để giữ thông số và công thức chính xác. Mục tiêu hoạt động là **M8-S14.2 tăng trần đánh lái 450/200 nhưng vẫn đồng bộ angle/velocity**, không phải speed-only. HEAD GitHub đã kiểm tra `605b937f8de5328d7b8c74c58accb9387392df90`, code và helper C đã commit/host-tested; **chưa có CubeIDE Clean Build/Flash hoặc log hardware của bản mới**. Hành động đầu tiên là xác minh HEAD/working tree và build, không tự kết luận firmware đã nạp. Giữ nguyên NC/STOP/SOFT/R1/R2 và dữ liệu người dùng. Người dùng **đã báo FIX lỗi FIRST-STOP RIGHT CCR0**; đừng nhắc nó là lỗi hiện hữu. RIGHT R2 `code=9` là sự kiện riêng chưa có root cause. Chỉ hỏi dữ kiện thiếu khi thật sự làm thay đổi bước tiếp theo; làm phần đã đủ nguồn trước. Không tự sửa những subsystem đã tạm đạt và không gọi bản source-staged là hardware PASS. Khi nhận log/đề bài mới, cập nhật trạng thái theo bằng chứng, không chỉ đọc câu assistant cũ.

**Cách phối hợp:** dùng tiếng Việt; thẳng và chính xác; không ép người dùng điền checklist dài trước khi được hỗ trợ; nếu cần log thì đưa **một lệnh chạy được và rõ thời điểm thao tác**. Không hỏi lại mục tiêu 450/200, thứ tự boost-first, hoặc FIRST-STOP đã fix. Sau mỗi commit báo link/SHA và trạng thái kiểm chứng thực. Không tự phát động công việc chưa được đề nghị ngoài chuỗi S14.2, trừ khi phát hiện vấn đề an toàn cụ thể cần bảo vệ.

### B update S14.3, 2026-10-31

Next assistant must inspect new HEAD/Release; require evidence clean build and Flash; collect M8S00_S143 plus CSV, diagnose actual CCR and AS5600 lag. Do not ask repeated COM4 or D:\STM32 task\control_motor unless changed.

## 16. Kiểm tra chất lượng và khoảng trống bàn giao

**Cập nhật bàn giao 11/11/2026.** Đã cập nhật cả17 mục, giữ nguyên bản386dòng và lịch sử. Hồ sơ hai phiên có nội dung báo cáo, nguồn và điểm dừng. Giới hạn còn lại là transcript nguyên văn, raw logs riêng tư, SHA MCU mới/định lượng và trạng thái đổi bộ nhớ. CI tài liệu phải đối chiếu ở commit thực tế.


[CONTINUOUS HOLD CORRECTION] Kết quả HOST là logic/liveness và mô hình giả lập, không hardware. Bảo toàn386 dòng,17 mục và raw UART riêng tư. Tài liệu profile3 là lịch sử đã thay thế.


[CHẤT LƯỢNG FINAL] Bảo toàn nguyên văn386 dòng, đủ0–16 và footer; cập nhật roadmap/handoff theo code. Raw UART riêng tư không được đưa vào repository. Kết quả CI/ARM phải kiểm đúng SHA; hardware profile3 không được tự nâng PASS.


**Hiện hành — LEFT300/200 sau báo đứng yên:** Original386lines,17sections/footer giữ nguyên. Core/main/SysTick và sanitizer kiểm config mới;CI exactSHA,targetCubeIDE,flash vàhardware tách riêng. Giới hạn còn lại:hiệu quả cơ lực/return,dòng/nhiệt,H3.

**Kết quả MCU184953:** 13dòng,2seq,fullSHA,raw/goal/error và tăngkick kiểm đúng. Seq3 không có terminal được thu trước seq4;dropped2→3 phản ánh bản tin chờ bị thay thế. Không dựng hành vi nút/lệnh gâyNEW_OWNER hoặc kết quả điện/nhiệt.

**Hiện hành — trợ lực sau181422:** Kiểm core/production profile/main/HAL và chính SysTick_Handler bằng HOST: pulsebound,tickwrap,mainblocked,expiredno-rearm,STOP/NC/highoutput,newowner không bị deadlinecũ cắt; sanitizer và regressions.17mục/386dòng/footer giữ;target/hardware còn thiếu.

**Kết quả MCU181422:** Đã kiểm19dòng,2seq,cùngfullSHA,goal−raw=error,state/reason,thống kê và STOP terminal. dropped1→2 là thống kê bản tin chờ bị thay thế,không suy lỗi điều khiển. Seq2 không có terminal được thu trước seq3; không dựng kết quả STOP choseq2.

**Hiện hành — bỏ ngưỡng lệch sau175629:** Ghi đúng trạng thái source/host/CI/target/flash/hardware. Bản gốc386dòng,17mục và footer giữ nguyên. Kiểm ca33RAW lẫn400RAW, hard-envelope và STOP/NC; không tuyên bố motor đã tự trở về.

**Hiện hành — retry 09/11:** Bản gốc386 dòng giữ nguyên;17mục0–16 và footer được giữ. Kiểm chứng source/core/main/formatter và CI tách khỏi CubeIDE/FLASH/HARDWARE. Các khoảng trống còn lại: cơ khí tự trở về, dòng/nhiệt và nghiệm thu tải; không dựng số đo.

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

### M8-S15 — rà soát 17 mục sau log 19:34–19:35 (2026-10-31)

0. **Phạm vi/nguồn:** GitHub API hai branch refs, commit/20 commit gần nhất/diff/Actions ở `cee4d8b`, GitHub source/docs S15, ba file riêng do người dùng chỉ định trong `D:\STM32 task\control_motor\logs`. Không có ảnh thao tác, build log, ELF/flash hash, dòng điện hoặc đo cơ khí độc lập. Chỉ kiểm tra file có sẵn; không truy cập toàn bộ chat trước.
1. **Tiếp tục ngay:** source S15 và HOST/Actions PASS, S15 có frame trên MCU, nhưng kết quả D-pad LEFT chưa tới SOFT cùng lúc. Giữ thuật toán/giới hạn, đối chiếu dữ liệu per-stroke và chứng cứ build/flash trước mọi kết luận hoặc thử powered tiếp.
2. **Mục tiêu:** hai góc LEFT/RIGHT độc lập dùng một T hoàn thành chung để phục vụ Ackermann; không ép góc, vận tốc hay PWM bằng nhau. Pilot vật lý hiện vẫn chỉ dùng hai SOFT target D-pad LEFT.
3. **Yêu cầu/quyền:** người dùng cho phân tích log và cập nhật hồ sơ GitHub sau kết quả quan trọng; giữ nguyên đặc tả 386 dòng, không tự đổi firmware, giới hạn an toàn hay công bố CSV/TXT thô. Chỉ thử controller ON, không bắt lặp PI OFF.
4. **Milestone:** M8-S15 là nhánh hiện hành; sáu lượt manual đạt SOFT riêng từng trục nhưng chưa đạt **cùng T**. Gate A tích hợp và Gate B mặt đất có tải vẫn chưa đóng. M7-S15 cũ là rollback trước S14.7.
5. **Đã làm/kiểm:** HEAD S15 `cee4d8b` và Actions `41200962485` PASS; meta CSV 1071/1071 sạch; so CSV với M8-S00 counters và M8-S06 timing. PASS CI chỉ là contract/static/GCC, không phải CubeIDE hoặc nghiệm thu phần cứng.
6. **Điểm dừng:** sau phân tích test và cập nhật tài liệu, trước bất cứ thay đổi firmware/điều khiển PWM. `M8S00_S15` chỉ giữ session cuối, CSV không có S15 ref/T per-row; cần chứng cứ mục tiêu và điều kiện dừng nếu hiệu chỉnh controller.
7. **Tài nguyên:** `M8S11_six_motor_20261031_193425.csv`, `.meta.txt`, `M8_S00_20261031_193556.txt` ở Windows, không commit vì log riêng tư; `docs/M8_S15_DUAL_TARGET_COMMON_TIME.md`, `Core/Inc/steer_s15_common_time.h`, `Core/Src/main.c`, verifier/test và roadmap ở đúng GitHub SHA.
8. **Quyết định:** giữ controller S15 độc quyền cho pilot D-pad LEFT Level2, không chồng S147/S148/S149; chưa quyết định tăng cap, PWM terminal hoặc đổi T. Đề xuất 2026-11-01 ưu tiên cải thiện RIGHT và giữ LEFT, không coi hãm LEFT là hướng mặc định; encoder motor lái hiện chỉ chẩn đoán, chưa chuyển vào vòng điều khiển. Không xem `last=0` cuối snapshot là thất bại kích hoạt, vì started/frames khác 0.
9. **Lỗi/kết quả:** không có FAULT trong snapshot; một D-pad LEFT bị nhả sớm, sáu lượt tới SOFT nhưng lệch thời điểm. Hai lượt từ gần L_MIN→SOFT L_MAX / gần R_MAX→SOFT R_MIN theo CSV skew 448/669ms tại SOFT, chỉ 114/57ms tại 180 RAW; M8-S06 `id=11` ghi skew628ms. Không gán một nguyên nhân firmware/cơ khí khi chưa có số liệu phân biệt.
10. **Rủi ro/phụ thuộc:** T mô hình có thể ngắn hơn khả năng RIGHT ở vùng terminal. [USER] báo R_MIN→NC còn 3 mm; log manual không có NC trip, nên không ghi va NC là sự cố hiện hành. Trong D-pad LEFT đầu LEFT tương ứng là L_MAX, không phải L_MIN; khoảng hở NC không cần để tính skew/tới T. Chưa có dòng, quãng trôi sau CCR0, ngắt nguồn độc lập và flash SHA; số đo tại R_MIN không tự bao quát mọi điều kiện khác. NC=4/6 trong CSV thuộc R2, không phải fault của các lượt S15 manual.
11. **Bước tiếp:** bảo toàn local CubeIDE/log changes; dùng log hiện có xác định skew terminal và đối chiếu M8-S06 encoder-vs-AS5600 cả stroke. Thiết kế nguồn telemetry từng stroke và tỷ số encoder có dấu trước khi đề xuất sửa S15; ưu tiên RIGHT trong trần hiện tại, giữ LEFT. Không yêu cầu thêm motor hoặc lượt chạy từ bước phân tích này. Nếu có sửa source, cập nhật đủ hồ sơ/roadmap và xem CI đúng SHA.
12. **Thiếu:** Clean Build log, ELF/Flash SHA chính xác, dòng/độ nóng, quãng trôi sau CCR0, khả năng cắt nguồn độc lập, T/ref/err và encoder lái có timestamp theo từng stroke, thử góc đích bất kỳ. Khoảng hở R_MIN→NC 3 mm là số đo người vận hành báo, không phải phần còn thiếu cho bài thử hiện tại; không coi im lặng về các mục khác là đã đạt.
13. **Thời gian:** commit S15 `2c5c6e7` rồi CI maintenance `310fbc1`/`cee4d8b`; capture CSV 19:34:25–19:35:31 và M8-S00 19:35:56 theo tên file máy người dùng. Board ms độc lập với giờ PC.
14. **Thông số/kết quả:** `enable=1,started=7,frames=844,skip=7,rejectedT=0,T=1200` ở S00; CSV board 192384–250923, sáu lượt SOFT hai bên, 1 nhả sớm; M8-S06 `id=15` CENTER→MIN LEFT1117/RIGHT1766/skew649ms. T=1200 là giá trị session cuối, không áp vào mọi stroke.
15. **Bàn giao:** luôn đọc HEAD của `feature/m8-s15-dual-target-common-time` trước, dùng prompt/roadmap hiện hành và CI cùng SHA; giữ lịch sử nhánh M7-S15 cũ, không yêu cầu Release. Trình bày SOURCE, HOST, CUBEIDE, FLASH, HARDWARE riêng.
16. **Kiểm tra chất lượng:** không suy SHA flashed từ UART; không gọi sáu lần chạm SOFT là hoàn thành đồng thời; không phủ nhận khoảng hở 3 mm do [USER] báo; không gọi STOP/NC/current PASS vượt phạm vi quan sát. Đặc tả gốc trong Git tree vẫn 386 dòng và SHA-256 `13d8976ec0d74057af94bf84c8445bddbb48715b308964e22eb6684711f708a4`; các khoảng trống khác nêu trên còn mở.


## M8-S15.1 — Telemetry-only; RIGHT Level2 ceiling proposal (2026-11-01)

- [USER] LEFT đã tốt, không giảm LEFT để chờ RIGHT. Yêu cầu tăng RIGHT ceiling 100 CCR **chưa tăng PWM thực tế**.
- [SOURCE] RIGHT Level2 200 CCR ACTIVE không thay, 300 CCR PROPOSED khai báo riêng chỉ để ghi báo cáo; không nối vào các hàm owner, envelope hay TIM14.
- [SOURCE] Trace RAM 96 mẫu của stroke D-pad LEFT S15 mới nhất, 50ms hoặc state đổi; lưu T/elapsed, AS5600 RAW/góc/timestamp/speed, encoder signed delta và dt/valid, owner/cap/candidate/actual post-atomic CCR, hai state và NC. M8S00 xuất `M8S151_META` và cặp `M8S151_A`/`M8S151_B` sau disarm. Không đổi M8S11 CSV. Nếu vượt dung lượng báo dropped, dòng cuối STOP được giữ. Không có UART blocking lúc motor chạy.
- [VALIDATION] GCC/helper và static checks tích hợp CI, chỉ gọi HOST PASS sau khi xác nhận đúng SHA. CubeIDE Build, flashed SHA, current/coast/hardware vẫn chưa có bằng chứng. Không tự chạy motor hoặc công bố log riêng tư.


[M8-S15.1 CORRECTION] Chỉnh chỉ escape CRLF của 4 dòng UART M8S151 (không đổi cấu trúc/motor), bổ sung static regression; cần CI đúng SHA. Không cập nhật trần PWM đang kích hoạt.
### M8-S15 roadmap plan 2026-11-01 — 17-section update guard

[0] Nguồn người vận hành/trao đổi mới và GitHub source/docs ở branch S15, theo SHA kiểm lúc bắt đầu.
[1] Điểm dừng: S15.1 telemetry đã commit, bước tiếp theo S15.2 SHADOW manual relation.
[2] Mục tiêu: manual quan hệ góc, known-target common-T; không đánh đổi LEFT.
[3] Ràng buộc user: Fast Start/Level2 boot defaults, không giảm LEFT, RIGHT ưu tiên, R2 CENTER-only common-T.
[4] S15.1 → S15.2 → S15.3 → S15.4 (phạm vi S15 hiện hành) → VC02-I1/I2/I3 (kế hoạch) + Gate A/B riêng. S15.5 Ackermann và S15.6 cũ tạm hoãn, KHÔNG chặn VC-02.
[5] Hiện trạng SOURCE/HOST trên S15.1; roadmap mới DOCS ONLY.
[6] Chưa có manual relation mapping/threshold, CubeIDE/Flash/HW cho S15.1 chưa xác minh.
[7] Canonical: docs/M8_S15_MILESTONE_PLAN_2026_11_01.md, M8_S15_DUAL_TARGET_COMMON_TIME.md, PROJECT_MILESTONES.md, main.c, S14 headers.
[8] Không đồng tốc tuyệt đối, không hãm LEFT; S14.8 positive boost và S14.9 forecast là hướng tham khảo, không tự chạy song song.
[9] S14 taper-only có thể làm LEFT chậm; script FAIL khác hardware FAIL.
[10] RIGHT terminal 180RAW, trần 200 active/300 proposed; không thay ngay vì current/coast/NC.
[11] Việc đầu tiên: S15.2 pure-C SHADOW geometry-proxy/angle error/speed prediction + HOST/CI, không đổi PWM; sau S15.4 chuyển sang VC-02 pipeline/owner mapping.
[12] Còn mở: cơ sở vật lý quan hệ góc, ngưỡng sai số, encoder ratio/direction, điều kiện bảo vệ.
[13] 2026-11-01 định hướng điều khiển được sửa; ngày lịch sử S14/S15 giữ nguyên.
[14] Giá trị active LEFT450/RIGHT200 CCR, Fast Start ON, Level2=2, +12, 180/100RAW, R2 NC MIN.
[15] Hội thoại tiếp theo phải đọc plan và HEAD cùng SHA, không lấy tài liệu lịch sử làm code hiện hành.
[16] Đây chỉ là docs; source/CI cần check đúng SHA, không ghi CubeIDE/HW PASS khi chưa có.
### Quyết định scope S15→VC-02 (2026-11-01; ưu tiên mới nhất)

[0] USER quyết định S15 chỉ làm tới S15.4 trong đợt hiện tại.
[1] Điểm dừng SOURCE: S15.1; việc kế tiếp S15.2 SHADOW; sau S15.4 tích hợp Voice.
[2] Mục tiêu: manual quan hệ góc/RIGHT, R1/R2 route, sau đó lệnh giọng nói điều khiển bánh và lái.
[3] Không dùng hình học Ackermann chưa có làm đầu vào giả.
[4] S15.5 và S15.6 lịch sử tạm hoãn; Voice VC02-I1/I2/I3 là đề xuất quản lý, Gate A/B riêng.
[5] Parser/builder đã có; executor chưa điều khiển TURN/ANGLE/ROTATE.
[6] Chưa có firmware Voice mới, CubeIDE/Flash/HARDWARE của route mới.
[7] Nguồn: main.c Voice_FinalizeCommand, Voice_IsOpenLoopCommandSupported, Voice_ExecuteMotorControl và docs/M8_S15_MILESTONE_PLAN_2026_11_01.md.
[8] Không khôi phục ép đồng tốc, không giảm LEFT, không bật RIGHT cap 300.
[9] Khác biệt: valid voice command builder không đồng nghĩa đã chạy bánh/lái.
[10] STOP>PS2>Voice>Idle và NC/soft/stall/atomic kiểm tra phải giữ.
[11] Cần bắt đầu S15.2 shadow trước; sau S15.4 kiểm tra code VOICE để thiết kế mapping thật.
[12] Lệnh Voice yêu cầu góc có thể cần động học chưa có; giữ từ chối an toàn nếu không hỗ trợ.
[13] Quyết định mới nhất sau milestone commit S15.2–S15.6 bản trước.
[14] Level2 LEFT450/RIGHT200; Fast Start ON; 300CCR only proposed; S15.5 geometry deferred.
[15] Cập nhật đủ docs roadmap/handoff/prompt; không sửa motor code trong commit này.
[16] Docs/CI only, không gộp với motor acceptance.
[S15.2 source update, 17-section crosscheck] (0) exact SHA CI required; (1) stage S15.2 observer; (2) geometry proxy not physical angle; (3) keep LEFT/Fast Start/Level2; (4) S15.2→S15.3→S15.4→VC02; (5) source+host+docs created; (6) target build/flash/hardware unknown; (7) main.c, steer_s152_manual_relation.h, test and verifier; (8) no slowing LEFT, no T for manual; (9) historical S14 skew not geometry; (10) 180/100RAW and NC current guards stay; (11) check CI then safe user-side build; (12) physical mapping and angular tolerance not established; (13) 2026-11-01 shadow; (14) 96 rows, 50ms, tau120ms, +25 hypothetical; (15) private test logs stay private; (16) no actuator change or hardware PASS.

### S15.2 terminal shadow row correction (2026-11-01)

[BUGFIX SOURCE] M8S152_Record(force=1) executes after zeroing CCR in Steering_ManualStop while STEER_MANUAL_MOVING enum may still be retained. Set moving=0 explicitly for forced terminal sample so predValid=0, shadowR stays actual PWM=0, never report post-disarm predictive eligibility. No change to output or stop logic. [PROCESS] First correction push omitted the paired required handoff+roadmap update and handoff-contract CI failed despite earlier source HOST pass; this corrective documentation commit restores both records. Recheck full GitHub Actions at exact final SHA. No STM32/flash/hardware acceptance.
[PowerShell-only correction, S15.2] 0=User prohibits PC serial apps/Live Expressions; 1=single PowerShell command for four traces; 2=preserve S15.2 proxy and shadow-only; 3=one file desired, no app; 4=S15.2->S15.3->S15.4->VC02; 5=new capture_m8_s152.ps1 and verifier/docs/CI; 6=physical COM/USB test still pending; 7=docs/M8_S152_POWERSHELL_CAPTURE.md; 8=do not use S11 app/Live Expressions; 9=old capture_m8_s00.ps1 one-run only; 10=USBUART after full disarm not live motor; 11=one command + USER short after each stroke; 12=real wheel-angle geometry unknown; 13=2026-11-01 explicit correction; 14=4 sections each M8S152 meta A/B; 15=privacy: local data file only, send one TXT; 16=CI != hardware COM validation.

[S15.3 17-section handoff addendum] 0=base 70f304b, S15.3 candidate SHA must verify; 1=RIGHT-only new mode OFF; 2=4 complete S15.2 logs confirm RIGHT RMIN 625-649ms later; 3=active Level2/FastStart preserved when OFF; 4=S15.2->S15.3->S15.4->VC02 unchanged; 5=header/main/host test/verifier/CI and milestones; 6=HOST!=MCU flash nor coast clearance; 7=docs/M8_S153_RIGHT_ONLY_STAGE_2026_11_02.md; 8=PowerShell-only captures, no apps/Live Expressions; 9=kept S14.2 historical behavior when pilot OFF; 10=physically bounded <=200CCR and >180RAW no NC; 11=firmware gate, bench before active tests; 12=geometry proxy not Ackermann; 13=2026-11-02 user four tests; 14=M8S153 after-disarm telemetry; 15=private logs never committed; 16=target full hardware and LEFT parity pending.
[S15.3 verifier correction, 2026-11-02] First CI run at b2c84e2 halted only at verify_m8_s153_right_only.py due nonexistent static 'Steering_PositionUpdate(void)' marker, before running pure C host. Fixed function slice to full body from Steering_ManualUpdate; MCU source unchanged. Verify exact next SHA; no target/flash/physical PASS.
[S15.3 source refinement 2026-11-02] Maintain LEFT positive boost from prior S14.2, discard S14.2 RIGHT temporary and do not trim LEFT below baseline; new S15.3 controls only RIGHT. This corrects earlier staging that bypassed S14.2 entirely and risked LEFT slowdown. Source/host CI final SHA after follow-up required; hardware timing/currents/coast unverified. No UART apps; PowerShell-only.
[S15.3 CubeIDE build blocker fix, 2026-11-02] Real operator build SHA 0cb9df9 FAILED 3 errors+1 warning: Steering_S153RightApply placed before Steering_PositionRawInRange definition (implicit declaration/conflicting type) and before STEER_MANUAL_MOVING macro. Fixed by moving exact same S153 function immediately before Steering_ManualUpdate, after both dependencies; strict HOST static ordering check added. No firmware PWM/owner changes. HOST CI final SHA and operator's new CubeIDE clean build required. Never claim 0cb9df9 target passed; S15.3 boot OFF and HW not validated.
[Build-provenance v1 addendum (2026-11-02)] 0=last fixed S15.3 b87566a baseline, future exact new SHA via embedded stamp; 1=Git HEAD generated by Windows PowerShell before CubeIDE compile; 2=uploaded 14:04 log S15.3 OFF, RIGHT late 528ms, SHA UNKNOWN; 3=clean/dirty flag and built UTC, no hardcoded self SHA; 4=do not change S15.3 control route; 5=.cproject both Debug/Release prebuild + firmware stamp header + MCU M8S00_FW + capture verification; 6=CI parser/runtime stamp doesn't equal target CubeIDE build; 7=docs/M8_FIRMWARE_LOG_IDENTITY_2026_11_02.md; 8=PowerShell-only UART, no terminal/Live Expressions; 9=prior logs remain historical unidentifiable SHA; 10=never override STOP/NC/soft safety; 11=clean build, verify stamp, flash ELF, read MCU M8S00_FW; 12=exact SHA 40 hex and dirty=0 required; 13=2026-11-02 log provided, no SHA; 14=old captures preserve originals; 15=unknown/mismatch/dirty produces saved partial/complete TXT and capture ERROR; 16=hardware provenance must be verified and S15.3 ON not supported merely by version tag.

## 2026-11-02 — Build provenance for future M8S00/PowerShell logs

[USER LOG] `M8S152_four_tests_20261102_140417.txt` contained M8S153 `enable=0,last=0` and D-pad LEFT finishL=2006ms finishR=2534ms (RIGHT +528ms), 46 rows dropped0, but **NO firmware SHA**. Therefore exact flashed commit UNKNOWN and NOT S15.3 active. [SOURCE] New MCU `M8S00_FW,proto=1,sha=<full40>,dirty=0/1,branch=...,builtUtc=...,target=STM32F767ZI` emitted ONLY inside gated post-disarm M8S00 dump; `Core/Inc/firmware_build_id_generated.h` is generated/ignored and required for `main.c` compilation. Both Debug/Release `.cproject` prebuild invoke native Windows PowerShell `tools/stamp_firmware_build.ps1` to read exact Git HEAD + dirty + timestamp, fail closed without Git. No source SHA hardcoded in a self-referential commit. `capture_m8_s152.ps1` defaults expected SHA to current repository HEAD (or explicit -ExpectedCommit full40), checks each actual MCU identity, errors on missing/mismatch/dirty while preserving TXT; no serial apps/Live Expressions. SOURCE/CI don't prove CubeIDE prebuild in operator's setup or actual flash. [DOC] `docs/M8_FIRMWARE_LOG_IDENTITY_2026_11_02.md`. [KEEP] all controls, S15.3 boot OFF, STOP/NC/SOFT and PS2/VC-02 intact. [NEXT] Clean Build exact SHA, verify STAMPED console and generated header, program that ELF, capture first physical M8S00_FW; never reinterpret old logs as SHA-proven.

[2026-11-02 FWID fix] Windows Parser caught `$complete:` in firmware-mismatch warning; use `${complete}:` before asserting PowerShell success. Firmware motor unchanged. Exact new SHA and target CubeIDE + MCU serial required.
[FWID Windows parser follow-up 2026-11-02] Corrected both remaining $complete: in DIRTY and UNKNOWN status strings to ${complete}: to eliminate invalid PowerShell interpolation. No MCU control changes; require CI exact new SHA, CubeIDE Clean Build and physical log.
[FW identity Windows PS5.1 runtime fix 2026-11-02] $PSScriptRoot was empty in param default expression in CI; now derive $RepoRoot from running script path AFTER param binding. Exact new SHA and Windows CI runtime required, MCU behavior unchanged. Never claim actual CubeIDE prebuild verified until user reports.

[2026-11-02 S15.2 logging 17-section current addendum]
[0] Host-only PowerShell change; [1] actual MCU SHA in each log, no local Git query; [2] user reported four complete frames 16:06, SHA 815231a, dirty1 due CubeIDE prefs, old invalid4; [3] S15.3 remains OFF and Fast Start/Level2 same; [4] S15.2 baseline -> S15.3 gated -> S15.4 -> VC02; [5] modified capture/verifier/docs, exact new SHA CI required; [6] no new CubeIDE/Flash or active motor PASS; [7] M8_S152_POWERSHELL_CAPTURE.md and M8_FIRMWARE_LOG_IDENTITY_2026_11_02.md; [8] native PowerShell one TXT four tests, no serial GUI/Live Expressions; [9] old fail-on-dirty policy superseded, historical logs unchanged; [10] STOP/NC/SOFT and physical R_MIN clearance still critical; [11] pull receiver update, no reflash, submit private TXT; [12] manual label not SHA proof, firmware M8S00_FW is compile provenance; [13] user explicitly requested no dirty-blocked capture; [14] preserve four framed META/A/B rows and PWM0 final plus MCU SHA; [15] never upload raw operator logs publicly; [16] CI source is not S15.3 active hardware validation.

[2026-11-03 AS5600 debug byte-shift hypothesis and CENTER; 17-section crosscheck]
[0] Read-only diagnostics + requested center constant, no motor algorithm rewrite; [1] current: Live Expressions invalid RAW while S15.1 observed good; [2] S15.1 17ff3d7 at CENTER L1873/R665; newer firmware Live L19968/R39175, configured PWM85 shown 21760 and entry15 shown3840, strong one-byte offset hypothesis not yet independently proved; [3] LEFT center explicitly requested 1886, RIGHT stays662, S15.3 default OFF; [4] S15.2 shadow->safety gated S15.3->S15.4->VC02 unchanged; [5] 32-bit read-only raw mirrors, fully disarmed MCU UART line, static verifier and docs; [6] CI and MCU build/flash and powered safety are separate; [7] docs/M8_FIRMWARE_LOG_IDENTITY_2026_11_02.md; [8] native PowerShell log not serial GUI/Live Expressions for verification; [9] old S15.1 center1873 remains historical; [10] original sensor read, STOP, NC, SOFT, caps, right terminal clearance unchanged; [11] check exact SHA CI then CubeIDE build/flash only with motor power physically isolated; [12] offset likely debugger, actual RAM/symbol mismatch not excluded; [13] user requested LEFT CENTER=1886; [14] MCU diagnostic M8S00_AS5600 includes raw16, mirror32, comm, center and match12; [15] original private logs remain local; [16] no new target/HARDWARE PASS, no active S15.3 trial or new center mechanical acceptance claimed.

[2026-11-03 RIGHT terminal 50-to-60 diagnostic, crosscheck 0-16]
[0] Opt-in only 50->60 RIGHT R_MIN floor last100RAW; [1] user observed right lag near SOFT, suspected 50CCR; [2] a85438e baseline lag D-pad LEFT396ms, RX LEFT517ms; [3] boot OFF, S15.3 boot OFF, no default motion changes; [4] S15.2 data -> opt-in evidence gate -> S15.3 -> S15.4 -> VC02; [5] firmware source, static verifier, docs, CI exact SHA, target build pending; [6] no new powered on-trial evidence; [7] M8_S153_RIGHT_ONLY_STAGE_2026_11_02.md; [8] post-disarm native PowerShell log retained; [9] manual floor50 in steer_manual_right_rx_min_pwm, not R1/R2; [10] STOP/NC/SOFT/motor limits and late atomic control unchanged; [11] current, temp, powered-off coast/NC safety gate needed before enable; [12] 50CCR suspected but unproven root; [13] user explicitly requested increase to evaluate; [14] M8S15_TAIL60 candidateFrames PRE-atomic and M8S152_B real PWM; [15] no user logs published; [16] no hardware PASS/Flash/ON result, ~3mm physical clearance risk.

[2026-11-03 correction of operator 60CCR requirement — current crosswalk 0–16]
[0] Trial is now DEFAULT ON, not OFF; [1] operator expressly objected to previous 9418d4a boot-OFF candidate; [2] baseline a85438e RIGHT lags 396ms D-pad LEFT / 517ms RX LEFT; [3] scope RIGHT-to-R_MIN in last100RAW taper only, S15.3 default OFF; [4] after S15.2 data, 60-CCR trial gated by current/coast/NC safety before S15.3; [5] source main=1U, static verifier updated, docs; CI target Build distinct; [6] no powered/flash proof currently; [7] appended latest M8_S153_RIGHT_ONLY_STAGE_2026_11_02.md; [8] native PowerShell log unchanged; [9] last100 RAW taper floor50->60 not constant-duty flat60 or R1/R2; [10] STOP/NC/soft/final CCR0 and left unchanged; [11] physical margin ~3mm and unmeasured current/coast do not authorize powered full stroke; [12] terminal 50 cause hypothesized, not proven; [13] user explicitly insists boot-ON 60 candidate; [14] stopped-only M8S15_TAIL60 status and M8S152_B actual CCR; [15] original raw TXT remains private; [16] GitHub HOST PASS must not be presented as physical motor safety PASS.

[S15.3 current 2026-11-03 0–16]
[0] Full terminal PWM60 source removal, not only OFF; [1] operator rejected tail60 experiment and asked start from valid 50CCR a85438e; [2] 50CCR baseline D-pad LEFT lag396/440ms and RX LEFT517/561ms; [3] old S15.3 misses D-pad LEFT mid ePred -20..-50 despite final RIGHT late; [4] S15.3 remains OFF and R_MIN early preview is a HOST candidate, no S15.4/VC02 advance; [5] main.c/header/static verifier/C regression; [6] CI host vs actual MCU build/flash/real movement separate; [7] docs/M8_S153_RIGHT_ONLY_STAGE_2026_11_02.md latest section; [8] native PowerShell UART capture unchanged; [9] S14.2 trimRf counts intended reductions, not actual delivery; [10] S15.3 right-to-min and both320<remaining<=650RAW, virtual +80pm, max+12 CCR, original caps; [11] STOP/NC/SOFT, raw50 and left logic unchanged; [12] forecast is a testable assumption, not identified physical cause; [13] user explicitly requested revert60 and optimized S15.3; [14] M8S153 earlyFrames/avoidedReductionR preatomic; appliedFrames postatomic; [15] maintain private TXT logs, git history, no destructive reset; [16] no ON test, measured current/coast, target build/flash or safety PASS claimed.

[2026-11-03 19:09 current source S15.3 ON right-RMIN crosswalk 0..16]
[0] S15.3 default ON but RMIN command latch only; [1] preceding 50CCR log was S15.3 OFF; [2] run1 DpadLEFT RIGHT incomplete RAW225 vs SOFT216, other skews73/541/109ms; [3] terminal RIGHT RAW229 almost still for300ms at PWM56; [4] S15.3 trial active only RMIN, S15 commonT/S15.4/VC02 deferred; [5] main.c default1 direction latch plus static verifier and docs; [6] HOST CI is not target compile/flash/motor proof; [7] newest S15.3 stage doc section; [8] only native PowerShell four-run dump after disarm; [9] S14.2 right trim requests per run41/27/57/23, not necessarily delivered; [10] preview both remain >320 <=650RAW, max+12CCR, original <=180RAW veto; [11] LEFT, STOP, NC, SOFT and terminal floor50 unchanged; [12] 80pm preview is hypothesis and may not cure 13RAW stall; [13] always send firmware update command together with capture log command; [14] on RMIN expect M8S153 enable1,last1,appliedFrames postatomic, on MAX last0; [15] no force reset or private log upload; [16] no powered testing or target build already verified.

[2026-11-03 RIGHT RMIN 195 measured correction crosswalk 0–16]
[0] RIGHT R_MIN boot geometry 195; reported NC range195..197; R2 reference195, both replacing older166; [1] user selected 195 as deterministic lower reading; [2] older 19:19 four-run afc42cd has RMIN166 SOFT216 and RMIN skews +297/+374ms; [3] new SOFT RMIN=195+50=245, 29RAW earlier cutoff; not proof of lag cause; [4] S15.3 ON RMIN-only 320..650RAW max+12CCR remains; no S15.4/VC02 change; [5] main.c, independent CI verifier, docs/context; [6] CI HOST vs CubeIDE target build/flash vs motor result separate; [7] S15.3 stage doc latest appended paragraph and milestone; [8] PowerShell capture four-run command included with update, project main path only; [9] do not compare stale 166 SOFT216 arrival timing to 195 SOFT245 as identical distance test; [10] RIGHT50 last100RAW taper unchanged; [11] NC, STOP, R2 hardware-own NC and LEFT independent; [12] confirm M8S00_CAL_RUNTIME minR195 r2R195 and M8S00_STEER softR245 in live UART; [13] don't assume default195 after SELECT full calibration; [14] older 19:19 S15.3 enable1 last1 request0 applied0 is observable but not causal attribution; [15] prior operator TXT private, no git overwrite/reset; [16] hardware current/coast/cutoff unverified, no safety PASS claim.

[2026-11-03 195 geometry HOST verifier fix] Exact C source contains right-min guard as an OR subclause; static check correctly matches guard subclause rather than assuming standalone if. No firmware behavior changed after f015998; CI must pass this final SHA before target Build/Flash.

[2026-11-03 S15.3 ETA-only RMIN crosswalk 0..16]
[0] S15.3 RMIN-only ON, angle-bias -80 veto replaced by remaining-time ETA mid-stroke; [1] user explicitly authorized implementation; [2] 19:33 RMIN195/SOFT245 4/4 SOFT, DpadLEFT right late220ms RXLEFT363ms, S15.3 boosts0; [3] last180RAW L495/473ms R880/891ms, S15.2 velocity is q-permille/s; [4] S15.3 ETA-only pilot, no commonT S15.4/VC02; [5] pure header/main UART diagnostics/static verifier/pure C tests/docs; [6] CI HOST vs CubeIDE target Build/Flash and actual powered improvement distinct; [7] newest S15.3 stage doc 19:33 ETA section; [8] native PowerShell capture four-run main directory, no worktree; [9] S14.2 RIGHT reduction proposals are discarded on RMIN active path, LEFT legacy positive corrections preserved; [10] ETA last180 measured means LEFT484/R886ms, plausible span700..1000, vel>=250q/s, both (320,650]RAW, <=12CCR; [11] NO S15.3 extra at <=320RAW, RIGHT last100 floor50, NC/STOP/SOFT and atomic unchanged; [12] empirical estimates only two traces, physical improvement unproven; [13] send update code and log capture together; [14] M8S153 etaFrames,peakEtaLagMs,early/request/applied plus arrival/NC/fault to judge; [15] old logs private, avoid reset/stash destruction; [16] current/coast/cutoff/target build not yet validated.

[2026-11-03 S15.3D after A/B FAILED — crosswalk 0–16 for next chat]
[0] S15.3 NOT PASS, diagnostic firmware S15.3 defaults OFF; [1] operator requested implementing next version and updating GitHub MD progress; [2] f790f48 A/B OFF19:59: DpadLEFT +385ms RXLEFT +660ms RXRIGHT LEFT STALL13; ON20:01 DpadLEFT RIGHT STALL13 RAW257 vs SOFT245, RXLEFT stopCode5 incomplete, ETA boosts31/38; [3] STALL13 last best-distance progress >650ms, stopCode5 neutral-required or reversal; [4] S15.3 diagnostics before next three-mode isolation, S15.4 then VC02 only after acceptance; [5] main.c+new pure helper/test+static CI verifier+roadmap/stage/handoff; [6] GitHub HOST check is not CubeIDE Build/Flash or powered motor evidence; [7] latest S15.3 stage doc and M8_S15_MILESTONE_PLAN current addendum; [8] PowerShell 4-run capture on COM4 after full DISARM via USER button, user works only project main folder; [9] S14.2/LEFT unchanged, ETA opt-in code remains but OFF at boot; [10] middle ETA bounded +12CCR when enabled deliberately, terminal floor50 and RMIN195/SOFT245 remain; [11] STOP/NC/SOFT/atomic, STALL650 unchanged and independent; [12] OFF also stalled on LEFT, do not assert ETA is sole cause or classify early stopCode5 as STALL; [13] always send update command together with capture command; [14] new M8S153_FAULT axis L/R one-shot progressAge/sampleAge/encoder/CCR/NC and M8S153_STOP5 path 1=neutral 2=direct reverse; [15] private raw logs kept out of GitHub, previous history and full 17 sections preserved; [16] target Clean Build/Flash, physical current/coast/NC/STOP power cutoff and new diagnostic capture not yet independently validated.

[2026-11-03 S15.3D UART empty-event safety correction] M8S00 USER dump requires first STOP/FAULT, calibration or joint-steering event since boot/clear; boot+USER alone can return status3 and no frame. Do not intentionally stall/energize motors just to create a diagnostic frame. After hardware safety inspection and a legitimate safe manual command, only then DISARM and USER. Capture PowerShell script requires 4 real events. HOST/CI cannot replace hardware cutoff/coast tests.

## 2026-11-03 — Manual NC two-press regression: 17-section crosscheck
[0] Source-stage M8 branch; historical snapshots preserved; new HEAD/CI to verify.
[1] User confirms D-pad/RX two press while leaving NC, R1 normal.
[2] Source old release forced SETTLING and origin IRQ can fault9; exact hardware edge path not independently logged.
[3] Rule NC blocks INTO for own axis, permits valid AWAY same held command; neutral direct reversal gate stays.
[4] NC manual fix higher priority; S15.3D still BLOCKED, S15.4/VC-02 not PASS.
[5] ESCAPE release clean 120ms→MOVING, origin NC IRQ one-pass CCR0, static+C HOST verifier.
[6] Source stage vs CI vs CubeIDE/Flash/hardware are separate.
[7] main.c, tools/verify_m8_manual_nc_escape.py, workflow, docs/M8_MANUAL_NC_DIRECTIONAL_RELEASE_2026_11_03.md, handoff, milestones.
[8] R1 one press CENTER previously fixed, user says works; R1 unchanged.
[9] Source suspects bounce and forced settle; NC0 in S15.3D STALL separate.
[10] Existing ISR local CCR0, final atomic NC/DIR, hard/soft AS5600, STOP, timeout1000, STALL650 maintained.
[11] Exact SHA CI then motor supply isolated CubeIDE; check 4 NC/dir/cutoff/mechanics before limited outward-only bench test.
[12] Need physical NC evidence and current/coast clearance; no motor acceptance.
[13] 2026-11-03 after S15.3D log, user reported D-pad/RX regression, R1 explicitly excluded.
[14] Pass requires one held outward D-pad/RX press through release to opposite SOFT with no INTO PWM, false FAULT9 or second press; R1 unaffected.
[15] Continue from branch SHA/CI, PowerShell-only logs, private logs not committed.
[16] All 17 sections preserved, original 386-line spec untouched, no HOST=hardware conflation.


[CI follow-up 2026-11-03] cc82185 manual D-pad/RX source C HOST PASS; total Actions FAILED only because old S15.3D verifier sliced fault by deleted comment marker. Changed verifier to balanced C function slicing, no motor source/CCR/NC changes. Verify new SHA CI; CubeIDE and real motor remain PENDING.

## 2026-11-03 — S15.3 Mode B 17-section current crosscheck
[0] New M8 source trim-isolation pilot after 8cbec2d, preserve full history.
[1] User requests pass S15.3; requires real motor angle-tracking acceptance, not CI.
[2] 20:36 four UART frames ETA OFF: only 2/4 complete, RIGHT STALL at RAW257/255 CCR56/55 and NC0.
[3] Keep LEFT fast, preserve RIGHT RMIN195/SOFT245 and floor50; no STALL timeout weakening.
[4] Mode0 baseline vs mode1 no RIGHT trim vs mode2 no trim+ETA; S15.4/VC-02 still blocked.
[5] Mode1 code uses temp discarded_right and preserves LEFT positive boost, fresh stroke latch and read-only UART mode.
[6] Source/host static gate separate from CubeIDE Clean Build, flashed SHA and motor PASS.
[7] Core/Src/main.c, tools/verify_m8_s153_trim_ab.py, workflow, docs/M8_S153_TRIM_ISOLATION_2026_11_03.md, roadmap.
[8] R1 and previous D-pad/RX NC direction-only repair preserved, no new terminal power.
[9] Driver current/mechanical binding or current limiting remain unverified; CCR55 does not prove output torque.
[10] NC/STOP/atomic CCR, hard/soft AS5600, 650ms STALL, 50CCR terminal and physical cutoff intact.
[11] Exact SHA CI then motor-power-isolated target Clean Build/Flash, verify cutoff/NC/coast/current, safe PowerShell 4-run.
[12] No certified current/coast and no agreed angular-error pass threshold; cannot claim hardware S15.3 PASS.
[13] 2026-11-03 post-NC fix user requested pass S15.3.
[14] Mode1 needs 4/4 fault-free, no STOP5/NC, real RIGHT response and no LEFT slowdown before assessing mode2.
[15] Next chat use HEAD and CI, only PowerShell native UART from main Windows repo, private logs not committed.
[16] Original 386-line requirement and 17 sections preserved; no source=hardware conflation.


## 2026-11-04 — PS2 1-byte Live Expressions offset vs actual L1/L2 spin NC gate
[OBSERVED] L2 shows R2, R2 shows L1, L1 shows R1, R1 shows Triangle in Live Expressions; exact adjacent 8-bit struct fields. Code PS2 bit assignments standard; cannot yet conclude physical receiver protocol or LIVE address truth. Existing prior AS5600 narrow-expression shift reinforces debugger hypothesis, not proof.
[CODE] New read-only debug_ps2_frame_buttons32 from raw byte3/4 active-low packet before decode; debug_ps2_button_mask32 and debug_ps2_button_seen_mask32 from decoded struct. Preserve original PS2 actuation and no motor power from observers.
[GATE] L1/L2 WheelControl spin only if physical NC mask exactly 0x06 (L_MIN+R_MIN, both MAX clear); ps2_spin_nc_block_frames counts blocked requests. Do NOT bypass NC or drive at ~3mm RMIN clearance merely to prove button.
[TEST] tools/verify_ps2_button_layout.py compiles/tests ACTUAL C ps2.c 16 buttons + invalid/SPI error; HOST+CI separate from CubeIDE target and real 8-bit RF packet. Doc docs/PS2_BUTTON_LAYOUT_AUDIT_2026_11_04.md.
[USE] Motor supply disconnected, confirm exact new SHA/ELF; hold one physical key and observe both 32-bit masks: L2=256,R2=512,L1=1024,R1=2048,Triangle=4096. On mismatch pursue raw/controller or Live Expressions cause; no blind swapping of controls. S15.3 status unchanged, not hardware PASS.


## 2026-11-04 — S15.3 far-zone phase candidate, fe4c58f functional mode1 baseline
[0] Feature branch continues from passive PS2 diagnosis 6d343e6; original fe4c58f mode1 RIGHT independent owner is unchanged by default.
[1] User requests S15.3 improvement and postpones visual PS2 layout; reports mechanical NC spin issue corrected.
[2] 2026-11-04 12:51 file contains ONE Dpad LEFT fe4c58f dirty1 mode1 stroke: LEFT1741ms RIGHT1994ms +253, both SOFT, faults0, NC0, isolation144 avoided66.
[3] Mid normalized RIGHT lead reaches -291pm with LEFT already cap450, but near SOFT RIGHT is slower; do not raise terminal floor.
[4] S15.3 candidate is far-zone shadow ON and actuation OFF; S15.4/VC02 still blocked.
[5] Pure C M8S153_FarTrim permits only when both remain >420 RAW to own SOFT, valid actual and predicted initial-relative RIGHT lead >120pm, fresh AS5600, both MOVING, NC0, caps reduction16CCR and fades to0 at420.
[6] Optional actuation latches only at a new eligible Level2 RMIN mode1 stroke; pilot enable defaults0 and fe4c58f applied RIGHT PWM is preserved.
[7] main.c, new Core/Inc/steer_s153_far_trim.h, host C regression, workflow, stage doc, milestone, handoff updated; PS2 unchanged.
[8] Post-DISARM M8S153_FAR fields shadowEnable, applyEnable, applyLast, frames, proposed, actuationRequests, peakProposal; proposals are not measured CCR/current.
[9] Early RIGHT reduction may improve mid-stroke relationship but WORSE +253ms RIGHT arrival, not verified.
[10] Source/HOST CI, target CubeIDE build/flash, physical 4-stroke and current/coast/cutoff checks are separate.
[11] STOP/NC/atomic, ESCAPE fix, RIGHT floor50, RMIN195/SOFT245, LEFT cap450 RIGHT cap200, STALL650, R1/R2 preserved.
[12] Historical ~3mm contact gap and unknown current/coast/cutoff bar automatic powered activation.
[13] Repository parent contains 6d343e6 read-only 32-bit PS2 diagnostics, not any key remapping; no PS2 changes for this pilot.
[14] Next post-DISARM log mode1 plus M8S153_FAR shadowEnable1 applyEnable0 applyLast0, compare suggested proposal and S15.2 angle trace.
[15] Main user folder D:\STM32 task\control_motor, native PowerShell post-DISARM UART, do not overwrite dirty local work.
[16] Preserve original 386-line specification, all 17 sections and trailing handoff footer; no hardware PASS claim.


## 2026-11-04 — operator constraint restored: NEVER pace manual by reducing LEFT
[0] Correction to unauthorized candidate after f2137556: no LEFT ETA pacing. Operator explicitly disallows intentional LEFT slowdown for manual D-pad/RX synchronization.
[1] Source of truth docs/M8_S15_MILESTONE_PLAN_2026_11_01.md lines 23-25 and 45,58 and docs/PROJECT_MILESTONES.md section "operator current decision": LEFT must not be tapered/held/slowed to mask RIGHT lag; S15.3 RIGHT-only.
[2] The b21a973..0ad0040 SHADOW LEFT ETA Pacing implementation was unauthorized even with actuation OFF. This correction RESTORES the full motor/control/workflow/test tree to the f2137556 baseline, rather than merely setting a flag to zero.
[3] The original f2137556 RIGHT FAR shadow remains read-only/default actuation OFF; steering owner mode1 remains independent RIGHT and LEFT positive-only (max of baseline).
[4] Do NOT use the 7-19% endpoint arrival skew objective to justify a LEFT reduction or false S15.3 PASS. Any new RIGHT-only method must respect NC/STOP/CCR cap/current/coast safety.
[5] No PS2 button map change, no analog/manual pin change, no R1/R2/SELECT change; passive PS2 debug variables remain.
[6] Baseline mode1 guard updated to assert the offending LEFT PACE symbols and include are ABSENT; host/CI is not target MCU/hardware proof.
[7] Important inherited legacy S14.2 header M8S142_SyncPlan has conditional p.trim_l, which can lower LEFT in other mode paths; mode1 explicitly restores LEFT >= its owner baseline. This correction does NOT silently change historical S14.2 algorithm. A separate source/physical safety review is warranted for the legacy possibility.
[8] 2026-11-04 f2137556 4/4 log remains evidence of completion but time skew remains 7.18%,7.00%,19.29%,14.46%, NOT under2%.
[9] No powered test on corrective commit solely from CI; motor cut/current/coast/NC clearance need independent evidence.
[10] Source: Core/Src/main.c exact f2137556 copy; far pilot intact. No code that calculates LEFT ETA reduction may remain.
[11] New head is forward-only corrective commit: history retains prior erroneous commits but HEAD contents contain no left pace source/telemetry/tests.
[12] Keep original user workflow Git status -> safe pull --ff-only -> CubeIDE clean build -> native PowerShell post-DISARM, no destructive reset of local files.
[13] Firmware SHA required after new commit and generated header, do not infer from old 0ad0040 log.
[14] S15.3 NOT PASS; S15.4 and VC-02 deferred until reviewed.
[15] 17-section project handoff and 386-line original spec remain untouched.
[16] No user confirmation required to remove unauthorized functionality; new changes to control strategy must follow approved constraints.


## 2026-11-04 — Chốt 49 command words/HEX VC-02 từ 4 ảnh USER (S15.4)
[USER SOURCE/DECISION] 4 ảnh bảng Control details xác nhận 49 hàng UART1_TX Send it; user nói sẽ tạo firmware rồi nạp VC-02, CHƯA thông báo đã nạp. Toàn bộ 49 hàng, byte và nhị phân lưu ở docs/VC02_OPERATOR_COMMAND_MAP_2026_11_04.md. Đây là quyết định giao thức VOICE được chốt, không phải kết quả runtime UART.
[KEY] A4 Rotate left (thay A4 rotate không hướng); A7 Rotate right (mã mới, KHÔNG A8); A5/A6 Turn left/right; B0 Zero degrees và alias center, B1..B9 góc 5..45°; C1..C5 speed; D0 XX duration 5..120s bước 5; F0 clear; FF STOP; A1 wake, A2/A3 forward/back. Wake phrase ảnh hơi mờ, chưa khẳng định chữ tuyệt đối.
[CRITICAL CODE] Core/Src/vc02.c tại baseline f2137556/7095aca chưa decode A7; A4 đang là undirected ROTATE; B0 Voice_SetStraight chỉ xóa pending TURN/ANGLE, KHÔNG đưa vật lý về CENTER. Voice executor chỉ forward/backward straight; TURN/ANGLE/ROTATE chưa vận hành. Không tuyên bố đã hỗ trợ góc chỉ vì VC02 UI có B0..B9.
[S15.4] Người vận hành yêu cầu test S15.4 qua các lệnh góc VC-02. Giữ điều khiển vị trí đã có (R1/R2), STOP>PS2>Voice, NC/atomic và giới hạn an toàn; không tự đổi PWM manual/S15.3, không Ackermann tùy ý khi chưa biết mô hình cơ cấu. Handoff cần dẫn tới bảng mapping ở file mới.
[ACTUAL ACTION] Commit này chỉ thêm tài liệu và cập nhật handoff/milestone/plan, không sửa Core/Src, Core/Inc, GPIO, UART decoder hay PWM. VC-02 flash/hardware PASS đều chưa có.


## 2026-11-04 — [USER-APPROVED] S15.4 two independent Voice channels, safety cancel/neutral
[REQUIREMENT] Four wheel group commands A2 forward/A3 backward/A4 rotate-left/A7 rotate-right must EACH be completed by D0 XX duration before acting. Angle steering A5 Turn left or A6 Turn right plus B0..B9 degrees completes immediately without TIME. B0/Zero degrees/center standalone is physical CENTER request in the NEW DESIGN, not the current Voice_SetStraight behavior. Wheel and steering group states must remain independent and may operate simultaneously; LEFT and RIGHT steering goals separate, speeds/feasible common-T inferred from targets; DO NOT treat wheel duration as steering common T. Angle-to-RAW mapping/wheel geometry not validated yet.
[STOP/PS2] FF STOP and any valid PS2 takeover cancel ALL voice pending/ready/active wheel+steer, no replay. Initial PS2 input used for cancel must NOT actuate. Require release every held PS2 button, LY and RX neutral, valid Analog packet; only a subsequent NEW PS2 input after this release may control. Held PS2 input may otherwise continue normal control when already authorized. Preserve STOP>PS2>Voice, RF/loss, NC/EXTI, independent cutoff.
[SOURCE GAP] Current main.c at be26f52 still has monolithic VoicePending/Complete/Active requiring movement and duration, cannot execute turn+angle alone, supports forward/back only, Wheel Voice calls Turn_Motors_Stop; vc02.c lacks A7 and A4 spin direction. Existing ps2_cancel_hold logic already approximates first-input-cancels and neutral gating, but needs explicit all-button+both-stick+Analog valid rearm check when new Voice split is implemented.
[PLAN DOC] docs/M8_S154_VC02_INDEPENDENT_COMMAND_CONTRACT_2026_11_04.md contains truth table, two-actor state design, safety arbitration, HOST test matrix and MCU/hardware gates. New commit is docs-only, no MCU or VC02 firmware changes or hardware PASS. User showed UniOne COM3 download completed and said 'rồi ok', but no proof STM32 bytes/physical steering/pivot.
[CAUTION] Finalized VC02 command map at docs/VC02_OPERATOR_COMMAND_MAP_2026_11_04.md remains 49 rows. Avoid reintroducing LEFT manual slowdown, advancing arbitrary Ackermann S15.5, changing Right limits/floor or remapping PS2. S15.3 result 4/4 completions not 2% PASS.


## 2026-11-04 — USER correction to VC02 speed independence / R2-pivot semantics
[USER SOURCE] All C1..C5 voice speed levels are independent, standalone commands: no duration/angle/movement needed, no actuation by speaking speed. VC02 rotate left A4 and rotate right A7 must first make the steering behave as R2 (position both to physical MIN), THEN spin: A4 triggers equivalent PS2 L2 (-1); A7 triggers equivalent PS2 L1 (+1). The original "rotate is direct wheel-only" simplification is superseded.
[REQUIRED SAFETY] A4/A7 must still receive D0 XX to become a complete command; do not cause R2 output from incomplete byte. R2 uses protected position owner, not raw fake PS2 input; spin requires both physical MIN NC asserted and both MAX clear, continuous monitoring, reject/abort missing NC, STOP/PS2/fault. Rotate temporarily needs BOTH wheel and steering owners; concurrent unrelated angle target cannot override it. PS2 takeover neutral rearm and STOP cancel ALL phases. Time counter start after R2 success is a design proposal, not explicit user approval.
[SOURCE] Core/Src/main.c at f100921: PS2 L2 invokes WheelControl_SetSpinDirection(-1), L1 +1; both MIN NC must be physically asserted (not simply target RAW), both MAX clear or spin blocked. Current voice executor does not implement this sequence. Full contract and test matrix recorded in docs/M8_S154_VC02_INDEPENDENT_COMMAND_CONTRACT_2026_11_04.md; VC02 map 49 bytes unchanged. No new motor firmware or target/hardware PASS.

## 2026-11-04 — Pre-S15.4 VC02-I1 firmware command realization (USER priority)
[USER DIRECTIVE] Implement operator-approved VC02 commands into NEW STM32 firmware and TEST correctness FIRST. Begin S15.4 dual steering speed/arrival synchronization ONLY AFTER these command routes are verified. Do not confuse I1 independent target servo with later S15.4 common-T or physical 5..45-degree wheel mapping.
[SOURCE] Core/Inc/vc02.h + Core/Src/vc02.c append right-rotate event for A7 without reordering older IDs. Core/Inc/voice_s154_route.h pure C validates duration, maps normalized shaft angle goals bounded to LEFT/RIGHT SOFT and physical NC required for pivot; NEVER claim wheel angle physical calibrated. Core/Src/main.c splits wheel movement + D0 TIME and steer A5/A6+Bn immediately, B0 centered position, standalone C1..C5 SPEED, FF STOP/PS2 cancel both with complete neutral rearm. Rotate A4 -> R2 -> both MIN NC asserted and MAX clear -> L2 spin(-1); A7 -> R2 then L1 spin(+1), timeout/nonblocking/NC/STOP. Reuse existing protected Steering_PositionUpdate owner (command 4 for angle, 1 CENTER,2 R2), not simulated PS2 buttons. Voice wheel executor no longer stops concurrent steering. Speed sync NOT added, still independently positioned to goal.
[SAFE BOOT] voice_pos_enable=0, voice_angle_mapping_enable=0, voice_pivot_enable=0 (all new physical steering/pivot routes default OFF). No opt-in until actual CubeIDE target build/flash, real UART, independent motor power disconnect, NC/coast/current; HOST C tests do not validate hardware. Forward/back wheel behavior baseline existing.
[TEST] tools/verify_m8_vc02_i1.py real parser C+pure geometry/NC C and source invariants, tools/verify_m8_vc02_command_core.py compiles original main.c event functions with HAL stubs; workflow exact SHA. Check CI result. Full integration hardware S15.4 status remains PENDING.
[DOC] docs/M8_VC02_I1_IMPLEMENTATION_2026_11_04.md and docs/M8_S154_VC02_INDEPENDENT_COMMAND_CONTRACT_2026_11_04.md, 49-command map docs/VC02_OPERATOR_COMMAND_MAP_2026_11_04.md. Handoff sections 0..16 preserved, appended only; source 386-line original spec untouched.


[2026-11-04 VC02-I1 additional source-C HOST] Added tools/verify_m8_vc02_executor.py compiling extracted ACTUAL C Voice_StartExecution and Voice_ExecuteMotorControl plus Voice_S154CancelAll with bounded mock owners. Tests opt-in boot OFF, R2-before-SPIN, physical L/R MIN NC and MAX clear, L2 -1/L1 +1, 400CCR cap, duration only after R2, partial/missing NC abort, WAIT timeout, STOP cancel, independent wheel+steer concurrent. This is simulated flags only; not CubeIDE, real encoder, motor, NC latency or electrical cutoff evidence. No motor source changes.

[2026-11-04 VC02-I1 executor host test harness fix] Exact SHA 8ca3d8c first workflow failed only in newly added host harness: source extraction compiled Voice_S154CancelAll referencing a global that the mock omitted and an unsigned >=0 assertion flagged by -Werror. Firmware Core/Src/main.c, parser, HAL, NC and PWM are NOT modified in the harness fix. Add mock VoiceBuildResult_t voice_last_build_result and remove vacuous assertion, rerun CI exact SHA. No real hardware acceptance.


## 2026-11-04 — PLC-style VOICE command update after explicit USER request
[0] Code on new branch feature/m8-vc02-plc-latest-command from exact e96ad4f, preserving earlier branch da38014.
[1] User contract: incomplete WHEEL movement (A2/A3/A4/A7) overwritten by latest, paired with D0 XX only; incomplete STEER direction (A5/A6) overwritten by latest, paired with angle B1..B9 only. B0 independent CENTER.
[2] Supports both-part orders within 20s expiry per group, never combines stale partials. No command replay or unsanctioned active override.
[3] Wheel and steering owner independent; rotate is deliberate R2/NC then L2/L1 exception. Common-T S15.4 remains future work.
[4] C1..C5 is wheel speed setting only. F0 clears incomplete input registers, never an already committed/active motor; FF cancels all Voice.
[5] Any PS2 mapped input during Voice pending/active cancels both and consumes that action; require all 16 buttons and four axes neutral Analog frame before NEW PS2 action may move.
[6] All three Voice opt-in flags ON in experimental new branch, but steering master permit still boot OFF until PS2 START arming checks pass; retain STOP/NC/SOFT/fault/timeout and existing PWM.
[7] Added USART6 raw byte/last byte/error/rearm/begin status for distinguishing UART no-byte from parse rejection.
[8] SOURCE/CI C-host test complete only when exact SHA PASS. No CubeIDE/FLASH/POWERED PASS assumed.
[9] Keep 386 original operator lines and 17 handoff numbered sections; append history only.
[10] No automatic merging/deleting/force-pushing older feature branches. No edits to user's local D:\\STM32 task\\control_motor.
[11] User said no more essential questions; implement only explicit scope and don't claim S15.4 timing completed.
[12] Windows PowerShell UART capture M8S152 is post-disarm per-run logging, not a live VC02 raw transcript.
[13] Hardware VC-02 configuration may differ from 49 intended commands; verify captured UART bytes on actual flashed SHA.
[14] PS2 neutral and Voice STOP cannot be validated from HOST fake alone; user must observe input and outputs separately.
[15] Do not alter S15.3 LEFT deliberate slowdown/no-S15.4 physical Ackermann.
[16] Next handoff should provide exact new branch SHA, CI, target build and safe local pull commands, no claims beyond evidence.


## 2026-11-04 — Continuous PLC incomplete-register expiry fix after 8e72c740
[0] Core/Src/main.c main loop now calls Voice_PendingExpire every pass, not only when an event arrives.
[1] Wheel/steer partial input memory expires after 20s independently, active/committed owners unaffected.
[2] This is a precision fix after source tests passed at 8e72c740, preserving source safety, electrical gates and all prior 17 original handoff sections.
[3] Existing exact-SHA CI policy requires same checked range update to CONTEXT_HANDOFF and PROJECT_MILESTONES whenever source changes.


## 2026-11-04 — C1..C5 immediate live speed regulation during wheel Voice command
[0] User observation: independent Cn speed command stored successfully but old snapshot affected only NEXT wheel command, not current movement; user requires immediate update during active Go forward/backward/Rotate.
[1] Updated existing feature/m8-vc02-plc-latest-command branch based on cd20a016; not main/older branches.
[2] Voice_ExecuteMotorControl re-reads voice_speed_mode while active each cycle, changes active_command.speed_level only when valid; does not reset duration or direction.
[3] FORWARD/BACKWARD signed voice_execution_pwm computed from new level then passed to existing WheelControl_SetTargetPWM ramp/PID. No direct CCR writes.
[4] ROTATE WAIT_R2 updates speed selection but no spin; ROTATE stage2 SetSpinDirection uses updated speed and retains 400-CCR cap plus NC checks.
[5] Current levels C1 250, C2 400, C3 550, C4 750, C5 999; in Rotate C2..C5 clip at 400, not five distinguishable rotation speeds.
[6] Existing F0 pending only, FF STOP, first PS2 input cancel + neutral/rearm, independently owned steering unaffected.
[7] New voice_speed_active_update_count debug counter for applied live changes; host real-C test covers signed wheel outputs, both rotate signs, waiting before R2, time stability, STOP, NC-release cut.
[8] No target Clean Build/Flash/actual UART/motor physical speed verified; CI host alone insufficient.
[9] No S15.4 common-T sync changes in this speed fix.
[10] Keep original operator 386-line spec, 17-section handoff, log provenance, and existing milestones intact; this is an append-only project update.


## 2026-11-04 — NEW trial branch FF soft STOP / PS2 hard cancel FROM a1396e7d
[0] The user expressly requested keep a1396e7d as the known-good setpoint; new branch `feature/m8-vc02-stop-ps2-brake-swap` starts exactly at `a1396e7d568eea1e05aa0e7d54a0947436c1871d`. No reset/force push to original branch.
[1] Voice FF STOP now cancels Voice, requests existing M6 soft wheel ramp via zero target, and does NOT call Wheel_EmergencyStop on FF path. Steering owner still stops immediately.
[2] Control_Arbitrate uses STOP latch until Steering_PositionWheelsStopped confirms actual wheel halt. It does not permit automatic Voice or PS2 control during coast.
[3] First PS2 active input while Voice pending/ready/active now calls Wheel_EmergencyStop immediately; action consumed and ps2_cancel_hold requires full analog valid neutral then NEW input.
[4] PS2 input during FF soft-stop coast triggers the same emergency cut and PS2 neutral rearm; RF-loss while wheel spin brakes still triggers immediate emergency.
[5] NC/EXTI, steer safety trip, RF-loss steering, and physical cutoff remain independent emergency mechanisms; do NOT soften these.
[6] No changes to M6 PID/ramp tuning, PWM, speed commands, VC-02 packet grammar, S15.4 status or S15.3 tuning.
[7] Real C HOST test `tools/verify_m8_vc02_stop_swap.py` and CI job added. Exact SHA PASS must be checked; no claims about STM32 target build or motor physical braking without evidence.
[8] OpenAI write tool previously blocked one attempted update without an explained technical reason; this distinct new branch was successfully updated via the normal GitHub API, not by resetting original refs.
[9] Voice FF is no longer a physical emergency stop; separate hardware E-stop must remain functional.
[10] Observe `control_stop_latched`, `ps2_cancel_hold`, `wheel_target_pwm`, `wheel_current_pwm`, measured RPM before enabling powered verification.
[11] Original 386-line specification and first 17 context sections retained. These notes are additive only.


## 2026-11-04 — HOST test stub correction for STOP/PS2 swap
[0] New C compiler test originally missed pre-existing global steer_test_running, referenced in production Control_Arbitrate.
[1] Test stub now declares this owner flag. No production source or braking policy changed; rerun exact-SHA CI to confirm test outcome.


## 2026-11-04 — User-approved guarded auto-ARM and reserved PS2 START logging TODO

[0] New source is on feature/m8-vc02-auto-arm-guarded, from exact 536796e2051d6b43f0fb18532f48b85616e2100f, preserving previous branch and a1396e7d baseline.
[1] Operator says NO START hold or alternative manual control to arm: after valid restored conditions, steer_safety_motion_permit must automatically become 1.
[2] New Steering_PS2ManualArmUpdate auto-qualifies at 500ms cadence; connected valid PS2 Analog all-buttons/axes neutral, wheels stopped, initialized non-contradictory NC, BOTH AS5600 transport/magnet/envelope valid, no Voice STOP/cancel/busy owner.
[3] At boot arm flags remain zero; successful preflight raises steer_manual_enable, steer_pos_enable, steer_safety_motion_permit, steer_ps2_manual_armed to 1; ARM grants permission only, never triggers motor motion by itself.
[4] Transient pre-arm invalid NC/sensors/radio/neutral/owner auto-retries after recovery, no action from operator. After radio disconnect, disarm; a valid neutral radio packet and preflight later re-enable.
[5] Existing latched steer_safety_trip_latched is a persistent safety incident; NEVER auto-clear. Even if the physical symptom disappears, auto ARM stays off until safety incident is resolved via separate safe procedure. Distinguish transient preflight invalid from a latched hard event.
[6] START is removed from PS2_IsManualControlActive takeover and Steering_PS2ManualArmUpdate no longer uses it. The full-neutral preflight still requires START released.
[7] USER TODO FOR FUTURE COMMIT: use PS2 START to control logging (capture/start-stop/dump details to be designed later). Existing USER-button RamLogger_ButtonUpdate remains; do not claim START logging implemented.
[8] Voice FF soft ramp, PS2 takeover emergency, physical NC/fault hard stops, C1..C5 live speed, R2/NC spin and F0 pending-only unchanged.
[9] Added tools/verify_m8_vc02_auto_arm.py plus workflow exact-SHA CI; no CubeIDE target build or flash/physical success claims.
[10] Keep original 386-line operator requirements and first 17 handoff sections intact. No merges/force pushes.


## 2026-11-04 — Latest USER correction: strict primary-then-parameter VC02 command strings

[0] User reports misrecognition: VC-02 often hears only trailing D0 TIME or B angle; earlier code cached the trailing parameter and then automatically actuated when the leading primary was later spoken. This is unwanted and unsafe.
[1] NEW overriding rule: wheel command needs fresh A2/A3/A4/A7 BEFORE fresh D0 XX. A D0 XX received without a preceding wheel primary is discarded outright and never cached. New primary overwrites and clears entire incomplete wheel group (movement+duration), even if repeated.
[2] NEW overriding rule: steering requires fresh A5/A6 BEFORE fresh B1..B9. Orphan B1..B9 discarded, never cached. Every new steering primary resets whole incomplete steering group (direction+angle), even if repeated. B0 still executes standalone center R1.
[3] Preserve independent wheel and steering pending streams so A2 A5 B6 D0 0A can still complete both; do not cancel a completed/active motor owner as part of the parser change.
[4] Existing FF Voice soft wheel stop, PS2 cancel emergency, physical NC/EXTI fault protections, C1..C5 live speed updates and guarded auto-ARM from 9a315915 remain untouched.
[5] F0 continues to clear ONLY incomplete buffers; FF cancels owners as prior policy. 20s partial pending timeout retained; late parameter after expiry discarded rather than buffered.
[6] New diagnostic counts voice_orphan_duration_discard_count and voice_orphan_angle_discard_count; use VC02 raw/event counters for missed primary debugging too.
[7] New branch feature/m8-vc02-strict-ordered-phrases created from exact 9a315915129e5b577845a9e5806d27e86c467f6c; old branch and rollback points 536796e2/a1396e7d retained.
[8] HOST compiled actual C command-builder regression and source integration checks updated; exact-SHA CI required; CubeIDE build, flash and physical actuation not proven.
[9] New rule supersedes older spec lines permitting TIME-before-movement or ANGLE-before-turn: do NOT reintroduce backwards pairing just to satisfy outdated tests.
[10] PS2 START logging integration still TODO in later commit; this work does not touch logger or ARM.


## 2026-11-04 — CI follow-up: exact static check formatting synchronization

[0] After initial strict-order parser implementation, tools/verify_m8_vc02_i1.py static predicate compared `voice_pending_command.duration_valid=0U;` / `angle_valid=0U;` without the spaces present in actual production source.
[1] Updated only the static test predicates to match `duration_valid = 0U;` and `angle_valid = 0U;`. No motor behavior, parser semantics or safety gates changed in this follow-up.
[2] Keep strict movement-first TIME and turn-first ANGLE semantics, missing primary parameter discarded, independent owners, F0/FF and guarded auto-ARM.
[3] Exact SHA HOST CI still required. Do not claim target STM32 build/flash or real motion tests.


## 2026-11-05 — LATEST USER: Voice 45 MUST reach physical NC; 5–40 interpolate full calibrated stroke

[0] Requirement confirmed: 0=existing CENTER, 5/10/.../40 uses linear fraction of CENTER->calibrated physical MIN/MAX, 45 uses the corresponding own physical NC endstop for each steering motor. Do NOT call the manual soft endpoint 45° anymore.
[1] Voice LEFT45 (A5 B9) expected L_MAX + R_MIN. Voice RIGHT45 (A6 B9) expected L_MIN + R_MAX. This is a convention that NC marks ±45, not a separate physical angle measurement.
[2] New feature/m8-vc02-45deg-nc-endstops branch rooted at b407c0dbb9f40c2843763483f55ce5bee936990b; prior strict-parser, auto-ARM and brake-swap commits/branches preserved.
[3] Source Core/Inc/voice_s154_route.h maps angles 5–45 to CENTER + scaled delta toward runtime CALIBRATED MAX/MIN raw landmarks. For 45, target raw is approach safety BOUND only; actual completion requires own expected NC, never merely target RAW.
[4] New STEER_POS_VOICE45_NC=5 is only for Voice45; steering angle5..40 stays command4, B0 CENTER stays command1, Rotate/R2 stays command2. No alteration of voice UART grammar.
[5] Steering_PositionExpectedNc/Steering_PositionAtExpectedNc reject wrong/dual/absent NC. All initialization, early IRQ, late IRQ and interrupt-masked GPIO checks use the same decision. Each side immediately cuts its own PWM; both left_result and right_result must be 3 for successful Voice45.
[6] Steering_Voice45RawBoundaryReached fails CLOSED if AS5600 reaches calibrated end before NC, no driving beyond potentially failed endstop. Ordinary 5s timeout/stall/sensor envelope and NC EXTI unchanged; no PWM/power cap increases.
[7] Voice45 physical seek requires wheels stopped, unlike 5..40. STOP/PS2 cancellation and physical safety latch retain existing priority.
[8] R2 still accepts own MIN only, not MAX. Ordinary 5..40 still reports failure if unexpectedly touching NC. Unexpected NC never becomes a safe arrival.
[9] Test tools/verify_m8_vc02_45_nc.py compiles real C NC helper functions and checks actual main.c wiring; test tools/verify_m8_vc02_command_core.py and tools/verify_m8_vc02_i1.py updated. Exact SHA PASS required; HOST tests do not equal target build, flash or powered hardware.
[10] Physical commissioning risk: NC may be after an outdated runtime landmark or mechanically too late to prevent coast; do not loosen limits, disable EXTI or increase drive just to force switch. Validate switch and position with motor power removed before a controlled supported bench test.
[11] Prior C1..C5 live wheel speed, strict primary-then-parameter parser, FF soft STOP, first PS2 takeover hard STOP, guarded auto-ARM and START logging TODO remain untouched.


## 2026-11-05 — Critical additional Voice45 isolation from R1/R2 PWM pacing
[0] During control-path audit found Steering_PositionUpdate's R1/R2 shared waypoint branch originally used condition steer_pos_command != 4U, so new mode5 would incorrectly enter and pace toward BOTH R2 MIN raw endpoints even for LEFT45 L_MAX+R_MIN or RIGHT45 L_MIN+R_MAX.
[1] Production source now also excludes STEER_POS_VOICE45_NC from both Steering_AngleWaypointApply and Steering_PhaseLimit R1/R2 paths. 45 uses only independent NC-position axes and existing per-axis PWM limit/stall/timeout. No new timing/PWM increase.
[2] Added static test assertion that excludes mode5 from R1/R2 waypoint path while preserving original R1/R2 behavior and hardware NC preflight.
[3] Requires exact SHA HOST CI and, independently, STM32CubeIDE Clean Build, physical endstop checks and suspended chassis acceptance before powered use.


## 2026-11-05 — HOST test forward declaration extraction correction
[0] Exact SHA CI test tools/verify_m8_vc02_45_nc.py initially selected the forward declaration of Steering_SafetyExpectedNcMask(), not the later C function definition; assertion therefore checked the wrong body.
[1] Fixed the test signature to include the opening brace after the full function name, reading only the real implementation. Production motor source unchanged by this follow-up. Re-run exact SHA CI.
## 2026-11-06 — CHUYỂN M9: active torque hold, cập nhật đầy đủ khung 0–16

[0] **Nguồn/giới hạn:** USER trao đổi 2026-11-06: muốn giữ trục bằng mô-men motor DC chổi than, ngại phanh bật ngược lúc tải mất; không đầu tư thêm phần cứng; yêu cầu branch M9 từ d367618d và cập nhật MD. Repo/firmware M8, manual CCS_SHB12 có sẵn. Chưa có log HOLD hay bằng chứng điện/nhiệt; đây là quyết định phạm vi và docs, không phải biên bản nghiệm thu.

[1] **Điểm tiếp quản:** Repository Spring-Mustache/control_motor, nhánh **feature/m9-steering-active-position-hold**, base **d367618d4d9c4c382a9a7208f9b878dfefa21ace**. Truy vấn HEAD lúc tiếp quản, không dùng base SHA làm HEAD sau khi commit MD. M9 chỉ docs plan; bắt đầu rà soát motor owner/idle sample rồi triển khai H0 passive nếu được giao phát triển.

[2] **Mục tiêu/phạm vi:** Giữ góc đã hoàn tất trước ngoại lực bằng servo PD AS5600 và PWM/DIR CCS_SHB12, trên hai trục lái, tận dụng STM32F767ZI; không thay hardware. Tách giữ góc có cấp mô-men với khóa cơ khí khi mất nguồn; chưa hứa giữ cứng mọi tải.

[3] **Yêu cầu điều khiển:** Không cấp PWM cố định một chiều coi là khóa hai chiều. Phải có phản hồi vị trí + vận tốc, không dùng I trong bản đầu, giảm lực khi tải mất, không tranh PWM/owner M8; user chưa cấp đặc tả sai số/ngoại lực nên thông số PD/duty/time TBD.

[4] **Roadmap:** M9-H0 read-only đo drift, M9-H1 shadow PD PWM0, M9-H2 ACTIVE opt-in giới hạn lực và nghiệm thu giá treo, M9-H3 tích hợp PS2/VC-02/wheel và Gate an toàn/tải. Tất cả **PLANNED**. M8 verified baseline S15.4/S15.6 chỉ sáu Voice 15/30/40, RIGHT CENTER d367 chưa physical pass. Đọc kế hoạch MD chuyên biệt.

[5] **Đã làm thực:** Tạo nhánh M9 từ exact d367, kiểm tra HEAD ban đầu. Cập nhật tài liệu M9, README, startup prompt, handoff, roadmap; không thay `Core/Src/main.c`, PWM/NC/STOP hoặc script thử. Khi ghi lại commit docs mới phải xác nhận GitHub HEAD/CI riêng.

[6] **Đang dở:** H0 chưa code/test; H1/H2/H3 chưa thiết kế/thử nghiệm chi tiết. Không có kết luận driver PWM0 brake hay coast. Chưa số liệu drift, load-step, nhiệt/current; chưa tiêu chí độ cứng cuối cùng. M8 RIGHT CENTER trial tách biệt vẫn pending operator log.

[7] **Tệp:** `CONTEXT_HANDOFF.md`, `docs/PROJECT_MILESTONES.md`, `docs/M9_STEERING_ACTIVE_POSITION_HOLD_PLAN_2026_11_06.md`, `docs/CHAT_B_START_PROMPT.md`, `README.md`, `docs/CONTEXT_RELEASE_POLICY.md` và nguyên bản đặc tả 386 dòng; `Core/Src/main.c` read-only khi lên kế hoạch. Source M8 R1 trial `docs/M8_R1_VOICE_CENTER_RIGHT_LATE_FINE_2026_11_05.md`.

[8] **Quyết định:** User chọn phần mềm, không thêm phanh điện từ/cơ khí, không thay driver trong giai đoạn này. Chọn giải pháp servo PD + observer, không PWM cố định; lý do: có thể chống ngoại lực hai hướng và hãm bằng tốc độ, nhưng phải xác minh tính thực dụng. Driver không công bố BRAKE mode; không kết luận driver chắc chắn không thể brake.

[9] **Rủi ro kỹ thuật:** Motor stalled nóng do I²R; PWM không phải cảm biến dòng/mô-men; backlash, độ trễ ACCE/DECCE, AS5600 noise/stale/wrap, bật ngược khi tải đột ngột biến mất, cấp PWM khi đã NC/STOP hoặc cướp quyền manual/voice. Chưa gán nguyên nhân thực nghiệm khi không có log.

[10] **Giả định/phụ thuộc:** Có thể đọc AS5600 trong idle nhưng tốc độ poll hiện ~50ms xen kẽ (~100ms mỗi trục) chưa đủ bảo đảm PD. Cần hiểu điện đầu ra CCS_SHB12 khi CCR0, dải motor an toàn và vị trí lắp encoder. Không dùng trị PWM position M8 làm ngưỡng giữ an toàn mà không đo.

[11] **Hành động ưu tiên:** Rà soát `Steering_PositionFinish`, `Steering_PositionAxisDone`, `Steering_PositionTelemetryIdleUpdate`, NC/STOP arbitration, status I2C; thiết kế dữ liệu H0 + unit test không phát motor; tiến lên H1 shadow sau xác nhận, rồi H2 thử có ngắt nguồn động lực độc lập. Không lập lịch thử chạy powered như đã được phép.

[12] **Câu hỏi mở:** CCS_SHB12 tại PWM0 coast/brake? Có dòng/nhiệt stalled mức nào? RAW tương ứng độ bánh thực và độ rơ bao nhiêu? tải/overshoot cho phép? Dữ liệu này cần đo khi tới cổng H2, không bắt user chẩn đoán trước H0.

[13] **Dòng thời gian:** 2026-11-05 M8 S15.4/S15.6 6/6 user PASS; 2026-11-05 RIGHT R1/Voice CENTER hunting observed và thử 10RAW source d367; 2026-11-06 trao đổi mô-men chống lực, rủi ro bật ngược, chọn chỉ lập trình; 2026-11-06 tạo nhánh M9 exact base và yêu cầu cập nhật roadmap.

[14] **Phụ lục kỹ thuật:** `T≈Kt*I` ở motor đứng yên, `u=Kp*error-Kd*angular_velocity` ở hold. `STEER_POS_TOL_RAW=5` là dung sai position hiện hành, không tự chứng minh dung sai HOLD hay bánh. LEFT CENTER1886, RIGHT CENTER662 RAW mặc định, runtime endpoints có thể cập nhật. H0/H1 không can thiệp PWM.

[15] **Chỉ dẫn tiếp quản:** Lấy HEAD nhánh M9 thật, không dùng mặc định HEAD main/M8; đọc 17 mục và cập nhật cuối, roadmap M9, M9 plan và diff/CI đúng SHA. Không mặc nhận yêu cầu cũ đã biến mất: NC/STOP/PS2/VC02/motor arbitration phải giữ; user chỉ muốn đầu tư phần mềm.

[16] **Kiểm tra/giới hạn:** M8 6-case USER PASS khác với M9 chưa triển khai. Bản MD cập nhật chỉ là tài liệu. Chưa bằng chứng CubeIDE/Flash/HOLD/current/stall/load; thiếu log riêng tư của trial R1. Original `docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md` 386 dòng/SHA phải giữ bất biến, footer giữ nguyên. Nếu CI docs PASS, nó chỉ xác thực source/regression/contract ở SHA đó, không chứng minh giữ góc trên xe.

## 2026-11-06 — M9-H0 implementation handoff, đủ khung mục 0–16

[0] **Nguồn:** USER yêu cầu thực hiện mốc tiếp theo M9-H0; repo HEAD nền bbfff0699f7473d6b6dbc45bc28c3d6fd56ef237. Code mới chỉ tạo quan sát sau position DONE, không thay đổi bộ điều khiển vị trí, quyền motor, giới hạn NC/STOP hay dòng điện.

[1] **Tiếp quản ngay:** Branch feature/m9-steering-active-position-hold, xác minh HEAD mới nhất; H0 source đã được triển khai, CI đúng SHA cần xem, chưa có MCU Build/Flash/UART logfile. Hành động gần nhất: CI, rà source grep motor-write exclusions, bảo vệ local worktree khi CubeIDE Clean Build/Flash.

[2] **Phạm vi:** M9-H0 post-DONE read-only drift, không Active Hold, không motor torque/PWM. Cửa sổ 3000ms chỉ R1 center hoặc Voice mode4 5–40° đều hai motor result2; NC endpoint/STOP/fault không khởi tạo H0.

[3] **Yêu cầu người dùng:** Chỉ phát triển chương trình STM32/CCS_SHB12/AS5600, không đầu tư phanh mới. Log bằng PowerShell, tránh gán dữ liệu không có. PWM giữ trục luôn OFF ở H0.

[4] **Roadmap:** H0 SOURCE-STAGED / await exact-SHA CI + CubeIDE Build/Flash/log; H1 pure/shadow PD chưa triển khai; H2 ACTIVE bounded boot OFF chưa được phép motor; H3 PS2/Voice/wheels integration chưa triển khai. Đọc docs/M9_H0_PASSIVE_DRIFT_2026_11_06.md và M9 plan.

[5] **Thực hiện:** New pure-C Core/Src/m9_h0_drift.c và header; hooks StartAfterDone và IdleUpdate vào main; new M9H0_DRIFT one-row UART3 safe idle; tools/verify_m9_h0_drift.py GCC exact module, native Windows PowerShell collector + workflow syntax step; tài liệu + roadmap + handoff cập nhật cùng commit.

[6] **Chưa làm:** Không có vòng PD, không H2 PWM, không H3 tích hợp, chưa đo drift thật, chưa Clean Build/Flash hoặc xác minh tác động jitter/độ trễ STM32. Bản H0 không đánh dấu HOLD PASS.

[7] **Tài nguyên:** Core/Src/main.c, Core/Src/m9_h0_drift.c, Core/Inc/m9_h0_drift.h, tools/m9_h0_host_test.c, tools/verify_m9_h0_drift.py, tools/capture_m9_h0_drift.ps1, docs/M9_H0_PASSIVE_DRIFT_2026_11_06.md, docs/M9_STEERING_ACTIVE_POSITION_HOLD_PLAN_2026_11_06.md, .github/workflows/context-handoff-release.yml; original 386-line source unchanged.

[8] **Quyết định:** Không gọi I2C mới ở H0; dùng dữ liệu cập nhật khi idle (50ms luân phiên ~100ms/side), không live UART khi đang chạy, chỉ một record tổng kết và không overwrite unsent; mở rộng thành time series H1 nếu cần xác định damping, nhưng không tự áp dụng PWM.

[9] **Rủi ro:** Thời gian idle poll/AS5600 không đều, tốc độ delta RAW chỉ là ước lượng tốc độ theo mẫu, chưa đo load; log bị trễ do bánh xe/motor còn chạy, pending có thể drop session sau; ST-LINK UART3 TX không chứng minh driver thật cắt điện hay nhiệt an toàn.

[10] **Giả định:** Cơ cấu sử dụng AS5600 12-bit và các mốc M8 hiện hành; lỗi magnet/status được ghi invalid; packet có MCU SHA. Số count RAW không quy trực tiếp độ lái thực khi chưa biết truyền động.

[11] **Bước kiểm tra:** Chạy CI exact SHA; trên Windows bảo vệ working tree, fetch/switch M9, xác nhận SHA, CubeIDE Clean Build/Flash, preflight NC/STOP ngắt nguồn motor. Chỉ sau khi cơ cấu được bảo vệ, thực hiện lệnh vị trí M8 đã có, chờ ≥3s idle, chạy collector PowerShell để lấy M9H0_DRIFT; không cần tác động ngoại lực cho test đầu tiên.

[12] **Vẫn cần biết:** PWM0 coast/brake? RAW drift khi idle bao nhiêu? Có sample invalid/jump hoặc drop? Clock/sample gap có đủ cho PD? Data chưa có đến khi user gửi private TXT.

[13] **Timeline:** USER tạo branch M9 từ d367618d, commit docs HEAD bbfff069; USER nói 'hãy thực hiện', nghĩa triển khai H0; source H0 thụ động ghi một summary mỗi lệnh DONE, CI và hardware cần xác minh sau commit.

[14] **Giá trị đo:** mode1 CENTER (goalL1886,goalR662 mặc định); Voice mode4 sử dụng mục tiêu `voice_pos_goal_l/r`. `reason=1` hết 3000ms, 2 chủ khác, 3 STOP/fault. `peakDrift{L,R}` là |AS5600 RAW from initial| có wrap 4096; `peakErr` là max |RAW-goal|, `peakVel` là RAW/s từ hai mẫu hợp lệ. Missing-sample age=4294967295, không tự coi bằng0.

[15] **Chuyển sang chat tiếp:** Luôn kiểm tra branch HEAD và Actions SHA, H0 là read-only; chỉ mở H1 sau test data, không cầm nhầm M8-S15.4 USER PASS thành M9. Lệnh PowerShell collector trong docs/M9_H0_PASSIVE_DRIFT_2026_11_06.md, không gửi log private lên GitHub.

[16] **Bằng chứng/giới hạn:** local GCC pure drift tracker có PASS trước GitHub; exact-SHA GitHub CI kiểm tra riêng sau push. Chưa chứng thực CubeIDE target/flash, không có kết quả idle drift, không tăng dòng/khóa trục. Handoff original 386-line giữ nguyên; các 17 section lịch sử và footer vẫn nguyên vẹn.

## 2026-11-06 — M9-H0 có private MCU UART: cập nhật 17 mục theo bằng chứng mới

[0] Nguồn là bốn dòng M9H0_DRIFT người dùng cung cấp trong hội thoại riêng, không phải ảnh/chứng nhận build; không copy raw TXT lên repo. So sánh firmware stamped SHA với source+CI cùng SHA.

[1] Tiếp quản: nhánh feature/m9-steering-active-position-hold, nguồn H0 tại 8c2b3fe7d57154db7d11f8abca40351c4753d362, HEAD sau commit docs phải xác minh lại; H0 thu log trên MCU đã hoạt động bốn cửa sổ.

[2] Mục tiêu vẫn software-only steering Active Hold, mới có H0 passive đo drift sau DONE, chưa có PD/torque.

[3] User gửi log thay vì khẳng định PASS; assistant phân tích có nguồn và phân biệt H0 capture với hiệu quả phanh/giữ lực, giữ private log ngoài public repository.

[4] Roadmap: M9-H0 SOURCE/HOST CI/4-window MCU DATA CAPTURE PASS (giới hạn static); H1 SHADOW PD là bước kế tiếp; H2 ACTIVE opt-in và H3 integration chưa triển khai.

[5] Quan sát xác minh từ file: 4/4 seq1..4, MCU sha 8c2b3fe7..., duration3002..3003ms, source3 ba lượt/source2 một lượt, 268..269 mẫu/trục, peakDrift tối đa1 RAW/trục, max peakErr5 RAW, bad/jump/zeroDt/drop0, wheelsMoved0; không có PS2 source1.

[6] Còn dở: chưa biết ảnh hưởng ngoại lực khi tải tác động/ngắt đột ngột; không có dòng motor, nhiệt, PWM0 coast/brake; không có chứng cứ độc lập CubeIDE Build/Flash hoặc test NC/STOP trong lượt H0 này.

[7] Đọc docs/M9_H0_PASSIVE_DRIFT_2026_11_06.md, docs/PROJECT_MILESTONES.md, Core/Src/m9_h0_drift.c, Core/Src/main.c, tools/capture_m9_h0_drift.ps1. Log đính kèm hội thoại là private, không phải file GitHub.

[8] Quyết định: ghi H0 DATA CAPTURE PASS giới hạn, không nâng thành H0 powered-hold PASS và không tự triển khai H2. H1 phải có đánh giá lượng tử/derivative noise trước khi cấp lực.

[9] Rủi ro mới: peakVel 90 RAW/s có thể do sai biệt 1 RAW qua khoảng11ms; dùng trực tiếp cho PD sẽ gây jitter; không gán thành chuyển động thực lớn.

[10] Tại firmware nguồn, khi motion permit1 và chưa trip, safety loop đọc hai AS5600 mỗi vòng; tốc độ đo ~89Hz/side cao hơn idle-only 50ms alternating. H0 chỉ nhận cached samples, không làm phát sinh I2C.

[11] Kế tiếp: thiết kế H1 SHADOW PD có lọc velocity 50–100ms/differentiator robust, hysteresis, dt/stale guards, wrap-around, test release-load profile bằng HOST, hoàn toàn không ghi CCR/DIR. Phân biệt test điều kiện mới khi có log thích hợp.

[12] Còn mở: độ cứng chịu ngoại lực cần bao nhiêu, trạng thái CCS_SHB12 PWM0, motor current/thermal, vị trí encoder trước/sau backlash, mục tiêu sai số có tải và điều kiện được phép powered.

[13] Dòng thời gian: 2026-11-06 tạo branch/docs M9; 2026-11-06 commit 8c2b3fe7 H0 CI PASS; cùng ngày người dùng gửi bốn bản ghi UART 3s, phân tích static drift.

[14] Tổng hợp gọn: driftL/R tối đa1 RAW, peakErrL/R tối đa5 RAW, seq1..4 reason1, source2/3, không source1; 90 RAW/s peak không phải vận tốc duy trì. Dữ liệu chưa kiểm nghiệm tiêu chí chống ngoại lực.

[15] Chat sau đọc đầu handoff, mục này, roadmap, H0 log summary và exact SHA CI. Đừng yêu cầu lại dữ liệu đã có, nhưng cần proof mới để chốt HOLD ACTIVE.

[16] Giới hạn: 4 static windows và MCU stamped SHA; user-supplied UART chưa là independent proof build/flash or 2-sided load. Bằng chứng không chứng minh NC/STOP, current, actual torque. Đặc tả 386 dòng và hồ sơ 0–16 giữ bất biến.

## 2026-11-06 — M9-H1 PD SHADOW SOURCE implementation; cập nhật đầy đủ khung 0–16

[0] **Nguồn:** USER yêu cầu thực hiện H1 sau khi 4 log M9-H0 đã được phân tích; repo base trước thay đổi source là `20bb0a98a06fe422d4933768e5cb0b61f71ff4ff`. Không công khai TXT thực tế, không tự giả mạo load-step on-MCU.

[1] **Mốc hiện hành:** nhánh `feature/m9-steering-active-position-hold`, cần xác minh HEAD sau commit; H0 4-window static MCU capture PASS giới hạn; H1 mới SOURCE-STAGED, H2/H3 pending.

[2] **Mục tiêu:** PD vị trí + tốc độ bằng dữ liệu AS5600 để thiết kế mô-men giữ góc không văng khi tải đột ngột biến mất, nhưng H1 chỉ tính đơn vị SHADOW, không tác động driver CCS_SHB12.

[3] **Ràng buộc USER:** không thêm phần cứng/phanh. Giữ nguyên hành vi M8 vị trí, NC/STOP, bảo vệ motor, quyền PS2/Voice; không cần người dùng điều chỉnh debug flags. H1 bật `m9_h1_shadow_enable=1U` chỉ tính toán, không mở nguồn motor.

[4] **Roadmap:** H0 SOURCE/HOST CI + private 4-log DATA CAPTURE PASS (static); H1 compile pure C / host simulation + safe-idle UART pending exact CI/target. H2 bounded ACTIVE và H3 wheel-drive integration CHƯA triển khai.

[5] **Code triển khai:** pure C `m9_h1_shadow.c/.h`; hook `M9H1_Begin` sau H0 hợp lệ, `M9H1_Feed` sau từng cached sensor success/fail, `M9H1_Tick` stale, `M9H1_Finish` khi hết cửa sổ/bị STOP hoặc owner mới. Log `M9H1_SHADOW` ở `M8S11_StreamUpdate` khi safe idle.

[6] **Việc chưa hoàn thành:** CubeIDE Clean Build, Flash, UART H1 real log chưa có; chưa thử ngoại lực thực, dòng/nhiệt motor hoặc PWM0 coast/brake. H1 không kết luận độ cứng giữ góc.

[7] **Tệp:** Core/Src/main.c, Core/Src/m9_h1_shadow.c, Core/Inc/m9_h1_shadow.h, tools/m9_h1_host_test.c, tools/verify_m9_h1_shadow.py, tools/capture_m9_h1_shadow.ps1, docs/M9_H1_PD_SHADOW_2026_11_06.md, .github/workflows/context-handoff-release.yml, H0 doc/plan/roadmap/handoff/original 386-line req.

[8] **Quyết định thiết kế:** 100ms derivative baseline, ≥2RAW net mới tạo velocity, stale>250ms loại; vị trí enter8/exit4, deadband5RAW, Q8 `Kp=256,Kd=128`, cap ±40 shadow units. Không có integral, không được dùng cap này làm duty H2. Cửa sổ thử H1 chỉ 3s theo H0 và mỗi khi DONE2.

[9] **Rủi ro:** 1 RAW/11ms từ log H0 tạo đỉnh velocity90RAW/s giả; filter 100ms chậm tín hiệu hơn, khó khăn khi tải mất cực nhanh. Encoder wrap/noise/độ rơ và ACCE/DECCE driver chưa đo; shadow thành công không bảo đảm active no-overshoot.

[10] **Phụ thuộc:** sample loop thực ~89Hz trong log H0 khi motion permit1, nhưng có thể thưa hơn; lọc dt và stale nên không giả định 100Hz luôn có. NC/STOP owner guards thừa kế H0, chỉ thêm tính toán và UART idle.

[11] **Kế hoạch kiểm tra:** exact SHA CI Linux GCC actual-C + Windows PowerShell parser; trên STM32 nếu người dùng muốn kiểm chứng, bảo vệ local worktree, git fetch/switch, CubeIDE Clean Build/Flash đúng SHA, NC/STOP preflight ngắt nguồn động lực, chạy lệnh vị trí đã an toàn, thu H0 + H1 3s qua PowerShell; không gây lực ngoài mạnh khi H1 chưa có tác dụng giữ.

[12] **Câu hỏi mở:** độ trễ phản lực, mô-men motor ở stall, phát hiện tải giải phóng trong thực tế, tỷ số RAW→góc bánh, an toàn dòng liên tục; chưa yêu cầu trả lời trước kiểm thử shadow.

[13] **Lịch sử:** 2026-11-05 M8 S15.4/S15.6 6/6 user PASS; 2026-11-06 nhánh M9/H0 và private 4-window static capture; 2026-11-06 USER yêu cầu thực hiện H1, source shadow được bổ sung.

[14] **Số liệu/công thức:** H0 tĩnh peak drift ≤1RAW, peakVel90RAW/s có thể 1RAW/11ms; `e=shortest(measured,goal)`, `v=shortest(ref,now)*1000/dt` với dt≥100ms & |delta|>1; `u_shadow=clamp(Kp*e_eff-Kd*v,±40)`, Q8 256/128. Đơn vị u là **shadow score**, không điện áp/PWM/mô-men.

[15] **Tiếp quản:** không làm lại 4 H0 logs; đọc M9_H1 plan, H0 data summary, CI exact SHA, code và diff; nếu sửa code mới cập nhật cả handoff 0–16 và roadmap. H2 vẫn không được bật.

[16] **Giới hạn chứng cứ:** HOST synthetic release-load không phải tải thật; H1 output chỉ UART/candidate trong RAM, không chứng minh NC/STOP thực nghiệm, CubeIDE flashed binary, current/thermal hoặc angle lock; original 386 dòng giữ nguyên, không sửa M8 tested branch.

## 2026-11-06 — M9-H1 exact-C CI first attempt: signedness compile fix

[0] Source independent CI run 42237554302 on SHA 524ab68 flagged `-Werror=sign-compare` at `m9_h1_shadow.c:62` in `M9H1_Candidate` when signed `int32_t e` was compared to unsigned `M9H1_DEADBAND_RAW`.
[1] Corrected by explicitly casting the threshold and e_eff arithmetic to `int32_t`, preserving negative-error behavior without relying on unsigned wrap.
[2] This was an **HOST GCC compiler finding**, not a result of STM32 motor/servo test. Other Windows PowerShell parser and handoff contract jobs succeeded at first SHA. Full new SHA CI is still required before any on-target test.
[3] No motor output was introduced; no H2 active hold. Existing 17-section historical handoff preserved.

## 2026-11-06 — M9-H1 kết quả HOST CI sau sửa sign-compare (17 mục)

[0] Ghi nhận kiểm tra trên đúng GitHub SHA 7d8c98e14a0fd0cc35fc95c1a45d51988d464c2b, Actions 42237917589 completed success, không có mới MCU log.
[1] Mốc bàn giao: nhánh feature/m9-steering-active-position-hold, H1 SOURCE + HOST CI PASS, chờ CubeIDE/Flash/private UART; kiểm tra HEAD commit docs mới.
[2] Mục tiêu giữ góc chống lực vẫn chưa nghiệm thu: software-only PD SHADOW, H2/H3 chưa code.
[3] User yêu cầu triển khai H1, đã được thực hiện đúng phạm vi không cấp lực. Không hỏi lại H0 log 4 dòng đã có.
[4] Roadmap H0 static data PASS giới hạn, H1 HOST CI PASS, H1 on-target pending; H2 ACTIVE/H3 integration future.
[5] Chạy C thật với GCC -Wall/-Wextra/-Werror, synthetic load-release sign reversal/damping PASS, PowerShell syntax và workflow context PASS.
[6] CubeIDE target build/flash, H1 MCU telemetry, tốc độ thực under load/ngoại lực release, current/thermal/PWM0 coast/brake vẫn chưa có bằng chứng.
[7] Source exact 7d8c98e, workflow run 42237917589, docs/M9_H1_PD_SHADOW_2026_11_06.md, tools/verify_m9_h1_shadow.py, tools/capture_m9_h1_shadow.ps1.
[8] Không biến shadow units thành PWM vật lý. Giữ Kp/Kd là giả định simulation, không sử dụng qua driver.
[9] Rủi ro lượng tử 1RAW/11ms, derivative window100ms, dynamics chưa đo, delay ACCE/DECCE, NC/STOP kiểm chứng vật lý còn thiếu.
[10] Sensor H0 tần suất ~89Hz/trục lúc permit1, không bảo đảm fixed sampling hay safe H2 realtime.
[11] Next: bảo vệ local files, fetch/switch M9, CubeIDE Clean Build/Flash và NC/STOP preflight nếu operator muốn; dùng PowerShell capture M9H0+M9H1, so SHA và gửi riêng TXT.
[12] Câu hỏi thực nghiệm: real load-release effect, torque/current/thermal, sensor ratio/backlash và driver PWM0; không cần trả lời trước khi có H1 passive log.
[13] Timeline: H0 4 static real records; H1 first CI 524ab68 failed compiler signedness; 7d8c98e fixed and full CI success.
[14] PD shadow Kp_q8=256, Kd_q8=128, hyst 8/4 RAW, deadband5 RAW, speed window100ms/1RAW suppression, stale250ms, cap±40 arbitrary units.
[15] Chat mới tiếp tục từ HEAD M9 và H1 code; không chốt hardware PASS từ CI hoặc tư vấn powered H2 khi chưa có bảo vệ thực nghiệm.
[16] Đúng bằng chứng: SOURCE + HOST CI PASS, hardware HOLD PASS chưa có; original 386-line source và 17 section/footer vẫn được giữ.

## 2026-11-07 — M9-H1 private MCU log: cập nhật đủ 0–16

[0] Người dùng gửi file riêng `M9H1_shadow_20261107_175758.txt`; 8 lines có 4 H0 + 4 H1. Không push raw TXT public.
[1] Handoff nhánh feature/m9-steering-active-position-hold; source firmware đã flash tự báo SHA `00ccaeace99a6948880ecf85061dea5d2f77514e`. HEAD sau commit docs sẽ khác, phải check GitHub. H1 on-MCU UART static capture PASS giới hạn, not active HOLD.
[2] Mục tiêu dài hạn software-only active hold; H1 chỉ shadow PD mô phỏng, chưa cấp điện để giữ trục.
[3] User đã tự build/debug và thu 4 lượt theo script, không cần lặp hỏi SHA hay yêu cầu gửi lại 8 dòng.
[4] M9-H0 static 4/4 pass trước; M9-H1 static MCU 4/4 pairs capture pass; H2 bounded active/H3 integration chưa làm, H1 dynamic load response chưa test.
[5] Số đo: all 4 reason1, 3000..3010ms, samples 294..296/side, H0 peakDrift≤1RAW, H0 peakErr≤6RAW, H1 peakVel0, peakCmd0, noise-suppressed total28, invalid/stale/jump/zeroDt/dropped=0. Voice source3 x3, source2 x1.
[6] Chưa có H1 candidate nonzero, reversal/brake dưới tải, current/thermal, CCS_SHB12 coast/brake, explicit CubeIDE Clean Build command/log và NC/STOP under load.
[7] Docs: docs/M9_H1_PD_SHADOW_2026_11_06.md + docs/PROJECT_MILESTONES.md + source Core/Src/m9_h1_shadow.c. Log origin private conversation, not repo.
[8] Kết luận: xác minh H1 filters 1RAW static jitter; peakCmd0 vì |e|≤6 < enter8, không nói PD giữ trục hiệu quả hoặc thiếu công suất.
[9] Đỉnh H0 velocity100RAW/s tương thích 1RAW/10ms, H1 rejects; lượng tử và cửa sổ >=100ms có thể trì hoãn phản ứng khi load thật đột ngột, rủi ro chưa được đo.
[10] Tốc độ mẫu ~98Hz/side lần này từ firmware safety loop hiện có, không guarantee invariant dt, H1 stale/invalid gates vẫn cần.
[11] Next: thiết kế dynamic stimulus SHADOW >8RAW với mô phỏng/HOST có tiêu chí, hoặc bench protected nếu user sẵn sàng; đừng cho H2 lên motor cho đến khi có dữ liệu phản lực, cap current/thermal và ưu tiên STOP/NC đã kiểm chứng.
[12] Vẫn chưa biết allowable motor current/stall duty, backlash/encoder mounting, real response to unloading, actual external torque, PWM0 brake/coast.
[13] Timeline: M9-H1 firmware/source HOST CI completed at SHA 00ccaeace; 2026-11-07 user sends MCU 4 paired records; repository docs updated with accurate evidence boundary.
[14] 4 records: peakDrift 0..1RAW, peakErr0..6RAW, peakVel H1 0, peakCmd0, noise sum28; 8 independent axis windows but not mechanical load tests; enter8/exit4 RAW is current shadow setting.
[15] Khi chuyển chat, verify branch HEAD, history source firmware SHA different from later docs commit, read H1 plan and raw analysis summary; do not ask data again.
[16] Evidence SOURCE/HOST CI and MCU UART static capture only; independent CubeIDE/Flash logs absent; no active holding and H2 not allowed to infer PASS. Historical original requirements 386 lines retained, footer unchanged.

## 2026-11-07 — M9-H1 REAL DRIVE CAPTURE development; 17-section trace

[0] USER yêu cầu thực hiện một lần đo dữ liệu thật trên xe để hiệu chỉnh H1, không mô phỏng và không buộc Live Expressions/đẩy cơ cấu.
[1] HEAD nền `18961493b9a3eb551825e236d46ec9c5f99fca5b`; nhánh feature/m9-steering-active-position-hold. Sau commit phải xác minh HEAD và full Actions. Source H0/H1 predecessor MCU SHA `00ccaeace` đã có 4 lượt static-data pass giới hạn.
[2] Đáp án thực thi: flight recorder thụ động đo motor M8 đánh lái thật + AS5600, tính u_shadow không áp dụng; không mô phỏng chuyển động bằng dữ liệu giả trong firmware.
[3] Người dùng chỉ cần giữ project STM32CubeIDE hiện có, bảo vệ local changes trước pull, flash đúng SHA mới, kiểm tra NC/STOP, chạy 4 Voice/R1 commands cũ trong cùng collector PowerShell; không mở Live Expressions hoặc tự sửa flags.
[4] M9 H0 static 4/4 PASS, H1 static 4/4 PASS, H1 real motion data tool SOURCE-STAGED pending CubeIDE/flash/MCU log, H2 active/H3 chưa làm.
[5] New `m9_h1_real_trace.c/.h` RAM recorder 80 frames, sampled 80ms while M8 owner1/4 moving, DATA: t/rawL/R/errorL/R/vL/R/uL/R, measured CCR L/R, sample age, NC mask, state; BEGIN/FRAME/END UART3 safe-idle plus H0/H1 logs.
[6] No extra AS5600 ReadRaw call, no new PWM set, DIR GPIO change, owner command or delays. UART only after physical motors/wheels stopped, prior M8/H0/H1 summaries priority. CubeIDE/Flash logs and actual M9H1_REAL records unavailable until operator runs.
[7] files: Core/Src/main.c, Core/Src/m9_h1_real_trace.c, Core/Inc/m9_h1_real_trace.h, tools/m9_h1_real_host_test.c, tools/verify_m9_h1_real.py, tools/capture_m9_h1_real.ps1, docs/M9_H1_REAL_DRIVE_CAPTURE_2026_11_07.md, workflow and original H0/H1 docs.
[8] H1 u not motor PWM; telemetry uses real M8 CCR but no force applied by H1. High u during moving measures hypothetical PD, not actual hold behavior.
[9] Bounded 80 samples captures first ~6.4sec movement; overflow counter exposes longer runs. Sample timestamp/age reveals stale/cached values and I2C variability. Large serial output happens only after idle.
[10] Run gates: STEER Axis M8 owner command1 or4, arm from already-authorized start; excludes R2/Voice45 NC, shadow monitor enable1, refuses pending overwritten. STOP/fault records result and safe-serial idle. Keep NC/STOP checked at bench.
[11] First check exact GitHub CI + Windows PS parse; on hardware PowerShell -MaxRuns4. Commands already safe: Voice15/30/40 and CENTER with wait REAL_END/3sec each. No manual force/stress test.
[12] Later data need mapping of observed dynamics to safe H2 design and distinct external-load-release trials if permitted. No measured stall current/thermal/brake type currently.
[13] 2026-11-06 M9-H0/H1 source and host CI; 2026-11-07 private static H1 4/4 logs; 2026-11-07 user requests real drive trace without simulation.
[14] Sample format frame record includes signed err, vel, u in raw/s and shadow units, actual timer CCR from M8, raw from real AS5600. Verify sha/reason/event seq/overflow and sample age; do not treat 0 shadow as failure in idle.
[15] Next chat verify exact SHA CI and inspect real TXT data privately if provided. Do not require prior H0/H1 static logs again.
[16] No H2 CURRENT HOLD PASS, no artificial stimulation in firmware, no commit of private traces; original 386-line requirements and all prior history preserved.

## 2026-11-07 — H1 REAL capture exact CI PASS (17 mục)

[0] Đã kiểm tra Actions thực tế run 42368389841 trên commit source 93551551422559914052ec8b8671a521715714b4.
[1] Nhánh feature/m9-steering-active-position-hold, HEAD docs mới cần verify khác; firmware source tested 93551551.
[2] Mục tiêu USER cần REAL data động motor, không mô phỏng xe; H1 shadow không phát motor.
[3] Trên máy USER vẫn dùng CubeIDE project cũ, bảo vệ dirty .settings/startup/CSV trước fetch/merge.
[4] H1 source/host CI PASS, H1 MCU real-data drive trace PENDING; H0/H1 static trước đã PASS giới hạn. H2/H3 chưa triển khai.
[5] GCC compiled real trace C, no HAL motor/I2C writes, H1 source wiring, timing and UART safe idle; workflow all required jobs success.
[6] Chưa có CubeIDE Clean Build/Flash source mới, không có file REAL_BEGIN/FRAME/END từ người dùng.
[7] New script tools/capture_m9_h1_real.ps1 and document docs/M9_H1_REAL_DRIVE_CAPTURE_2026_11_07.md.
[8] Không đổi H1 gains, output +/-40 shadow units không phải PWM lực.
[9] UART capture 80frame fixed RAM, overflows tracked, không gửi khi motor chạy. Nguồn H0/H1 static log đã có cần bảo toàn.
[10] Source tests không chứng minh worst-case execution time mọi đường đi trên target, đọc dữ liệu RAM không làm thay đổi motor được kiểm tra code.
[11] Next user local update safely, CubeIDE clean build flash, protected rig 4 actual M8 Voice/R1 commands and one private TXT file.
[12] Không có thực nghiệm phản lực motor khi ngoại lực biến mất hoặc điện/thermal.
[13] Source commit 93551551 + Actions PASS; docs followup accurate evidence note.
[14] Capture frames t/raw/err/v/u/actual CCR/age/nc/state, compare real motion, 80ms, 80max.
[15] Chat mới phải kiểm tra correct GitHub HEAD SHA/CI and avoid user repaste old data.
[16] H1 no real-drive hardware PASS yet; no H2 ACTIVE, no Release, original requirements untouched.


## 2026-11-07 — REAL MOTOR measured private 4-run trace, handoff 0–16

[0] Nguồn trực tiếp file user private `M9H1_REAL_20261107_183513.txt` 146 lines 4 H0/4 H1 static + 4 BEGIN/4 END + 130 FRAME, sha3dbdfaf..., no file published.
[1] Branch feature/m9-steering-active-position-hold, firmware sha3dbdfaf678b700f03f3517cf0e70664c5557f6ba; docs HEAD after this commit will differ, do not infer firmware reflash needed.
[2] Software-only active steering HOLD remains goal; real M8 movement measured while H1 predicts but never applies u.
[3] User specifically wanted one real data session, not synthetic Live Expressions or manual force; user has delivered it. Do not demand same 4 runs repeated.
[4] H1 REAL DYNAMIC DATA CAPTURE 4/4 pass; H1 HOLD ACTIVE external load response not done, H2/H3 not implemented.
[5] 130 frames seq 1=19/1550ms, 2=37/3098, 3=36/2954, 4=38/3110; all M8 result2, sample gaps84ms, fresh ages0..1ms, no bad/stale/overflow/dropped, nc0.
[6] H0 seq3 reason3 after1180ms not the full3s reason1; specific STOP/neutral/monitor condition not isolated. No current/temperature/real external load data.
[7] Primary docs real drive capture; source main.c/m9_h1_real_trace.c; user's local TXT private (must not commit). H1 baseline SHA3dbdfaf and previous CI.
[8] Derived: goal reached within ±5RAW at completion M8, peak vL277..407 and vR240..333RAW/s, PWM peak measured CCR L300/R100, cannot assume torque equality.
[9] Current H1 Kd/Kp=0.5s yields sign reversal far from target: seq2 t1178 eL150 vL305 uL−7 while actual M8 ccrL300, seq4 t1010 eL180 vL361 uL−5 while ccrL300. Critical no H2 direct mapping.
[10] 69/130 LEFT frames and72/130 RIGHT frames at ±40 shadow cap; not physical PWM. Per-feed brake counts in END are diagnostics, not physical actuation.
[11] Engineering next: analyze sampled coast+fine correction M8 (e.g seq2 t2858 LEFT CCR175 at e6) and plan safety review (current/thermal/NClimit/owner/release) before H2.
[12] Not known CCS_SHB12 PWM0 coast/brake, signed motor DIR behavior, continuous stall torque/current; zero-CCR frame not necessarily M8 owner DONE.
[13] User delivered real file 2026-11-07 evening; verified source/GitHub source exact SHA; docs-only followup record outcome.
[14] H0 static reason1 for seq1/2/4, reason3 for seq3; H1 idle peakCmd0 all four, moving peakCmd40 both axes in four runs.
[15] Next chat answer from existing real data first; do not ask new hardware test unless a specific remaining safety gate cannot be addressed offline.
[16] No HOLD torque applied, H2 not active; source/CI vs MCU hardware evidence separated, original requirements untouched.

## 2026-11-07 — M9-H2-A GUARD SOURCE (17 checkpoints)

[0] USER yêu cầu triển khai H2 ngay sau 4-run H1 real trace, không chấp nhận bịa current/stall torque.
[1] Parent branch M9 exact HEAD fffea1bd6fe3b616e28a0e31190ced70adbb0f4f trước H2 source. HEAD sau commit phải fetch/verify exact Actions.
[2] Goal full M9 Active Position Hold vẫn giữ, nhưng H2 chia H2-A fail-closed supervisor và H2-B cấp lực với điện/thermal gate; không tự đổi mục tiêu sang M8.
[3] Khả năng dữ liệu: H1 130 real AS5600/CCR frames done, nhưng không có lực ngoài hoặc dòng stall, CCS_SHB12 PWM0 coast/brake unknown. Không tự ước lượng cap CCR như an toàn.
[4] H0/H1 static/dynamic capture PASS trong phạm vi cũ. H2-A SOURCE-STAGED pending CI/target; H2-B powered chưa implement; H3 integration pending.
[5] New Core/Inc/m9_h2_hold_guard.h + Core/Src/m9_h2_hold_guard.c: enum OFF/MONITOR/SHADOW/ARMED/ACTIVE/FAULT/COMPLETE, runtime uses only monitor/shadow/fault/complete. M9H2_POWER_PERMITTED=0U hard close, duty=0 no GPIO/PWM calls.
[6] main.c add M9H2_StartAfterDone only AFTER left/right PWM0 and both DONE2 no NC, R1/Voice5..40; M9H2_IdleUpdate from cached AS5600, invalid/stale, NC, safety STOP, wheel moving, owner change/mode change flags, score calculated with near-goal PD 128/32 Q8 cap±20 arbitrary. No extra I2C. UART emits one M9H2_GUARD record at complete SAFE idle.
[7] Files: Core/Src/main.c; Core/Inc/m9_h2_hold_guard.h, Core/Src/m9_h2_hold_guard.c, tools/m9_h2_host_test.c, tools/verify_m9_h2_guard.py, tools/capture_m9_h2_guard.ps1, docs/M9_H2_GUARD_SOURCE_2026_11_07.md and updated workflow.
[8] Control rules: boot/timeout OFF, no startup hold, 3s H2A shadow observation, no active. STOP/neutral/trip, NC mask, stale/corrupt, jump >90RAW, error>32RAW, wheel activity or other steering owner cause cancel/Fault, never PWM.
[9] Safety note: realtime H2-B motor owner arbitration not yet certified; current M8 idle path forces CCR0. Never bypass M8/NC ISR with shadow candidate. Code H2A contains no motor writes by design.
[10] H2A logs bitset blockBits: 0 trip,1 stopLatch,2 control STOP,3 neutralReq,4 monitorDisabled,5 motionPermit off,6 wheel moving,7 NC input,8 owner/mode changed,9 debug poll; aids analysis of prior H0 seq3 reason3 without claiming cause.
[11] Test strategy strict GCC actual-C host + wiring invariants + Windows PowerShell syntax; if PASS only H2A SOURCE/HOST. Separate CubeIDE Build/Flash/MCU logger and H2B physical bench still pending.
[12] Need empirical CCS_SHB12 PWM0 coast/brake, ACCE/DECCE lag, motor current at stall/duty, temperature, safe holding time, actual force removal, independent power E-stop.
[13] Sequence prior H0 passive/H1 shadow, H1 real 4 run SHA3dbdfaf; new H2A additive code no H1 gains or M8 tested steering modified.
[14] H2A near-goal shadow KP_Q8=128/KD_Q8=32, cap±20 arbitrary, 32 RAW deviation rejection, stale≤250ms, 3s window, 100ms velocity; NOT motor CCR/current.
[15] After code commit inspect CI exact SHA, docs status, user local worktree preserved before pull, no reset/restore files; if logs sent analyze them before requesting new physical tests.
[16] H2A no electrical hold, no H2 powered PASS; H2B cannot be honestly claimed without safety evidence and review, original project req 386 lines remain unchanged.

## 2026-11-07 — H2-A lần CI đầu: sửa cú pháp PowerShell 5.1

- Commit `b9b75aff0bba91fedf62e4e67b244ad8f91fa1b4`, Actions `42370416232`: Linux exact-C H2-A `verify_m9_h2_guard.py` cùng full context contract **PASS**; Windows PowerShell parser phát hiện biến `$MaxEvents:` trong interpolated string (`:` theo sau tên biến), không phải lỗi motor hay logic NC.
- Đã đổi thành `${MaxEvents}:` để parser PowerShell 5.1 hiểu chính xác. Mã H2/C motor guard không thay đổi; kiểm tra lại tất cả workflow ở SHA commit mới là bắt buộc.

## 2026-11-07 — H2-A exact-SHA verified PASS (17 handoff points)

[0] CI source SHA55db22f67c81517ada48106f5a506b7b367b7e4c run42370515934 COMPLETE SUCCESS confirmed, not inferred.
[1] Branch feature/m9-steering-active-position-hold HEAD after docs must be verified; source H2A does not apply power.
[2] Goal software-only steering HOLD continues but physical H2B pending characterization.
[3] USER wanted H2 done; shipped guarded H2A rather than arbitrary current/PWM command.
[4] M9-H0/H1 known static/real-dynamic data, H2A SOURCE/HOST CI PASS, H2A MCU capture pending, H2B/H3 not built.
[5] All CI checks success: Linux actual-C GCC supervisor NC/STOP/owner/stale/unpowered cases, Windows PowerShell 5.1 parsing, context and M8 regressions.
[6] No target CubeIDE Clean Build/Flash, H2 UART MCU log or current/thermal/torque/loaded evidence.
[7] Source docs/M9_H2_GUARD_SOURCE_2026_11_07.md and CI run 42370515934, script tools/capture_m9_h2_guard.ps1, pure C core.
[8] Boot no H2 actuation and M9H2_POWER_PERMITTED 0, all outputs always0; no enable by Live Expressions.
[9] No new PWM/driver owner pathways, single safe-idle UART record only after DONE, close NC/safety gates.
[10] Sensor stale 250ms, 32 RAW max H2 guard error, 3s shadow observation, explanatory blockBits.
[11] Next target proof requires local source update with dirty worktree preserved, CubeIDE build/flash and passive H2A log; STOP/NC preflight remains.
[12] H2B still depends on motor current/thermal, PWM0 CCS_SHB12 coast/brake, ACCE/DECCE, independent motor cutoff.
[13] Initial H2A b9b75a passed C full contract but PS parser failed because MaxEvents-colon; corrected SHA55db22f full CI PASS.
[14] H2A shadow Kp_Q8=128 Kd_Q8=32 cap20 score arbitrary, not physical CCR.
[15] New chat should read exact source SHA vs doc HEAD and avoid asking old 130 H1 data again.
[16] No H2 powered PASS, original requirements unchanged, no user TXT published.


## 2026-11-08 — H2-A normal owner handoff fix, 17 checkpoints

[0] User private 2 TXT files: first valid M9H2_GUARD sha8214610, second malformed H0+M8R1 joined line without H2.
[1] Source base branch HEAD8214610cadac396f7502dea5fbde847de784fe9d, after fix HEAD and Actions must be verified.
[2] User goal H2 software-only hold project continues; H2-A supervisor, H2-B powered depends on current/thermal.
[3] Real log 13ms reason3 blockBits256, 0 NC/sensor error, 0 duty and powered0. No 3sec H2A PASS yet; old H1 130 measured frames remain.
[4] Root from source: M9H2_StartAfterDone captures VOICE before finish clears Voice owner; Control_Arbitrate naturally sets IDLE. Strict mode equality canceled immediately.
[5] New pure-C M9H2_ModeAllowed permits source2/3 originalVOICE to VOICE/IDLE if no new pending Voice, PS2 activity, cancel; source1 PS2 same PS2 or clean release to IDLE.
[6] Revoke on STOP, new Voice or PS2 job, PS2 cancel, unexpected mode; original STOP/NC/wheel/owner/CCR/sensor/age gates still mandatory.
[7] Files changed main.c, m9_h2_hold_guard.c/.h, tools/m9_h2_host_test.c, tools/verify_m9_h2_guard.py, collector warning and H2 docs/roadmap/handoff.
[8] No motor changes: M9H2_POWER_PERMITTED=0U, dutyL=dutyR=0, no new TIM/GPIO/I2C, still H2B not coded.
[9] UART H2 new modeStart/modeNow/modeOK/voiceBusy directly diagnoses expected VOICE mode1 -> IDLE mode0.
[10] Original second TXT one H0 line physically truncated/concatenated with M8R1_FINE; cannot assume UART sender bug; file count H2=0. Collector warns malformed H0 summary.
[11] Host GCC tests positive normal Voice->IDLE, PS2->IDLE and negatives new job, STOP, manual takeover, PS2 cancel, no output.
[12] Need exact CI after commit; local CubeIDE Build/Flash then one passive safe Voice command for ≥3sec H2 log. Not retest H1 4 runs.
[13] In first H2 log: source3, goalL1545 goalR351, updates1 freshL0 freshR1, duration13, sensorErr0, deniedOwner1, bit8 set, duty0.
[14] Old H0 seq3 reason3 from previous H1 log separate event, do not conflate with this H2 reason3.
[15] User local worktree has modified .settings files safely restored with stash apply; new update should preserve local config and no duplicate stash apply.
[16] H2-A full hardware PASS pending, H2-B active/loaded untested, original specs not changed.

## 2026-11-08 — H2-A real-case mode handoff fix exact CI PASS (17 sections)

[0] Exact source commit 977533f79194e2ac3a6b97768675b6647348dbd8 Actions run42459137638 SUCCESS.
[1] Feature branch M9 docs-only HEAD newer may differ, must fetch on next turn.
[2] Goal remains future powered hold but H2A is only guarded source/telemetry.
[3] User previously submitted real 13ms H2 abort and malformed second TXT; no retest of H1 130 frames.
[4] H2A source/host CI PASS exact SHA; target MCU 3s after fix PENDING, H2B/H3 not implemented.
[5] Linux strict actual-C C-host normal Voice->IDLE, PS2 R1->IDLE after release, STOP, new command/PS2, NC/sensor and hardzero tests PASS.
[6] Windows native PowerShell5.1 collector parser PASS, no new GPIO/PWM/I2C activity.
[7] See docs/M9_H2_GUARD_SOURCE_2026_11_07.md, tools/capture_m9_h2_guard.ps1.
[8] Control mode change allowed only for original Voice/PS2 near post-DONE and no new commands/PS2 cancel; all other arbitration checks still revoke.
[9] UART new modeStart/modeNow/modeOK/voiceBusy explains accepted handoff.
[10] Original log 8214610 had reason3, blockBits256 after13ms and powered0. New code verified HOST but not MCU.
[11] Next user can git fetch + ff-only merge preserving .settings, CubeIDE Refresh/Clean Build/Debug/Resume, single Voice 15/Center safe, collect one guard TXT.
[12] Do not force physical load tests until driver PWM0 behavior, motor current/stall/thermal and independent motor cutoff are known.
[13] CI run42459137638 SUCCESS, source SHA977533f; docs followup SHA may differ but MCU firmware source unchanged.
[14] H2A mode handoff code and host test exact source; no changes to H1 moving motor control/gains.
[15] Preserve private user UART files, do not upload raw TXT public, verify new log sha and reason1 duration~3000.
[16] No H2B powered PASS, H3 pending, original 386 requirements unchanged.

## 2026-11-08 — H2-A guard MCU 3s one-run real validation, 0–16

[0] User provided private M9H2_GUARD_20261108_143827.txt, MCU sha5dd1502ac606c5e254ddda3484baffbf2a70a534.
[1] Feature branch HEAD at report start 5dd1502, may now have newer docs-only HEAD after commit; no firmware source change.
[2] Goal M9 active hold unchanged; this observation is guard-only H2A, not H2B powered.
[3] User already ran STM32 firmware and supplied three complete H0/H1/H2 lines; do not demand repeat of same run.
[4] H0 static3s PASS, H1 static3s PASS, H2-A guard on-MCU 3s PASS **one Voice CENTER run**, H2-B/H3 pending.
[5] H2 record seq2 source2 state6 reason1 duration3000 updates294 freshL294 freshR293 peakErrL5 peakErrR3.
[6] H2 blockBits0 deniedNC0 deniedOwner0 sensorErr0, modeStart1 modeNow0 modeOK1 voiceBusy0: normal Voice->IDLE fix confirmed on MCU.
[7] H2 dutyL0 dutyR0 powered0 shadowOnly1; hard lock M9H2_POWER_PERMITTED0 retained. No motor hold torque evidence.
[8] H2 peakScoreL/R0 due quiet near-goal; cannot claim dynamic disturbance PD or powered response.
[9] H0 goal1886/662 raw final1882/659 peakDrift1/1 samples295/294 bad0 jump0 wheelsMoved0.
[10] H1 samples295/294 errL4 errR3 peakVel0/0 peakCmd0/0 no bad/stale/sat; reason1.
[11] Prior MCU abort13ms at sha8214610 fixed in source and now new 3000ms proof on sha5dd1502.
[12] H2-B requires verified current/thermal/stall, driver PWM0 brake/coast, independent cutoff and STOP/NC tests; never infer from idle.
[13] Source normal mode fix exact CI pass and new real MCU passed only limited healthy one-run scope.
[14] Private source TXT do not upload GitHub; summary only in docs/M9_H2_GUARD_SOURCE_2026_11_07.md.
[15] Follow-up could plan H2-B electrical safe evidence and design, no forced hand load under powered hold without safety verification.
[16] Do not change M8 PWM/owner, H2 powered remains OFF, release not warranted.

## 2026-11-08 — H2-B0 bounded policy source, handoff 17 items

[0] USER explicitly requested continuation of H2-B, after H2-A 3s real MCU pass.
[1] Base HEAD 39ab98e92f6bf53ea84596d9ffa1af1a8c000606; verify source exact GitHub Actions after commit.
[2] Goal remains eventual H2 powered hold, but current source H2-B0 only computes bounded candidate, not physical holding.
[3] User-provided CCS_SHB12 manual: H-bridge 12A continuous/30A peak DRIVER, no safe motor hold current or PWM0 coast/brake verified.
[4] H0/H1 static/dynamic and H2-A one-run 3s passed; H2-B0 source staged, H2-B1/H3 not.
[5] Pure C m9_h2b_hold_policy.c/.h stores same H2-A seq/time and score candidate, no new I2C/PWM/owner.
[6] Candidate hyst enter8 exit4 RAW, cap20 arbitrary score, slew3/tick, veto sign opposite position error, cancel on guard revoke.
[7] Power stage compile hardlocks H2B_OUTPUT0, PWM_LIMIT0, ON_TIME0 and independent H2A PERMITTED0; no motor outputs.
[8] main.c H2B_Begin immediately after H2A_Begin, Step after H2A_Step, safe idle UART record has bState/bReason/bPeakL/R/bVeto/bCcr zero.
[9] Host test source verifies no motor writes and GCC actual-C candidate gates and failure cases.
[10] Exact GitHub Linux and Windows CI still pending until source commit; MCU on-target Build/Flash status pending.
[11] User local project settings modified, preserve; a single passive Voice test only after CI and no new manual force.
[12] H2-B1 needs motor specific current/temp/stall and real CCS_SHB12 PWM0 coast/brake, independent power cut, NC/STOP tests.
[13] Prior H1 130 samples and prior H2A 3s healthy log not to be recollected.
[14] Raw user UART logs remain private and should not be published to repository.
[15] Documentation docs/M9_H2B_POWER_GATE_2026_11_08.md plus roadmap and CONTEXT_HANDOFF updated.
[16] No physical active HOLD pass or commissioned H2-B1; original project requirements retained.

## 2026-11-08 — H2-B0 source exact CI PASS, 17 checkpoints

[0] User explicitly requested H2B after H2A 3s real MCU pass, implemented candidate with motor output hard-locked.
[1] Exact source SHA9e4f4533b9c7de820295f9c36ceab28541048dc0; docs-only HEAD may differ.
[2] User goal active HOLD remains, but source stage H2-B0 not H2-B1 powered torque.
[3] Previous H2A on-MCU one-run 3s PASS sha5dd1502, no need to repeat H1 130 frames.
[4] H2B0 source/host CI PASS, CubeIDE target build/flash H2B0 log pending, H2B1 and H3 pending.
[5] GitHub Actions run42460572191 completed SUCCESS Linux strict GCC actual-C and source safety gates.
[6] Windows PowerShell5.1 parser PASS exact SHA, prior H0/H1/H2A regressions PASS.
[7] Added Core/Inc/m9_h2b_hold_policy.h, Core/Src/m9_h2b_hold_policy.c, main.c integration.
[8] Candidate hyst8/4RAW score cap20 arbit units slew3/tick, opposite-sign veto, seq/time consistency and guard revocation.
[9] Compile H2B_POWER_STAGE_ENABLED0, verified motor PWM0/on-time0; independent H2A POWER_PERMITTED0.
[10] main.C H2B_Begin after H2A_Begin, Step after H2A_Step, no extra I2C/TIM/GPIO control.
[11] M9H2_GUARD UART adds bState/bReason/bPeak/bEligible/bVeto/bBlocked/bCcrL/R with powered0.
[12] Need user local safe ff-only update preserving .settings, CubeIDE Refresh/Clean Build/Debug/Resume, one passive safe Voice log.
[13] H2B1 powered requires measured motor stall-current/temp, SHB12 PWM0 brake/coast and independent motor cut.
[14] Driver manual rating 12A continuous/30A peak is H-bridge rating not a safe continuous motor hold rating.
[15] Private user UART data not published publicly, only aggregates and source docs.
[16] Do not claim active HOLD or load release PASS, original requirement contract unchanged.


## 2026-11-08 — H2-B0 MCU static integration/telemetry PASS, 17 checkpoints

[0] Phạm vi: user supplied private M9H2_GUARD_20261108_151606.txt; reviewed all three complete H0/H1/H2 records; raw TXT remains private.
[1] Điểm tiếp tục: MCU exact firmware 819f635aafbe9dde3ee0e4b9c5c8e6cbc282e0c2, seq1/source2 Voice CENTER; current source branch checked at this SHA before docs-only commit.
[2] Mục tiêu: software-only active steering position hold and controlled disturbance-release response using existing STM32/AS5600/CCS_SHB12.
[3] Yêu cầu USER làm rõ: động học chưa hoàn thành để tích hợp/nạp; SELECT tạm hoãn. PowerShell-only logging and preserve existing Windows project/local .settings.
[4] Milestones: H0/H1 prior measurement results retained; H2-A healthy MCU validation retained; H2-B0 one-run STATIC INTEGRATION/TELEMETRY PASS; H2-B1/H3 pending.
[5] Kết quả: H2 state6/reason1, duration3003ms, updates268, freshL267/freshR268; H2-B0 bState3/bReason1.
[6] Điểm dừng: compiled H2-B0 now observed on MCU, but bPeak0/bEligible0 means no physical nonzero candidate or disturbance validation; no powered hold.
[7] Tài nguyên: private log filename above, docs/M9_H2B_POWER_GATE_2026_11_08.md detailed aggregate, M9 roadmap and current source at MCU SHA.
[8] Quyết định giữ: M9H2B_POWER_STAGE_ENABLED0, verified PWM/time0, independent H2A PERMITTED0; do not map arbitrary score to motor PWM.
[9] Lỗi/bài học: normal VOICE1->IDLE0 accepted (modeOK1); deniedOwner0, deniedNC0, sensorErr0, blockBits0, dropped0; no new firmware defect indicated.
[10] Rủi ro/giới hạn: quiet run does not test STOP/NC fault injection, loaded response, current/thermal/stall or PWM0 driver coast/brake; no claim of full hardware HOLD PASS.
[11] Kế hoạch tiếp: preserve measured log and use existing data; H2-B1 needs actuator/driver evidence and guarded owner/output design before powered commissioning; no repeat of completed H1/static test.
[12] Đầu vào thiếu: motor safe current/temperature/stall and installed driver PWM0/ACCE-DECCE behavior, cutoff/STOP/NC qualification for powered hold; independent CubeIDE transcript not supplied in this upload.
[13] Dòng thời gian: after source/CI and user update819f635, user supplied new3s log151606; docs update follows with no firmware changes.
[14] Thông số chính xác: goals1886/662RAW; H2 peakErr6/3RAW; H0 peakDrift1/1; H1 final err5/2, filtered peakVel0/0, peakCmd0/0, noise7/0.
[15] Chỉ dẫn tiếp quản: distinguish MCU SHA819f635 from later docs-only HEAD; no reflash for docs, no raw UART publication, no unrequested motor-power change or duplicate stash apply.
[16] Chất lượng: one matched healthy static window verified; H2-B0 power values all0, bVeto0/bBlocked0; no candidate/dynamic/active/loaded PASS inferred; original386-line spec unchanged.


## 2026-11-08 — Motor-lead current baseline, 17-point handoff

[0] Phạm vi dữ liệu: USER reports current during CENTER and after stop, measured on driver-to-motor wires; no raw telemetry file or instrument model.
[1] Phiên bản: M9 branch checked at3cc874df, MCU remains819f635; this commit is documentation only.
[2] Mục tiêu: active steering position hold with existing STM32/AS5600/CCS_SHB12; reduce rebound after disturbance release.
[3] Yêu cầu: running-current values are baseline; earlier5A/10A from80/200W are estimates, not manufacturer safe HOLD ratings; kinematics incomplete for integration, SELECT deferred.
[4] Milestones: prior H2-B0 one healthy3s static MCU PASS retained; powered H2-B1 and H3 not implemented.
[5] Kết quả USER: during CENTER LEFT0.8–1A, RIGHT≈2A; after stopping meter reads0A each; explicitly motor leads, not driver supply.
[6] Điểm dừng: baseline available for design; PWM/on-time/thermal envelope and explicit bench activation mechanism not selected; no motor-power change.
[7] Tài nguyên: chat report; docs/M9_H2B_POWER_GATE_2026_11_08.md contains aggregate baseline and source-based B1 bench design.
[8] Quyết định: no current-to-PWM assumption and no physical-current guarantee without current feedback; preserve independent H2A/H2B compile locks.
[9] Source lesson: IDLE arbitration calls Turn_Motors_Stop, H2A owners_idle requires CCR0; B1 must have designed actuator ownership rather than attach shadow score to CCR.
[10] Rủi ro còn thiếu dữ liệu: current display vs peak/RMS, motor thermal/stall envelope, PWM0 coast/brake, driver ACCE-DECCE/current thresholds;0A stationary does not establish brake mode.
[11] Kế hoạch: separate single-axis finite bench session, other axis and wheels0, bootOFF and deliberate request; measure actual HOLD before disturbance-release and H3.
[12] Đầu vào phụ thuộc: suitable current/temperature observation, defined safe PWM/time limits, installed driver behavior/cutoff and reviewed activation semantics; no request to repeat finished H1.
[13] Dòng thời gian: after H2B0 MCU static log, user first estimated5/10A, then supplied running0.8–1/2A, confirmed stopped0A and motor-lead measurement.
[14] Thông số: retain CENTER1886/662RAW and current firmware819f635; user current baseline is approximate, no sample-synchronized PWM/current trace.
[15] Tiếp quản: carry this qualified measurement forward; no more interpretation as source current, no reflash for docs, private UART stays private.
[16] Kiểm chứng: user-reported electrical baseline only; no HOLD current/thermal or active/load PASS, no source modification, original386-line specification unchanged.


## 2026-11-08 — FULL persistent H2-B1 source implementation, 17 checkpoints

[0] Phạm vi: user corrected H1 to H2-B1 and explicitly requires complete persistent two-axis HOLD, not delivery as isolated/short powered trials.
[1] Phiên bản: source work from exact78271f4e25accdbe353d9ab8108965bb449ef6ca; user MCU remains819f635 until a later build/flash is verified.
[2] Mục tiêu: retain both completed steering goals, correct displacement and damp return/release, integrated with existing motor ownership and safety.
[3] Yêu cầu: no fixed3s HOLD expiry, no single-axis bench mode; kinematics incomplete for integration and SELECT deferred; preserve existing controls and private logs.
[4] Milestones: H2-B0 one healthy static MCU PASS retained; H2-B1 full software SOURCE-STAGED/local HOST PASS, physical power commissioning and MCU/load acceptance pending.
[5] Đầu ra: m9_h2b1_hold.c/.h, commissioning header, main.inc, main hooks/UART, two actual-C tests, verifier, PowerShell receiver and CI integration.
[6] Điểm dừng: controller and actuator route exist; power flag0 and unknown physical gains/caps0 prevent real HOLD output. No CubeIDE build/flash or physical H2-B1 log yet.
[7] Tài nguyên: docs/M9_H2B1_FULL_HOLD_2026_11_08.md is current scope/design/evidence; older single-axis finite bench proposal is superseded as delivery scope.
[8] Quyết định: valid goals only after BOTH M8 DONE2 R1/VoiceCENTER or Voice5..40; no R2/Voice45 NC hold or implicit goal from manual release; no auto-resume after STOP/fault.
[9] Lỗi xử lý: IDLE and ordinary Position/Manual/Cal zeroing retain a valid HOLD owner; pre-owner release precedes new M8 writes; explicit STOP still revokes.
[10] An toàn: fresh sensors/geometry, raw NC plus pending EXTI plus retained NC epoch, atomic rechecks before/after DIR and between CCR writes; no new I2C or blocking UART.
[11] Kế hoạch: GitHub exact-SHA CI, then independently verify target build/MCU; commissioned force needs actual motor/driver calibration, not a new reduced product scope.
[12] Đầu vào thiếu: physical PWM/gain/reversal/thermal limits and installed driver behavior/current protections; firmware has no current/temperature feedback from external meter.
[13] Timeline: user rejected single-axis short-session delivery, confirmed full H2-B1; full persistent controller/owner/source implementation and host regressions follow.
[14] Thông số: manual motor-lead running current L0.8–1A/R≈2A, rest meter0A; prior5/10A from80/200W estimates only. Host fixture PWM/gains are synthetic, never calibration.
[15] Tiếp quản: read new full-HOLD report and exact branch CI; distinguish SOURCE/HOST, commissioning0, target build/flash and loaded hardware; keep H2-B0/H1 prior data without repetition.
[16] Chất lượng: tests cover continuous120s two-axis demand, damping/reversal,60s production main retention, NC ISR/pending/STOP/new owner and compiler interlock; no active/load/thermal PASS inferred.



## 2026-11-05 — M8-S15.4 Voice dual-AS5600 synchronized-speed trial (latest)
[0] Operator confirmed existing A5/A6 B9 45° NC motor motion works if VC-02 receives direction then angle with a spoken pause. This is the BASELINE; do not redesign parser. Operator now orders S15.4 speed synchronization using actual AS5600 encoder target geometry.
[1] Forked new feature/m8-s154-voice-dual-steer-common-progress from 66053b61ea3890ed7a56c80a98eced758908d911; that parent branch preserves user-confirmed behavior.
[2] In Core/Src/main.c, new steer_voice_s154_enable=1U defaults to trial; 0 restores the immediately preceding independent Voice position behavior at runtime (change only when stationary). Command4 (5..40 interpolated encoder) and command5 (45 must reach both corresponding physical NC) feed old bounded M8-S00 steering normalized progress and velocity feedback. Existing AS5600 start/goal separately passed: steer_left_axis.escape_start_raw/voice_pos_goal_l, right analog.
[3] Steering_VoiceS154Sync runs AFTER Steering_PositionAxisUpdate for both motors and BEFORE late IRQ / atomic NC / CCR write, only both coarse MOVING, both fresh <=40ms, NC-clear, nonzero PWM and valid per-axis floor; cannot create PWM where owner zero, cannot bypass safety, limit caps remain 300/100 or lower. Independent 45 NC completion, RAW safety stop before missing NC, fine correction, timeout, watchdog remain original.
[4] Added observability: steer_voice_s154_start_l/r, progress_l/r (permille), gap_permille and peak, pwm_l/r after coordinator, l_eta_ms/r_eta_ms/common_eta_ms from real sample rates, status 0 idle/1 pace/2 disabled/3 ineligible state/4 stale or zero/5 asserted NC, done_l_ms/r_ms, arrival_skew_ms. These do not certify equal arrival.
[5] New tools/verify_m8_s154_voice_sync.py extracts and HOST compiles production C normalized stroke progress/speed observer/sync wrapper, and static-checks real wiring, NC guards, pause-independent UART parser, range caps, and R1/R2 unaffected. Exact SHA Actions CI needed.
[6] Detailed design and safe A/B physical acceptance in docs/M8_S154_VOICE_DUAL_SYNC_2026_11_05.md; append-only project milestones updated.
[7] IMPORTANT: S15.4 actual powered MOTOR sync NOT hardware-tested. CubeIDE Clean Build and flash matching ELF, no-power NC preflight, supported chassis with emergency power cut, A/B enabled-vs-disabled measured phase gap and arrival skew required. Preserve fail-closed and do not tune PWM/NC thresholds simply to force skew.


## 2026-11-05 — S15.4 HOST strict compilation and exact-SHA handoff update
[0] The first new Voice S15.4 test compile under HOST gcc -Werror failed on two unused TEST STUB start-observation globals (not production firmware). Removed unused fixture-only variables; actual production S15.4 coordinator unchanged.
[1] GitHub exact-SHA workflow enforces updating this append-only handoff and docs/PROJECT_MILESTONES.md after any source or test change. These entries record that final additional test modification. Re-run full CI on the completed SHA.
[2] Final CI may verify HOST extraction/decoding/NC/STOP, but not ARM target build or physical synchronization. Keep all safety and existing operator-proven Voice45 behavior unchanged.
## 2026-11-05 — Passive RIGHT Voice fine diagnosis, operator hardware report and 17-section update
[0] New separate trial feature/m8-s154-right-fine-passive-diag branches from known S15.4 HEAD 1f9334088f4d06349abc862316f5e5ef8800ee05, preserves append-only historical context.
[1] Operator reported S15.4 physically makes both coarse motors visibly synchronize; LEFT finishes well, RIGHT Voice 5..40 performs multiple direction-changing final alignments. Not a quantitative log or new branch HARDWARE PASS.
[2] Goal is isolate RIGHT fine overshoot, weak pulses and quiet/coast settling causes before active changes.
[3] User explicitly requires new trial flags boot=1, no manual variable settings. Actual servo S15.4 and Voice flags already boot1, new diagnostic flag boot1.
[4] Milestone S15.4 PASS HOST baseline, physical sync report qualitative; new fine diagnostic SOURCE-staged only.
[5] Actual RIGHT fine servo fields already count pulses/weak/good/backtrack; new timer and reverse counter are observational, not actuation.
[6] Snapshot at Steering_PositionFinish before Voice clears; UART transmission deferred until stopped/no conflict, nonblocking IT.
[7] Tools: Core/Src/main.c, tools/verify_m8_s154_right_fine_diag.py, tools/capture_m8_s154_right_fine.ps1, docs/M8_S154_RIGHT_FINE_PASSIVE_DIAG_2026_11_05.md.
[8] No tuning performed. R1/LEFT manual, NC Voice45, all motor PWM envelopes and STOP/PS2 unaffected.
[9] Older R1 fine settings RIGHT PWM85, pulse12ms, entry15 RAW and quiet wait are known code, not established Voice fault cause.
[10] UART log itself cannot prove actual motor supply cutoff, current or coast; maintain independent hardware cut and safe rig.
[11] Run exact-SHA CI; operator CubeIDE Clean Build/Flash, safe incremental Voice angles, collect private PowerShell TXT, then decide if new tuning is warranted.
[12] Missing physically validated serial output and hardware measurements, plus operator test date/flash identity.
[13] 2026-11-05 continuation only, no rewriting historical snapshots.
[14] Tests remain HOST (actual C capture and static wiring); no target ARM binary compiled by connector.
[15] Next chat must validate new branch HEAD and not assume former branch is latest; don't publish private TXT.
[16] SOURCE-STAGED, HOST CI, CubeIDE BUILD, FLASH, HARDWARE PASS tracked separately; unchanged original 386-line contract.

## 2026-11-05 — S15.4 passive fine diagnostic follow-up: verifier and UART literal correction
[0-16] CI at initial diagnostic SHA 00c63930a762834a70975a80c07d990739188129 failed ONLY new static/C HOST fixture because verifier used a literal backslash-n rather than a newline anchor for the existing M8S11 definition. All prior S15.4 actual C and STOP/NC regression steps had passed. Corrected verifier anchor and C UART suffix from literal escaped CRLF to on-wire CRLF. No changes to motor actuation, STOP, NC, RIGHT fine timing/PWM or diagnostic data field semantics. Re-run full exact-SHA Actions before operator build/flash; neither the failed run nor correction is target/hardware PASS.

## 2026-11-05 — User-reported PowerShell parser failure repaired (S15.4 capture only)
[0] Operator ran native tools/capture_m8_s154_right_fine.ps1 and got ParserError at log display line 48: the variable expansion "$MaxEvents:" is an invalid PowerShell variable reference because colon follows it. This occurs before COM open and before ANY log capture; not a firmware/AS5600 issue.
[1] Fix only PowerShell interpolation to "${MaxEvents}:" without changing serial protocol, log fields, firmware flags or motor control. Add the new script to Windows PowerShell 5.1 Parser.ParseFile CI matrix, which previously only parsed S15.2 and stamping scripts; that gap permitted error.
[2] No CubeIDE clean/build/flash required for this host-only correction; user needs git pull on unchanged trial branch. New exact SHA CI must succeed and captured results carry flashed MCU SHA, which may legitimately be prior good firmware SHA.
[3] HOST/CI syntax success does not verify actual ST-LINK VCP, COM selection, UART or safe motor functionality. Await operator's physical log before speed/fine actuation changes.
[4] All 17-section historical handoff content, original 386-line requirements, Voice45 NC, existing S15.4 and STOP/PS2/NC protection unchanged.

## 2026-11-05 — S15.4F REAL 6-run Voice RIGHT fine log and isolated 10RAW handoff trial
[0] User uploaded private M8S154_right_fine_20261105_190812.txt physically collected from MCU SHA a47d7e7fb057a9b817c64a4c2eb99c82cf30125e. Private data is NOT placed in GitHub.
[1] Six Voice 15/30/40 two-way runs ALL resultL=2 resultR=2 finish=2, RIGHT final error within +/-5 RAW, dropped0. Qualitative observation LEFT good and RIGHT direction RAW increasing fine oscillation confirmed by 6 pulses/1 reversal for 30 and 40; decreases 1/1/2 pulses, no reversal.
[2] RIGHT RAW rising first fine at error14..15, first quiet settles approx +6..7 RAW afterward, leaving error ~7..8; hypothesis cutoff later at error10 permits settled error ~3..4 under similar coast. Not physically confirmed.
[3] Baseline fineMs time is from RIGHT fine entry until WHOLE COMMAND finishes, not true RIGHT-only time. Baseline skewMs loses order. Add rightFineMs, leftDoneMs/rightDoneMs on new log. Baseline peakGap0 all six; add syncFrames to distinguish disabled/fallback S15.4 from genuinely near-zero gap.
[4] Branch feature/m8-s154-right-voice-rise-late-fine-10raw created from exact known logged SHA a47d7e7fb057a9b817c64a4c2eb99c82cf30125e; baseline untouched.
[5] New helper Steering_VoiceRightLateFineEntry active ONLY Voice command4 RIGHT RAW increasing (err>0, right axis, voice owner, new flag1), goal at least30 RAW from physical R_MAX. Returns 10 RAW, otherwise original clamped right 15..35 entry; left and RIGHT negative error unchanged.
[6] Operator requires test flags boot1 without manual edits: existing steer_voice_s154_enable=1, diagnostic=1 and new steer_voice_s154_right_late_fine_enable=1. No bypass of motion permit or implicit auto-driving; rest of auto-ARM guard untouched.
[7] Coarse motor keeps same PWM and DIR another ~5 RAW before fine cutoff. Fine pulses PWM85/12ms, max8, +/-5 acceptance, 200ms off,100ms quiet,120ms capture, physical NC, STOP, PS2 and stall/timeouts unchanged. Do not claim no physical overshoot from HOST tests.
[8] S15.4 coordinator code and LEFT, R1/R2, Voice45 NC and manual steering untouched; additional S15.4 per-command paced-frames counter is read-only diagnostic.
[9] Existing automatic nonblocking UART3 M8S154_FINE appends entryLimit,syncFrames,leftDoneMs,rightDoneMs,rightFineMs for post-run data; previous fields same, existing PowerShell collector works and auto waits safe idle.
[10] Validate exact-SHA HOST actual C helper with test of Voice4/right/err>0/clearance; test fallback at LEFT/R1/Voice45/error negative/disabled/NC-near. Keep Windows PowerShell parser check.
[11] User next: clean worktree, fetch/switch new branch, inspect SHA, CubeIDE Clean Build/Flash matched ELF, NC/STOP preflight power disconnected, protected rig, same six Voice tests, use existing PowerShell capture, send one TXT privately. No manual var writes.
[12] Missing new branch target/physical acceptance; no user claim of faster speed yet. Stop on oscillation/NC faults and rebuild baseline SHA a47d7e7 if needed, rather than overriding PWM/safety.
[13] Logs are single-robot observations, not repeated statistics; six commands enough to target cautious one-variable experiment, not proof of speed improvement.
[14] Evidence file external/private, code docs/M8_S154_RIGHT_LATE_FINE_2026_11_05.md plus tools/verify_m8_s154_right_late_fine.py; exact commit and CI must be checked.
[15] Next assistant MUST read new branch HEAD, 17-section handoff and roadmap; preserve original 386-line spec, no public log upload.
[16] State layers: SOURCE-STAGED pending, HOST CI pending until workflow confirms, CubeIDE BUILD pending, FLASH pending, HARDWARE PASS pending. Release optional.

## 2026-11-05 — S15.4F exact-source extracted HOST harness update
[0] First new trial SHA ad15c89371c2629f9ba6ad3256c65fb64edf6bd8 failed at existing tools/verify_m8_s154_voice_sync.py: its isolated C fixture must explicitly declare the newly added read-only steer_voice_s154_run_sync_frames, just as it declares prior global counters. Core source code compiles in its normal translation unit, but extracted HOST test failed from missing fixture declaration.
[1] Add only fixture declaration (zero initialized) to S15.4 actual C extracted test. No motor algorithm, PWM timing, LEFT, Voice45 NC, STOP/PS2 or new right-late-fine change; preserve original 386-line handoff and run whole exact-SHA CI before operator build/flash.
[2] DO NOT call previous run PASS. Wait for new exact-SHA full SUCCESS. The physical log remains prior baseline SHA a47d7e7 and private.
[3] CubeIDE target build/flash and new physical right fine experiment still pending.

## 2026-11-05 — S15.4F second actual-C HOST fixture extension
[0] At first fixture correction a1b6450a, existing RIGHT passive capture HOST test failed because its isolated C typedef and globals did not include five newly captured read-only fields/timestamp globals in production source. Extend only the test C struct and fixture variables plus test assertions for leftDoneMs,rightDoneMs,rightFineMs,syncFrames,entryLimit. No changes to production firmware or safety/actuation. This is HOST harness upkeep, not CubeIDE or hardware validation.
[1] Re-run full exact-SHA CI and only deliver a verified success SHA. Previously observed private six-run log remains baseline a47d7e7 and is not published.

## 2026-11-05 — S15.4F entryLimit first-transition latch (post-CI diagnostic review)
[0] Exact-SHA 946adba8f02bca91627acc7583ac3ef45a863687 passed all GitHub Actions, but a new telemetry review found the reported fine entry threshold was assigned each frame, allowing later sign-changing error in the fine phase to rewrite measured entryLimit from 10 to 15. Fix assignment only on RIGHT Voice4 IDLE->FINE transition (abs_error<=fine_enter), not on subsequent fine frames. No PWM or direction logic change; repeat exact-SHA full CI.
[1] This changes telemetry correctness only, preserving safety and trial threshold. The operator should build/flash only the final verified SHA, not intermediate CI SHA. Private prior 6-run TXT remains off GitHub.

## 2026-11-05 — S15.4 sample clock bug hypothesis after actual RIGHT10 fine log
[0] User provided private 6-line file M8S154_right_fine_20261105_192543.txt, firmware-stamped SHA 699932abbe09e875a776f53403a18d7743171b9f; do not publish raw data publicly.
[1] Real RIGHT RAW-increasing Voice15/30/40 fine-entry=10; RIGHT pulses=0/0/0, reversals all0, rightFineMs205, end error +4/+1/+2, both servo result2, no dropped. Previous 15RAW baseline pulses 1/6/6, reversals0/1/1. Total command time 3159/3710/4226 -> 1563/2319/2835 ms. This supports preserving RIGHT10 but does not prove causal generality from single sequences.
[2] RIGHT RAW-decreasing Voice15/30/40 retains entryLimit15; pulses1/2/3, right error -4/-5/-5, leftDoneMs1155/1887/2919, rightDoneMs1167/2427/3387. No tuning for this direction yet.
[3] All six recent hardware logs show syncFrames=0 and peakGap=0. This means no actively applied S15.4 pacing recorded, regardless of visually synchronized coarse travel. Do not report S15.4 sync as hardware-PASS.
[4] SOURCE hypothesis: Steering_PositionUpdate takes now=HAL_GetTick before both axis I2C reads. Each Steering_PositionReadOne saves last_sample_ms at later tick. Old Steering_VoiceS154Sync(now,... ) unsigned age check rejects future sample stamps as stale. Actual reason not proven without skip counters.
[5] New isolated branch feature/m8-s154-fresh-tick-sync-telemetry forks exact 699932abbe09e875a776f53403a18d7743171b9f; call VoiceS154Sync(HAL_GetTick(), ...) AFTER both per-axis reads, leaving all other original now uses unchanged. This may activate formerly inert pacing; cautious motor bench, explicit NC/STOP and power cut required.
[6] Auto-boot flags existing steer_voice_s154_enable=1, steer_angle_wp_enable=1, RIGHT10 and fine diag=1. No manual variable setting, no new grant.
[7] Add per-run read-only syncDisabled/syncPhase/syncStale/syncNc/syncWaypoint counters and append to existing auto M8S154_FINE UART line; syncFrames remains actual active count.
[8] Preserve RIGHT10 handoff, LEFT, Voice45 NC, R1/R2, fine PWM85/12ms, 200ms off, <=8 pulses, tolerance +/-5, 300/100 caps, STOP/NC/PS2, 650ms stall, global Voice5s.
[9] Existing Windows PowerShell auto capture script reused, Windows5.1 parser CI preserved; no Live Expressions needed.
[10] CI status pending until exact pushed SHA and workflow success verified; no CubeIDE target/hardware success claimed.
[11] Operator next: locally protect edits, fetch/switch new branch SHA, CubeIDE Clean Build/Flash and verify MCU stamp, preflight power-off NC/STOP/AS5600, secured rig, first SMALL angle then gradual tests. Stop if pacing causes unexpected movement.
[12] One private TXT with six runs is sufficient if safe; compare syncFrames and new rejection counters, completion times, terminal errors. If syncFrames remains0, diagnose which gate without changing PWM.
[13] No manual enable writes or faked sync. Default1 avoids extra user effort but doesn't bypass motion permit or safe test setup.
[14] Source docs/M8_S154_SYNC_SAMPLE_CLOCK_2026_11_05.md and tools/verify_m8_s154_sync_sample_clock.py. Original 386-line requirement file unchanged.
[15] Future chat read new branch HEAD+full handoff, commit SHA+Actions and operator logs privately, not just historical line counts.
[16] Stages remain SOURCE-STAGED -> HOST PASS -> CubeIDE BUILD -> FLASH -> HARDWARE PASS, all independent.

## 2026-11-05 — S15.4 fresh-tick first HOST fixture correction
[0] Initial source SHA 1ece00ae9ccac2316d3b357b4ad89fa40cbc6c83 passed Windows PowerShell syntax but failed extracted real-C S15.4 HOST compilation: the fixture lacked five new passive gate counters. Add only C fixture globals and correct a malformed assertion in the new skip-counter verifier. No firmware/source, motor/NC, fine, or UART changes, and no hardware PASS claimed.
[1] Exact latest SHA and full CI success required before CubeIDE Build/Flash and user log.

## 2026-11-05 — Fourth trial: RIGHT Voice fall 10-RAW threshold
[0] User physically tested exact c2c2d09eb56e3b3e5be7474853655c6cdfb25395 and sent private log M8S154_right_fine_20261105_194122.txt. Do not publish raw TXT.
[1] Six Voice15/30/40 both directions: all resultL=resultR=finish=2, RIGHT final abs(error)<=5 RAW; no reverse, dropped=0.
[2] S15.4 sync now physically ACTIVE: syncFrames=80/119/150/183/204/232, syncDisabled/stale/NC/waypoint=0. Peak gap positive 44-213, not exact simultaneous movement.
[3] RIGHT RAW-increase 10RAW entryLimit, 0 fine pulses all 3, rightFineMs210, skew11/77/77; proven condition unchanged.
[4] RIGHT RAW-decrease (Voice LEFT) entryLimit15 RAW, one fine pulse each, rightFineMs441/441/452; skew231/154/495; at 30 RIGHT finished EARLIER by154 so shorter RIGHT time may worsen skew there.
[5] Isolated branch feature/m8-s154-right-voice-fall-10raw forks tested c2c2d09e; prior tested branch untouched.
[6] New boot1 flag steer_voice_s154_right_fall_late_fine_enable. RIGHT only, Voice mode4, voice owned, err<0, goal>=R_MIN +30 RAW selects 10RAW fine entry. Reverts original threshold when any gate fails.
[7] RIGHT RAW-increase behavior unchanged, S15.4 post-I2C time and coordinator unchanged, LEFT/R1/R2/manual/Voice45 untouched. NC/STOP/PS2, physical encoder safety, CCR300/100 caps, 85/12ms RIGHT fine, 200/100/120ms quiet, 8-pulse/+-5 tolerance, stall and timeouts preserved.
[8] All test flags boot1; no Live Expressions, no automatic motor command or safety grant.
[9] Existing auto UART M8S154_FINE and native PowerShell script unchanged; entryLimit reflects actual first fine entry.
[10] Extend existing actual-C helper host test for negative direction, clearance thresholds, independent toggles; full exact-SHA CI required.
[11] Operator future: clean worktree switch feature branch, CubeIDE Clean Build and matched flash, physical NC/STOP preflight, fixed rig, independent motor cut, Voice15 LEFT first then six Voice 15/30/40 both directions if safe, send private TXT.
[12] Performance improvement is hypothesis only; may degrade near endpoint or increase skew at30 LEFT, stop on overshoot/error.
[13] No physical acceptance for new branch until test and log.
[14] docs/M8_S154_RIGHT_FALL_10RAW_2026_11_05.md and tools/verify_m8_s154_right_late_fine.py, legacy required docs unchanged.
[15] Rollback source SHA c2c2d09eb56e3b3e5be7474853655c6cdfb25395.
[16] SOURCE-STAGED / HOST CI / CubeIDE BUILD / FLASH / HARDWARE PASS independent.

## 2026-11-05 — M8 S15.5 >90% normalized progress goal, isolated LEFT15 leader taper trial
[0] User requested improve both steering axes to >90% tracking on ALL six Voice 15/30/40 LEFT/RIGHT cases. Prior exact physical-good SHA 9cd6067cf9e7a9885ee4c36121e3912e80956860, private log M8S154_right_fine_20261105_195646.txt; no raw private log publicly.
[1] Correct peakGap series = 214,52,40,25,49,62 permille; worst point % = 78.6,94.8,96.0,97.5,95.1,93.8. Only LEFT15 fails. All 6 resultL/R/finish=2, RIGHT fine pulses0, syncFrames75..209, all errors abs<=5 RAW. >90 requires peakGap<100, NOT just shared arrival or average over command.
[2] Source common S15.4 waypoint controls only leader nonzero taper and follower bounded boost. Existing taper deadband=lead30 and ramp120 => full floor only at gap150. Trial only on Voice mode4, active owner, last angle15 and last turn LEFT, new boot1 enable steer_voice_s155_left15_taper_enable.
[3] On this eligible command, leave deadband=lead30 but shorten taper ramp120 to70 => full prevalidated floor at gap100 (left175,right50), never zero PWM. Normal cases retain bit-exact legacy math per HOST compiled actual C test.
[4] No changes to LEFT/R1/R2/Voice45 NC, existing two-direction RIGHT10 RAW fine-entry, sensor reading, S15.4 post-I2C fresh tick, physical NC IRQ/late CCR, STOP PS2 or motor max300/100, ±5 acceptance, voice 5s watchdog or 650ms stall.
[5] Keep all required flags boot=1 and do not demand manual Live Expressions. New S15.5 flag boot1 but is not motor/auto-ARM permission.
[6] Auto M8S154_FINE line appends peakLeader (1L/2R), peakAtMs and earlyTaperFrames. Record per-run with original fields, UART safe idle, Windows PowerShell collector unchanged, no public private logs.
[7] New isolated branch feature/m8-s155-voice15-left-early-taper from tested 9cd6067c; baseline and earlier branches preserved.
[8] New tools/verify_m8_s155_voice15_left_taper.py compiles real C helper, checks gating, bit-exact fallback thousands of samples, never below floor and effective trial at gap100. CI step added plus old exhaustive safety suites.
[9] Real vehicle >90% improvement NOT established. No claim that changing normalized taper guarantees <100 peak; may increase total time/timeout or terminal skew. Only after exact SHA full Actions success, CubeIDE Clean Build/Flash and protected rig test.
[10] Operator next tests 15 LEFT FIRST, stops if unsafe, then safe 6 Voice 15/30/40 both directions, sends ONE private TXT. Compare all peakGap <100, earlyTaperFrames >0 at 15LEFT, peakLeader/AtMs, no faults, pulse/reverse, right error, syncFrames, timing. If not >90, use new peak diagnostic to choose next controller fix.
[11] Roll back to exact known physically-good 9cd6067cf9e7a9885ee4c36121e3912e80956860, clean build/flash.
[12] Original original 386-line requirements preserved; docs/M8_S155_VOICE15_LEFT_EARLY_TAPER_2026_11_05.md.
[13] SOURCE_STAGED until exact Actions; HOST then target CubeIDE BUILD, FLASH and HARDWARE PASS independently.
[14] No code speed/pulse changes after build until user real log as requested.
[15] Do not overstate visual "two motors bám" from peakGap: it is normalized progress maximum, not mean speed or average.
[16] Safe stop, physical NC and supply cut remain mandatory if altered pacing stalls.

## 2026-11-05 — M8 S15.5 HOST extracted fixture upkeep
[0] Initial S15.5 trial SHA 2d486d15245cb83a1143af39bb56b128326c6e0b Windows PowerShell syntax PASS but full context HOST FAIL: preexisting extracted-C S15.4 coordinator fixture had no new read-only peakLeader,peakAtMs,steer_pos_start_ms fields. Update test fixture ONLY.
[1] Also update existing extracted-C RIGHT snapshot HOST fixture for three new diag fields/globals and check values. Correct new regression's unnecessary conditional assert to direct workflow assertion.
[2] Firmware control, NC/STOP, UART protocol, PWM and S15.5 ramp untouched in this follow-up. Full exact-SHA CI must PASS before CubeIDE.
[3] Physical >90% still pending on trial hardware. Known tested rollback 9cd6067c, never force reset user's local worktree.

## 2026-11-05 — S15.6 Voice LEFT15 startup RIGHT cap from actual S15.5 log
[0] Operator supplied private TXT M8S154_right_fine_20261105_203007.txt with flashed physical SHA 2f3b3a250601d77add3bdab219e6385a98e4456d. Do not upload original private TXT publicly.
[1] Actual S15.5 LEFT15 peakGap201permille = 79.9% worst-point (baseline 9cd6067c peak214=78.6); peakLeader2 means RIGHT was ahead, peakAtMs245, earlyTaperFrames17. Reactive early taper ran but only improved 1.3 percentage points, insufficient for strict >90 target.
[2] Other five logged commands: RIGHT15 peakGap56 (94.4%), LEFT30 37 (96.3%), RIGHT30 20 (98%), LEFT40 47 (95.3%), and RIGHT30 repeated peakGap55 (94.5%). Sixth item is NOT RIGHT40; true six-distinct cases not yet completed for S15.5. Prior 9cd6067c had RIGHT40 peak62 (93.8%).
[3] All logged commands in latest file resultL=resultR=finish=2, RIGHT fine pulses0/reversals0 and errors within +-5 RAW, syncFrames>0, no stale/NC/waypoint failures. Do not change fine thresholds.
[4] New isolated S15.6 branch feature/m8-s156-voice15-left-start-right-cap from tested 2f3b3a2, no baseline overwritten. New global steer_voice_s156_start_cap_enable=1U at boot, does not ARM motors or override safety.
[5] After actual Voice S15.4 normal two-axis fresh AS5600 coarse waypoint and other eligibilities, and BEFORE existing atomic late NC/CCR commit: only Voice4 15deg LEFT, when elapsed<=350ms and RIGHT normalized progress >= LEFT, reduce final RIGHT proposed PWM to existing nonzero clamp floor50CCR; if behind, elapsed>350, invalid mode/ownership or no headroom, preserve original proposed PWM. Never increases duty or goes below floor.
[6] Existing S15.5 leader ramp70 remains, as do S15.4 normalized coordinator math/post-I2C sample tick, RIGHT10RAW fine entry both directions, all caps300/100, NC/STOP/PS2, Voice45 expected NC, R1/R2, stall and 5s timeout.
[7] Existing safe-idle automatic UART M8S154_FINE appends startCapFrames (only frames actually reduced). Retains peakGap,peakLeader,peakAtMs,earlyTaperFrames and sync fields. Windows PowerShell collector unchanged; no Live Expressions edits.
[8] HOST actual-C helper tests cover 15 LEFT only, active window 0..350ms, RIGHT>=LEFT, mode/angle/turn/owner/flag exclusions, not below nonzero floor, never above proposed duty. Older extracted real-C sync and snapshot fixtures adjusted to include helper/new telemetry.
[9] CI initial SHA 659758fe1ca387c5b2fd25ed24b6c9e8b58b1d8f failed the CONTEXT_HANDOFF/PROJECT_MILESTONES contract because documentation update had not yet been committed, not an STM32 build error; follow-up commit updates both, full exact SHA CI then required.
[10] S15.6 physical tracking goal ALL six distinct (15LEFT/RIGHT, 30LEFT/RIGHT, 40LEFT/RIGHT) peakGap<100permille (strictly >90%), resultL/resultR/finish=2, syncFrames>0, RIGHT fine0, no reversal/fault, acceptable completion/skew. One 6-run sample is preliminary; repeat if passes.
[11] RIGHT cap could cause mechanical under-speed/stall; protected fixed rig, NC and STOP power-off preflight, independent motor cutoff; Voice15LEFT first and stop if irregular, then 5 remaining only if safe.
[12] Operator does not manually edit flags; all required new and old trials boot1. Sender is passive and deferred to safe UART idle; capture script defaults 6 rows.
[13] No confirmation >90 until user returns exact-SHA physical TXT. Do not claim real-world pass from HOST CI.
[14] Physically tested reliable fallback source SHA 9cd6067cf9e7a9885ee4c36121e3912e80956860. S15.5 SHA 2f3b3a2 also physically ran five distinct commands, but not RIGHT40 in newest log.
[15] Read docs/M8_S156_VOICE15_LEFT_START_CAP_2026_11_05.md for rationale, uncertainty and acceptance. Preserve original 386-line spec, historical 17-section handoff and roadmap.
[16] Explicit stages: SOURCE COMMITTED -> HOST CI -> CubeIDE BUILD -> FLASH -> physical PASS; only assert stage with actual evidence.

## 2026-11-05 — R1/CENTER RIGHT hunting improvement experiment after S15.4 PASS
[0] USER explicitly marked M8 S15.4/S15.6 PASS after new private 20261105_205603.txt at MCU 9db12228f38c1589bc6229409e3e210f598561ed; LEFT/RIGHT15,30,40 all 6 correct, peakGap28/41/45/47/51/56 => worst-point 97.2/95.9/95.5/95.3/94.9/94.4%, all DONE2, RIGHT pulses0, errors <=5. Do not claim 45/STOP/NC covered by this file. Original TXT remains private.
[1] New user problem: PS2 R1 AND voice standalone CENTER cause RIGHT to repeatedly oscillate/correct until target. Both use steer_pos_command1 and same RIGHT center662 RAW. This is operator observation; NO R1-specific hardware trace yet.
[2] Source original mode1 RIGHT minimum coarse->fine cutoff15 RAW, fine85CCR/12ms, bounded 8 pulses, ±5 tolerance, wait 200/100/120ms. Other successful Voice4 RIGHT10 cut and coast6..8 suggests test later handoff RIGHT R1 only; not proven identical coast at center.
[3] Isolated feature/m8-r1-voice-center-right-late-fine-10raw forks exact tested 9db12228..., default ON new steer_r1_right_late_fine_enable=1U. Eligibility: RIGHT only, mode1 (both PS2 R1 and Voice B0), valid RIGHT runtime calibration, RIGHT center >=30RAW from BOTH NC ends. Then entry10; otherwise fallback old15.
[4] RIGHT Voice4 two-way10RAW is explicitly unchanged, LEFT R1 and all R2/Voice45 physical NC unchanged, safety/STOP/PS2/owner, cap300/100, fine85/12ms, max8, ±5 and timeouts unchanged.
[5] Passive new mode1 R1 snapshot M8R1_FINE auto logs BOTH origins source1 PS2/source2 VoiceB0 on safe idle UART3 using existing nonblocking sender. Entry limit latched at first RIGHT fine; includes progress times, pulses/reversals, short-pulse evidence and final outcomes. Prior automatic M8S154_FINE unchanged. Script tools/capture_m8_r1_center.ps1 max8, no command injection/Live Expressions.
[6] Existing source R1 right error fields are not sufficient to prove cause; no physical pass for NEW branch until user tests with protected rig and sends new private log. New HOST helper tests actual C, route exclusions, geometry and UART guards; WindowsPowerShell parser CI must pass exact SHA.
[7] User next: local worktree protect; git fetch/switch trial; CubeIDE Clean Build, Flash matching SHA; NC/STOP unpowered preflight; secured motor test with independent supply cut; PS2 R1 from moderate offset first, then Voice CENTER, then return from MIN/MAX in each direction if safe; max8 serial lines. STOP immediately on unsafe jitter/NC/overshoot.
[8] Desired success: resultL=resultR=finish2, RIGHT error±5, significant reduction in pulses/reversals vs real R1 prior; no lengthening or new fault/NC event. If no historical R1 counts, compare physical response and repeated trials before claiming improvement.
[9] Baseline rollback exact physical 9db12228f38c1589bc6229409e3e210f598561ed preserved. Do not modify tested branch.
[10] New doc docs/M8_R1_VOICE_CENTER_RIGHT_LATE_FINE_2026_11_05.md, actual-C tool tools/verify_m8_r1_right_center_late_fine.py.
[11] 386-line original requirements unchanged, append history under original 17-section handoff.
[12] SOURCE COMMITTED / HOST CI PASS / CubeIDE BUILD / FLASH / HARDWARE PASS are separate.
[13] No manual debug flags; new boot flag ON but does not grant motor power or bypass NC.
[14] R1 right from MIN/MAX safety guards also unchanged, separate per-motor escape and NC release debounce preserved.
[15] Source pass doesn't ensure hardware smoothness; only user physical data adjudicates next step.
[16] The user explicitly asked to improve both trigger origins, so one common center-mode patch avoids treating voice misrecognition as the cause of this R1 hunting.


## Cập nhật 2026-11-09 — M8 Voice Rotate, M9 paused

- Đổi phạm vi theo người dùng: dừng M9 HOLD; dùng đúng nền d367618d, giữ nhánh M9 riêng.
- Nguồn chứng minh lỗi: Voice gọi `WheelControl_SetSpinDirection` một lần rồi hủy
  ngay nếu API chưa active; API thực có thể defer vì PID target/encoder/RPM/PWM.
  PS2 L1/L2 giữ nút thì gọi lại. Chưa có log Rotate trên xe để kết luận đây là
  nguyên nhân duy nhất của hiện tượng người dùng báo.
- Sửa: stage2 chờ API chấp nhận tối đa 2000ms, không tiêu hao duration;
  bắt đầu đồng hồ khi accepted; đã vào spin mà mất owner thì hủy, không tự khởi động lại.
  R2 giữ deadline6500ms, cả hai MIN physical NC và không MAX; đang chờ/spin mất NC
  vẫn emergency; FF/PS2 và PID/ramp/cap400 không đổi.
- `M8_ROTATE` là snapshot thụ động, UART IRQ sau motor idle, lưu lý do dừng,
  R2/wait/spin duration, NC và kết quả hai trục. Pending chưa gửi không bị ghi đè.
- HOST integration dùng actual C executor + arbiter + PS2 ownership + real wheel
  spin admission/target API. Mã cũ fail tại lần defer đầu; mã mới PASS LEFT/RIGHT
  5/10/120s, R2/wait excluded, C1..C5, FF/PS2/NC, timeout, lost owner, rollover.
- Chưa có CubeIDE target build, flash hoặc xác nhận chuyển động trên xe của bản sửa.
  GitHub CI của SHA mới phải được kiểm tra riêng; không tự ghi PASS trước khi run xong.
- Bước tiếp theo: switch nhánh M8 và ff-only, Clean Build/flash/Resume từ thư mục
  Windows chính; chạy `tools/capture_m8_vc02_rotate.ps1`, thử LEFT5/RIGHT5/LEFT10/RIGHT10.
  Không yêu cầu bật Live Expressions. Nếu còn dừng ngắn, đọc result/started/NC trong
  chính log MCU mới, không tăng lực hoặc bỏ NC theo giả thuyết.


## Cập nhật R2 continuation — 2026-11-09

- Evidence mới: MCU d4894029, cả LEFT/RIGHT, duration5–30s đều hủy stage1,
  R2 170–350ms, pos9/result4, started0/spinMs0, NC0/trip0. File gửi lại có cùng
  nội dung, không phải một lần test độc lập. Không đưa raw log riêng tư lên repo.
- Bản sửa trước chỉ giải quyết defer lúc bắt đầu spin; lần đo này chưa tới bước đó.
  Lỗi source bổ sung: gate Voice R2/45 tái kiểm tra RPM đứng yên ở mọi vòng điều khiển.
  Khi đĩa lái kéo bánh thụ động, gate hủy position9 rồi Voice result4. PS2 R2 không
  có điều kiện RPM này. HOST với actual production guard tái hiện đúng chuỗi lỗi;
  log cũ không đủ để khẳng định RPM là nguồn hủy thực tế duy nhất.
- Sửa admission/continuation riêng: admission vẫn full WheelsStopped; active
  Voice NC seek chỉ chấp nhận wheel command/ramp/PWM idle, không spin/FF test.
  NC/AS5600/timeout/STOP/PS2/motion permit vẫn kiểm tra. Sau R2, real wheel spin API
  vẫn đợi RPM dừng, giữ bounded startup wait và đồng hồ từ lúc accepted.
- Test actual guard + arbiter + executor + wheel admission bao phủ 4 thời điểm log,
  RPM từng bánh ±3 và biên2, 14 điều kiện cấm thực, Voice45, 5..40, spin/time/STOP.
  Source cũ fail pos9/result4/started0, source mới PASS. Per-axis servo/HAL mô phỏng;
  chưa chứng minh chuyển động thực, CubeIDE target build hoặc flash bản mới.
- Telemetry mới snapshot trước cleanup: passiveFrames, abortSrc, abortMask, permit;
  UART IRQ chỉ sau toàn bộ motor idle. Script PowerShell cũ tự nhận các trường mới.
- Tiếp tục bằng update ff-only trên nhánh M8, Clean Build/flash/F8, thu 2 lần
  Rotate LEFT5/RIGHT5; nếu được cả hai thì kiểm tra thêm10s. Không yêu cầu thao tác
  Live Expressions; đọc MCU SHA và result của log mới trước khi đánh dấu hardware PASS.


## Hardware acceptance update — 2026-11-09

**XÁC NHẬN HIỆN HÀNH 2026-11-09 — ROTATE ĐÃ HOẠT ĐỘNG TRONG HAI LẦN THỬ MỚI.**
Người vận hành xác nhận đã sửa được lỗi. Log MCU `b382354ec1592589843f4b579643e5b1abbbecf0`
ghi RIGHT yêu cầu5s, thời gian lệnh xoay5006ms; LEFT yêu cầu10s, thời gian10002ms.
Cả hai result1/started1, hai MIN NC hợp lệ, trip0, không hủy. Đây là PASS cho hai
lần thử được ghi và xác nhận chuyển động của người vận hành. R2 chỉ12–13ms và
passiveFrames0, nên log này không kiểm chứng riêng toàn hành trình R2 từ CENTER
hoặc chứng minh RPM thụ động là nguyên nhân duy nhất của log lỗi cũ.
Giữ `b382354e` làm mốc firmware Rotate đã thử thành công; M9 vẫn tạm dừng.
Không tiếp tục chỉnh Rotate hoặc tự quay lại M9 khi chưa có chỉ thị mới.

This is a documentation-only acceptance update. Firmware source is unchanged from b382354e.
The two captured commands complete within one control-loop interval of the requested time.
No new firmware flash or repeat test is requested merely to record this result.
Raw operator log is not committed; only the scoped result summary is recorded.
## 2026-11-08 — H2-B1 uncommissioned MCU log and motor identification, 17 checkpoints

[0] Phạm vi: tiếp tục FULL persistent two-axis H2-B1; không chuyển sang chương trình một trục/phiên ngắn.
[1] Phiên bản: private MCU UART4d004757f48f67c1b1b5192b8020c3e8d196ba9a; branch HEAD pinned at same SHA before this docs-only update.
[2] Mục tiêu: ghi đúng motor được tài liệu mới mô tả và phân loại bằng chứng MCU mới; không tự đặt dòng/gain.
[3] Yêu cầu: động học chưa hoàn thành để tích hợp/nạp; SELECT tạm hoãn; raw UART cá nhân không publish.
[4] Milestones: limited MCU uncommissioned target-retention/telemetry PASS for two sessions, not physical powered HOLD PASS.
[5] Kết quả: seq1/source2 goal1886/662 actual1882/664 errors4/-2 at5002ms updates465; seq2/source3 goal2160/906 actual2157/901 errors3/5 at5001ms updates471.
[6] Điểm dừng: cả4dòng active1/state2/reason7 commissioned0/lease0; PWM,peakPWM,appliedMs,everApplied đều0; không xác nhận mô-men.
[7] Tài nguyên: private M9H2B1_HOLD_20261108_165103.txt and technical DOCX v2.0 dated2026-10-28 newly supplied; publish only these aggregates.
[8] Quyết định: DC775 is recorded as family/type only; manufacturer/winding/gearbox variant not inferred from generic775 or30/32rpm.
[9] Chẩn đoán: dropped rises0->1 across sessions; actual main.inc increments when a pending status is replaced at new Begin. Not sensor/loop loss; no proof of delivery of the previous terminal reason.
[10] An toàn: no source/actuator/driver changes; power commissioning stays0; user running-current measurements remain baseline only.
[11] Kế hoạch: keep continuous two-axis controller; establish actual PWM/gain/reversal and stationary current/thermal envelope before activating physical HOLD.
[12] Thông tin còn thiếu: motor exact variant/data, standstill limits and installed driver response; no request to repeat already-supplied log/H1.
[13] Lịch sử: earlier statements MCU819f635 / no B1 MCU log are superseded by this4d004757 UART evidence, retained as dated history.
[14] Thông số: DOCX §1 DC775 geared wheel110rpm x4, LEFT30rpm, RIGHT32rpm. DOCX §24 still lists motor model/gear ratio/stall/torque unknown;80/200W and5/10A earlier user estimates remain separate.
[15] Tiếp quản: read newest top status/report; target execution inferred from UART SHA, not a recorded CubeIDE build transcript; no torque/thermal/fault/load conclusion.
[16] Kiểm chứng: four complete rows checked for SHA,state,zero-power,goal-raw=error,monotonic per-session updates and elapsed>5s; docs-only scope preserves original handoff headings/spec and firmware.

## 2026-11-09 — Powered two-axis H2-B1 tuning, 17 checkpoints

[0] Phạm vi: USER explicitly requests actual powered full H2-B1 with existing STOP/NC and rejects motor-supply disconnect test as prerequisite. Do not require confirmation for that same rejected test.
[1] Phiên bản: source base exact227f75a0b81d61b971528bc0879b6f5f79afa6e4; last physical UART4d004757 is old unpowered evidence only.
[2] Mục tiêu: persistent two-axis correction after eligible BOTH-DONE, with real CCR power allowed by explicitly experimental tuning profile.
[3] Yêu cầu: no session expiry/single-axis substitute; keep kinematics incomplete/SELECT deferred and private UART private; no new controls or motor-supply disconnect request.
[4] Milestones: H2-B1 powered tuning source and actual-C HOST tests ready; target build/flash and powered MCU/current/thermal/mechanical acceptance still pending.
[5] Kết quả: power_mode LOCKED0/TUNING1/COMMISSIONED2; production mode1 permits bounded power while POWER_COMMISSIONED0 honestly records unaccepted physical envelope.
[6] Điểm dừng: publish/verify exact-SHA Actions, then user CubeIDE Clean Build/flash/Resume and one true two-axis HOLD disturbance/release/STOP capture; cannot call it executed on physical car here.
[7] Tài nguyên: docs/M9_H2B1_POWERED_TUNING_2026_11_09.md is current parameters/protocol; full-HOLD08 report retained as source/history.
[8] Quyết định: initial caps30/10CCR=3%/1% of1000counts; Kp2/0.75,Kd0.125/0.046875,rise60/20CCR/s,zero200ms. These are engineering test defaults, not motor/datasheet/current ratings.
[9] Chẩn đoán: reason11 NO_PROGRESS if acknowledged output persists without2RAW error improvement in1000ms; reason12 EFFORT_BUDGET. First fault-axis bit retained; cuts both/no auto-resume.
[10] An toàn: STOP/NC/owner/EXTI/epoch/age/jump/range/atomic guards retained. Test budget1500ms nonzero per axis replenishes only during zero at1500/8500; no fixed-window boundary reset. Not amp/I2t/thermal protection or independent hardware cutoff.
[11] Kế hoạch: expected first quiet state3/profile1/commissioned0; then need nonzero acknowledged output and actual observed angular correction, both-axis logs and terminal STOP. Do not auto-raise caps after guard trip.
[12] Thông tin còn thiếu: motor physical variant, calibrated standstill-current/thermal envelope, installed driver stop/reversal behavior; user request authorizes tuning but does not establish those facts.
[13] Lịch sử: previous insistence on a motor-disconnect pretest superseded by explicit USER instruction09/11. Existing4d unpowered log remains limited PASS, no repeat.
[14] Thông số: latest DOCX DC775 wheel110rpm x4,LEFT30/RIGHT32rpm; user running motor-wire currents0.8–1/2A and stopped0A retained without converting to safe HOLD ratings.
[15] Tiếp quản: collector -UntilStopped -MaxEvents100 -TimeoutSeconds300, passive RX; ending collector never ends HOLD. UART adds profile,peakErr,effort,faultAxes,appliedLms/Rms; commissioned0 alone no longer means output locked.
[16] Kiểm chứng: actual production mode1/core/HAL/main60s quiet+real mock writes/stall cut, budget recovery/tick wrap, compiler invalid-mode/guard/cap rejection, actual snprintf compile format and UART545<640; ASan/UBSan and H0/H1/H1REAL/H2A/B0/Voice STOP/autoARM regressions PASS locally. Hardware acceptance pending.

## 2026-11-09 — First powered-tuning MCU evidence,17checkpoints

[0] Phạm vi: analyze the user's actual first powered-tuning UART, preserve full two-axis HOLD and no motor-disconnect prerequisite.
[1] Phiên bản: exact MCU621a5ce87385600314404bc8866662c0102f4df4; same pinned branch HEAD before docs-only update. Source CI42599987381 already SUCCESS.
[2] Mục tiêu: separate register-output/guard evidence from successful mechanical holding or torque/current acceptance.
[3] Yêu cầu: no automatic cap/gain/timeout increase, no old H1/B0/unpowered-log repeat, private raw TXT remains private.
[4] Milestone: MCU output+NO_PROGRESS cutoff observed LEFT in one trial; H2-B1 mechanical HOLD/current/thermal not PASS and RIGHT powered response not exercised.
[5] Kết quả:3 valid rows WAIT37ms/HOLD109ms/FAULT1868ms, same seq1/source2 CENTER/profile1; dropped0 and goal-raw=error agree.
[6] Điểm dừng: state5/reason11 active0 lease0 faultAxes1, both commandedPWM0; terminal is a response-guard fault, not Voice STOP2.
[7] Tài nguyên: private M9H2B1_HOLD_20261109_172834.txt; aggregates appended to current powered-tuning report only, no raw upload to public repo.
[8] Quyết định: source unchanged. First gather observable release timing before selecting force/gain/guard changes.
[9] Chẩn đoán: LEFT cap30 reached, acknowledged output1007ms and final/peak error23RAW; NO_PROGRESS indicates insufficient measured error improvement, not proof of a unique physical cause.
[10] An toàn: guard removed both outputs/no auto-resume; this is a software observation, not independent driver-current measurement or thermal protection.
[11] Kế hoạch: ask whether LEFT was released promptly after displacement or held externally through the interval; evaluate any existing observed motor current afterward.
[12] Thông tin thiếu: release timing, actual recovery behavior/current/temperature; dense trace not present. Do not infer user did or did not release from this log.
[13] Lịch sử: commissioning0/profile1 correctly means experimental powered permission; prior4d unpowered5s evidence stays separate.
[14] Thông số: LEFT goal1886/raw1909/error-23/peak23,RIGHT goal662/raw660/error+2/peak3; RIGHT peakPWM0 and applied0.
[15] Tiếp quản: preserve running MCU621a5ce. Collector UntilStopped can end at FAULT before subsequent spoken Stop; absence of STOP row does not prove recognition failure.
[16] Kiểm chứng: file3 realCRLF rows, fullSHA, source/seq/profile/state sequence, zero final outputs/lease, per-axis times and arithmetic checked. Docs-only change preserves original386-line specification,17sections and required footer.



## Tiếp tục M9 từ nền Rotate đã nghiệm thu — 2026-11-09

**HIỆN HÀNH 2026-11-09 — TIẾP TỤC M9 TRÊN NỀN ROTATE ĐÃ FIX.**
Theo chỉ thị mới, M9 đã được tiếp tục trên `feature/m9-steering-active-position-hold`.
Nền firmware là `b382354ec1592589843f4b579643e5b1abbbecf0`, đã có xác nhận Rotate
RIGHT5s/LEFT10s; hồ sơ nền là `22e7eb5cd18c2441e89af1170831efe39fbc38a8`.
Tích hợp toàn bộ M9 mới nhất từ `3df43edde8f53b5e84561704189a83ac8286a56b`.
Commit tích hợp dùng22e7eb5 làm parent thứ nhất và3df43ed làm parent thứ hai,
giữ cả lịch sử, không force-push và không đổi nhánh M8 đã thử thành công.
Các ghi chú M9 tạm dừng bên dưới là lịch sử đã được chỉ thị này thay thế.
Cả hai fix Rotate được giữ: R2 tiếp tục khi chỉ có RPM bánh thụ động; timer spin
chỉ tính từ lúc wheel API nhận lệnh. M9 nhường HOLD ngay khi Rotate hoàn chỉnh
đang chờ arbitration, cắt lease/deadline cũ trước khi R2 ghi PWM đầu tiên.
Giữ candidate M9 LEFT300CCR/<=200ms, RIGHT85CCR/<=12ms và PD30/10 như3df43ed;
chưa có log hardware xác nhận candidate HOLD này. Rotate PASS của b382354e
không tự chuyển thành hardware PASS của bản M9 tích hợp. Xem [báo cáo tích hợp](docs/M9_ROTATE_BASE_INTEGRATION_2026_11_09.md).

END OF CONTEXT HANDOFF
