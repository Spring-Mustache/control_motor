#!/usr/bin/env python3
"""S15.4 Voice: HOST-compile actual normalized AS5600 observer and Voice glue.

This is a HOST-only test. Neither actual STM32 HAL compilation, NC contacts
nor motor speed/arrival precision can be validated without the target.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess,shutil
R=Path(__file__).resolve().parents[1]
SRC=(R/"Core/Src/main.c").read_text(encoding="utf-8")

def function(signature):
    start=SRC.index(signature)
    brace=SRC.index("{",start)
    depth=0
    for i in range(brace,len(SRC)):
        if SRC[i]=="{":depth+=1
        elif SRC[i]=="}":
            depth-=1
            if depth==0:return SRC[start:i+1]
    raise AssertionError(signature)

sync=function("static void Steering_VoiceS154Sync(")
speed=function("static void Steering_AngleSpeedObserve(")
progress=function("static uint16_t Steering_AngleStrokeProgress(")
position=function("static void Steering_PositionUpdate(")
axis=function("static void Steering_PositionAxisUpdate(")
finish=function("static void Steering_PositionFinish(")
done=function("static void Steering_PositionAxisDone(")
waypoint=function("static void Steering_AngleWaypointApply(")
stage=function("static void Voice_S154StageSteerAngle(")
# Ensure the exact production wiring, not a host-implemented surrogate.
assert 'Steering_PositionAxisUpdate(1U, raw_mask, now, &left_cmd, &left_dir);' in position
assert 'Steering_PositionAxisUpdate(0U, raw_mask, now, &right_cmd, &right_dir);' in position
assert 'Steering_VoiceS154Sync(now, raw_mask, &left_cmd, &right_cmd);' in position
assert position.index("Steering_PositionAxisUpdate(0U") < position.index(
    "Steering_VoiceS154Sync(") < position.index("late_pending = steer_pos_irq_pending_mask;")
assert "Steering_PositionAtExpectedNc(1U, late_pending)" in position
assert "Steering_PositionAtExpectedNc(0U, late_pending)" in position
assert "Steering_PositionNcDirectionAllowed(1U, raw_mask, left_dir)" in position
assert "Steering_PositionNcDirectionAllowed(0U, raw_mask, right_dir)" in position
assert "Steering_PositionLeftOff();" in finish and "Steering_PositionRightOff();" in finish
assert "steer_voice_s154_active = 0U;" in finish
assert "steer_voice_s154_done_l_ms = t;" in done and "steer_voice_s154_done_r_ms = t;" in done
assert "steer_voice_s154_arrival_skew_ms" in done
assert "STEER_POS_TIMEOUT_MS" in position
assert "Steering_Voice45RawBoundaryReached(left, raw, goal)" in axis
assert "Steering_PositionFineUpdate(left, axis, raw, err, step, now," in axis
assert "if ((steer_pos_command == 1U) || (steer_pos_command == 4U))" in axis
assert "((angle==45U)?STEER_POS_VOICE45_NC:4U)" in stage
assert "STEER_POS_LEFT_MAX_RAW,STEER_POS_RIGHT_MIN_RAW" in stage
assert "STEER_POS_LEFT_MIN_RAW,STEER_POS_RIGHT_MAX_RAW" in stage
assert "steer_voice_s154_enable = 1U;" in SRC
assert "steer_voice_s154_common_eta_ms" in sync
assert "steer_voice_s154_progress_l = steer_angle_wp_progress_left;" in sync
assert "steer_voice_s154_progress_r = steer_angle_wp_progress_right;" in sync
assert "Steering_AngleStrokeProgress(sl, gl, al, &vl)" in waypoint
assert "Steering_AngleStrokeProgress(sr, gr, ar, &vr)" in waypoint
assert "steer_speed_obs_l" in waypoint and "steer_speed_obs_r" in waypoint
assert "Steering_AngleSpeedObserve(&steer_speed_obs_l" in waypoint
assert "Steering_AngleSpeedObserve(&steer_speed_obs_r" in waypoint
assert "dist_l > 100U" in waypoint and "dist_r > 100U" in waypoint
assert "if ((mode <= 3U) && (kind >= 1U) && (kind <= 4U))" in waypoint
assert "M8S00_ArrivalBegin(" in position
assert "steer_voice_s154_start_l = v;" in position
assert "steer_voice_s154_start_r = v;" in position
assert "(steer_pos_command != STEER_POS_VOICE45_NC)" in position
assert "(steer_pos_command != 4U)" in position
print("PASS exact production wiring: Voice 4/5 only, after per-axis checks, before atomic NC, R1/R2 unchanged")

top=r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define STEER_POS_VOICE45_NC 5U
#define STEER_AXIS_MOVING 1U
#define STEER_AXIS_DONE 3U
#define STEER_POS_FINE_PHASE_IDLE 0U
#define STEER_LIMIT_L_MIN_MASK 4U
#define STEER_LIMIT_R_MIN_MASK 2U
#define STEER_LIMIT_L_MAX_MASK 8U
#define STEER_LIMIT_R_MAX_MASK 1U
typedef struct {
 uint32_t sample_ms;
 uint16_t progress,speed_pms;
 uint8_t valid,updated;
} SteeringAngleSpeedObs_t;
typedef struct {uint16_t escape_start_raw; uint8_t fine_phase;} Axis;
static Axis steer_left_axis={0},steer_right_axis={0};
static uint8_t voice_pos_owned=0,steer_pos_command=0;
static uint8_t steer_pos_left_state=0,steer_pos_right_state=0;
static uint8_t steer_voice_s154_enable=1,steer_angle_wp_enable=1;
static uint8_t steer_voice_s154_active=0,steer_voice_s154_last_mode=0;
static uint8_t steer_voice_s154_status=0;
static uint16_t steer_pos_left_pwm_min=175,steer_pos_right_pwm_min=50;
static uint16_t voice_pos_goal_l=0,voice_pos_goal_r=0;
static uint16_t debug_as5600_left_raw=0,debug_as5600_right_raw=0;
static uint32_t steer_pos_left_last_sample_ms=0,steer_pos_right_last_sample_ms=0;
static uint32_t steer_angle_wp_frames=0,steer_voice_s154_frames=0,steer_voice_s154_skipped_frames=0;
static uint16_t steer_angle_wp_progress_left=0,steer_angle_wp_progress_right=0;
static uint16_t steer_voice_s154_progress_l=0,steer_voice_s154_progress_r=0;
static uint16_t steer_voice_s154_gap_permille=0,steer_voice_s154_peak_gap_permille=0;
static uint16_t steer_voice_s154_pwm_l=0,steer_voice_s154_pwm_r=0;
static uint32_t steer_voice_s154_l_eta_ms=0,steer_voice_s154_r_eta_ms=0,steer_voice_s154_common_eta_ms=0;
static SteeringAngleSpeedObs_t steer_speed_obs_l={0},steer_speed_obs_r={0};
static uint16_t steer_speed_measured_l_pms=0,steer_speed_measured_r_pms=0;
static unsigned calls=0;
static uint16_t arg_start_l=0,arg_goal_l=0,arg_start_r=0,arg_goal_r=0;
static uint8_t arg_mode=0;
static uint16_t Steering_PositionClampDuty(uint16_t x){return x>400U?400U:x;}
static void Steering_AngleWaypointApply(uint8_t mode,
    uint16_t start_l,uint16_t goal_l,uint16_t raw_l,
    uint16_t start_r,uint16_t goal_r,uint16_t raw_r,
    uint8_t allow_l,uint8_t allow_r,uint16_t floor_l,uint16_t floor_r,
    uint16_t *duty_l,uint16_t *duty_r)
{
 (void)raw_l;(void)raw_r;
 assert(allow_l==1&&allow_r==1&&floor_l==175&&floor_r==50);
 arg_start_l=start_l;arg_goal_l=goal_l;
 arg_start_r=start_r;arg_goal_r=goal_r;arg_mode=mode;
 calls++;steer_angle_wp_frames++;
 steer_angle_wp_progress_left=400;
 steer_angle_wp_progress_right=360;
 steer_speed_obs_l.valid=1;steer_speed_obs_r.valid=1;
 steer_speed_measured_l_pms=200;steer_speed_measured_r_pms=180;
 if(*duty_l>175)*duty_l-=10;
 if(*duty_r>50)*duty_r-=5;
}
"""
bottom=r"""
int main(void) {
 uint16_t lp=220,rp=90;
 /* Actual C stroke progress (each travel distance is different). */
 uint8_t ok=0;
 assert(Steering_AngleStrokeProgress(0,4500,2250,&ok)==500&&ok);
 assert(Steering_AngleStrokeProgress(-4500,0,-2250,&ok)==500&&ok);
 assert(Steering_AngleStrokeProgress(2500,-2500,0,&ok)==500&&ok);
 assert(Steering_AngleStrokeProgress(0,300,150,&ok)==0&&!ok);
 assert(Steering_AngleStrokeProgress(0,4500,-1,&ok)==0&&ok);
 assert(Steering_AngleStrokeProgress(0,4500,5000,&ok)==1000&&ok);
 /* Actual C velocity observer uses per-axis sample timestamps. */
 SteeringAngleSpeedObs_t o={0};
 Steering_AngleSpeedObserve(&o,100,100);
 assert(!o.valid);
 Steering_AngleSpeedObserve(&o,110,120);
 assert(o.valid&&o.updated&&o.speed_pms==2000);
 Steering_AngleSpeedObserve(&o,110,130);
 assert(!o.updated);
 Steering_AngleSpeedObserve(&o,120,115);
 assert(!o.valid);
 Steering_AngleSpeedObserve(&o,130,125);
 assert(o.valid&&o.speed_pms==1000);
 /* Voice only. Non-Voice R1/R2 must not cross into S15.4. */
 steer_pos_left_state=steer_pos_right_state=STEER_AXIS_MOVING;
 steer_pos_left_last_sample_ms=steer_pos_right_last_sample_ms=100U;
 steer_left_axis.escape_start_raw=1886;
 steer_right_axis.escape_start_raw=662;
 voice_pos_goal_l=1374;voice_pos_goal_r=195;
 debug_as5600_left_raw=1800;debug_as5600_right_raw=600;
 steer_pos_command=5U;
 Steering_VoiceS154Sync(110,0,&lp,&rp);
 assert(calls==0);
 /* Real 45: expected left MAX/right MIN with independent stroke starts. */
 voice_pos_owned=1U;
 Steering_VoiceS154Sync(110,0,&lp,&rp);
 assert(calls==1&&arg_mode==5U);
 assert(arg_start_l==1886&&arg_goal_l==1374);
 assert(arg_start_r==662&&arg_goal_r==195);
 assert(lp==210&&rp==85);
 assert(steer_voice_s154_active==1&&steer_voice_s154_gap_permille==40);
 assert(steer_voice_s154_l_eta_ms==3000);
 assert(steer_voice_s154_r_eta_ms==3555);
 assert(steer_voice_s154_common_eta_ms==3555);
 assert(steer_voice_s154_frames==1);
 /* Wrong/end NC prevents any new proposed PWM. */
 Steering_VoiceS154Sync(111,STEER_LIMIT_L_MAX_MASK,&lp,&rp);
 assert(calls==1&&steer_voice_s154_status==5);
 steer_pos_left_state=STEER_AXIS_DONE;
 Steering_VoiceS154Sync(111,0,&lp,&rp);
 assert(calls==1&&steer_voice_s154_status==3);
 steer_pos_left_state=STEER_AXIS_MOVING;
 steer_right_axis.fine_phase=1;
 Steering_VoiceS154Sync(111,0,&lp,&rp);
 assert(calls==1&&steer_voice_s154_status==3);
 steer_right_axis.fine_phase=0;
 steer_pos_left_last_sample_ms=1;
 Steering_VoiceS154Sync(111,0,&lp,&rp);
 assert(calls==1&&steer_voice_s154_status==4);
 steer_pos_left_last_sample_ms=110;
 steer_voice_s154_enable=0;
 Steering_VoiceS154Sync(111,0,&lp,&rp);
 assert(calls==1&&steer_voice_s154_status==2);
 /* 5..40 also uses its own saved normalized targets. */
 steer_voice_s154_enable=1;steer_pos_command=4;
 steer_left_axis.escape_start_raw=1454;
 steer_right_axis.escape_start_raw=245;
 voice_pos_goal_l=1715;voice_pos_goal_r=506;
 Steering_VoiceS154Sync(111,0,&lp,&rp);
 assert(calls==2&&arg_mode==4);
 assert(arg_start_l==1454&&arg_goal_l==1715);
 assert(arg_start_r==245&&arg_goal_r==506);
 /* Sensor freshness and explicit 0 PWM prevent re-energization. */
 lp=0;Steering_VoiceS154Sync(111,0,&lp,&rp);
 assert(calls==2&&lp==0);
 printf("PASS actual C normalized progress/velocity and bounded Voice S15.4 routing\n");
 return 0;
}
"""
cc=shutil.which("gcc") or shutil.which("cc") or shutil.which("clang")
if not cc:raise SystemExit("HOST C compiler not available")
with TemporaryDirectory() as d:
    test=Path(d)/"s154.c"
    test.write_text(top+"\n"+progress+"\n"+speed+"\n"+sync+"\n"+bottom,encoding="utf-8")
    exe=Path(d)/"s154"
    b=subprocess.run([cc,"-std=c11","-O2","-Wall","-Wextra","-Werror",
                     str(test),"-o",str(exe)],text=True,capture_output=True)
    if b.returncode:raise SystemExit("HOST compile FAIL:\n"+b.stdout+b.stderr)
    r=subprocess.run([str(exe)],text=True,capture_output=True)
    if r.returncode:raise SystemExit("HOST run FAIL:\n"+r.stdout+r.stderr)
    print(r.stdout.strip())
print("PASS real firmware call-graph guards: no NC bypass, no new motor permissions, no autonomous PWM.")
