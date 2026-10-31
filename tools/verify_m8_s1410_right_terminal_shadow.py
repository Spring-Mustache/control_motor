#!/usr/bin/env python3
"""M8-S14.10: production pure C helper and source-only no-actuation guard."""
from pathlib import Path
import os,shutil,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'Core/Src/main.c').read_text(encoding='utf-8')
h=(root/'Core/Inc/steer_s1410_right_terminal_shadow.h').read_text(encoding='utf-8')
start='/* M8-S14.10 passive, RIGHT terminal observation. DO NOT write CCR.'
end='/* Do not re-arm automatically if held against a stop or stalled.'
segment=s.split(start,1)[1].split(end,1)[0] if start in s else ''
checks={
  'S1410 header included':'#include "steer_s1410_right_terminal_shadow.h"' in s,
  'Level2 default intact':'steer_s142_trial_level = 2U;' in s,
  'S149 controller unchanged':'M8S149_ChooseRight(owner_r,hybrid.right_pwm' in s,
  'S147 and S149 boot opt-in':'steer_s147_velocity_pi_enable = 0U;' in s and
      'steer_s149_early_lead_enable = 0U;' in s,
  'right-only Dpad LEFT fresh-latched active observation':
      '(steer_s149_latched!=0U) &&' in segment and
      '(left_direction>0) && (right_direction<0)' in segment and
      '(steer_manual_right_state==STEER_MANUAL_MOVING)' in segment,
  'reuse existing filtered speed':'steer_manual_right_axis.speed_filtered_raw_s' in segment and
      'steer_manual_right_axis.speed_valid' in segment and
      'steer_manual_right_axis.speed_sample_ms' in segment,
  'shadow only after existing atomic gate':
      s.index(start)>s.index('primask = __get_PRIMASK();\n  __disable_irq();\n  mask = Steering_TestRawLimitMask();'),
  'no output paths in shadow':all(t not in segment for t in
      ('__HAL_TIM','HAL_GPIO','Steering_ManualAxisUpdate(',
       'steer_manual_right_pwm =','ra =','*pwm_r=','right_cmd =','Steering_ManualRightOff(')),
  'right current owner is read only':
      'uint16_t owner=steer_manual_right_pwm;' in segment and
      'M8S1410_Plan(' in segment and
      'shadow.candidate_pwm' in segment,
  'terminal stats and UART':'M8S00_S1410,mode=shadow' in s and
      'M8S00_S1410_BIN,band=%u,frames=%lu' in s,
  'no >180 RAW shadow':'remaining_raw>M8S1410_TERMINAL_RAW' in h,
  'no shadow gain within 40 RAW':'remaining_raw<=M8S1410_APPROACH_OFF_RAW' in h,
  'shadow capped at existing owner':'existing_owner_cap-owner_pwm' in h,
  'motor-io-free production header':all(x not in h for x in
      ('HAL_GPIO','__HAL_TIM','I2C_','malloc(','volatile ')),
  'independent owner/NC/STOP untouched':
      'Steering_ManualAxisUpdate(0U, right_direction, mask, now, rpwm, &ra);' in s and
      'if ((pending & (STEER_LIMIT_R_MIN_MASK | STEER_LIMIT_R_MAX_MASK)) != 0U)' in s,
}
for label,ok in checks.items(): print(('PASS' if ok else 'FAIL')+': '+label)
if not all(checks.values()):raise SystemExit(1)
compiler=next((x for n in ('gcc','clang','cc') if (x:=shutil.which(n))),None)
if not compiler:raise SystemExit('FAIL: no C compiler')
with tempfile.TemporaryDirectory() as tmp:
  exe=Path(tmp)/('m8s1410.exe' if os.name=='nt' else 'm8s1410')
  r=subprocess.run([compiler,'-std=c11','-Wall','-Wextra','-Werror',
   '-I',str(root/'Core/Inc'),str(root/'tools/test_m8_s1410_right_terminal_shadow.c'),
   '-o',str(exe)],capture_output=True,text=True)
  if r.returncode:raise SystemExit(r.stderr)
  r=subprocess.run([str(exe)],capture_output=True,text=True)
  if r.returncode:raise SystemExit(r.stdout+'\n'+r.stderr)
  print(r.stdout)
print('HOST PASS only; passive observation, NOT physical terminal safety validation.')
