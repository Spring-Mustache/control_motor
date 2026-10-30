# M8-S12 — R1 RIGHT slow/repeated fine correction (A/B stage)

## Evidence actually reviewed, not just assumption

- 10 M8-S00 `kind=1` timing entries from 2026-10-28..29: 9/10 have longer
  RIGHT fine alignment than LEFT. Median 1213 ms RIGHT vs 209 ms LEFT.
  One run recorded RIGHT fine 2425 ms vs LEFT 216 ms, with RIGHT
  4 short fine pulses (85 PWM, 40 ms); the controller did reach CENTER.
- In the M8-S11 2026-10-30 steering CSV, three separate R1 presses
  finished both axes at (L,R) (1868,657), (1872,661), (1870,659),
  all within CENTER LEFT=1873 +/-5 and RIGHT=662 +/-5.
  Fine-phase duration from 100ms-sampled CSV is only approximate;
  RIGHT took roughly 436,758,437ms; LEFT 1089,218,327ms.
  Thus RIGHT is NOT slower in every individual run. The recent CSV
  does NOT establish repeated physical R1 button presses: each run
  completed after one R1 press, with internal fine corrections.
- Historical M8-S00 logs also contain LEFT fine code=10 failures (8 pulses
  still outside tolerance). Do not claim all fine problems were RIGHT-only.

## Source-backed mechanism

- Prior firmware switched RIGHT from continuous coarse PWM into zero-PWM
  settle and discrete fine pulses at |CENTER error| <=35 RAW.
  LEFT is deliberately later, at <=15 RAW from MAX and <=10 from MIN.
- RIGHT nominal fine uses PWM85 for 40ms, followed by at least
  200ms power-off and 100ms quiet; valid <=5 RAW must remain stable
  for 120ms before accepted. Each additional pulse may add hundreds
  of milliseconds and may make the motor look as though it repeatedly
  tries to align.
- RIGHT fine power/pulse is not proved defective. Different inertia,
  play, friction, driver response, or supply load remains possible;
  current logs cannot electrically distinguish them.

## Exactly what M8-S12 changes

- Introduces `steer_pos_right_fine_enter_raw = 20U` for the RIGHT R1
  coarse-to-fine transition only. The firmware clamps the used threshold
  to [15,35] RAW; set to 35 to reproduce the previous behavior.
- No changes to LEFT R1, RIGHT R2, traction wheel PID or steering sync.
- RIGHT coarse PWM remains 50..100, fine PWM85 x40ms remains fixed;
  maximum 8 fine pulses, 9-second timeout, ±5 RAW acceptance, NC IRQ,
  sensor validation, STOP, owner arbitration and no immediate reversal
  remain unchanged.
- This is an *experimental* reduction in fine-correction work.
  Continuing coarse PWM closer to CENTER may cause overshoot on a
  particular load/direction. Therefore THIS BRANCH IS NOT YET HARDWARE PASS.

## Controlled A/B on suspended chassis

1. Import this branch in a separate worktree/workspace. Run the updated
   `python tools/verify_m8_r1_center_concurrent.py` and the other M8 guards;
   Clean Build in CubeIDE, with ALL SIX MOTORS DISARMED initially.
2. With all motors stopped, set `steer_pos_right_fine_enter_raw=35`
   (legacy baseline). Log 3 R1 returns from D-pad LEFT, from D-pad
   RIGHT, and from R2 physical MIN if all preceding tests remain safe.
3. Stop all motors/disarm; change ONLY `steer_pos_right_fine_enter_raw=20`.
   Repeat from comparable start RAW locations with the same duty and
   control conditions. Capture M8-S11 CSV to PC at 100ms.
4. After the stream ends and all axes/owners are disarmed, dump M8-S00
   text for the last R1 using the existing stopped-only workflow.
   Save both CSV and M8-S00 TXT, including `M8S00_TIMING`,
   `M8S00_FINE` and `M8S00_ARRIVAL`.
5. Acceptance requires BOTH axes reached from ONE R1 press at
   LEFT 1873 +/-5 RAW and RIGHT 662 +/-5 RAW after settle, no
   reverse pulse/overshoot outside acceptance, no code 10, no
   unexpected NC, trip or STOP failure; measure total command-to-DONE
   duration and RIGHT fine-phase latency against baseline.
6. Any excessive overshoot, transient wrong direction, unexpected
   NC/contact, failed CENTER or inconsistent target: stop immediately
   and restore RIGHT handoff to 35 while stopped. Do not increase PWM
   or weaken NC/STOP as a workaround.

**No new target build or on-vehicle verification has been performed
for the M8-S12 threshold modification. Do not merge into the
operational M8-S11 branch before this A/B acceptance.**
