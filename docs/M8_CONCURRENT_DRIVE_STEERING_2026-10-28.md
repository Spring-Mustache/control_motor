# M8 — independent wheel drive and steering (2026-10-28)

**Status:** source change committed on feature branch. No fresh CubeIDE target build, flash, powered bench validation, or ground-load acceptance was performed after this change. This is **not** hardware PASS.

## Why the architecture changed

A captured ground-load M8-S00 sample recorded PS2 LEFT held, no wheel drive PWM, RIGHT UP wheel encoder +2.8 RPM, `manualBlock=4`, no NC trip and valid steering progress. The operator independently observed passive wheel displacement when the offset steering pivot scrubs against the ground. The old universal 2-RPM wheel-stopped gate caused a manual steering stop and forced a new D-pad press. This does **not** imply an encoder electrical fault.

Normal PS2 driving requires the 4 traction motors and 2 steering actuators to be independently commandable at the same time. The old code could erase steering CCR when LY/Up/Down was active, zero wheel target while R1/R2 was active, and reject D-pad/RX and R1/R2 due to nonzero wheel RPM.

## Changes in main.c

- `PS2_ApplyManualControl`: LY/Up/Down continues through `WheelControl_SetTargetPWM(pwm_duty)` even when D-pad/RX or R1/R2 steering is active. Remove the old throttle-induced `Turn_Motors_Stop`. Idle steering CCR is zeroed only without an active/pending manual or position steering owner.
- `Steering_ManualUpdate`: no LY/Up/Down neutral gate and no `Steering_PositionWheelsStopped()` gate. Normal wheel encoder motion no longer triggers `manualBlock=4` when steering. L1/L2 in-place spin, other steering owners, STOP, voice, debug conflicts and required steering permits still block.
- `Steering_PositionUpdate`: R1/R2 steering is allowed with LY/Up/Down traction; L1/L2 active/decaying spin remains exclusive. Other mode ownership, NC, AS5600 and position bounds remain.
- `Steering_ManualDirectionNeutral`: a steering-direction release resets the manual steering rearm latch even while LY/Up/Down stays active. The original `Steering_ManualInputsNeutral` is unchanged for START arm and global steering safety rearm.
- `PS2_RestoreRouteUpdate`: normal wheel+steering is not reported as a prohibited combination. SELECT calibration and L1/L2 spin are still incompatible; the wheel-stop observer bit applies only to SELECT.
- `ps2_drive_steer_request_frames`: read-only counter of concurrent wheel+steer input request frames; not proof of actual physical actuation or safe operation.

## Safety boundaries retained

- START held ~1.5 s with all controls neutral and wheels stopped still authorizes steering; fresh START press disarms.
- Four NC switches cut corresponding steering PWM in IRQ and are checked before further PWM. AS5600 live validity, calibrated hard envelope, soft clearances, stalls/timeouts, anti-reverse and per-axis completion still apply.
- Global STOP, steering safety trip, RF/PS2 loss, Voice cancellation priority remain.
- L1/L2 in-place spin and SELECT calibration are **not** part of normal concurrent drive+steer. SELECT still requires full neutral and wheels stopped.
- Blocking M8 snapshot UART export continues to require complete disarm and standstill. It is **never** permitted while motor control is active.
- Zero PWM is not an independent motor power disconnect. The RIGHT driver has previously exhibited unintended movement at zero request; confirm a working physical motor supply cutoff before powered testing.
- The code does **not yet implement** chassis speed compensation/steering inverse kinematics for the four rolling wheel contacts. Concurrent mode might scrub tires, pull against linkages or increase steering motor load.

## Static checks (run in PowerShell at repository root)

```powershell
python tools/verify_m8_concurrent_drive_steer.py
python tools/verify_m8_s00_capture.py
python tools/verify_ps2_restore.py
python tools/test_s16_reassembly_geometry.py
```

Next run CubeIDE **Debug Build**; do not regenerate from the currently divergent .ioc file. Build success verifies only compilation, not physical safety.

## Staged commissioning (not yet performed)

1. Keep motor supply independently disconnectable. Test NC, driver OFF behavior, PS2 radio STOP/disarm and steering sensor validity in a controlled setup before powering any wheel or steering movement. If any motor energizes without command, cut external supply, do not continue.
2. Secure the chassis on a rated stand with all wheels clear of the ground and no person within actuator sweep. Start each test with neutral inputs and no motor motion. Hold START 1.5 s then release, return to neutral and let the steering neutral rearm settle.
3. Test separately: low LY wheel drive alone; low-angle D-pad/RX steering alone. Observe correct direction, limits, stops and no uncontrolled motion. Then test low LY **while** D-pad or RX is held briefly, return steering input to neutral **while keeping LY**, then release LY. Expected: traction continues after steering releases; steering can move while traction is commanded. Repeat with one controlled direction reversal only after neutral. Do **not** test L1/L2 or SELECT concurrently.
4. Only after manual combo is verified, check R1/R2 with a low LY input, under the same suspended conditions, away from endpoints. Observe both axes' AS5600 position, PWM and timeout/limit behavior; do not run into physical NC on an unvalidated coast distance.
5. Disarm with a fresh START press, release all inputs, wait for all motion to stop. Collect one M8-S00 TXT via `powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s00.ps1` then a **short USER press**, without resetting MCU first. Send entire file.
6. Ground-load validation is a separate stage after stand tests, with a clear test zone, proven independent motor cutoff and restrained low-speed operation; no full-speed racing or uncontrolled turn in this phase.

