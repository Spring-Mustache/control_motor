# M8 Voice Rotate startup and duration repair — 2026-11-09

Active branch: `feature/m8-r1-voice-center-right-late-fine-10raw`.
Exact parent baseline: `d367618d4d9c4c382a9a7208f9b878dfefa21ace` (pre-M9).
M9 branch `feature/m9-steering-active-position-hold` stays paused at
`3df43edde8f53b5e84561704189a83ac8286a56b`; no M9 HOLD source is carried here.

## Symptom and confirmed source defect

Operator reports LEFT/RIGHT + seconds moves only a short distance instead of
matching R2 then L2/L1. No Rotate MCU trace has yet identified its exact stop reason.
The source defect reproduced on HOST is narrower: after R2, the executor calls
`WheelControl_SetSpinDirection` once; if the real wheel API defers admission it
immediately cancels Voice. Held PS2 L1/L2 keeps requesting the same API.
Admission legitimately defers for nonzero PID target, feedback RPM/PWM, unavailable
encoders, disabled PID, FF tests or missing motor objects. Steering's stationary
check accepts target RPM <=0.5, while spin admission requires <=0.01; an admission
refusal is therefore not an impossible state. These wheel gates remain unchanged.

## Change

- LEFT `A4 D0 05`: physical R2 MIN on both axes, then spin direction -1 (L2).
- RIGHT `A7 D0 05`: physical R2 MIN on both axes, then spin direction +1 (L1).
- R2 retains its 6500ms deadline and both successful NC results.
- After R2, stage2 requests spin each loop for at most 2000ms until the real API
  accepts. While deferred, elapsed spin duration stays zero; no direct PWM writes.
- The requested duration begins on API acceptance, excluding R2 and startup wait.
  This is commanded spin time; physical wheel onset still follows existing ramp/PID.
- Once accepted, missing spin ownership cancels instead of automatically restarting.
- Durations remain the existing 5..120 seconds in 5-second increments.
- C1..C5 still updates live: C1=250 command, C2..C5 capped at existing400 for spin.
- Continuous physical MIN/noMAX gate, sensor trip, FF, PS2 takeover/neutral rearm,
  wheel PID/FF/ramp and PS2 manual spin behavior remain in force.

## Automatic passive record

`M8_ROTATE,sha=...,seq=...,dir=-1,result=1,stage=2,started=1,requestedMs=5000,r2Ms=...,waitMs=...,spinMs=...,deferred=...,nc=6,pos=2,resultL=3,resultR=3,trip=0,dropped=0`

This is a format example, not a hardware measurement. Rows are snapshotted before
owner cleanup, sent through the existing nonblocking UART IRQ buffer only after
all motors stop, and never overwrite unsent evidence. If a new event happens before
the previous row is sent, the dropped counter increases. A normal duration finish
ramps down through the existing wheel controller, so the row can arrive later than
the requested spin duration. No Live Expressions or manual UART trigger is needed.

| result | Meaning |
|---|---|
| 1 | Requested spin time completed |
| 2 | R2 request rejected or safety/permission blocked |
| 3 | R2 timeout |
| 4 | R2 did not finish at the required physical NC pair |
| 5 | NC/safety cut while waiting for or executing spin |
| 6 | Spin admission remained deferred for 2000ms |
| 7 | Spin ownership lost after startup |
| 8 | PS2 takeover |
| 9 | Voice FF STOP |
| 10 | Other owner cancellation |

`started=0` means the spin API never accepted; `spinMs` is then zero. `dir=-1`
is LEFT, +1 RIGHT. `nc=6` means both physical MIN with neither MAX. A cut can
report another NC mask. Rows describe firmware state, not proof of wheel motion.

## Verification

`tools/verify_m8_vc02_rotate_integration.py` compiles actual Voice start/executor,
arbiter, PS2 ownership detector, and wheel spin/target admission API. Steering
completion and measured feedback are simulated. It reproduces baseline immediate
cancellation after a feedback refusal; the repair passes LEFT/RIGHT5/10/120s,
R2 and wait exclusion, live speed, PS2 neutral ownership/takeover, FF, NC/MAX,
R2/startup timeout, no restart after owner loss, clock rollover and pending evidence.
The existing executor and stop tests compile the added passive record as well.
Other firmware/source HOST checks remain in the normal exact-SHA Actions workflow.
Native Windows CI parses the new capture script. HOST/CI do not prove a CubeIDE
firmware build, Flash, physical duration, traction or NC wiring. Those are pending.

## Windows update and hardware check

Use the existing project, keep local changes; no reset/clean/stash or worktree.
These Git commands fail safely if a local edit conflicts with switching branches:

```powershell
Set-Location 'D:\STM32 task\control_motor'
git fetch origin
if ($LASTEXITCODE -ne 0) { throw 'git fetch failed' }
git switch feature/m8-r1-voice-center-right-late-fine-10raw
if ($LASTEXITCODE -ne 0) { throw 'Switch blocked; keep local edits and report Git output' }
git merge --ff-only origin/feature/m8-r1-voice-center-right-late-fine-10raw
if ($LASTEXITCODE -ne 0) { throw 'Update blocked; keep local edits and report Git output' }
git log -1 --oneline
```

In CubeIDE Refresh the project, **Project > Clean**, Build, Flash/Debug, then
Resume (F8). Clean here removes stale build output from M9; it is not `git clean`.
Verify the build succeeded before flashing. Start the passive receiver:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\capture_m8_vc02_rotate.ps1 -MaxEvents 4
```

On the existing hardware test setup, issue LEFT5, RIGHT5, LEFT10, RIGHT10, waiting
for a complete stop between commands. First R2 steering movement is separate from
the timed wheel spin. Existing protective stops may interrupt a test; preserve its
record rather than retrying blindly. Return the generated `logs\M8_ROTATE_*.txt`.
If it still stops early, use actual result, NC, started, spinMs and MCU SHA to select
the next fix. Do not infer that a short move confirms a weak-motor or timing cause.
