#!/usr/bin/env python3
"""M8-S14.8 static routing + GCC production helper regression."""
from pathlib import Path
import os,shutil,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'Core/Src/main.c').read_text(encoding='utf-8')
h=(root/'Core/Inc/steer_s148_level2_hybrid.h').read_text(encoding='utf-8')
c=(root/'Core/Inc/steer_s142_speed.h').read_text(encoding='utf-8')
checks={
 'Level2 boot default':'volatile uint8_t steer_s142_trial_level = 2U;' in s,
 'PI remains opt-in OFF':'volatile uint8_t steer_s147_velocity_pi_enable = 0U;' in s,
 'disarmed manual and PS2':'volatile uint8_t steer_manual_enable = 0U;' in s and
    'volatile uint8_t steer_ps2_manual_armed = 0U;' in s,
 'both Level1 and Level2 can latch unless S15 owns':
    'steer_s147_latched = ((steer_s15_latched==0U) &&' in s and
    '((steer_s142_latched == 1U) ||' in s and
    '(steer_s142_latched == 2U)) &&' in s,
 'S14.5 remains mutually exclusive':'(steer_s144_rmin_catchup_enable == 0U)' in s,
 'Dpad LEFT pilot only':'(PS2_Pad.Left != 0U) && (PS2_Pad.Right == 0U)' in s,
 'hybrid computes P+D and PI separately':'M8S142SyncPlan_t pd_plan=M8S142_SyncPlan(' in s and
    'Steering_S147PIApply(travel_l,length_l,dist_l,cap_l,' in s and
    'M8S148_Merge(owner_l,owner_r,cap_l,cap_r,' in s,
 'per-axis max not sum':'if(pd>demand)demand=pd;' in h and 'if(pi>demand)demand=pi;' in h,
 'P+D trim not used': 'pd->trim' not in h and 'pd->boost_l' in h,
 'zero CCR guard': 'if(pd==0||owner_l==0U||owner_r==0U)return out;' in h,
 'SOFT source guard':'if(dist_l<=180U||dist_r<=180U)return;' in s,
 'independent 375/150 and 450/200': '375U:150U' in c and '450U:200U' in c,
 'owner approach taper intact':'STEER_MANUAL_APPROACH_WINDOW_RAW 100U' in s,
 'atomic late IRQ NC guard':'primask = __get_PRIMASK();\n  __disable_irq();\n  mask = Steering_TestRawLimitMask();' in s,
 'new telemetry':'M8S00_S148,frames=%lu,pdLf=%lu,pdRf=%lu,piLf=%lu,piRf=%lu' in s,
 'no HAL or GPIO in pure helper':all(x not in h for x in
    ('HAL_GPIO','__HAL_TIM','GPIO_Pin','I2C','printf(','malloc(')),
 'R1 R2 original cap':'steer_pos_left_pwm_max = 300U;' in s and
    'steer_pos_right_pwm_max = 100U;' in s,
}
for name,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+name)
if not all(checks.values()):raise SystemExit(1)
cc=next((p for n in ('gcc','clang','cc') if (p:=shutil.which(n))),None)
if cc is None:raise SystemExit('FAIL: C compiler required')
with tempfile.TemporaryDirectory() as d:
    exe=Path(d)/('s148.exe' if os.name=='nt' else 's148')
    p=subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror',
      '-I',str(root/'Core/Inc'),str(root/'tools/test_m8_s148_level2_hybrid.c'),
      '-o',str(exe)],capture_output=True,text=True)
    if p.returncode:raise SystemExit(p.stderr)
    p=subprocess.run([str(exe)],capture_output=True,text=True)
    if p.returncode:raise SystemExit(p.stdout+'\n'+p.stderr)
    print(p.stdout)
print('HOST/static only; CubeIDE, Flash, bench and loaded gates remain unverified')
