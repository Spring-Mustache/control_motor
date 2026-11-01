# M8-S15.2 — Four D-pad/RX tests: ONE PowerShell command, ONE TXT file

**Operator requirement (2026-11-01):** all data transfer and file creation via native **Windows PowerShell only**. No terminal application, Live Expressions, third-party serial application, Python or other additional software. It was incorrect to instruct the operator to use an app or Live Expressions for S15.2 collection.

## Why this needs no new MCU firmware

Existing S15.2 firmware (at least commit 762534d) already records every manual stroke into a last-stroke 96-row RAM ring-like bounded array and can export it via the existing **M8S00 USART3 / ST-LINK Virtual COM Port** UART protocol on a **short NUCLEO USER press**, but only after STOP, PS2 disarm and other safe conditions. There is no need to enable M8S11 or modify a steering PWM/owner. A native PowerShell COM reader receives the UART bytes and writes a single .txt file with **four individually framed M8S00 exports**.

Source reference: \`tools/capture_m8_s00.ps1\` (prior single-export PowerShell reader), \`tools/capture_m8_s11.ps1\` (S11 native PowerShell stream transport); this milestone adds \`tools/capture_m8_s152.ps1\` for four consecutive exports in one run.

## One PowerShell command

After updating/preserving local edits and flashing the correct S15.2 firmware, with ST-LINK USB connected, open **only PowerShell**, from repository root:

    powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s152.ps1 -Port COM5 -TimeoutSeconds 1200 -OutputDirectory .\logs

Replace COM5 with the actual **ST-LINK VCP** COM port if necessary. If only one VCP can be identified, omit \`-Port\` and the script autodetects it. No other program may hold the same COM port. USART3 configuration: 115200, 8 data, no parity, one stop bit (already set by the script).

### Four test runs, one PowerShell session

1. With isolated emergency motor-power cutoff available, supported chassis, NC/AS5600 and STOP verified, PS2 analog/neutral, **hold START ~1.5 seconds to arm**. Release START, wait for a neutral frame.
2. **D-pad LEFT**: steer briefly **without reaching SOFT/NC**. Release, press START to DISARM, ensure all motion ceased. **Short press and release USER once** (not a 3-second hold) to cause one safe M8S00 UART dump. The PowerShell script prints \`Captured 1/4\`.
3. Re-arm only after ensuring safe state. **D-pad RIGHT**, then release, disarm, short USER. Expect \`Captured 2/4\`.
4. Repeat for **RX LEFT**, then **RX RIGHT** with joystick returned to center and START disarm after each. Expect \`Captured 3/4\` and \`Captured 4/4\`. DO NOT close PowerShell between runs and do not perform next stroke before the previous M8S00_END; each new manual stroke overwrites the 96-row MCU buffer.
5. Send **only the single** \`logs/M8S152_four_tests_YYYYMMDD_HHMMSS.txt\`. Every section begins \`#M8S00_BEGIN,1\`, has \`M8S152_META/A/B\` and ends \`#M8S00_END\`. A partial file is retained on error/timeout; do not claim a PASS if an END or final zero-PWM marker is missing.

PowerShell script validates four records, increasing run IDs, \`kind\` and \`dirL\`:
- D-pad LEFT: \`kind=3, dirL=1\`
- D-pad RIGHT: \`kind=3, dirL=-1\`
- RX LEFT: \`kind=4, dirL=1\`
- RX RIGHT: \`kind=4, dirL=-1\`

\`M8S152_B,final=1\` with \`pwmL=0,pwmR=0\` shows the final *requested/actual MCU compare values*, NOT measured absence of voltage at the motor driver. Monitor physical stop and maintain independent power cutoff.

## Safety and limits

- **No app, no Live Expressions.** Only native PowerShell uses \`System.IO.Ports.SerialPort\` for reception, writing to disk continuously. A short USER press posts the existing export request on the MCU; this is a physical button, not a PC application.
- The existing M8S00 exporter **refuses UART transmission until all actuator permits and activity are disarmed**. It uses a blocking UART method while fully disarmed. This is not S11's continuous nonblocking live stream and must NOT be run concurrently with the S11 capture. The captured data is correctly the 96-row S15.2 **post-atomic passive shadow**, not S11's 60-column CSV.
- The MCU keeps **only the latest manual stroke**. Export EACH stroke before starting the next one; four exports go into one file.
- Do not press USER while armed: export is rejected (busy/unsafe). If disarm or stop is not successful, do not try to force exporter flags or bypass safety.
- \`S15.2\` is a **calibrated-AS5600 normalized angle proxy**, not measured Ackermann geometry or active RIGHT speed correction. \`shadowR\` is hypothetical; the source must never actuate it. LEFT, Level2, Fast Start, RIGHT 200 active (300 proposed only), NC, STOP and SOFT are unchanged.
- Source/static and Windows PowerShell syntax verification do **not** prove serial capture with hardware, CubeIDE build, physical NC cutoff or motor safety. Real test remains pending.

## Failure diagnosis from script output

If \`No COM port\`: check ST-LINK USB/driver; specify \`-Port COMx\` if multiple. If no \`#M8S00_BEGIN\`, ensure disarm completed, brief USER release, no other COM program. If only a partial log is saved, preserve the TXT and inspect UART/gating. If \`M8S152_META\` is missing, verify flashed firmware SHA and that an eligible D-pad/RX manual stroke was executed. The script does not manipulate any MCU motor-control flag.
