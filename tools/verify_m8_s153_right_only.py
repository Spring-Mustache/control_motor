#!/usr/bin/env python3
"""S15.3 route and safety static plus pure C regression; MCU proof separate."""
from pathlib import Path
import os,subprocess,shutil,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'Core/Src/main.c').read_text(encoding='utf-8')
h=(root/'Core/Inc/steer_s153_right_only.h').read_text(encoding='utf-8')
reset=s[s.index('static void Steering_S14ManualReset(void)'):s.index('static void Steering_S14Observe(',s.index('static void Steering_S14ManualReset(void)'))]
active=s[s.index('static void Steering_S153RightApply('):s.index('static void Steering_ManualUpdate(void)\n{',s.index('static void Steering_S153RightApply('))]
manual=s[s.index('static void Steering_ManualUpdate(void)\n{'):]
a=manual.index('if(steer_s15_latched!=0U)\n      Steering_S15Apply(')
b=manual.index('else if (steer_s142_latched != 0U)',a)
checks={
 'S15.3 function dependencies and state macros declared before use':
  s.index('static uint8_t Steering_PositionRawInRange(uint8_t left, uint16_t raw)')<
  s.index('static void Steering_S153RightApply(') and
  s.index('#define STEER_MANUAL_MOVING     1U')<
  s.index('static void Steering_S153RightApply(') and
  s.index('static void Steering_S153RightApply(')<
  s.index('static void Steering_ManualUpdate(void)'),
 'S15.3 ON only for RIGHT-to-R_MIN in new Level2 command':
  'steer_s153_right_only_enable=1U;' in s and
  '(right_direction<0))?1U:0U;' in s and
  'steer_s142_trial_level = 2U;' in s and
  'steer_s143_fast_start_enable = 1U;' in s,
 'S15 common-T remains OFF, RIGHT300 proposal inactive':
  'steer_s15_common_time_enable = 0U;' in s and
  s.count('M8S151_RIGHT_PROPOSED_CAP')==1,
 'enable latched only at fresh manual command':
  'steer_s153_latched = ((steer_s153_right_only_enable==1U)' in s and
  '(steer_s142_latched==2U)' in s and
  '(steer_s15_latched==0U)' in s and
  'steer_s153_last=steer_s153_latched;' in s,
 'S14.7/9 suppressed by pilot selection':
  '(steer_s153_latched==0U)' in s and
  'steer_s149_latched=((steer_s147_latched!=0U)' in s,
 'S15.3 preserves legacy LEFT boosts, drops RIGHT trim, no LEFT taper':
  'else if(steer_s153_latched!=0U)\n    {' in manual[a:b] and
  'uint16_t baseline_l=la,legacy_r=ra;' in manual[a:b] and
  '&la,&legacy_r);' in manual[a:b] and
  'if(la<baseline_l)la=baseline_l;' in manual[a:b] and
  'left_direction,&la,&ra);' in manual[a:b] and
  'ra=legacy_r' not in manual[a:b],
 'observer reset each command; no stale mode after stop':
  'steer_s153_latched=0U;' in reset and 'steer_s153_obs=(M8S152State_t){0};' in reset,
 'RIGHT-only path no modification LEFT or owner direction':
  '*pwm_r=p.right_pwm;' in active and
  not any(t in active for t in ('*pwm_l=', '__HAL_TIM_SET_COMPARE',
    'HAL_GPIO_WritePin','HAL_UART_Transmit','Steering_PositionReadOne(', 'WheelControl_')),
 'S15.2 independent shadow unchanged':
  'M8S152_Record(0U); /* passive D-pad/RX SHADOW, post-atomic */' in s and
  'M8S152_ShadowRight' in s,
 'fresh AS5600 and geometry required and no NC bypass':
  'M8S152_Observe(' in active and
  'Steering_PositionRawInRange(1U,raw_l)' in active and
  'Steering_PositionRawInRange(0U,raw_r)' in active and
  'Steering_TestRawLimitMask()==0U' in active,
 'All terminal PWM60 trial source removed; baseline 50CCR retained':
  'steer_manual_right_rx_min_pwm = 50U;' in s and
  not any(x in s for x in ('steer_right_min_tail60',
      'M8S15_TAIL60','floorTrial=60','trial_pwm = (uint16_t)(60U')),
 'S15.3 bounded ETA of q/s -> RAW/s, no increase near either SOFT':
  'M8S153_PlanRightMid(' in active and
  '(uint8_t)(direction_l>0)' in active and
  'M8S153_RMIN_MID_MIN_RAW 320U' in h and
  'M8S153_RMIN_MID_MAX_RAW 650U' in h and
  'M8S153_ETA_TAIL_LEFT_MS 484U' in h and
  'M8S153_ETA_TAIL_RIGHT_MS 886U' in h and
  'M8S153_ETA_MIN_SPAN_RAW 700U' in h and
  'M8S153_ETA_MIN_Q_SPEED 250' in h and
  'M8S153_ETA_DEADBAND_MS 60L' in h and
  'right_to_min!=1U' in h and
  'remain_l<=M8S153_RMIN_MID_MIN_RAW' in h and
  'remain_r<=M8S153_RMIN_MID_MIN_RAW' in h and
  'obs->velL<M8S153_ETA_MIN_Q_SPEED' in h and
  'obs->velR<M8S153_ETA_MIN_Q_SPEED' in h and
  'obs->angle_valid==0U||obs->pred_valid==0U' in h and
  '(uint32_t)obs->velL*span_l)/2000U' in h and
  '(uint32_t)obs->velR*span_r)/2000U' in h and
  'if(want==0U)return p;' in h and
  'M8S153_RMIN_MID_BIAS_PM' not in h,
 'Pre-atomic S14.2 RIGHT reduction instrumentation, LEFT unchanged':
  'if((legacy_r<ra)&&' in manual[a:b] and
  'steer_s153_avoided_reduction_r_frames++' in manual[a:b] and
  'earlyFrames=%lu,avoidedReductionR=%lu' in s and
  'etaFrames=%lu,peakEtaLagMs=%u' in s and
  'steer_s153_eta_frames=0U;' in s and
  'steer_s153_peak_eta_lag_ms=0U;' in s and
  'if(p.eta_valid!=0U&&steer_s153_eta_frames!=0xffffffffUL)' in active and
  'steer_s153_early_frames=0U;' in s,
 'S15.3 pure C only capped RIGHT and far from both terminal regions':
  all(t in h for t in ('M8S153_RIGHT_MAX_EXTRA_CCR 12U',
   'remain_l<=M8S153_TERMINAL_RAW','remain_r<=M8S153_TERMINAL_RAW',
   'owner_l==0U||owner_r==0U','obs->pred_valid==0U',
   'moved_l<M8S153_START_MOTION_RAW','moved_r<M8S153_START_MOTION_RAW')),
 'S15.3 has after-atomic post-disarm-only diagnostic':
  'M8S153_Dump() == 0U' in s and 'M8S153,enable=%u,last=%u,run=%lu,' in s and
  'steer_s153_last_applied_r=steer_manual_right_pwm;' in s and
  'if ((steer_safety_motion_permit != 0U)' in s,
 'late atomic NC/STOP/CCR guard still present':
  'primask = __get_PRIMASK();\n  __disable_irq();\n  mask = Steering_TestRawLimitMask();' in manual and
  '__HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, ra);' in manual,
 'PS2/Voice and PowerShell capture intact':
  'Control_Arbitrate();' in s and
  (root/'tools/capture_m8_s152.ps1').is_file(),
}
for k,v in checks.items():print(('PASS' if v else 'FAIL')+': '+k)
if not all(checks.values()):raise SystemExit('S15.3 static verification FAIL')
cc=shutil.which('gcc') or shutil.which('clang') or shutil.which('cc')
if not cc:raise SystemExit('No HOST C compiler: FAIL, not PASS')
with tempfile.TemporaryDirectory() as tmp:
 exe=Path(tmp)/('s153.exe' if os.name=='nt' else 's153')
 p=subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror','-I',
  str(root/'Core/Inc'),str(root/'tools/test_m8_s153_right_only.c'),
  '-o',str(exe)],capture_output=True,text=True)
 if p.returncode:raise SystemExit(p.stdout+p.stderr)
 p=subprocess.run([str(exe)],capture_output=True,text=True)
 if p.returncode:raise SystemExit(p.stdout+p.stderr)
 print(p.stdout)
print('S15.3 HOST/STATIC PASS, not target MCU build/flash, real-time or motor safety acceptance')
