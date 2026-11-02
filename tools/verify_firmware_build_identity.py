#!/usr/bin/env python3
"""Verify embedded firmware identity and Git-independent UART capture.

Static/CI only: does not prove CubeIDE ran the prebuild step or flashed ELF.
"""
from pathlib import Path
import re
import xml.etree.ElementTree as ET
R=Path(__file__).resolve().parents[1]
main=(R/'Core/Src/main.c').read_text(encoding='utf-8')
ps=(R/'tools/stamp_firmware_build.ps1').read_text(encoding='utf-8')
cap=(R/'tools/capture_m8_s152.ps1').read_text(encoding='utf-8')
ignore=(R/'.gitignore').read_text(encoding='utf-8')
conf=ET.parse(R/'.cproject').getroot()
config=[e for e in conf.iter() if e.tag=='configuration' and
        e.get('name') in ('Debug','Release')]
checks={
 'Both Debug/Release CubeIDE prebuild steps configured':
  len(config)==2 and all('stamp_firmware_build.ps1' in e.get('prebuildStep','') and
    'powershell.exe' in e.get('prebuildStep','') for e in config),
 'Explicit exact Git HEAD with dirty check and timestamp':
  all(x in ps for x in ("'rev-parse', '--verify', 'HEAD'",
     "'status', '--porcelain', '--untracked-files=normal'",
     "yyyy-MM-ddTHH:mm:ssZ", "FW_BUILD_GIT_SHA", "FW_BUILD_GIT_DIRTY",
     "FW_BUILD_GIT_BRANCH", "FW_BUILD_UTC")),
 'Generated header always excluded from Git':
  '/Core/Inc/firmware_build_id_generated.h' in ignore,
 'Compilation requires auto-generated identity (no silent fallback)':
  '#include "firmware_build_id_generated.h"' in main,
 'Version line comes from MCU after disarm not host':
  'static uint8_t M8S00_DumpFirmwareId(void)' in main and
  'M8S00_FW,proto=1,sha=%s,dirty=%u,branch=%s,builtUtc=%s,target=STM32F767ZI' in main and
  main.index('(M8S00_DumpFirmwareId() == 0U)') <
  main.index('(M8S00_DumpOne("STOP", &m8_s00_first_stop) == 0U)'),
 'Version dump gated with legacy atomic motor/off checks':
  main.index('static void M8S00_DumpUpdate(void)') <
  main.index('if ((steer_safety_motion_permit != 0U)',main.index('static void M8S00_DumpUpdate(void)')) <
  main.index('(M8S00_DumpFirmwareId() == 0U)'),
 'PowerShell optionally validates SHA and never generates MCU result':
  '$ExpectedCommit' in cap and "'^[0-9a-f]{40}$'" in cap and
  "if ($line -match '^M8S00_FW,')" in cap and
  "if ($firmwareLine -match '^M8S00_FW,proto=1,sha=([0-9a-f]{40}),dirty=([01]),')" in cap,
 'Dirty is diagnostic; explicit mismatch or missing identity still fails':
  'FW MISMATCH' in cap and 'FW DIRTY' in cap and
  'FW UNKNOWN' in cap and 'throw "$badRuns invalid frame(s):' in cap and
  'git -C' not in cap and 'rev-parse' not in cap and
  '[string]$VersionLabel' in cap and
  'CAPTURE_LABEL,source=operator_unverified' in cap and
  '# CubeIDE .prefs may be dirty; preserve frame and continue.' in cap,
 'No asynchronous UART/M8S11 or motor writes added to identity path':
  all(x not in main[main.index('static uint8_t M8S00_DumpFirmwareId(void)'):
    main.index('static void M8S00_DumpUpdate(void)')] for x in
     ('__HAL_TIM_SET_COMPARE','HAL_GPIO_WritePin','HAL_I2C','HAL_UART_Transmit'))
}
for k,v in checks.items():
 print(('PASS' if v else 'FAIL')+': '+k)
if not all(checks.values()):
 raise SystemExit('FW identity protocol/producer/consumer source verification failed')
print('PASS: host/static and .cproject XML. Target build and MCU proof separate.')
