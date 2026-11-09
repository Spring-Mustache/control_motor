# Xe tăng cướp bóng — STM32F767ZI

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

> **Motor-lead current baseline (USER,2026-11-08):** khi CENTER trái0,8–1A/phải≈2A; sau dừng đồng hồ đọc0A. Mức5/10A từ công suất chỉ là ước lượng, không phải giới hạn HOLD. [B1 bench design và giới hạn dữ liệu](docs/M9_H2B_POWER_GATE_2026_11_08.md). H2-B1 chưa cấp lực; firmware819f635.

> **Trạng thái mới 2026-11-08:** H2-B0 MCU **STATIC INTEGRATION/TELEMETRY PASS một lượt Voice CENTER**, 3003ms tại firmware819f635; đầu ra HOLD0, candidate0. H2-B1 cấp lực/H3 tích hợp chưa hoàn thành. [Kết quả và giới hạn](docs/M9_H2B_POWER_GATE_2026_11_08.md). **USER clarification (2026-11-08):** Động học chưa hoàn thành để tích hợp/nạp vào firmware; SELECT tạm hoãn. Không tự đồng nhất hai trạng thái hoặc coi nội suy RAW của Voice là động học bánh xe hoàn chỉnh.

Firmware 4 motor bánh + 2 motor đánh lái, PS2, VC-02, encoder/AS5600, công tắc NC/STOP.

## Mốc hiện hành (2026-11-06)

