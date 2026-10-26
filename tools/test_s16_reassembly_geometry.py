#!/usr/bin/env python3
"""Verify post-reassembly S16 AS5600 geometry and strict hard-stop envelope.

Usage: python tools/test_s16_reassembly_geometry.py
Checks the six firmware defaults and, if a host cc/gcc/clang is available,
compiles/runs the EXACT C geometry + hard-envelope helpers extracted from
Core/Src/main.c. No motor PWM, no hardware or target build.
"""
from pathlib import Path
import re
import shutil
import subprocess
from tempfile import TemporaryDirectory

src = (Path(__file__).resolve().parents[1] / "Core/Src/main.c").read_text(
    encoding="utf-8"
)

expected = {
    "steer_cal_left_min_runtime": 2297,
    "STEER_POS_LEFT_CENTER_RAW": 1873,
    "steer_cal_left_max_runtime": 1374,
    "steer_cal_right_min_runtime": 191,
    "STEER_POS_RIGHT_CENTER_RAW": 662,
    "steer_cal_right_max_runtime": 982,
}
for name, number in expected.items():
    if name.startswith("steer_cal_"):
        pattern = rf"\b{re.escape(name)}\s*=\s*{number}U\s*;"
    else:
        pattern = rf"#define\s+{re.escape(name)}\s+{number}U\b"
    assert re.search(pattern, src), f"wrong or missing {name}={number}"

lmin, lcenter, lmax = 2297, 1873, 1374
rmin, rcenter, rmax = 191, 662, 982
assert lmax < lcenter < lmin and rmin < rcenter < rmax
assert lmin - lmax == 923
assert rmax - rmin == 791
assert lmax + 100 < lcenter < lmin - 100
assert rmin + 40 < rcenter < rmax - 40
assert "static uint8_t Steering_PositionGeometryValid(" in src
assert "static uint8_t Steering_SafetyHardEnvelopeValid(" in src
assert "return Steering_PositionGeometryValid(left, axis->min_raw," in src
assert "Steering_SafetyHardEnvelopeValid(1U, lraw) == 0U" in src
assert "Steering_SafetyHardEnvelopeValid(0U, rraw) == 0U" in src
assert "STEER_MANUAL_LEFT_SOFT_MARGIN_RAW" in src
assert "STEER_MANUAL_RIGHT_SOFT_MARGIN_RAW" in src
assert "volatile uint8_t steer_safety_motion_permit = 0U;" in src
assert "volatile uint8_t steer_manual_enable = 0U;" in src
assert "Steering_SyncApply(HAL_GetTick(), la, ra, &la);" in src
print("PASS: six AS5600 defaults, physical ordering/spans, soft-limit "
      "clearance, retained motion permit and speed sync")

start_g = src.index("static uint8_t Steering_PositionGeometryValid(")
end_g = src.index("#define STEER_POS_TOL_RAW", start_g)
geometry_c = src[start_g:end_g].strip()
start_h = src.index("static uint8_t Steering_SafetyHardEnvelopeValid(")
end_h = src.index("static uint8_t Steering_PositionWheelsStopped(", start_h)
hard_c = src[start_h:end_h].strip()
cc = shutil.which("cc") or shutil.which("gcc") or shutil.which("clang")
if cc is None:
    print("SKIP: host C compiler not found; actual C helper tests not run")
    raise SystemExit(0)

stub = """
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define STEER_POS_LEFT_CENTER_RAW 1873U
#define STEER_POS_RIGHT_CENTER_RAW 662U
#define STEER_SAFETY_HARD_RAW_OVERSHOOT 8U
static uint16_t steer_cal_left_min_runtime = 2297U;
static uint16_t steer_cal_left_max_runtime = 1374U;
static uint16_t steer_cal_right_min_runtime = 191U;
static uint16_t steer_cal_right_max_runtime = 982U;
#define STEER_POS_LEFT_MIN_RAW (steer_cal_left_min_runtime)
#define STEER_POS_LEFT_MAX_RAW (steer_cal_left_max_runtime)
#define STEER_POS_RIGHT_MIN_RAW (steer_cal_right_min_runtime)
#define STEER_POS_RIGHT_MAX_RAW (steer_cal_right_max_runtime)
static int16_t AS5600_ShortestDeltaCounts(uint16_t a, uint16_t b)
{
  int32_t d = (int32_t)b - (int32_t)a;
  if (d > 2047) d -= 4096;
  if (d < -2048) d += 4096;
  return (int16_t)d;
}
"""
tests = """
int main(void)
{
  assert(Steering_PositionGeometryValid(1U, 2297U, 1374U) == 1U);
  assert(Steering_PositionGeometryValid(0U, 191U, 982U) == 1U);
  assert(Steering_PositionGeometryValid(0U, 2934U, 3680U) == 0U);
  assert(Steering_PositionGeometryValid(1U, 1374U, 2297U) == 0U);
  assert(Steering_PositionGeometryValid(0U, 982U, 191U) == 0U);
  assert(Steering_PositionGeometryValid(0U, 4090U, 982U) == 0U);
  assert(Steering_PositionGeometryValid(1U, 2297U, 4090U) == 0U);
  assert(Steering_PositionGeometryValid(0U, 191U, 4000U) == 0U);
  assert(Steering_SafetyHardEnvelopeValid(1U, 1873U) == 1U);
  assert(Steering_SafetyHardEnvelopeValid(0U, 662U) == 1U);
  assert(Steering_SafetyHardEnvelopeValid(1U, 2297U) == 1U);
  assert(Steering_SafetyHardEnvelopeValid(0U, 191U) == 1U);
  assert(Steering_SafetyHardEnvelopeValid(1U, 2305U) == 1U);
  assert(Steering_SafetyHardEnvelopeValid(1U, 2306U) == 0U);
  assert(Steering_SafetyHardEnvelopeValid(1U, 1366U) == 1U);
  assert(Steering_SafetyHardEnvelopeValid(1U, 1365U) == 0U);
  assert(Steering_SafetyHardEnvelopeValid(0U, 183U) == 1U);
  assert(Steering_SafetyHardEnvelopeValid(0U, 182U) == 0U);
  assert(Steering_SafetyHardEnvelopeValid(0U, 990U) == 1U);
  assert(Steering_SafetyHardEnvelopeValid(0U, 991U) == 0U);
  assert(Steering_SafetyHardEnvelopeValid(0U, 3398U) == 0U);
  steer_cal_right_min_runtime = 2934U;
  steer_cal_right_max_runtime = 3680U;
  assert(Steering_SafetyHardEnvelopeValid(0U, 662U) == 0U);
  puts("PASS: actual extracted C geometry & safety envelope: 22 cases");
  return 0;
}
"""
with TemporaryDirectory() as temp:
    path = Path(temp) / "test_geometry.c"
    out = Path(temp) / "test_geometry"
    path.write_text(stub + "\n" + geometry_c + "\n" + hard_c
                    + "\n" + tests, encoding="utf-8")
    subprocess.run([cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                    "-o", str(out), str(path)], check=True)
    subprocess.run([str(out)], check=True)
