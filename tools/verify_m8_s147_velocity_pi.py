#!/usr/bin/env python3
"""M8-S14.7 exact C host tests and opt-in routing/static safety contract."""
from pathlib import Path
import os,shutil,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'Core/Src/main.c').read_text(encoding='utf-8')
h=(root/'Core/Inc/steer_s147_velocity_pi.h').read_text(encoding='utf-8')
checks={
 'PI boot OFF':'volatile uint8_t steer_s147_velocity_pi_enable = 0U;' in s,
 'PI latched only on new manual stroke':
     'steer_s147_latched = ((steer_s142_latched == 1U)' in s and
     'steer_s147_last = steer_s147_latched;' in s,
 'Dpad LEFT only and S144 exclusive':'(PS2_Pad.Left != 0U)' in s and
     '(steer_s144_rmin_catchup_enable == 0U)' in s,
 'independent R1/R2 unaffected':'Steering_PositionAxisUpdate(' in s and
     'Steering_S147PIApply(' in s,
 'owner checks precede PI':
     s.index('Steering_ManualAxisUpdate(1U, left_direction, mask, now, lpwm, &la);') <
     s.index('Steering_S142SyncApply(\n          steer_manual_left_axis.phase_start_raw,'),
 'atomic late NC guard follows PI':
     s.index('Steering_S142SyncApply(\n          steer_manual_left_axis.phase_start_raw,') <
     s.index('primask = __get_PRIMASK();\n  __disable_irq();\n  mask = Steering_TestRawLimitMask();'),
 'PI swaps rather than stacks S14.2 planner':
     'if(steer_s147_latched!=0U)' in s and
     'Steering_S147PIApply(' in s,
 'same AS5600 timestamps':'steer_s14_obs_l.sample_ms' in s and
     'steer_s14_obs_r.sample_ms' in s,
 'legacy owner taper unchanged':'STEER_MANUAL_APPROACH_WINDOW_RAW 100U' in s and
     'M8S142_Envelope(left,steer_s142_latched,distance)' in s,
 'legacy cap unchanged':'if(level==1U)return (left!=0U)?375U:150U;' in
     (root/'Core/Inc/steer_s142_speed.h').read_text(encoding='utf-8'),
 'boot motion remains unarmed':'volatile uint8_t steer_manual_enable = 0U;' in s,
 'no I/O in pure helper':all(x not in h for x in
      ('HAL_GPIO','__HAL_TIM','GPIO_Pin','I2C','printf(','malloc(')),
 'helper enforces near-soft no boost':
     'distance_raw<=M8S147_MIN_DIST_RAW' in h and
     'if(owner_pwm==0U' in h,
 'host telemetry':'M8S00_S147,enable=%u,last=%u,frames=%lu,' in s
}
for name,good in checks.items():print(('PASS' if good else 'FAIL')+': '+name)
if not all(checks.values()):raise SystemExit(1)
cc=next((p for n in ('gcc','clang','cc') if (p:=shutil.which(n))),None)
if cc is None:raise SystemExit('FAIL: compiler needed to verify PI implementation')
with tempfile.TemporaryDirectory() as d:
  exe=Path(d)/('test_s147.exe' if os.name=='nt' else 'test_s147')
  p=subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror',
    '-I',str(root/'Core/Inc'),str(root/'tools/test_m8_s147_velocity_pi.c'),
    '-o',str(exe)],capture_output=True,text=True)
  if p.returncode:raise SystemExit(p.stderr)
  p=subprocess.run([str(exe)],capture_output=True,text=True)
  if p.returncode:raise SystemExit(p.stdout+'\n'+p.stderr)
  print(p.stdout)
print('HOST/static only: CubeIDE Build, Flash and safe stand A/B remain unverified')