**M9 — Steering Active Position Hold**: [nhánh phát triển M9](https://github.com/Spring-Mustache/control_motor/tree/feature/m9-steering-active-position-hold) đã được tạo từ exact `d367618d`; người dùng chọn **software-only** STM32F767ZI + AS5600 + CCS_SHB12, đã có H0 read-only observer (chưa có bộ điều khiển HOLD ACTIVE). H0 đã có **4/4 log trên MCU, DATA CAPTURE PASS giới hạn tĩnh**. **H1 PD SHADOW SOURCE + HOST CI PASS**, đã thu **4/4 cặp UART trên MCU** với SHA `00ccaeace` và xác nhận bộ lọc ổn định khi đứng yên, **chưa thử phản ứng ngoại lực**; H2/H3 vẫn **PLANNED**. M8 Voice 15°/30°/40° six-case được người dùng xác nhận PASS riêng; M8 RIGHT CENTER R1/Voice trial chưa xác nhận trên motor.

**M9-H1 REAL data recorder (source 2026-11-07):** Đã thêm bộ chụp chuyển động thật của M8 khi chạy lệnh vị trí, lưu RAW + measured CCR + PD SHADOW trong RAM rồi xuất UART chỉ khi idle, không mô phỏng/chỉnh Live Expressions/đổi motor control. Thu 4 lượt với `tools/capture_m9_h1_real.ps1`. **SOURCE + HOST CI PASS và đã có 4/4 lượt REAL trên MCU** tại `3dbdfaf`, đủ130 frame AS5600/CCR/u_shadow; chưa thử ngoại lực hoặc giữ góc powered H2; không phải H2 HOLD ACTIVE. [Hướng dẫn một lượt REAL](docs/M9_H1_REAL_DRIVE_CAPTURE_2026_11_07.md).

**M9-H2-A GUARD (SOURCE/HOST CI + one-run ON-MCU 3s PASS at `5dd1502`, 2026-11-08):** Đã có log MCU 13ms `reason3/blockBits256`, source xác định VOICE->IDLE sau DONE bị guard cũ coi mất quyền, đã sửa narrow normal handoff; new 3s MCU log/CI SHA mới cần xác minh. **M9-H2-A trước đó (SOURCE + HOST CI PASS `55db22f`, 2026-11-07):** Đã thêm supervisor OFF/MONITOR/SHADOW/FAULT chỉ sau M8 DONE hợp lệ, kiểm tra STOP/NC/owner/sensor/wheel, near-goal PD score; **duty=0 và compile hard lock M9H2_POWER_PERMITTED=0U**, không HOLD cấp lực. H2-B powered chưa có vì chưa đo dòng/nhiệt/PWM0 driver. [M9-H2-A hướng dẫn](docs/M9_H2_GUARD_SOURCE_2026_11_07.md).

**M9-H2-B0 SOURCE + HOST CI PASS (2026-11-08, 9e4f4533):** bounded candidate HOLD policy after H2-A guard, hyst 8/4 RAW, veto score ngược chiều mục tiêu; power output vẫn hard lock 0, không phải active holding. H2-B1 sẽ cần dữ liệu dòng/nhiệt motor và PWM0 driver. See docs/M9_H2B_POWER_GATE_2026_11_08.md.

## Mở cuộc hội thoại ChatGPT mới

**[Sao chép lời nhắn khởi đầu tại đây](docs/CHAT_B_START_PROMPT.md)** vào hội thoại mới. Bạn không cần tải lại file nếu AI có quyền đọc repository.

Nguồn chính của dự án là **Git commit + tài liệu được cập nhật tại SHA của đúng nhánh**. GitHub Actions **kiểm tra** mỗi push. Context Release/ZIP **chỉ tạo theo yêu cầu hoặc milestone**, không còn được tạo sau mọi push. **Không có Release cho HEAD là bình thường**, không phải thiếu ngữ cảnh.

## Hướng dẫn AI tiếp quản

1. Xác minh HEAD SHA thực tế nhánh **M9 đang làm** `feature/m9-steering-active-position-hold` (fork chính xác từ `d367618d4d9c4c382a9a7208f9b878dfefa21ace`, không coi base là HEAD vĩnh viễn). Nếu người dùng chuyển nhánh, dùng nhánh họ chỉ định; đừng lấy HEAD `main`, M8 cũ hoặc Release.
2. Đọc `CONTEXT_HANDOFF.md` **mục 1 hiện hành**, rồi **toàn bộ 17 mục (0–16) và cập nhật lịch sử**, `docs/PROJECT_MILESTONES.md` ở đầu và cuối, chính sách, đặc tả gốc nguyên văn cùng [kế hoạch M9](docs/M9_STEERING_ACTIVE_POSITION_HOLD_PLAN_2026_11_06.md) và các tài liệu M7/M8 có liên quan. Tất cả ở cùng SHA.
3. Xem 20 commit gần đây, diff mới nhất và kết quả [GitHub Actions](https://github.com/Spring-Mustache/control_motor/actions/workflows/context-handoff-release.yml) trên đúng SHA. Phân biệt SOURCE-STAGED, HOST PASS, CubeIDE Build, Flash, giá treo và mặt đất có tải.
4. Tìm [Release](https://github.com/Spring-Mustache/control_motor/releases) **chỉ nếu muốn gói ZIP/milestone**. Kiểm tra tag `context-<FULL_SHA>`; nếu vắng, tiếp tục trực tiếp từ repository. Không coi bản của commit/nhánh khác là hiện hành.
5. **M9 H0 đã có dữ liệu tĩnh**; **H1 PD shadow đã có code**, tính toán thụ động không ghi motor. H2 opt-in ACTIVE → H3 tích hợp vẫn chưa triển khai. Không có HOLD ACTIVE hoặc hardware PASS. Bảo toàn M8 Voice 15/30/40 six-case USER PASS tại SHA `9db12228…`; trial M8 RIGHT R1/Voice CENTER nền `d367618d…` chưa có hardware PASS. Sau khi sửa code/ra quyết định mới, cập nhật handoff/roadmap và kiểm tra CI; không mặc nhiên có log riêng từ cuộc trò chuyện khác.

## Tài liệu tham khảo

- [Lời nhắn chuyển tiếp cho B/C/D](docs/CHAT_B_START_PROMPT.md)
- [Đặc tả gốc của người vận hành — nguyên văn 386 dòng](docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md)
- [Hồ sơ 17 mục: lịch sử bảo tồn + trạng thái cập nhật](CONTEXT_HANDOFF.md)
- [Roadmap và mốc đã/chưa kiểm chứng](docs/PROJECT_MILESTONES.md)
- [Milestone M9 — Steering Active Position Hold: thiết kế và acceptance gates](docs/M9_STEERING_ACTIVE_POSITION_HOLD_PLAN_2026_11_06.md)
- [M9-H0 — Passive post-DONE drift telemetry, test và collector](docs/M9_H0_PASSIVE_DRIFT_2026_11_06.md)
- [M9-H1 — PD SHADOW chống nhiễu, không cấp PWM](docs/M9_H1_PD_SHADOW_2026_11_06.md)
- [M9-H1 REAL — Một lần thu vận động xe thật, RAW/CCR/u_shadow](docs/M9_H1_REAL_DRIVE_CAPTURE_2026_11_07.md)
- [Chính sách và cách phát hành ZIP tùy chọn](docs/CONTEXT_RELEASE_POLICY.md)
- [Workflow CI kiểm tra và nút tạo Release thủ công](https://github.com/Spring-Mustache/control_motor/actions/workflows/context-handoff-release.yml)

**Lưu ý:** Source trên GitHub, CI PASS, Release ZIP, CubeIDE Clean Build, Flash và nghiệm thu phần cứng là **những trạng thái khác nhau**. FIRST-STOP RIGHT cấp điện khi CCR=0 đã được người dùng xác nhận fix; không coi các cảnh báo lịch sử cũ là lỗi mới. Không xóa Release, tag hoặc dữ liệu lịch sử.

