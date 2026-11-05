# M8-S15.4F — RIGHT Voice5–40 RAW-rise later fine entry, physical-log-directed trial

Date: 2026-11-05. Source baseline: `a47d7e7fb057a9b817c64a4c2eb99c82cf30125e` on `feature/m8-s154-right-fine-passive-diag`. New isolated trial branch: `feature/m8-s154-right-voice-rise-late-fine-10raw`. Existing S15.4 source and diagnostic baseline unchanged.

## Actual operator log, 6/6 successful

Private source `M8S154_right_fine_20261105_190812.txt` with firmware-stamped SHA `a47d7e7fb057a9b817c64a4c2eb99c82cf30125e` was user-supplied. Do NOT upload this private TXT to public GitHub. The values here are condensed evidence, not a copy of the file.

| Voice | RIGHT RAW | target | first fine entry | first settled quiet | final | fine pulses | direction flips | weak | worsened | logged fineMs | skewMs |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 15 left | decreases |507|522|515|508|1|0|0|0|474|33|
| 15 right | increases |784|769|776|784|1|0|0|0|1882|1441|
| 30 left | decreases |351|366|360|353|1|0|0|0|463|319|
| 30 right | increases |906|892|898|910|6|1|2|1|1596|1364|
| 40 left | decreases |247|262|257|251|2|0|0|0|672|77|
| 40 right | increases |988|974|981|992|6|1|2|2|1596|1496|

Every entry had resultL=2, resultR=2, finish=2 and dropped=0; final RIGHT error within +/-5 RAW. S15.4 peakGap=0 in every record; this alone does not prove its coordinator actually applied. Existing `fineMs` measures right fine entry to *whole two-axis command finish*, **NOT** to RIGHT done. Existing unsigned `skewMs` cannot reveal which motor finished last. Both ambiguities are corrected in next telemetry by separate `leftDoneMs`, `rightDoneMs`, `rightFineMs`, `syncFrames`. Do not overclaim physical simultaneity or speed from the existing skew.

For RAW-increase RIGHT, all three runs first entered fine with 14–15 RAW remaining. First settled quiet position moved approximately +6..7 RAW beyond that entry: 15° 769 ->776, 30° 892->898, 40° 974->981. The remaining error ~7–8 RAW is just outside +/-5; RIGHT needs fine pulses. On 30/40 the correction takes 6 pulses including 1 reversal, 2 weak and 1/2 worse pulses. Predicting a later coarse cutoff around 10 RAW would leave ~3–4 RAW if coast stays similar, allowing the SAME quiet settle to finish without fine pulses. This is a testable hypothesis, **not** physical validation of 10 RAW handoff.

## Narrow actuation experiment

New `steer_voice_s154_right_late_fine_enable=1U` boots ON (operator explicitly rejects manual flag editing). `Steering_VoiceRightLateFineEntry` selects 10 RAW fine-entry only while RIGHT axis `left==0`, active Voice position mode4 (5..40), OWNED, positive error (RIGHT RAW rising), calibrated physical R_MAX minus the voice goal >=30 RAW. If any gate fails, preserve original current-clamped 15..35 RAW threshold, including all R1, R2, Voice45, LEFT and RIGHT RAW-decreasing calls. A simple change to handoff threshold, not a change to fine PWM, 12ms pulse length, 200ms OFF, 100ms quiet, 120ms capture, max8 pulses, +/-5 RAW acceptance, motor cap100, motion enable, NC ISR, STOP/PS2 or S15.4 coarse coordinator. All existing range/timeout/stall/sensor/late IRQ checks remain, including NC direction checks.

No gain increase or force-to-target movement. This does keep existing coarse PWM active for ~5 additional RAW at the final approach; therefore Clean Build, fully verified NC/STOP with power disconnected, secured bench and independent power cutoff are mandatory before trying the new branch. If overshoot, fault, new reversal or higher absolute error appears, STOP and go back to proven passive branch.

## Enhanced automatic log, no manual variable editing

The boot-enabled `M8S154_FINE` output remains automatically collected with the same `tools/capture_m8_s154_right_fine.ps1` (native PowerShell, six lines, one TXT). Added append-only fields `entryLimit` (effective threshold, target 10 for eligible RIGHT rising), `syncFrames` (number of actual S15.4 paced frames for this command), `leftDoneMs`, `rightDoneMs`, and `rightFineMs` (time from RIGHT fine entry to RIGHT done). Existing `fineMs` is preserved for prior log compatibility but includes partner waiting time. Defaults `steer_voice_s154_enable=1`, `steer_voice_s154_right_fine_diag_enable=1`, new `steer_voice_s154_right_late_fine_enable=1`; none bypass auto-ARM or independently enables motor power. UART remains nonblocking, idle only.

## Acceptance and rollback

After exact-SHA HOST CI, operator builds and flashes new target; do not confuse source CI with CubeIDE or a powered motor PASS. On secured rig repeat 15,30,40 in two directions (same nominal start; actual start RAW can vary) and send ONE TXT privately. Compare per-axis result=2, final error <=5, no extra overshoot/reverse, fewer correction pulses in RAW-rising tests, no worse common syncFrames, NC/STOP and absence of faults. If physical behavior deteriorates, stop and restore previously tested `feature/m8-s154-right-fine-passive-diag` SHA `a47d7e7f...` with correct matched CubeIDE Clean Build/Flash. No Release is required for this handoff.
