#!/usr/bin/env python3
"""Static guard and compiled actual-C capture helper, no target or hardware PASS."""
from pathlib import Path
import tempfile, subprocess, shutil
R=Path(__file__).resolve().parents[1]
s=(R/"Core/Src/main.c").read_text(encoding="utf-8")
ci=(R/".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
ps=(R/"tools/capture_m8_s154_right_fine.ps1").read_text(encoding="utf-8")
def fn(signature):
    start=s.index(signature)
    b=s.index("{",start)
    depth=0
    for j in range(b,len(s)):
        if s[j]=="{":depth+=1
        elif s[j]=="}":
            depth-=1
            if depth==0:return s[start:j+1]
    raise AssertionError(signature)
f=fn("static void Steering_VoiceRightFineCapture(")
fine=fn("static void Steering_PositionFineUpdate(")
finish=fn("static void Steering_PositionFinish(")
pos=fn("static void Steering_PositionUpdate(")
uart=fn("static void M8S11_StreamUpdate(void)\\n{")
sync=fn("static void Steering_VoiceS154Sync(")
assert "steer_voice_s154_enable = 1U;" in s
assert "steer_angle_wp_enable = 1U;" in s
assert "voice_pos_enable = 1U;" in s
assert "voice_angle_mapping_enable = 1U;" in s
assert "steer_voice_s154_right_fine_diag_enable = 1U;" in s
assert "steer_voice_s154_right_fine_enter_ms = now;" in fine
assert "steer_voice_s154_right_fine_reversals++;" in fine
assert "Steering_VoiceRightFineCapture(result);" in finish
assert finish.index("Steering_VoiceRightFineCapture(result);") < finish.index("voice_pos_owned=0U")
assert "steer_voice_s154_right_fine_enter_ms = 0U;" in pos
assert "(voice_pos_owned == 0U) || (steer_pos_command != 4U)" in f
assert "m8_s154_right_fine_pending = 1U;" in f
for guard in ("steer_pos_active == 0U","steer_manual_active == 0U",
 "Steering_PositionWheelsStopped() != 0U","huart3.gState == HAL_UART_STATE_READY",
 "__HAL_TIM_GET_COMPARE(&htim12, TIM_CHANNEL_2) == 0U",
 "__HAL_TIM_GET_COMPARE(&htim14, TIM_CHANNEL_1) == 0U",
 "m8_s11_stream_state == 0U","m8_s11_stream_enable == 0U"):
    assert guard in uart, guard
assert "M8S154_FINE,sha=%s" in uart and "M8S11_SendLine(n)" in uart
assert "HAL_UART_Transmit(" not in uart
assert "Steering_AngleWaypointApply(mode," in sync
assert "Steering_VoiceS154Sync(now, raw_mask, &left_cmd, &right_cmd)" in pos
assert "verify_m8_s154_right_fine_diag.py" in ci
assert "M8S154_FINE," in ps and "SerialPort" in ps
cc=shutil.which("gcc") or shutil.which("cc") or shutil.which("clang")
if not cc:raise SystemExit("missing compiler")
top=r"""
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef struct {
uint32_t seq,total_ms,fine_ms,skew_ms,dropped;
uint16_t start,goal,entry,quiet,end,gap,pulse_max_ms;
int16_t error,last_move;
uint8_t angle,turn,pulses,reversals,weak,good,back;
uint8_t quiet_valid,phase,result_l,result_r,result_total;
} M8S154RightFineDiag_t;
typedef struct {
uint16_t fine_enter_raw,fine_first_quiet_raw,fine_max_pulse_ms;
int16_t fine_last_move_raw;
uint8_t fine_pulses,fine_weak_pulses,fine_good_pulses,fine_backtrack_pulses;
uint8_t fine_first_quiet_valid,fine_phase;
} SteeringPositionAxis_t;
static SteeringPositionAxis_t steer_right_axis={0};
static M8S154RightFineDiag_t m8_s154_right_fine_last={0};
static uint8_t m8_s154_right_fine_pending=0;
static uint8_t steer_voice_s154_right_fine_diag_enable=1;
static uint32_t steer_voice_s154_right_fine_log_count=0;
static uint32_t steer_voice_s154_right_fine_drop_count=0;
static uint32_t steer_voice_s154_right_fine_enter_ms=450;
static uint8_t steer_voice_s154_right_fine_reversals=2;
static uint8_t voice_pos_owned=1,steer_pos_command=5;
static uint32_t steer_pos_start_ms=100;
static uint32_t steer_voice_s154_arrival_skew_ms=12;
static uint16_t steer_voice_s154_peak_gap_permille=36;
static uint16_t steer_voice_s154_start_r=662,voice_pos_goal_r=850;
static uint16_t debug_as5600_right_raw=847;
static int16_t steer_pos_right_error=3;
static uint8_t voice_pos_last_angle_deg=30;
static int8_t voice_pos_last_turn=-1;
static uint8_t steer_pos_left_result=2,steer_pos_right_result=2;
static uint32_t HAL_GetTick(void){return 750;}
"""
bottom=r"""
int main(void){
steer_right_axis.fine_pulses=3;steer_right_axis.fine_enter_raw=835;
steer_right_axis.fine_first_quiet_raw=860;
steer_right_axis.fine_backtrack_pulses=1;
Steering_VoiceRightFineCapture(2);
assert(!m8_s154_right_fine_pending);
steer_pos_command=4;
Steering_VoiceRightFineCapture(2);
assert(m8_s154_right_fine_pending==1);
assert(m8_s154_right_fine_last.fine_ms==300);
assert(m8_s154_right_fine_last.total_ms==650);
assert(m8_s154_right_fine_last.pulses==3);
assert(m8_s154_right_fine_last.reversals==2);
assert(m8_s154_right_fine_last.back==1);
assert(m8_s154_right_fine_last.result_r==2);
Steering_VoiceRightFineCapture(2);
assert(steer_voice_s154_right_fine_drop_count==1);
printf("PASS actual C passive Voice4 RIGHT fine snapshot\n");
return 0;
}
"""
with tempfile.TemporaryDirectory() as d:
    fpath=Path(d)/"t.c"
    fpath.write_text(top+f+"\n"+bottom)
    exe=Path(d)/"test"
    b=subprocess.run([cc,"-std=c11","-O2","-Wall","-Wextra","-Werror",str(fpath),"-o",str(exe)],text=True,capture_output=True)
    if b.returncode:raise SystemExit("COMPILE FAILED\n"+b.stderr)
    run=subprocess.run([str(exe)],text=True,capture_output=True)
    if run.returncode:raise SystemExit("TEST FAILED\n"+run.stderr+run.stdout)
    print(run.stdout.strip())
print("PASS boot-enable, no actuator changes, UART idle/IRQ guards")
