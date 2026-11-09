# M9-H0 — Passive post-DONE Steering Drift (source implementation, 2026-11-06)

## Báo cáo nhận log H0 lần đầu (2026-11-06) — SOURCE + HOST CI + MCU DATA CAPTURE PASS giới hạn

- [USER-SUPPLIED PRIVATE UART] Bốn bản ghi đầy đủ từ MCU mang SHA nguồn `8c2b3fe7d57154db7d11f8abca40351c4753d362`, seq1..4, tất cả `reason=1` và `durationMs=3002..3003`. Ba lệnh `source=3` (Voice vị trí) và một `source=2` (Voice B0 CENTER), chưa có source1 PS2 R1.
- [MEASUREMENT] Cả 4 khoảng nghỉ: **peakDriftL/R không vượt 1 RAW**, **peakErrL/R không vượt 5 RAW**, `samplesL/R` mỗi bên 268..269, `ageL/R=1ms`, `bad/jump/zeroDt/dropped=0`, `wheelsMoved=0`. Độ trôi có thể là rung/độ lượng tử cảm biến; chưa có thử tải ngoài được mô tả.
- [CHECK ON SOURCE] Khi permit=1 và no trip, vòng an toàn ở `Core/Src/main.c` đọc cả hai AS5600 mỗi lượt lặp, ngoài idle 50ms alternating. Vì vậy ~89.2..89.6 mẫu/s/trục trong 4 records là **phù hợp source**, không do H0 gọi thêm I2C; không phải thời gian lấy mẫu đảm bảo dưới mọi owner.
- [CONTROL DESIGN] Giá trị `peakVel=90 RAW/s` có thể đến từ một bước **1 RAW / 11ms**, không phải bánh quay duy trì 90 RAW/s. H1 cần tốc độ lọc có window, chống nhiễu lượng tử, stale/dt/owner guards, chạy shadow trước.
- [EVIDENCE BOUNDARY] UART do người dùng cung cấp và stamp SHA khớp code/CI; **H0 DATA CAPTURE PASS (4 cửa sổ tĩnh, giới hạn)**, chưa có log CubeIDE Build/Flash độc lập, không chứng minh PWM0 brake/coast, chống ngoại lực hay HOLD ACTIVE. Không đăng TXT riêng tư vào repository.

## Phạm vi / bằng chứng

**Giai đoạn H0 chỉ ghi dữ liệu và KHÔNG tạo lực giữ trục.** User chọn giải pháp software-only cho STM32F767ZI + hai AS5600 + CCS_SHB12. H0 nằm trên `feature/m9-steering-active-position-hold`, fork từ d367618d và kế thừa M8 CENTER trial chưa có physical PASS. Thử GCC HOST của thuật toán thuần không thay CubeIDE Build/Flash/hardware.

**Không thay:** `Steering_PositionAxisUpdate`, PWM/DIR của LEFT/RIGHT, owner, NC EXTI/STOP, timer compare, sensor I2C transaction, PS2/Voice command mapping, calibrations và power limits. Chỉ thêm một observer và báo kết quả đã ghi.

## Điều kiện kích hoạt

- Tại `Steering_PositionFinish(result=2)` **sau** `Steering_PositionLeftOff()` và `Steering_PositionRightOff()`, nếu hai axis thực sự `STEER_AXIS_DONE` và `resultL=resultR=2`, không có trip: bắt đầu quan sát.
- Chỉ mode1 `R1 CENTER` (source1 PS2, source2 VC-02 B0) hoặc mode4 Voice5–40 (source3), mục tiêu đọc đúng từ runtime mode. Không quan sát mode2 R2 NC / mode5 Voice45 NC; không tự kết luận trạng thái NC an toàn từ góc RAW.
- Phần sampling `M9H0_IdleUpdate()` chạy sau `Steering_PositionTelemetryIdleUpdate()` trong main superloop, dùng **cache** của I2C polling có sẵn (nhánh idle riêng ~50ms xen kẽ, nhưng khi permit=1 vòng safety còn đọc hai AS5600 mỗi pass; log H0 thực tế ~89Hz/trục). Không tự gọi `AS5600_ReadRawAndStatus` hoặc HAL I2C mới. Nếu idle polling tắt/bị chặn, log có ít/0 valid samples.
- Cửa sổ **3000ms**: reason1=window elapsed; reason2=new steering owner/output/debug transaction; reason3=STOP/trip/safety. H0 chỉ quan sát đến lúc kết thúc; nếu record trước chưa gửi thì bỏ record mới và tăng `dropped`, không ghi đè record chưa phát.
- UART3 ST-LINK VCP (115200) xuất **một dòng** `M9H0_DRIFT` qua IRQ nonblocking dùng chung buffer cũ, chỉ khi cả motor/steer owner/wheel đã dừng, không có stream hoặc log dump khác, và UART sẵn sàng. Nếu wheels chạy, observer có thể ghi `wheelsMoved=1`, nhưng chưa gửi đến khi idle.

## Thông số một bản ghi

`M9H0_DRIFT,sha=<40 hex>,seq,source,mode,reason,durationMs,goalL,goalR,startL,startR,endL,endR,peakDriftL,peakDriftR,peakErrL,peakErrR,peakVelL,peakVelR,samplesL,samplesR,badL,badR,jumpL,jumpR,zeroDtL,zeroDtR,ageL,ageR,wheelsMoved,dropped`.

