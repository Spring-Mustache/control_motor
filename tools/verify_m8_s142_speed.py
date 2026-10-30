#!/usr/bin/env python3
"""Source-only S14.2 manual ceiling routing regression guard."""
from pathlib import Path
import shutil, subprocess, tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'Core/Src/main.c').read_text()
h=(root/'Core/Inc/steer_s142_speed.h').read_text()
checks={
  'OFF by default':'steer_s142_trial_level = 0U;' in s,
  'latched on new command':'steer_s142_latched = Steering_S142Level(steer_s142_trial_level);' in s,
  'speed-only manual cap':'M8S142_Envelope(left,steer_s142_latched,distance)' in s,
  'staged caps':'375U:150U' in h and '450U:200U' in h,
  'SOFT taper':'remaining<=180U' in h and 'remaining>=320U' in h,
  'R1/R2 caps retained':'steer_pos_left_pwm_max = 300U;' in s and 'steer_pos_right_pwm_max = 100U;' in s,
  'NC atomic CCR check':'primask = __get_PRIMASK();\n  __disable_irq();\n  mask = Steering_TestRawLimitMask();' in s,
  'old hard cap retained':'#define STEER_MANUAL_PWM_HARD_MAX     400U' in s,
  'S14 gain bypass':'if (steer_s142_latched != 0U)\n    {\n      /* Speed-priority experiment' in s,
  'NC escape original ceiling':'if (pwm > escape_cap) pwm = escape_cap;' in s and
      'uint16_t escape_cap = (left != 0U) ? 300U : 100U;' in s,
  'new telemetry':'M8S00_S142,level=%u,last=%u,' in s,
  'header has no motor IO':'__HAL_TIM_SET_COMPARE' not in h and 'HAL_GPIO_WritePin' not in h
}
for k,v in checks.items():print(('PASS' if v else 'FAIL')+': '+k)
if not all(checks.values()):raise SystemExit(1)
cc=next((p for n in ('gcc','clang','cc') if (p:=shutil.which(n))),None)
if cc is None:print('SKIP C host tests: no compiler')
else:
  with tempfile.TemporaryDirectory() as tmp:
    exe=Path(tmp)/'test_s142'
    p=subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror','-I',str(root/'Core/Inc'),str(root/'tools/test_m8_s142_speed.c'),'-o',str(exe)],capture_output=True,text=True)
    if p.returncode:raise SystemExit(p.stderr)
    q=subprocess.run([str(exe)],capture_output=True,text=True)
    if q.returncode:raise SystemExit(q.stderr)
    print(q.stdout)
print('Static only; STM32 build + hardware tests pending')
