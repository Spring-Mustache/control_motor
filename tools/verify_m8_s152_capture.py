#!/usr/bin/env python3
"""S15.2: PowerShell-only four-run MCU UART capture invariants.

STATIC and firmware protocol checks, not a Windows COM/hardware test.
"""
from pathlib import Path
import re
ROOT=Path(__file__).resolve().parents[1]
PS=(ROOT/'tools/capture_m8_s152.ps1').read_text(encoding='utf-8')
MAIN=(ROOT/'Core/Src/main.c').read_text(encoding='utf-8')
RAM=(ROOT/'Services/Src/ram_logger.c').read_text(encoding='utf-8')
S152=(ROOT/'Core/Inc/steer_s152_manual_relation.h').read_text(encoding='utf-8')

checks={
  'native PowerShell built-in COM only':
    '[System.IO.Ports.SerialPort]::GetPortNames()' in PS and
    'System.IO.Ports.SerialPort' in PS and
    '$serial.ReadLine().Trim()' in PS and
    'powershell -NoProfile -ExecutionPolicy Bypass' in PS,
  'exact M8S00 full-frame transport':
    "'#M8S00_BEGIN,1'" in PS and
    "'#M8S00_END'" in PS and
    "'#M8S00_BEGIN,1'" not in MAIN,
  'automatic four-run single-file capture':
    '[int]$ExpectedRuns = 4' in PS and
    '$complete -lt $ExpectedRuns' in PS and
    'M8S152_four_tests_' in PS and
    'StreamWriter' in PS and '$writer.AutoFlush = $true' in PS and
    '$writer.WriteLine($line)' in PS,
  'separate matching Dpad and RX strokes':
    '@(3, 3, 4, 4)' in PS and '@(1, -1, 1, -1)' in PS and
    'M8S152_META,run=' in MAIN and
    'm8s152_kind=m8_s05_manual_kind;' in MAIN and
    'm8s152_direction_l=left_direction;' in MAIN,
  'checks final zero CCR before accepting':
    'M8S152_B,i=' in PS and
    ',pwmL=0,pwmR=0,' in PS and
    "',final=1'" in PS and
    'M8S152_Record(1U); /* final PWM0 */' in MAIN,
  'no serial RX/MCU drive protocol needed':
    not any(x in PS for x in
      ('$serial.Write(', '$serial.WriteLine(', 'Set-Content ', 'Start-Process',
       'Invoke-WebRequest', 'python ', 'TeraTerm', 'putty.exe')),
  'existing request via physical USER button':
    'wheel_log_m8_s00_button_dump_request = 1U;' in RAM and
    'm8_s00_dump_request = 1U;' in MAIN and
    'M8S00_DumpUpdate();' in MAIN,
  'blocking MCU export gated until full disarm':
    'if ((steer_safety_motion_permit != 0U)' in MAIN and
    '(steer_manual_enable != 0U)' in MAIN and
    '(steer_manual_active != 0U)' in MAIN and
    '(WheelControl_SpinIsActive() != 0U)' in MAIN and
    '(M8S152_Dump() == 0U)' in MAIN,
  'proper MCU protocol':
    '#M8S00_BEGIN,1\\r\\n' in MAIN and
    '#M8S00_END\\r\\n' in MAIN and
    'M8S152_META,run=' in MAIN and
    'M8S152_A,i=' in MAIN and
    'M8S152_B,i=' in MAIN,
  'no live-stream flag required':
    'm8_s11_stream_enable' not in PS and
    'm8_s00_dump_request = 1' not in PS,
  'shadow still never actuates':
    'shadowApplied=0' in MAIN and
    'steer_s15_common_time_enable = 0U;' in MAIN and
    'M8S152_SHADOW_MAX_EXTRA 25U' in S152,
}
for key,value in checks.items():
    print(('PASS' if value else 'FAIL')+': '+key)
if not all(checks.values()):
    raise SystemExit('M8-S15.2 PowerShell capture SOURCE/STATIC FAIL')
print('M8-S15.2 capture static PASS; Windows PowerShell parser/COM/MCU needs separate validation.')
