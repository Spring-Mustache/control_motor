# M9-H2-A — HOLD supervisor có bảo vệ, mặc định khóa cấp điện (2026-11-07)

> **GIỚI HẠN:** Đã lập trình **H2-A guard/shadow**, chưa cho phép H2-B ACTIVE. Bản này H2 **không tạo mô-men giữ trục**, không đổi PWM/DIR của M8, không thêm I2C, không cấp nguồn tự động. Không dùng bản này để thử cưỡng ép bánh hoặc tháo tải đột ngột.

## Vì sao H2-A khóa công suất

H1 đã được đo trên xe thật 4/4 M8 DONE, 130 frame (SHA `3dbdfaf`), nhưng đề xuất `u_shadow` dễ bão hòa ±40 và có lúc muốn hãm từ khi còn cách đích 150–180 RAW. Đó là **dữ liệu chuyển động dưới servo M8**, không phải đặc tính dòng/mô-men HOLD của motor khi trục đứng yên. Manual CCS_SHB12 mô tả PWM/DIR, ACCE/DECCE, giới hạn dòng/over-temp, nhưng không xác minh PWM0 coast/short-brake của phần cứng lắp trên xe hoặc dòng stalled/nhiệt của motor này. Không suy từ max current driver sang mức dòng chịu đựng được của motor khi khóa trục.

## Đã viết source

- `Core/Inc/m9_h2_hold_guard.h`, `Core/Src/m9_h2_hold_guard.c`: C độc lập; state OFF/MONITOR/SHADOW/ARMED/ACTIVE/FAULT/COMPLETE. **H2-A chỉ vào MONITOR/SHADOW/FAULT/COMPLETE/OFF**, không có active power code.
- `Core/Src/main.c`: khi M8 PositionFinish đã tắt TIM12/TIM14 CCR=0 và cả hai axis DONE2 thành công của R1 CENTER/Voice5..40, ghi goal + source cho H2. Không cho R2/Voice45 NC, STOP, fault, pending bị ghi đè. Sau idle AS5600 observer, H2 đọc cache cảm biến và owner/STOP/NC, không tạo I2C và không động đến motor.
- `M9H2_POWER_PERMITTED=0U` compile-time; `m9_h2_bench_request=0` chỉ là debug request, kể cả đặt thành1 cũng vẫn **dutyL=dutyR=0**. H2 shadow score dùng Kp_Q8=128, Kd_Q8=32, giới hạn ±20 **đơn vị giả lập**, KHÔNG phải CCR/PWM, không dùng nguyên H1 Kd128 trong vùng HOLD.
- Cửa sổ H2A tối đa 3000ms; sensor age≤250ms, 100ms velocity window, loại nhiễu 1RAW, jump>90RAW, reject e>32RAW, source/owner/STOP/wheels/NC conflict thì CANCEL/FAULT. Đọc status theo từng trục, không tự tạo owner từ DONE hoặc cho điện ở NC.
- Một dòng `M9H2_GUARD` trên UART3 115200 khi motor/wheel/serial idle, ưu tiên sau M8/H0/H1. Các trường `sha,seq,source,state,reason,durationMs,goalL/R,updates,freshL/R,peakErrL/R,peakScoreL/R,deniedNC,deniedOwner,sensorErr,blockedPower,blockBits,dutyL/R,powered=0,shadowOnly=1,dropped`.
- `blockBits` khi abort: bit0 safety trip;1 stop latch;2 control STOP;3 neutral required;4 monitor disabled;5 motion permit off;6 wheels moving;7 NC;8 owner/mode changed;9 debug AS5600 poll. H0 seq3 `reason3` trước đây không xác định được nguồn, H2A có thể gợi ý khi có sự kiện mới tương tự, không suy diễn chuyện cũ.
- `tools/m9_h2_host_test.c` kiểm tra request_power=1 không cấp lực, STOP/NC/owner/wheel/sensor/stale/deviation/timeout và idle jitter. `tools/verify_m9_h2_guard.py` GCC strict actual-C + source wiring fail-closed, workflow Windows PowerShell parser.

## Khi muốn thu log H2-A — không cần Live Expressions

Bảo vệ sửa đổi local hiện có (đặc biệt startup asm và CubeIDE settings) trước khi cập nhật branch, kiểm tra `git status --short` và SHA. Trên project STM32CubeIDE **hiện có**, sau CI exact-SHA PASS, Clean Build/Debug/Flash, MCU Resume (F8), đặt xe trên giá an toàn và kiểm tra STOP/NC rồi chạy collector:

```powershell
cd "D:\STM32 task\control_motor"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\capture_m9_h2_guard.ps1" -TimeoutSeconds 600 -MaxEvents 4 -OutputDirectory ".\logs"
```

Thực hiện lệnh Voice5..40 hoặc CENTER đã từng thử an toàn; chờ ≥3s sau DONE cho 1 record. File `logs\M9H2_GUARD_*.txt` được lưu **riêng trên máy**, gửi trong chat để kiểm tra `dutyL=0,dutyR=0,powered=0`, state/abort flags, sensor health. Nếu safety trip/NC, dừng bài test. Không ép cơ cấu, không bật `m9_h2_bench_request`.

## H2-B trước khi cho phép PWM HOLD thật (chưa code/commission)

