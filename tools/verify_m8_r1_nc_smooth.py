#!/usr/bin/env python3
"""M8 R1 directional NC and no-kick/no-pause static plus host-C regression.

NOT a CubeIDE build, physical smoothness measurement or safety-rated NC test.
Run: python tools/verify_m8_r1_nc_smooth.py
"""
from pathlib import Path
import re
import shutil
import subprocess
from tempfile import TemporaryDirectory

src = (Path(__file__).resolve().parents[1] / "Core/Src/main.c").read_text(encoding="utf-8")


def body(name):
    pattern = r"static\s+(?:uint8_t|void)\s+" + re.escape(name) + r"\s*\([^;]*?\)\s*\{"
    m = re.search(pattern, src, re.S)
    assert m, "Missing definition: " + name
    i = m.end() - 1
    nesting = 0
    for j in range(i, len(src)):
        if src[j] == "{":
            nesting += 1
        elif src[j] == "}":
            nesting -= 1
            if nesting == 0:
                return src[m.start():j + 1]
    raise AssertionError("Unclosed function " + name)


gate = body("Steering_PositionNcDirectionAllowed")
axis = body("Steering_PositionAxisUpdate")
owner = body("Steering_PositionUpdate")
irq = body("Steering_PositionOnLimitInterrupt")
release = body("Steering_PositionEscapeReleased")
tests = []


def check(label, condition):
    condition = bool(condition)
    tests.append(condition)
    print(("[PASS] " if condition else "[FAIL] ") + label)


check("one-way own NC helper for both axes",
      all(v in gate for v in ("STEER_LIMIT_L_MIN_MASK", "STEER_LIMIT_R_MIN_MASK",
                               "STEER_LIMIT_L_MAX_MASK", "STEER_LIMIT_R_MAX_MASK",
                               "dir == GPIO_PIN_SET", "dir == GPIO_PIN_RESET")))
check("R1 remains ESCAPING until 120ms release classification completes",
      "STEER_POS_ESCAPE_RELEASE_QUIET_MS" in release
      and "steer_pos_left_state = STEER_AXIS_MOVING" in release
      and "steer_pos_right_state = STEER_AXIS_MOVING" in release)
check("no forced zero PWM when release is first observed",
      "Steering_PositionEscapeReleased(left, now);" in axis
      and "Steering_PositionEscapeReleased(left, now);\n      return;" not in axis
      and "left_cmd = 0U;\n      Steering_PositionEscapeReleased(1U, now);" not in owner
      and "right_cmd = 0U;\n      Steering_PositionEscapeReleased(0U, now);" not in owner)
check("ESCAPING uses normal bounded servo instead of separate MAX PWM kick",
      "Steering_PositionDuty(err, steer_pos_left_pwm_min," in axis
      and "Steering_PositionDuty(err, steer_pos_right_pwm_min," in axis
      and "*duty = left ? Steering_PositionClampDuty(steer_pos_left_pwm_max)" not in axis)
check("ESCAPING direction cannot go INTO own NC",
      "Steering_PositionNcDirectionAllowed(left, limit_mask, *direction)" in axis
      and "Steering_PositionAxisFault(left, 9U)" in axis)
check("atomic CCR guard independently checks both motor directions",
      "Steering_PositionNcDirectionAllowed(1U, raw_mask, left_dir)" in owner
      and "Steering_PositionNcDirectionAllowed(0U, raw_mask, right_dir)" in owner)
check("late own NC IRQ still kills one complete control frame",
      "if (left_escape_irq_hold != 0U) left_cmd = 0U;" in owner
      and "if (right_escape_irq_hold != 0U) right_cmd = 0U;" in owner
      and "Steering_PositionOwnEscapeIrq(1U, late_pending)" in owner
      and "Steering_PositionOwnEscapeIrq(0U, late_pending)" in owner)
check("physical R2 MIN still sole success condition",
      "Steering_PositionAxisDone(left, 3U)" in axis
      and "Steering_PositionAxisDone(left, 2U)" not in axis)
check("all safety/timeout gates retained",
      all(term in src for term in ("Steering_SafetyEnvelopeWithNcDiscovery(",
                                  "STEER_POS_STALL_MS", "STEER_POS_ESCAPE_MS",
                                  "STEER_POS_R1_TIMEOUT_MS",
                                  "STEER_SAFETY_HARD_RAW_OVERSHOOT 8U",
                                  "steer_safety_motion_permit != 1U")))
check("physical NC EXTI still zeros its owning motor PWM",
      "Steering_PositionLeftOff();" in irq and "Steering_PositionRightOff();" in irq)

cc = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
if cc:
    harness = r"""
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef enum { GPIO_PIN_RESET=0, GPIO_PIN_SET=1 } GPIO_PinState;
#define STEER_LIMIT_R_MAX_MASK 1U
#define STEER_LIMIT_R_MIN_MASK 2U
#define STEER_LIMIT_L_MIN_MASK 4U
#define STEER_LIMIT_L_MAX_MASK 8U
int main(void) {
  for (int left=0; left<=1; left++) {
    uint8_t mi = left ? 4U : 2U;
    uint8_t ma = left ? 8U : 1U;
    uint8_t other = left ? 2U : 4U;
    assert(Steering_PositionNcDirectionAllowed(left, 0U, GPIO_PIN_RESET));
    assert(Steering_PositionNcDirectionAllowed(left, 0U, GPIO_PIN_SET));
    assert(Steering_PositionNcDirectionAllowed(left, mi, GPIO_PIN_SET));
    assert(!Steering_PositionNcDirectionAllowed(left, mi, GPIO_PIN_RESET));
    assert(Steering_PositionNcDirectionAllowed(left, ma, GPIO_PIN_RESET));
    assert(!Steering_PositionNcDirectionAllowed(left, ma, GPIO_PIN_SET));
    assert(!Steering_PositionNcDirectionAllowed(left, mi|ma, GPIO_PIN_SET));
    assert(!Steering_PositionNcDirectionAllowed(left, mi|ma, GPIO_PIN_RESET));
    assert(Steering_PositionNcDirectionAllowed(left, other, GPIO_PIN_SET));
  }
  puts("PASS: C helper blocks into MIN/MAX and permits away on both axes");
  return 0;
}
"""
    with TemporaryDirectory() as td:
        c = Path(td) / "direction.c"
        exe = Path(td) / "direction"
        c.write_text(harness.replace("int main(void) {", gate + "\nint main(void) {"),
                     encoding="utf-8")
        subprocess.run([cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                        str(c), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
    check("host C directional scenarios", True)
else:
    print("[SKIP] No native host C compiler for dynamic helper scenarios")

print(f"\nM8 R1 NC smooth source checks: {sum(tests)}/{len(tests)} PASS")
print("No STM32 target Build, NC electrical test, or physical jerk measurement.")
raise SystemExit(0 if all(tests) else 1)
