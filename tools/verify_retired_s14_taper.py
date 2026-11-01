#!/usr/bin/env python3
"""Retired S14/S14.1 LEFT taper exclusion; preserve all active owners and safety.

SOURCE/static only, no STM32 build or physical timing proof.
"""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[1]
main=(ROOT/'Core/Src/main.c').read_text(encoding='utf-8')
asserts={
 'S14/S14.1 LEFT-taper feature fully removed':
   all(tok not in main for tok in (
      'steer_s14_manual_enable','steer_s14_manual_latched',
      'steer_s141_left_enable','steer_s141_left_latched',
      'Steering_S14ManualApply(', 'M8S141_PlanLeftSaturated(',
      'M8S14_Plan(', 'M8S00_S14,','M8S00_S141,',
      'steer_s14_trim_l_pwm','steer_s14_trim_r_pwm')),
 'S14 obsolete header not referenced in firmware':
   '#include "steer_s14_math.h"' not in main and
   not (ROOT/'Core/Inc/steer_s14_math.h').exists(),
 'AS5600 observer still used by active S14.2':
   main.count('Steering_S14Observe(')>=3 and
   'steer_s14_obs_l.speed_cdeg_s' in main and
   'steer_s14_obs_r.speed_cdeg_s' in main,
 'all active steering owner math preserved':
   all(k in main for k in (
     'Steering_S142SyncApply(', 'Steering_S147PIApply(',
     'Steering_S15Apply(', 'M8S152_Record(0U)',
     'M8S151_Record(', 'Steering_AngleWaypointApply(')),
 'manual shared reset preserved':
    'Steering_S14ManualReset();' in main and
    'steer_s142_latched = 0U;' in main and
    'steer_s15_latched = 0U;' in main,
 'operator defaults retained':
   'steer_s142_trial_level = 2U;' in main and
   'steer_s143_fast_start_enable = 1U;' in main and
   'steer_s15_common_time_enable = 0U;' in main,
 'stop/NC and late atomic safety checks unchanged':
   all(k in main for k in (
     'Steering_SafetyOnLimitInterrupt(uint8_t bit)',
     'primask = __get_PRIMASK();','__disable_irq();',
     'mask = Steering_TestRawLimitMask();',
     'if (primask == 0U) __enable_irq();',
     'Steering_ManualLeftOff();','Steering_ManualRightOff();')),
 'S15.2 PowerShell and M8S00 retained':
   (ROOT/'tools/capture_m8_s152.ps1').exists() and
   '(M8S152_Dump() == 0U)' in main and
   'M8S00_DumpUpdate();' in main,
 'S11 and original wheel logger retained for VC02':
   'M8S11_StreamUpdate();' in main and
   (ROOT/'Services/Src/ram_logger.c').exists() and
   'VC02_M2_ProcessEvent(' in main,
 'replaced old PI bypass never re-enabled':
   '((steer_angle_wp_enable == 1U) ||' not in main and
   'else if ((steer_angle_wp_enable == 1U) &&' in main
}
# Protect especially risky atomic PWM critical body against accidental diff
BASE='70f304b6463e3555e18068f04e7f227b1eb90dfd'
try:
    old=subprocess.run(['git','show',f'{BASE}:Core/Src/main.c'], cwd=ROOT,
        capture_output=True,text=True,check=True).stdout
except Exception as exc:
    print('FAIL: cannot verify baseline critical source:',exc)
    raise SystemExit(1)
def window(source,begin,end):
    return source[source.index(begin):source.index(end,source.index(begin))]
areas=[
 ('static void Steering_ManualStop(void)\n{',
   '/* Every endstop is a LOCAL steering stop.'),
 ('  /* NC inputs must be re-evaluated ATOMICALLY before CCR writes.',
   '  M8S151_Record(m8s151_ownerL,m8s151_ownerR,0U);'),
 ('static void M8S00_DumpUpdate(void)\n{',
  'static void Steering_SyncReset(void)\n{'),
 ('static void Steering_PS2ManualArmUpdate(void)\n{',
  '/* No new I2C: use cache refreshed by armed safety reads earlier this loop.')
]
for start,end in areas:
    asserts[f'bit-exact historical critical window: {start[:32]}'] = (
        window(main,start,end)==window(old,start,end))
for title,ok in asserts.items():
    print(('PASS' if ok else 'FAIL')+': '+title)
if not all(asserts.values()):
    raise SystemExit('S14/S14.1 retirement guard FAIL')
print('PASS retirement static guard. Not CubeIDE build, timing or hardware PASS.')
