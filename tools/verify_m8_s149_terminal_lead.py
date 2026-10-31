#!/usr/bin/env python3
"""M8-S14.9 source-only gates and exact production C helper regression."""
from pathlib import Path
import os,shutil,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'Core/Src/main.c').read_text(encoding='utf-8')
h=(root/'Core/Inc/steer_s149_terminal_lead.h').read_text(encoding='utf-8')
checks={
 'Level2 default unchanged':'steer_s142_trial_level = 2U;' in s,
 'PI opt-in still OFF at boot':'steer_s147_velocity_pi_enable = 0U;' in s,
 'terminal pilot opt-in OFF at boot':'steer_s149_early_lead_enable = 0U;' in s,
 'new fresh stroke latch':'steer_s149_latched=((steer_s147_latched!=0U) &&' in s,
 'Dpad LEFT+RMIN only inherited': '(PS2_Pad.Left != 0U) && (PS2_Pad.Right == 0U)' in s and
     '(right_direction < 0)' in s,
 'no coordinator within either 180 RAW':'if(dist_l<=180U||dist_r<=180U)return;' in s,
 'new predictor inside hybrid only':'M8S149_Predict(dist_l,dist_r,' in s and
     'M8S149_ChooseRight(owner_r,hybrid.right_pwm' in s,
 'no extra beyond original owner envelopes':'cap_r=M8S142_Envelope(0U,steer_s142_latched,dist_r);' in s and
     'if(result>cap)result=cap;' in h,
 'no additive boost':'if(want>result)result=want;' in h,
 'rise limited and zero stays zero':'(uint32_t)previous_final+12U' in h and
     'if(owner==0U||hybrid==0U||cap<=owner)return hybrid;' in h,
 'lag forecast bounded not hardware claim':'M8S149_TERMINAL_SKEW_MS 337L' in h and
     'M8S149_MAX_EXTRA_CCR 25U' in h,
 'owner 100 RAW taper unchanged':'STEER_MANUAL_APPROACH_WINDOW_RAW 100U' in s,
 'manual and PS2 boot unarmed':'steer_manual_enable = 0U;' in s and
     'steer_ps2_manual_armed = 0U;' in s,
 'atomic late limit guard unchanged':
     'primask = __get_PRIMASK();\n  __disable_irq();\n  mask = Steering_TestRawLimitMask();' in s,
 'R1/R2 original PWM limits':'steer_pos_left_pwm_max = 300U;' in s and
     'steer_pos_right_pwm_max = 100U;' in s,
 'bin snapshots and pilot counters':'M8S00_S149_BIN,bin=%u,frames=%lu' in s and
     'M8S00_S149,enable=%u,last=%u,frames=%lu' in s,
 'S149 passive bins match actual Dpad LEFT axis signs':
     'int8_t left_direction = (int8_t)(-command);' in s and
     'int8_t right_direction = command;' in s and
     '(left_direction>0) && (right_direction<0) &&' in
     s.split('/* M8-S14.9 passive, post-atomic bin snapshots.',1)[1]
      .split('  /* Do not re-arm automatically',1)[0] and
     '(left_direction<0) && (right_direction<0)' not in
     s.split('/* M8-S14.9 passive, post-atomic bin snapshots.',1)[1]
      .split('  /* Do not re-arm automatically',1)[0],
 'pure header no HAL motor access':all(w not in h for w in
      ('HAL_GPIO','__HAL_TIM','GPIO_Pin','I2C','printf(','malloc(')),
}
for name,good in checks.items():print(('PASS' if good else 'FAIL')+': '+name)
if not all(checks.values()):raise SystemExit(1)
cc=next((p for n in ('gcc','clang','cc') if (p:=shutil.which(n))),None)
if cc is None:raise SystemExit('FAIL: strict C host compiler required')
with tempfile.TemporaryDirectory() as tmp:
    exe=Path(tmp)/('m8s149.exe' if os.name=='nt' else 'm8s149')
    p=subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror',
      '-I',str(root/'Core/Inc'),str(root/'tools/test_m8_s149_terminal_lead.c'),
      '-o',str(exe)],capture_output=True,text=True)
    if p.returncode:raise SystemExit(p.stderr)
    p=subprocess.run([str(exe)],capture_output=True,text=True)
    if p.returncode:raise SystemExit(p.stdout+'\n'+p.stderr)
    print(p.stdout)
print('HOST/static only; CubeIDE/Flash/stand clearance and current unverified')