Cần xác minh điện và điều khiển: (1) đo dòng motor khi đứng yên/stall, nhiệt motor và ngưỡng chịu tải, (2) xác minh `PWM=0` của CCS_SHB12 là coast hay brake và độ trễ ACCE/DECCE, (3) độc lập cắt nguồn motor, STOP và 4 NC có fail-closed kiểm chứng, (4) DIR tăng/giảm RAW theo L/R và khoảng trống NC, (5) duty và duration budget dựa trên thông số motor **chứ không gán từ score SHADOW**, (6) thử từng trục/nhả tải có giám sát và rollback. Khi chưa đủ, chỉ đánh dấu H2-A SOURCE/HOST/MCU-LOG theo bằng chứng; không gọi H2 powered PASS.

## Báo cáo chính xác

**HOST CI PASS đã xác minh:** GitHub Actions `42370515934` completed SUCCESS trên commit `55db22f67c81517ada48106f5a506b7b367b7e4c`; Linux GCC H2-A actual C and no motor write tests, Windows PowerShell syntax PASS. Bản `b9b75a` ban đầu fail parser do `$MaxEvents:`, đã sửa thành `${MaxEvents}:`. Motor source không thay đổi.  tiếp theo phải kiểm tra Actions SHA. Chưa có CubeIDE Build/Flash hay dữ liệu H2-A MCU. H2-B vẫn cấm cấp điện, H3 chưa làm.

## 2026-11-07 — H2-A lần CI đầu: sửa cú pháp PowerShell 5.1

- Commit `b9b75aff0bba91fedf62e4e67b244ad8f91fa1b4`, Actions `42370416232`: Linux exact-C H2-A `verify_m9_h2_guard.py` cùng full context contract **PASS**; Windows PowerShell parser phát hiện biến `$MaxEvents:` trong interpolated string (`:` theo sau tên biến), không phải lỗi motor hay logic NC.
- Đã đổi thành `${MaxEvents}:` để parser PowerShell 5.1 hiểu chính xác. Mã H2/C motor guard không thay đổi; kiểm tra lại tất cả workflow ở SHA commit mới là bắt buộc.

## 2026-11-08 — H2-A on-MCU log: abort 13ms caused by normal VOICE -> IDLE handoff

**[USER-PROVIDED PRIVATE UART]** `M9H2_GUARD_20261108_142137.txt` has exactly one valid `M9H2_GUARD` record, MCU SHA `8214610cadac396f7502dea5fbde847de784fe9d`: `seq=1,source=3,state=0,reason=3,durationMs=13,goalL=1545,goalR=351,updates=1,freshL=0,freshR=1,deniedOwner=1,blockBits=256,deniedNC=0,sensorErr=0,dutyL=0,dutyR=0,powered=0,shadowOnly=1`. **Hardware H2-A no-actuation confirmed for this one 13ms window; H2-A 3-second MONITOR not yet PASS.**

**[INDEPENDENT SOURCE INSPECTION]** `M9H2_StartAfterDone()` latched `m9_h2_owner_mode=control_mode` at M8 Voice position DONE. `Steering_PositionFinish()` then clears `voice_pos_owned`, and `Control_Arbitrate()` normally transitions `CONTROL_MODE_VOICE` to `CONTROL_MODE_IDLE` on the next poll. Old H2-A treated any difference `control_mode != m9_h2_owner_mode` as takeover, explaining bit8/13ms; we have source evidence of this path, but no old-mode values in the uploaded record itself.

**[SECOND UPLOAD DATA QUALITY]** `M9H2_GUARD_20261108_142244.txt` has one incomplete **physically concatenated** line starting with `M9H0_DRIFT`, truncated after `peakDriftR=1,peak`, immediately followed by `M8R1_FINE` record. This is **not** a complete H0 line and has no `M9H2_GUARD` event. The file cannot establish why bytes were concatenated (UART sender arbitration, COM reception or capture timing all remain hypotheses). Do not treat it as a valid H0 3sec pass or as evidence of hardware safety incident. Do not publish either private original TXT.

**[FIX]** Introduce pure-C `M9H2_ModeAllowed(source,started_mode,current_mode,voice_busy,ps2_manual,ps2_cancel)`: only source3/2 VOICE -> same VOICE or normal IDLE with no pending Voice/PS2 job, and source1 PS2 -> same PS2 or IDLE when manual inputs released. STOP, PS2 takeover, new Voice request/command, cancel-hold and unexpected transitions are **revocations**, not whitelisted. Legacy `steer_pos_active/manual/cal/test`, both CCR=0, STOP/neutral/trip/motion-permit, NC, sensor fresh, wheel-stopped remain independent guards. No change to M8 PWM/owner/AS5600 I2C, H2 guard still HARD POWER LOCK `M9H2_POWER_PERMITTED=0U`, all `dutyL=dutyR=0`.

**[DIAGNOSTIC]** New `M9H2_GUARD` includes `modeStart/modeNow/modeOK/voiceBusy`, so a new 3sec passive log can confirm normal 1->0 transition without guessing. Pure-C GCC host regressions cover normal Voice DONE->IDLE and PS2 R1->IDLE plus reject STOP/PS2 takeover/new voice/manual/cancel while power remains zero. CI must be checked on NEW SHA separately; CubeIDE Build/Flash and new MCU log pending. **H2-B powered not implemented**.

