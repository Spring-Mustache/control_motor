#!/usr/bin/env python3
"""Host test the EXACT Steering_SyncApply C function extracted from main.c.

Usage: python tools/test_steering_speed_sync.py
Needs a host C99 compiler (cc/gcc/clang). Does not test HAL, interrupts,
AS5600 I2C communication, motor driver, safety circuit or STM32 flash.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
source = (root / "Core" / "Src" / "main.c").read_text(encoding="utf-8")
start = source.index("static void Steering_SyncApply(uint32_t now")
end = source.index("static void Steering_ManualUpdate(void)", start)
actual_function = source[start:end]
cc = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
if not cc:
    raise SystemExit("FAIL: host C compiler missing: install cc/gcc/clang")

stub = r"""
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define STEER_MANUAL_MOVING 1U
#define STEER_MANUAL_DONE 3U
#define STEER_MANUAL_PWM_HARD_MAX 400U
#define STEER_MANUAL_APPROACH_WINDOW_RAW 100U
#define STEER_SYNC_MAX_SAMPLE_AGE_MS 110U
#define STEER_SYNC_MIN_REF_SPEED_RAW_S 8U
#define STEER_SYNC_MAX_INTEGRAL 6000
#define STEER_POS_LEFT_MIN_RAW 2297U
#define STEER_POS_LEFT_MAX_RAW 1374U
#define STEER_POS_RIGHT_MIN_RAW 166U
#define STEER_POS_RIGHT_MAX_RAW 982U
typedef struct { uint32_t speed_sample_ms; uint8_t speed_valid; } Axis;
static Axis steer_manual_left_axis, steer_manual_right_axis;
static uint8_t steer_manual_left_state, steer_manual_right_state;
static uint8_t steer_manual_right_result;
#define STEER_MANUAL_RESULT_SOFT 11U
static uint8_t steer_sync_enable, steer_sync_active;
static uint16_t steer_sync_right_pwm_reference, steer_sync_left_pwm_command;
static uint16_t steer_sync_left_speed_raw_s, steer_sync_right_speed_raw_s;
static uint16_t steer_sync_kp_q4, steer_sync_ki_div;
static uint16_t steer_sync_left_pwm_ceiling, steer_manual_left_dpad_pwm;
static uint16_t steer_manual_left_rx_min_pwm, steer_sync_max_trim_pwm;
static uint16_t steer_sync_pwm_step, steer_sync_previous_left_pwm;
static int16_t steer_manual_left_soft_remaining;
static int32_t steer_sync_integral, steer_sync_error_raw_s;
static uint16_t steer_sync_left_target_raw_s;
static uint32_t steer_sync_reference_unavailable_count;
static uint32_t steer_sync_reference_hold_count, steer_sync_limit_count;
static uint32_t steer_sync_last_update_ms, steer_sync_update_count;