M8-S00 and concurrent control remain **UNVERIFIED** until the rebuilt image and staged actual motor tests pass.


## Nghiệm thu người vận hành mới — 2026-10-28

Phần mô tả chưa thử phần cứng ở đầu tài liệu là trạng thái cũ trước các lượt kiểm tra mới. Người vận hành đã xác nhận:

- PASS: R2 đi đến cả hai công tắc MIN vật lý.
- PASS: L1/L2 xoay tại chỗ.
- PASS: D-pad điều khiển đồng thời bốn motor bánh và hai motor đánh lái.
- R1: cả hai đã tự hoàn thành CENTER sau một lần nhấn, nhưng chưa đạt **độ mượt** khi rời NC từ vị trí R2.
- Đồng tốc hai motor đánh lái: **chưa đạt**.
- SELECT: tạm hoãn và không phải điều kiện kết thúc M8-S00 hiện tại.

### R1 rời công tắc theo một chiều, không giật-khựng

Code cũ dùng PWM riêng ở mức tối đa để thoát NC và chặn PWM thêm 120 ms khi NC nhả. Điều này phù hợp triệu chứng giật, khựng rồi mới về CENTER. Bản sửa nguồn hiện tại thực hiện:

1. MIN đang tác động chỉ cho phép DIR1 **rời MIN**; MAX đang tác động chỉ cho phép DIR0 **rời MAX**, áp dụng độc lập cho từng motor trong R1/R2. Công tắc kép trên một motor luôn bị chặn.
2. Trong trạng thái ESCAPING, PWM dùng cùng bộ điều khiển vị trí bình thường. Khoảng xác nhận NC nhả 120 ms vẫn còn để phân loại rung tiếp điểm, nhưng **không cưỡng bức dừng motor** cả 120 ms.
3. Ngắt NC vẫn cắt PWM ngay. Ngắt lặp lại từ công tắc xuất phát giữ một lượt xử lý bằng PWM 0 rồi chỉ được chạy rời công tắc sau khi kiểm tra lại. Các lỗi NC đối diện, cảm biến AS5600, quá thời gian, sai chiều và STOP vẫn có hiệu lực.
4. Giữ nguyên R2 yêu cầu cả hai MIN thật, CENTER, MIN/MAX, PWM trần, SELECT và phần đồng tốc hiện có.

**Trạng thái sửa R1: đã commit mã nguồn, CHƯA Build/flash/chạy phần cứng, CHƯA được nghiệm thu mượt.** Nếu công tắc rung liên tục, ngắt ISR vẫn có thể làm hụt nhịp ngắn; không được bỏ bảo vệ để che sự cố.

### Đồng tốc hai motor lái: cần đo sai lệch thực

Firmware đang có PI RIGHT-master/LEFT-follower cho D-pad/RX, chỉ khi cả hai motor ở trạng thái manual MOVING. Bộ điều khiển so tốc độ AS5600 đã chuẩn hóa theo tỷ lệ hành trình trái 923 RAW, phải 816 RAW, không nhất thiết cùng số RAW/giây. Chưa có bằng chứng PI thực sự đồng tốc tốt. R1/R2 hiện là hai servo vị trí độc lập, **chưa có đồng tốc hai trục trong R1/R2**.

Đã thêm dòng UART M8S00_SYNC, mode=manual_only, gồm: frames; actualL_raw_s; actualR_raw_s; referenceL_raw_s; emaAbsError_raw_s; peakAbsError_raw_s; syncUpdates; referenceHolds; limitCount. Các số này được ghi nhận trong lúc D-pad/RX đang chạy, xuất chỉ sau khi DISARM; không thay đổi PWM/PI.

Bài tiếp theo: kiểm tra R2 rồi R1 trên giá treo, quan sát mất giật nhưng vẫn giữ bảo vệ NC; sau đó thực hiện một lượt D-pad/RX hai motor lái cùng quay, DISARM và xuất TXT để xem M8S00_SYNC. Nếu muốn đồng bộ cả hành trình R1/R2, cần bộ điều khiển riêng cho các pha rời NC và chỉnh tinh; không giả định bộ PI manual có thể dùng nguyên trạng.

**Kết luận S00:** các mục R2, spin và D-pad + 4 bánh + 2 lái đã đạt theo người vận hành; còn hai điểm chặn là R1 từ NC phải mượt và đồng tốc hai motor lái. 
