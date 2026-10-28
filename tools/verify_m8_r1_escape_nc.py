#!/usr/bin/env python3
"""M8 R1/R2 origin-NC escape regression: source and optional host-C checks.

Run: python tools/verify_m8_r1_escape_nc.py
Not an MCU cross-build or proof that mechanical/electrical NC is healthy.
"""
from pathlib import Path
import re
import shutil
import subprocess
from tempfile import TemporaryDirectory

s = (Path(__file__).resolve().parents[1] / "Core/Src/main.c").read_text(encoding="utf-8")

def body(name):
    pat = r"static\s+(?:uint8_t|void)\s+" + re.escape(name) + r"\s*\([^;]*?\)\s*\{"
    m = re.search(pat, s, flags=re.S)
    assert m, f"Function definition missing: {name}"
    a = m.end()-1
    depth = 0
    for i in range(a, len(s)):
        if s[i] == "{":
            depth += 1
        elif s[i] == "}":
            depth -= 1
            if depth == 0:
                return s[m.start():i+1]
    raise AssertionError(f"Unbalanced function: {name}")

h = body("Steering_PositionOwnEscapeIrq")
update = body("Steering_PositionUpdate")
release = body("Steering_PositionEscapeReleased")
isr = body("Steering_PositionOnLimitInterrupt")
dump = body("M8S00_DumpUpdate")
assert h.count("state != STEER_AXIS_ESCAPING") == 1
assert "own_irq != axis->escape_origin" in h
assert "axis->escape_origin == 0U" in h
assert "axis->escape_release_pending = 0U;" in h
assert "Steering_PositionLeftOff();" in h and "Steering_PositionRightOff();" in h
assert update.count("Steering_PositionOwnEscapeIrq(1U,") == 2
assert update.count("Steering_PositionOwnEscapeIrq(0U,") == 2
assert "if (left_escape_irq_hold != 0U) left_cmd = 0U;" in update
assert "if (right_escape_irq_hold != 0U) right_cmd = 0U;" in update
assert "Steering_TestRawLimitMask();" in update
assert "if (irq_state == 0U) __enable_irq();" in update
assert "Steering_PositionAxisFault(1U, 9U);" in update
assert "Steering_PositionAxisFault(0U, 9U);" in update
assert "STEER_POS_ESCAPE_RELEASE_QUIET_MS" in release
assert "progress_ok == 0U" not in release  # fast movement cannot skip release quiet
assert "Steering_PositionAxisFault(left, 8U);" in release
assert "steer_pos_irq_pending_mask |= limit_bit;" in isr
assert "Steering_PositionLeftOff();" in isr and "Steering_PositionRightOff();" in isr
assert "M8S00_DumpEscapeCounters() == 0U" in dump
assert "steer_safety_motion_permit != 0U" in dump
assert "Steering_PositionWheelsStopped() == 0U" in dump
assert "M8S00_ESCAPE,scope=since_boot" in s
print("PASS: source gates, NC IRQ PWM cutoff, 120ms release, both axes, UART safety")

cc = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
if cc is None:
    print("SKIP: no host compiler, dynamic own-NC helper scenarios not executed")
    raise SystemExit(0)

stub = r"""
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define STEER_LIMIT_L_MIN_MASK 4U
#define STEER_LIMIT_L_MAX_MASK 8U
#define STEER_LIMIT_R_MIN_MASK 2U
#define STEER_LIMIT_R_MAX_MASK 1U
#define STEER_AXIS_ESCAPING 2U
#define STEER_AXIS_MOVING 1U
typedef struct { uint8_t escape_origin; uint8_t escape_release_pending; } SteeringPositionAxis_t;
static uint8_t steer_pos_active = 1U;
static uint8_t steer_pos_left_state = 2U;
static uint8_t steer_pos_right_state = 2U;
static SteeringPositionAxis_t steer_left_axis = {4U, 1U};
static SteeringPositionAxis_t steer_right_axis = {2U, 1U};
static uint32_t steer_pos_escape_origin_irq_left_count = 0U;
static uint32_t steer_pos_escape_origin_irq_right_count = 0U;
static uint32_t stopL = 0U, stopR = 0U;
static void Steering_PositionLeftOff(void) {stopL++;}
static void Steering_PositionRightOff(void) {stopR++;}
"""
tests = r"""
int main(void) {
  assert(Steering_PositionOwnEscapeIrq(1U, 4U) == 1U);
  assert(stopL == 1U && stopR == 0U);
  assert(steer_left_axis.escape_release_pending == 0U);
  assert(steer_pos_escape_origin_irq_left_count == 1U);
  assert(Steering_PositionOwnEscapeIrq(0U, 2U) == 1U);
  assert(stopR == 1U && steer_pos_escape_origin_irq_right_count == 1U);
  assert(Steering_PositionOwnEscapeIrq(1U, 8U) == 0U);
  assert(Steering_PositionOwnEscapeIrq(1U, 12U) == 0U);
  assert(Steering_PositionOwnEscapeIrq(0U, 1U) == 0U);
  assert(Steering_PositionOwnEscapeIrq(0U, 3U) == 0U);
  steer_pos_left_state = STEER_AXIS_MOVING;
  assert(Steering_PositionOwnEscapeIrq(1U, 4U) == 0U);
  steer_pos_left_state = STEER_AXIS_ESCAPING;
  steer_left_axis.escape_origin = 0U;
  assert(Steering_PositionOwnEscapeIrq(1U, 4U) == 0U);
  steer_left_axis.escape_origin = 4U;
  steer_pos_active = 0U;
  assert(Steering_PositionOwnEscapeIrq(1U, 4U) == 0U);
  assert(steer_pos_escape_origin_irq_left_count == 1U);
  assert(steer_pos_escape_origin_irq_right_count == 1U);
  puts("PASS: actual C helper: own-origin NC allowed only in ESCAPING on both axes");
  return 0;
}
"""
with TemporaryDirectory() as t:
    src = Path(t) / "escape.c"
    exe = Path(t) / "escape"
    src.write_text(stub + "\n" + h + "\n" + tests, encoding="utf-8")
    subprocess.run([cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                    str(src), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("No STM32 cross-build, physical NC edge, ground-load, or motor motion was tested.")
