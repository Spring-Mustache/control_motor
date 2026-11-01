#!/usr/bin/env python3
from pathlib import Path
import shutil,subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'Core/Src/main.c').read_text()
h=(r/'Core/Inc/steer_s151_trace.h').read_text()
c={
'proposed 300 separate from actual 200':'RIGHT_PROPOSED_CAP 300U' in h and 'RIGHT_ACTIVE_CAP 200U' in h and s.count('M8S151_RIGHT_PROPOSED_CAP')==1,
'original motor owner':'*pwm_r=plan.right.output_ccr;' in s and 'cap_r=M8S142_Envelope(0U,steer_s142_latched,dist_r);' in s,
'post atomic capture':'M8S151_Record(m8s151_ownerL,m8s151_ownerR,0U);' in s,
'UART disarmed gate':'(M8S151_Dump() == 0U)' in s and 'if ((steer_safety_motion_permit != 0U)' in s,
'no actuation from recorder':all(t not in s[s.index('static void M8S151_Record('):s.index('static void Steering_ManualStop(void)')] for t in ('__HAL_TIM_SET_COMPARE','HAL_UART_Transmit','HAL_GPIO_WritePin','Steering_PositionReadOne(')),
'legacy S15 boot off':'steer_s15_common_time_enable = 0U;' in s}
for k,v in c.items():print(('PASS' if v else 'FAIL')+': '+k)
if not all(c.values()):raise SystemExit(1)
cc=shutil.which('gcc') or shutil.which('clang')
if not cc:raise SystemExit('no host compiler')
with tempfile.TemporaryDirectory() as d:
 p=subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror','-I',str(r/'Core/Inc'),str(r/'tools/test_m8_s151_trace.c'),'-o',str(Path(d)/'run')],capture_output=True,text=True)
 if p.returncode:raise SystemExit(p.stderr)
 p=subprocess.run([str(Path(d)/'run')],capture_output=True,text=True)
 if p.returncode:raise SystemExit(p.stdout+p.stderr)
 print(p.stdout)
