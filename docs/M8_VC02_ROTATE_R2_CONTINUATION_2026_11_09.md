# Voice Rotate: keep R2 during passive wheel movement — 2026-11-09

**XÁC NHẬN HIỆN HÀNH 2026-11-09 — ROTATE ĐÃ HOẠT ĐỘNG TRONG HAI LẦN THỬ MỚI.**
Người vận hành xác nhận đã sửa được lỗi. Log MCU `b382354ec1592589843f4b579643e5b1abbbecf0`
ghi RIGHT yêu cầu5s, thời gian lệnh xoay5006ms; LEFT yêu cầu10s, thời gian10002ms.
Cả hai result1/started1, hai MIN NC hợp lệ, trip0, không hủy. Đây là PASS cho hai
lần thử được ghi và xác nhận chuyển động của người vận hành. R2 chỉ12–13ms và
passiveFrames0, nên log này không kiểm chứng riêng toàn hành trình R2 từ CENTER
hoặc chứng minh RPM thụ động là nguyên nhân duy nhất của log lỗi cũ.
Giữ `b382354e` làm mốc firmware Rotate đã thử thành công; M9 vẫn tạm dừng.
Không tiếp tục chỉnh Rotate hoặc tự quay lại M9 khi chưa có chỉ thị mới.


Parent: `d489402938f3eded60f54a928f390ef58430bf72` on
`feature/m8-r1-voice-center-right-late-fine-10raw`. This branch remains based on
pre-M9 d367618d; M9 HOLD stays paused on its separate branch.

## Evidence and scope

The uploaded MCU record for d4894029 shows four attempted rotations in both
directions, requested durations5–30s. All canceled during R2 after170–350ms:
position9, Rotate result4, stage1, started0, spinMs0, NC0, trip0. The duplicate
attachment has identical contents, not independent additional evidence. No raw
operator log is committed. This confirms the early R2 cancellation, not its exact
failed permission bit: the prior record did not capture that bit.

Source inspection found a concrete admission/continuation defect: Voice physical
NC seek (R2 and45°) required `Steering_PositionWheelsStopped()` every loop. Unlike
PS2 R2, it canceled even when wheel command and output were zero but feedback RPM
was >2 due to passive movement. Simulated passive movement using the actual
production position permission code reproduces `pos9 -> result4, started0,
spinMs0`, matching the reported signature. This is a reproduced source failure
and a supported hardware hypothesis; it does not prove the exact physical origin
of each old cancellation. The added snapshot distinguishes future causes.

## Behavior after repair

1. A new Voice R2/45 still requires all wheels stationary under the original
   `Steering_PositionWheelsStopped` policy. That helper preserves its behavior.
2. An active Voice R2/45 owner requires `Steering_PositionWheelDriveIdle`: no
   spin or FF test, zero raw/ramped wheel command, all four wheel PWM zero, and
   the existing <=0.5RPM target tolerance. Measured wheel RPM alone cannot revoke
   the active steering owner; the steering linkage can roll an unpowered wheel.
3. Any commanded drive, owner conflict, STOP/PS2, sensor trip, missing motion
   permit or diagnostic test still blocks/cancels. NC per-axis handling, angle
   envelope, direction checks, stall/timeout and completion are unchanged.
4. R2 must actually reach both physical MIN NC before Rotate can advance.
   The real wheel spin API still enforces its original RPM/PID/encoder/output
   gates and 2000ms startup deadline from the previous fix. Duration starts only
   after admission. This change does not make passive wheel motion count as spin.
5. PS2 R2 and ordinary Voice5..40/CENTER keep their existing ownership behavior.
   The shared Voice45 NC gate gets the same admission/continuation correction.

## Passive diagnosis

The existing stopped-only `M8_ROTATE` row adds:

| Field | Meaning |
|---|---|
| passiveFrames | Active Voice NC-seek frames observing non-stationary feedback while wheel drive is idle |
| abortSrc | Source of a position9 cancellation before cleanup |
| abortMask | Snapshot bit mask of relevant conditions at that cancellation |
| permit | Master motion permit when the Rotate result is captured |

`abortSrc`: 0=no recorded position permission cancellation, 1=continuation gate,
2=command changed, 3=PS2 disarm, 4=main safety quarantine. Direct FF/PS2 Voice
cancellation retains Rotate result9/8; it need not have a position abort source.
Mask values are a snapshot, not independent claims that every set bit caused stop.

| Bit | Condition |
|---|---|
| 0 | Control mode differs from position owner |
| 1 | Motion permit absent |
| 2 | Latched sensor/safety trip |
| 3 | STOP latch |
| 4 | PS2 cancel hold |
| 5 | PS2 spin/RF latch |
| 6 | Manual steering active |
| 7 | Calibration active |
| 8 | Wheel spin active |
| 9 | AS5600 diagnostic polling enabled |
| 10–12 | Steering test running/requested/armed |
| 13 | Wheel drive not idle |
| 14 | Full WheelsStopped check false (includes passive RPM) |
| 15 | Voice position disabled |
| 16 | PS2 disconnected or wrong mode |

Capture uses existing values, without UART/I2C or power writes in the motion
path. UART remains the existing nonblocking IRQ stream after all motors stop.
An unsent prior record remains immutable; dropped events remain counted.

## Verification and next check

The expanded `verify_m8_vc02_rotate_integration.py` compiles production position
permission/continuation statements and the real WheelDriveIdle/WheelsStopped/
VoiceNcWheelGate/AbortObserve helpers with actual Voice executor, arbiter, PS2
ownership and spin admission. Per-axis AS5600 servo and HAL remain simulated.

The old permission statements reproduce position9/Rotate4 before spin. The new
statements survive passive feedback at all four reported cancellation timings,
then require both R2 NC completion and RPM settling before accepted timed spin.
Preflight checks cover every wheel, positive/negative3RPM and the2RPM boundary.
Fourteen genuine conflicting/powered/safety conditions still reject active R2.
Existing LEFT/RIGHT timing, speed changes, FF/PS2, physical NC, timeout and clock
rollover checks continue to pass. Host CI cannot establish physical success.

Update the existing Windows project with `git fetch origin` followed by
`git merge --ff-only origin/feature/m8-r1-voice-center-right-late-fine-10raw` while
on that branch. Preserve local edits; do not force/reset/clean/stash. In CubeIDE,
Refresh -> Project Clean -> Build -> Flash/Debug -> F8. No Live Expressions.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\capture_m8_vc02_rotate.ps1 -MaxEvents 2
```

First record Rotate LEFT5s and RIGHT5s, with a full stop between commands. A
successful result has result1, started1 and spinMs >= requestedMs, with physical
motion separately observed. If either cancels, the new abort fields select the
next repair. Keep M9 paused until the operator resumes it.

## Acceptance after the repair

Operator confirmation and MCU-stamped log at b382354e support successful RIGHT5s and
LEFT10s execution: commanded spin durations5006/10002ms, normal completion, both
physical MIN NC and no trip/abort. These are command-timer values, not an independent
measurement of wheel angular displacement or physical onset. R2 elapsed13/12ms and
passiveFrames0 do not independently exercise a full R2 stroke from CENTER or the
passive-RPM continuation case. Keep the successful firmware as baseline. No further
Rotate changes or M9 work are started by this documentation-only acceptance update.
