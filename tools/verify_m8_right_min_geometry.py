#!/usr/bin/env python3
"""Verify measured RIGHT MIN boot defaults and independent SOFT/NC guards.

Static HOST gate only: no STM32 CubeIDE build, flash or physical acceptance.
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
source = (root / "Core" / "Src" / "main.c").read_text(encoding="utf-8")

def one(pattern, name):
    matches = re.findall(pattern, source, flags=re.MULTILINE)
    assert len(matches) == 1, f"{name}: expected one declaration, got {len(matches)}"
    return int(matches[0])

rmin = one(r"^volatile uint16_t steer_cal_right_min_runtime = (\d+)U;", "RIGHT R_MIN")
r2 = one(r"^volatile uint16_t steer_pos_right_r2_target_raw = (\d+)U;", "RIGHT R2")
margin = one(r"^volatile uint16_t steer_manual_right_soft_margin_raw = (\d+)U;", "RIGHT MIN SOFT margin")
center = int(re.search(r"^#define STEER_POS_RIGHT_CENTER_RAW\s+(\d+)U", source, re.MULTILINE).group(1))
rmax = one(r"^volatile uint16_t steer_cal_right_max_runtime = (\d+)U;", "RIGHT R_MAX")
assert (rmin, r2, margin, center, rmax) == (195, 195, 50, 662, 1029), (
    f"Unexpected RIGHT defaults: {rmin=} {r2=} {margin=} {center=} {rmax=}")
soft = rmin + max(30, min(margin, 120))
assert soft == 245
assert all(soft - nc >= 48 for nc in (195, 196, 197))
required = (
    "#define STEER_POS_RIGHT_MIN_RAW      (steer_cal_right_min_runtime)",
    "#define STEER_MANUAL_RIGHT_SOFT_MIN (STEER_POS_RIGHT_MIN_RAW + STEER_MANUAL_RIGHT_SOFT_MARGIN_RAW)",
    "if ((left == 0U) && (direction < 0) && (raw <= goal))",
    "(Steering_TestRawLimitMask()==0U)",
    "steer_manual_right_rx_min_pwm = 50U;",
    "steer_s153_right_only_enable=1U;",
    "steer_cal_right_min_runtime = steer_cal_right_axis.min_raw;",
    "Steering_PositionNcDirectionAllowed(",
)
for marker in required:
    assert marker in source, f"Missing guard/config: {marker}"
assert "steer_right_min_tail60" not in source
assert "M8S15_TAIL60" not in source
print("PASS: RIGHT R_MIN=R2 boot195, SOFT245, NC range195..197, right floor50, STOP/NC source gates retained")
print("HOST STATIC only; verify MCU build/flash and live CAL_RUNTIME/STEER.")
