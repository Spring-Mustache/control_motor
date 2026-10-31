#!/usr/bin/env python3
"""M8-S15 exact source routing and pure C common-time trajectory regression."""
from pathlib import Path
import os,shutil,subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'Core/Src/main.c').read_text(encoding='utf-8')
h=(r/'Core/Inc/steer_s15_common_time.h').read_text(encoding='utf-8')
checks={
 'new header included':'#include "steer_s15_common_time.h"' in s,
 'single controller opt-in boot OFF':'steer_s15_common_time_enable = 0U;' in s,
 'neutral to Dpad LEFT pilot latch':'steer_s15_latched = ' in s and
     '(PS2_Pad.Left != 0U) && (PS2_Pad.Right == 0U)' in s,
 'PI S147 disabled when S15 owns':'steer_s147_latched = ((steer_s15_latched==0U) &&' in s,
 'no S14 planner stacked when S15 owns':'if(steer_s15_latched!=0U)' in s and
     'Steering_S15Apply(' in s and 'else if (steer_s142_latched != 0U)' in s,
 'trajectory uses calibrated separate angles':'Steering_AngleFromRaw(1U,start_l' in s and
     'Steering_AngleFromRaw(0U,start_r' in s and
     'M8S15_Init(&steer_s15_session,' in s,
 'same clock for different goals':'p.left=M8S15_Axis(s->start_l,s->goal_l' in h and
     'p.right=M8S15_Axis(s->start_r,s->goal_r' in h and
     'phase_permille=(uint16_t)phase;' in h,
 'two axes sensor fresh and speed':'steer_manual_right_axis.speed_valid' in s and
     'steer_manual_left_axis.speed_valid' in s and
     'steer_manual_right_axis.speed_filtered_raw_s' in s,
 'no terminal positive boost':'distance_raw<=180U&&desired>(int32_t)owner' in h,
 'old owner and IRQ safety kept':'Steering_ManualAxisUpdate(1U, left_direction, mask, now, lpwm, &la);' in s and
     'Steering_ManualAxisUpdate(0U, right_direction, mask, now, rpwm, &ra);' in s and
     'mask = Steering_TestRawLimitMask();' in s,
 'no motor IO in pure controller':all(x not in h for x in
      ('__HAL_TIM','HAL_GPIO','I2C','printf(','GPIO_Pin','malloc(')),
 'M8S00 telemetry':'M8S00_S15,enable=%u,last=%u' in s,
 'PI remains boot OFF':'steer_s147_velocity_pi_enable = 0U;' in s,
 'manual arm remains boot OFF':'steer_manual_enable = 0U;' in s,
}
for name,good in checks.items(): print(('PASS' if good else 'FAIL')+': '+name)
if not all(checks.values()):raise SystemExit(1)
cc=next((p for n in ('gcc','clang','cc') if (p:=shutil.which(n))),None)
if not cc:raise SystemExit('FAIL no C compiler')
with tempfile.TemporaryDirectory() as d:
  exe=Path(d)/('m8s15.exe' if os.name=='nt' else 'm8s15')
  p=subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror',
      '-I',str(r/'Core/Inc'),str(r/'tools/test_m8_s15_common_time.c'),
      '-o',str(exe)],capture_output=True,text=True)
  if p.returncode:raise SystemExit(p.stderr)
  p=subprocess.run([str(exe)],capture_output=True,text=True)
  if p.returncode:raise SystemExit(p.stdout+'\n'+p.stderr)
  print(p.stdout)
print('HOST ONLY: CubeIDE build, flash and physical common-arrival not verified')
