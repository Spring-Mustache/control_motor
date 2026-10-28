#!/usr/bin/env python3
"""Verify SELECT uses physical NC events to learn four AS5600 endpoints.

Source-only checks: do NOT certify mechanics, NC wiring, cross-build, or
endstop travel safety. This does not bypass the bounded old-RAW guard.
Run from anywhere: python tools/verify_m8_select_nc_learning.py
"""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
src = (root / "Core" / "Src" / "main.c").read_text(encoding="utf-8")
tests = []

def check(label, passed):
    tests.append((label, bool(passed)))
    print(("[PASS] " if passed else "[FAIL] ") + label)

def has(pattern):
    return re.search(pattern, src) is not None

# SEEK_MIN/MAX capture live AS5600 only when their OWN physical NC is
# asserted; an old RAW estimate is merely used to taper the approach PWM.
check("own MIN NC records actual RAW and transitions to EXIT_MIN",
      has(r"if\s*\(stage == STEER_CAL_SEEK_MIN\)[\s\S]{0,170}?"
          r"if\s*\(limit == min_bit\)\s*\{\s*axis->min_raw = raw;") and
      has(r"Steering_CalAxisStage\(axis,\s*STEER_CAL_EXIT_MIN,\s*raw,\s*now\)"))
check("own MAX NC records actual RAW and transitions to EXIT_MAX",
      has(r"if\s*\(stage == STEER_CAL_SEEK_MAX\)[\s\S]{0,120}?"
          r"if\s*\(limit == max_bit\)\s*\{\s*axis->max_raw = raw;") and
      has(r"Steering_CalAxisStage\(axis,\s*STEER_CAL_EXIT_MAX,\s*raw,\s*now\)"))
check("explicit separate left/right NC contact readings",
      all(x in src for x in ("steer_cal_left_min_found = raw;",
                              "steer_cal_right_min_found = raw;",
                              "steer_cal_left_max_found = raw;",
                              "steer_cal_right_max_found = raw;")))
check("return to CENTER and atomically accept only 4 valid endpoints",
      all(x in src for x in ("Steering_CalAxisStage(axis, STEER_CAL_TO_CENTER,",
                              "(steer_cal_left_axis.stage == STEER_CAL_DONE)",
                              "(steer_cal_right_axis.stage == STEER_CAL_DONE)",
                              "Steering_CalAxisValidSpan(1U, &steer_cal_left_axis)",
                              "Steering_CalAxisValidSpan(0U, &steer_cal_right_axis)",
                              "steer_cal_left_min_runtime = steer_cal_left_axis.min_raw;",
                              "steer_cal_left_max_runtime = steer_cal_left_axis.max_raw;",
                              "steer_cal_right_min_runtime = steer_cal_right_axis.min_raw;",
                              "steer_cal_right_max_runtime = steer_cal_right_axis.max_raw;")))
check("SELECT still validates live NC and cuts PWM from EXTI",
      all(x in src for x in ("static void Steering_CalOnLimitInterrupt(uint8_t bit)",
                              "steer_cal_irq_pending_mask |= bit;",
                              "Steering_CalLeftOff();", "Steering_CalRightOff();",
                              "Steering_CalCommitAxisSafe(", "Steering_TestRawLimitMask()")))
check("SELECT remains bounded if NC fails",
      all(x in src for x in ("#define STEER_CAL_PROBE_MAX_EXTENSION_RAW 64U",
                              "Steering_SafetyEnvelopeWithNcDiscovery(",
                              "STEER_CAL_JUMP_MAX_RAW",
                              "Steering_CalStageTimeout(",
                              "Steering_CalMotionFault(",
                              "Steering_PositionGeometryValid(")))
check("NC scan does not silently rewrite independent R2 parking targets",
      "steer_pos_left_r2_target_raw = 2240U;" in src and
      "steer_pos_right_r2_target_raw = 223U;" in src and
      "steer_cal_left_min_runtime = 2297U;" in src and
      "steer_cal_right_min_runtime = 166U;" in src)
check("safe diagnostic export reports physical versus committed endpoints",
      all(x in src for x in ("M8S00_CAL_CONTACT,minL=%u,maxL=%u,minR=%u,maxR=%u",
                              "M8S00_CAL_RUNTIME,minL=%u,maxL=%u,minR=%u,maxR=%u",
                              "steer_cal_start_count",
                              "steer_cal_completed_count",
                              "steer_cal_last_run_result")))
check("SELECT-only USER capture available only after prior safety guard",
      all(x in src for x in ("(M8S00_CalHasSession() != 0U)",
                              "(M8S00_CalHasSession() == 0U)",
                              "(steer_safety_motion_permit != 0U)",
                              "(steer_cal_active != 0U)",
                              "(Steering_PositionWheelsStopped() == 0U)",
                              "(M8S00_DumpCalibration() == 0U)")))
check("SELECT code9 captures exact reason before stopping motors",
      "if ((steer_cal_active != 0U) && (result == 9U))" in src and
      "Steering_CalCaptureAbort();" in src and
      "steer_cal_abort_source = 1U;" in src and
      "steer_cal_abort_source = 2U;" in src)
check("abort identifies passive RPM separately from wheel drive",
      all(x in src for x in (
          "if (Steering_PositionWheelsStopped() == 0U) mask |= 1UL << 8;",
          "mask |= 1UL << 9;",
          "mask |= 1UL << 10;",
          "steer_cal_abort_rpm10_lu =",
          "steer_cal_abort_rpm10_ld =",
          "steer_cal_abort_rpm10_ru =",
          "steer_cal_abort_rpm10_rd =")))
check("cancellation evidence exported only by safely disarmed UART",
      "M8S00_CAL_ABORT,count=%lu,source=%u,mask=0x%08lx," in src and
      "steer_cal_abort_age_ms" in src and
      "(steer_safety_motion_permit != 0U)" in src and
      "(steer_cal_active != 0U)" in src and
      "(Steering_PositionWheelsStopped() == 0U)" in src)
check("original wheel gate and NC probe limits retained",
      "Steering_PositionWheelsStopped() == 0U))" in src and
      "#define STEER_CAL_PROBE_MAX_EXTENSION_RAW 64U" in src)

passed = sum(v for _, v in tests)
print(f"\nSELECT NC learning source checks: {passed}/{len(tests)} PASS")
print("NOT verified: STM32 build, selector behavior, actual NC switching or safe travel.")
print("Important: NC defines learned endpoint; previous RAW still bounds discovery.")
raise SystemExit(0 if passed == len(tests) else 1)