- `peakDrift`: đỉnh trị tuyệt đối sai lệch so với góc RAW lúc DONE, xử lý AS5600 wrap 4096.
- `peakErr`: max |sample-goal|, kể cả lỗi ban đầu; **không** đồng nghĩa trục cơ khí không đổi góc.
- `peakVel`: ước lượng RAW/s dựa trên hai mẫu AS5600 hợp lệ kế tiếp; phụ thuộc độ trễ I2C/jitter và không được dùng làm `Kd` HOLD khi chưa hiệu chuẩn.
- `bad`: đọc lỗi / status magnet invalid; `jump`: hai mẫu liên tiếp cách trên 90 RAW, được **đánh dấu** chứ không tự lọc; `zeroDt`: timestamp bằng nhau, không tính velocity.
- `age`: khoảng cách từ kết thúc record đến mẫu valid cuối; **4294967295** nghĩa là chưa có mẫu valid hậu-DONE, không phải giá trị tuổi bằng0.
- `durationMs` bình thường ~3000ms, ít hơn nếu interrupted; `dropped` đếm record bị bỏ do UART chưa gửi và hữu ích để đánh giá băng thông.

**Không được suy ra** dòng motor, mô-men, trạng thái driver short brake/coast, khả năng kháng ngoại lực hoặc nhiệt từ các cột trên. Firmware chưa đo dòng, không có chương trình giữ PWM.

## HOST và CI

- `python3 tools/verify_m9_h0_drift.py` kiểm tra điểm nối vào `main.c`, trình tự zero CCR trước observer, điều kiện DONE2 và loại NC/FAULT, không motor-write/I2C trong functions H0, safe UART gate; **biên dịch và chạy C thật** `Core/Src/m9_h0_drift.c` với `tools/m9_h0_host_test.c` (GCC `-Wall -Wextra -Werror`).
- GitHub Actions `verify-handoff` phải PASS ở **exact HEAD**, ngoài đó workflow Windows kiểm tra cú pháp `capture_m9_h0_drift.ps1`. Điều này **không** là CubeIDE target build/Flash.
- Quy tắc cập nhật hồ sơ: source code + `CONTEXT_HANDOFF.md` 17 mục + `docs/PROJECT_MILESTONES.md` cùng commit; original 386-line SHA bất biến.

## Thu log sau khi CI PASS và CubeIDE Clean Build/Flash

Trên Windows PowerShell, tại project chính (không `git reset --hard`, không làm mất local edits):

```powershell
cd "D:\STM32 task\control_motor"
git status --short
git fetch origin
git switch feature/m9-steering-active-position-hold
git pull --ff-only
git rev-parse HEAD
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m9_h0_drift.ps1 -TimeoutSeconds 600 -MaxEvents 4 -OutputDirectory .\logs
```

Trước khi flash và chạy: kiểm tra HEAD/dirty, CubeIDE **Clean Build đúng SHA**, Flash board, đọc `sha=` MCU để đối chiếu. Preflight NC/STOP với nguồn motor chưa cấp, đặt xe cố định, chuẩn bị **ngắt nguồn động lực độc lập**, giữ người tránh bộ phận quay. Collector **chỉ lắng nghe**, không phát lệnh; người vận hành mới chủ động chạy R1 CENTER hoặc Voice5–40 đã được thử an toàn trên gá, để 3s sau DONE, đợi wheel/motor idle và lấy record. Bắt đầu một lượt từ sai lệch vừa phải; không cần tạo ngoại lực lớn. Nếu có bất thường motor, STOP/cắt nguồn ngay theo quy trình hiện hành.

Nếu `git status --short` cho thấy sửa đổi local, **dừng thao tác chuyển nhánh/cập nhật** và bảo vệ thay đổi trước, không tự stash/reset ghi đè. Nếu không có COM ST-LINK, script báo lỗi; không tạo dữ liệu giả. Không đẩy log private lên repo public; gửi riêng khi cần phân tích.

## PASS gates / NEXT

1. **SOURCE**: logic H0 có trong GitHub branch và docs; **HOST CI**: xác minh PASS đúng SHA.
2. **CUBEIDE BUILD / FLASH**: riêng, chưa thể xác nhận từ GitHub. Đối chiếu stamp MCU SHA.
3. **H0 on-target OBSERVED**: đã nhận 4 bản ghi người dùng cung cấp với `reason=1`, thời gian 3s, mẫu hợp lệ ở hai bên; **DATA CAPTURE PASS giới hạn cho bốn lượt tĩnh**. Chưa có bằng chứng độc lập khẳng định không ảnh hưởng timing mọi đường đi hoặc có phản lực tải.
4. **H1**: sau khi có baseline và giới hạn lấy mẫu, xây PD shadow với giả lập bỏ tải; **không tự bật HOLD ACTIVE**.

**Lưu ý:** Source/CI không thể chứng minh CCS_SHB12 thật sự brake hay coast khi PWM0. H0 không tác động đến motor, nên mọi kết quả về holding torque vẫn là CHƯA CÓ BẰNG CHỨNG.

## 2026-11-06 — Lưu ý chuyển H1

H0 đã có 4/4 static data capture PASS giới hạn ở firmware 8c2b3fe7; giữ nguyên tiêu chí này. Firmware M9-H1 mới có thêm `M9H1_SHADOW` theo cùng cửa sổ H0, **không thêm PWM giữ**. Nếu chạy firmware H1, dùng `tools/capture_m9_h1_shadow.ps1` để thu đồng thời H0+H1. H1 không tự xác lập PASS cho H0 dưới tải. Đọc `docs/M9_H1_PD_SHADOW_2026_11_06.md`.
