# Resume M9 with the accepted Rotate baseline — 2026-11-09

## Requested base and history

The operator explicitly resumes M9 and selects the successful Rotate repair as
the development foundation. Active branch: `feature/m9-steering-active-position-hold`.
Firmware foundation: `b382354ec1592589843f4b579643e5b1abbbecf0`; its acceptance-only
follow-up is `22e7eb5cd18c2441e89af1170831efe39fbc38a8`. Latest M9 input is
`3df43edde8f53b5e84561704189a83ac8286a56b`.

The integration commit uses22e7eb5 as first parent and3df43ed as second parent.
Thus the accepted M8 history is the first-parent foundation, the entire M9 history
is retained, and the existing M9 branch can advance without rewriting history.
The M8 branch stays unchanged as the accepted reference. Do not merge an older
unfixed M9 snapshot over the result or reset the Windows project.

## Resulting firmware

- All current M9 H0/H1/H2/B1 source, configuration, SysTick cutoff, captures and
  native tests from3df43ed are retained.
- Both Rotate fixes from M8 are retained: stationary admission followed by
  wheel-drive-idle continuation of Voice R2/45; real wheel spin admission retries
  for up to2000ms; command duration begins only after accepted spin.
- Continuous NC, sensor, ownership, timeout, STOP/PS2 and encoder/PID guards remain.
  New abort source/mask/passiveFrames and both M8/M9 UART collectors are present.
- HOLD must release when a complete Rotate command is ready, before arbitration
  creates its R2 request in the same loop. `M9B1_MainInput.owners_free` now rejects
  only ready LEFT/RIGHT Rotate in addition to the existing owner exclusions.
  Pre-owner service then cuts HOLD's output lease and armed pulse deadline before
  R2 can write. A pending incomplete Rotate or ready FORWARD/BACKWARD does not
  unnecessarily drop independent steering HOLD.
- R2/NC completion cannot acquire HOLD. A new eligible CENTER or angle5..40 BOTH-DONE
  handoff can later acquire HOLD under the existing rules.

M9 tuning remains LEFT assist300CCR/<=200ms, RIGHT85CCR/<=12ms, continuous PD ceilings
30/10, with feedback-shortened pulses, settle/rest, effort/progress accounting,
physical travel, STOP/NC/sensor guards and the existing SysTick cutoff. No gain,
current setting or pulse ceiling is increased by this integration.

## Verification and acceptance limits

The old M9 adapter with the new handoff regression fails to revoke a live HOLD
before the Rotate R2 stage is assigned. The updated adapter passes for LEFT and
RIGHT with synthetic, tuning and production-assist profiles. The regression uses
the actual main adapter, controller, PWM port and SysTick handler, verifies zero
output and disarmed deadline before new R2 PWM, and checks that the old deadline
cannot cut a new owner's output. It also checks preservation for incomplete
Rotate/straight drive and no HOLD restart on R2 NC completion.

The M8 Rotate integration regression compiles the merged arbiter with M9 hooks
enabled (HOLD inactive in that fixture), actual R2 continuation guards and wheel
admission/executor. The extraction now distinguishes a function definition from
M9's earlier forward declaration of Voice_HasPendingData. Full M8/M9 native checks
and Windows PowerShell parser checks run in the combined exact-SHA workflow.
HOST/CI are distinct from a CubeIDE target build, Flash and powered behavior.

Accepted hardware evidence remains b382354e RIGHT5s/LEFT10s with normal result1.
The short R2 setup and zero passiveFrames in that log do not separately demonstrate
a complete R2 stroke from CENTER. Last recorded M9 hardware was b89b4d5 with weak
LEFT175/30ms response;3df43ed's LEFT300/200ms candidate had no returned hardware log.
Do not claim it has already solved the return-force problem. The combined firmware
needs its own build/flash and observation before hardware acceptance.

## Windows update and continuing M9

Use the existing `D:\STM32 task\control_motor` directory. Preserve local settings,
startup edits and logs; no reset/clean/stash or extra Windows worktree.

```powershell
Set-Location 'D:\STM32 task\control_motor'
git fetch origin
if ($LASTEXITCODE -ne 0) { throw 'Fetch failed' }
git switch feature/m9-steering-active-position-hold
if ($LASTEXITCODE -ne 0) { throw 'Switch blocked; preserve local edits' }
git merge --ff-only origin/feature/m9-steering-active-position-hold
if ($LASTEXITCODE -ne 0) { throw 'Update blocked; preserve local edits' }
git log -1 --oneline
```

CubeIDE Refresh -> Project Clean -> Build -> Flash/Debug -> Resume F8. This clean
is the IDE build clean, not `git clean`. No Live Expressions are required.

Continue the existing HOLD observation on the same test setup: open the collector,
issue CENTER, let it complete, displace lightly then release, and end with STOP.

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\capture_m9_h2b1_hold.ps1 -TimeoutSeconds 300 -MaxEvents 100 -UntilStopped
```

The collector ending does not stop motors. For a later separate Rotate check,
close the HOLD receiver before opening the Rotate receiver on the same COM port:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\capture_m8_vc02_rotate.ps1 -MaxEvents 2
```

Use LEFT5s/RIGHT5s from the normal permitted setup. Keep the log's MCU SHA with the
observed outcome; do not automatically tune more power based only on this merge.