static void init(void) {
  steer_manual_left_axis.speed_sample_ms = 100;
  steer_manual_right_axis.speed_sample_ms = 100;
  steer_manual_left_axis.speed_valid = 1;
  steer_manual_right_axis.speed_valid = 1;
  steer_manual_left_state = STEER_MANUAL_MOVING;
  steer_manual_right_state = STEER_MANUAL_MOVING;
  steer_sync_enable = 1;
  steer_sync_active = 0;
  steer_sync_integral = 0;
  steer_sync_previous_left_pwm = 0;
  steer_sync_last_update_ms = 0;
  steer_sync_left_speed_raw_s = 100;
  steer_sync_right_speed_raw_s = 200;
  steer_sync_left_target_raw_s = 0;
  steer_sync_kp_q4 = 2;
  steer_sync_ki_div = 40;
  steer_sync_left_pwm_ceiling = 300;
  steer_manual_left_dpad_pwm = 300;
  steer_manual_left_rx_min_pwm = 175;
  steer_sync_max_trim_pwm = 150;
  steer_sync_pwm_step = 24;
  steer_manual_left_soft_remaining = 200;
}
"""
tests = r"""
int main(void) {
  uint16_t left;
  init();
  left = 230;
  Steering_SyncApply(110, 230, 100, &left);
  assert(left > 230 && left <= 300);
  assert(steer_sync_right_pwm_reference == 100);
  assert(steer_sync_active == 1U);
  assert(steer_sync_error_raw_s > 0);

  init();
  steer_sync_left_speed_raw_s = 300; /* LEFT faster than span-normalized RIGHT target */
  left = 230;
  Steering_SyncApply(110, 230, 100, &left);
  assert(left < 230 && left <= 300);
  assert(steer_sync_error_raw_s < 0);

  /* Equal normalized travel rate: 816 RIGHT RAW/s means 923 LEFT RAW/s. */
  init();
  steer_sync_right_speed_raw_s = 816;
  steer_sync_left_speed_raw_s = 923;
  left = 230;
  Steering_SyncApply(110, 230, 100, &left);
  assert(steer_sync_left_target_raw_s == 923);
  assert(steer_sync_error_raw_s == 0);

  /* Equal raw/s is NOT synchronized travel across unequal spans. */
  init();
  steer_sync_right_speed_raw_s = 816;
  steer_sync_left_speed_raw_s = 816;
  left = 230;
  Steering_SyncApply(110, 230, 100, &left);
  assert(steer_sync_error_raw_s == 107);
  assert(left > 230);

  init();
  steer_sync_right_speed_raw_s = 0;
  left = 230;
  Steering_SyncApply(110, 230, 100, &left);
  assert(left == 0 && steer_sync_reference_hold_count != 0);

  init();
  steer_manual_right_axis.speed_valid = 0;
  left = 230;
  Steering_SyncApply(110, 230, 100, &left);
  assert(left == 0);

  init();
  steer_manual_right_axis.speed_sample_ms = 1;
  left = 230;
  Steering_SyncApply(200, 230, 100, &left);
  assert(left == 0); /* old measurement cannot enable follower */

  /* Normal RIGHT mechanical/soft stop must not strand LEFT. */
  init();
  steer_manual_right_state = STEER_MANUAL_DONE;
  steer_manual_right_result = 3U;
  left = 230;
  Steering_SyncApply(110, 230, 0, &left);
  assert(left > 0 && left <= 24);
  assert(steer_sync_active == 0U);

  init();
  steer_manual_right_state = STEER_MANUAL_DONE;
  steer_manual_right_result = STEER_MANUAL_RESULT_SOFT;
  steer_sync_previous_left_pwm = 180;
  left = 230;
  Steering_SyncApply(110, 230, 0, &left);
  assert(left > 180 && left <= 204);

  /* A fault/unclassified STOP of RIGHT still blocks the LEFT drive. */
  init();
  steer_manual_right_state = STEER_MANUAL_DONE;
  steer_manual_right_result = 0U;
  left = 230;
  Steering_SyncApply(110, 230, 0, &left);
  assert(left == 0);

  init();
  left = 230;
  Steering_SyncApply(110, 230, 0, &left);
  assert(left == 0); /* RIGHT zero duty even if its state says MOVING */

  init();
  steer_manual_left_soft_remaining = 0; /* right at LEFT soft stop */
  left = 230;
  Steering_SyncApply(110, 230, 100, &left);
  assert(left <= 175);

  init();
  steer_sync_left_speed_raw_s = 10;
  steer_sync_right_speed_raw_s = 10000;
  left = 230;
  Steering_SyncApply(110, 230, 100, &left);
  assert(left <= 300);
  assert(steer_sync_limit_count > 0);

  puts("PASS: actual Steering_SyncApply C logic, 14 host scenarios");
  return 0;
}
"""
with TemporaryDirectory() as temp:
    path = Path(temp) / "speed_sync.c"
    exe = Path(temp) / "speed_sync"
    path.write_text(stub + actual_function + tests, encoding="utf-8")
    subprocess.run([cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                    "-o", str(exe), str(path)], check=True)
    subprocess.run([str(exe)], check=True)
