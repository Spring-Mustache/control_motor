#!/usr/bin/env python3
"""Static source safety/route checks + production C helper HOST test."""
from pathlib import Path
import os,shutil,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'Core/Src/main.c').read_text(encoding='utf-8')
h=(root/'Core/Inc/steer_s152_manual_relation.h').read_text(encoding='utf-8')
a=s.index('static void M8S152_Record(uint8_t force)')
b=s.index('static void Steering_ManualStop(void)',a)
rec=s[a:b]
a=s.index('static uint8_t M8S152_Dump(void)')
b=s.index('static void M8S00_DumpUpdate(void)',a)
dump=s[a:b]
check={
 "Level2 and Fast Start unchanged":'steer_s142_trial_level = 2U;' in s and 'steer_s143_fast_start_enable = 1U;' in s,
 "S15 existing pilot boot OFF":'steer_s15_common_time_enable = 0U;' in s,
 "RIGHT 300 proposed remains unused":s.count('M8S151_RIGHT_PROPOSED_CAP')==1,
 "both D-pad directions and RX traced":'m8s152_kind=m8_s05_manual_kind;' in s and 'm8s152_direction_l=left_direction;' in s and 'm8s152_active=1U;' in s,
 "observe AFTER actual atomic PWM":s.index('M8S152_Record(0U);')>s.index('M8S151_Record(m8s151_ownerL,m8s151_ownerR,0U);')>s.index('if (primask == 0U) __enable_irq();',s.index('static void Steering_ManualUpdate(void)')),
 "record terminal PWM=0":'M8S152_Record(1U); /* final PWM0 */' in s,
 "AS5600 timestamps separately":'steer_pos_left_last_sample_ms,steer_pos_right_last_sample_ms' in rec,
 "read actual PWM only":'__HAL_TIM_GET_COMPARE(&htim12,TIM_CHANNEL_2)' in rec and '__HAL_TIM_GET_COMPARE(&htim14,TIM_CHANNEL_1)' in rec,
 "no actuation/UART/I2C in recorder":not any(z in rec for z in ['__HAL_TIM_SET_COMPARE','HAL_UART_Transmit','HAL_GPIO_WritePin','Steering_PositionReadOne(','WheelControl_SetTargetPWM']),
 "no actuation/I2C in helper":not any(z in h for z in ['__HAL_TIM_SET_COMPARE','HAL_UART_Transmit','HAL_GPIO_WritePin','HAL_I2C']),
 "dump only through disarm M8S00":'(M8S152_Dump() == 0U)' in s and 'if ((steer_safety_motion_permit != 0U)' in s,
 "dump CRLF":r'\r\n' in dump and r'\\r\\n' not in dump,
 "no imagined 300 boost":'remainingL<=180U||remainingR<=180U' in h and 'appliedR==0U' in h,
 "no output use of shadow":'ra=r->shadowR' not in s and 'steer_manual_right_pwm=r->shadowR' not in s
}
for k,x in check.items():print(('PASS' if x else 'FAIL')+': '+k)
if not all(check.values()):raise SystemExit('S15.2 static regression failure')
cc=shutil.which('gcc') or shutil.which('clang') or shutil.which('cc')
if cc is None:raise SystemExit('HOST compiler absent, no false PASS')
with tempfile.TemporaryDirectory() as d:
 exe=Path(d)/('s152.exe' if os.name=='nt' else 's152')
 p=subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror','-I',
                   str(root/'Core/Inc'),
                   str(root/'tools/test_m8_s152_manual_relation.c'),'-o',str(exe)],
                  text=True,capture_output=True)
 if p.returncode:raise SystemExit(p.stdout+p.stderr)
 p=subprocess.run([str(exe)],text=True,capture_output=True)
 if p.returncode:raise SystemExit(p.stdout+p.stderr)
 print(p.stdout)
print('S15.2 HOST/STATIC PASS, no STM32 build/flash or hardware acceptance')
