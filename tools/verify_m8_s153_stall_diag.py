#!/usr/bin/env python3
"""S15.3D passive STALL/STOP5 source and pure C regression.
HOST/static only; not MCU target build, flash or physical acceptance.
"""
from pathlib import Path
import os,shutil,subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/"Core/Src/main.c").read_text(encoding="utf-8")
h=(root/"Core/Inc/steer_s153_stall_diag.h").read_text(encoding="utf-8")
fault=s[s.index("static void Steering_ManualAxisFault("):s.index("/* After a bounded departure",s.index("static void Steering_ManualAxisFault("))]
capture=s[s.index("static void M8S153_CaptureManualFault("):s.index("static void Steering_ManualAxisDone(",s.index("static void M8S153_CaptureManualFault("))]
stop=s[s.index("static void Steering_ManualStop(void)"):s.index("/* Every endstop",s.index("static void Steering_ManualStop(void)"))]
capstop=s[s.index("static void M8S153_CaptureStop5("):s.index("static void Steering_ManualStop(void)",s.index("static void M8S153_CaptureStop5("))]
dump=s[s.index("static uint8_t M8S153_DiagDump(void)"):s.index("/* Identity embedded",s.index("static uint8_t M8S153_DiagDump(void)"))]
checks={
 "boot S15.3 OFF, geometry/terminal/stall guards unchanged":
   all(x in s for x in ("steer_s153_right_only_enable=0U;",
        "steer_cal_right_min_runtime = 195U;",
        "steer_manual_right_soft_margin_raw = 50U;",
        "steer_manual_right_rx_min_pwm = 50U;",
        "#define STEER_MANUAL_STALL_MS          650U")),
 "FAULT snapshot after encoder arrival sample and before own-side PWM OFF":
   "M8S00_ArrivalFault(left, reason);\n  M8S153_CaptureManualFault(left,reason);" in fault and
   fault.index("M8S153_CaptureManualFault(left,reason);")<fault.index("Steering_ManualLeftOff();"),
 "captures RAW, distance, progress/sample age, speed, encoder and CCR":
   all(x in capture for x in ("axis->best_distance","M8S153_DiagGap(",
         "M8S153_DiagAgeMs(","M8S153_DiagFuture(",
         "m8_s06_current.left","m8_s06_current.right","encoder_delta",
         "speed_filtered_raw_s","speed_valid",
         "__HAL_TIM_GET_COMPARE(&htim14,TIM_CHANNEL_1)",
         "Steering_TestRawLimitMask()")),
 "passive event functions never transmit UART, read I2C, write PWM or motion permits":
   not any(x in capture+capstop for x in ("HAL_UART_Transmit(", "HAL_I2C_",
      "Steering_PositionReadOne(", "__HAL_TIM_SET_COMPARE(",
      "steer_safety_motion_permit =", "HAL_GPIO_WritePin(")),
 "STOP5 captured before stop cleanup, distinguishes neutral from reversal":
   "if(steer_manual_block_reason==5U)M8S153_CaptureStop5();" in stop and
   stop.index("M8S153_CaptureStop5();")<stop.index("Steering_ManualLeftOff();") and
   "steer_s153_stop5_path=M8S153_STOP5_NEUTRAL_REQUIRED;" in s and
   "steer_s153_stop5_path=M8S153_STOP5_DIRECT_REVERSAL;" in s,
 "per-axis snapshots stay latched until NEW manual command":
   "if(d->valid!=0U)return;" in capture and
   all(x in s for x in ("steer_s153_fault_diag[0]=(M8S153FaultDiag_t){0};",
      "steer_s153_fault_diag[1]=(M8S153FaultDiag_t){0};",
      "steer_s153_stop5_diag=(M8S153Stop5Diag_t){0};")),
 "3 new UART rows only behind existing fully disarmed M8S00 gate":
   all(x in dump for x in ("M8S153_FAULT,axis=%c,valid=1",
      "M8S153_FAULT,axis=%c,valid=0",
      "M8S153_STOP5,valid=1","M8S153_STOP5,valid=0","M8S00_UartLine(")) and
   "(M8S153_DiagDump() == 0U)" in s and
   "if ((steer_safety_motion_permit != 0U)" in s,
 "ETA pilot still opt-in RMIN-only, no tail60":
   "M8S153_PlanRightMid(" in s and
   "(right_direction<0))?1U:0U;" in s and
   "steer_right_min_tail60" not in s and "M8S15_TAIL60" not in s,
 "portable helper has wrap-safe tick/AS jitter and stop-path constants":
   all(x in h for x in ("M8S153_DiagAgeMs(","M8S153_DiagGap(",
     "M8S153_DiagFuture(","M8S153_STOP5_NEUTRAL_REQUIRED",
     "M8S153_STOP5_DIRECT_REVERSAL")),
}
for name,ok in checks.items():print(("PASS" if ok else "FAIL")+": "+name)
if not all(checks.values()):raise SystemExit("S15.3D static FAIL")
cc=shutil.which("gcc") or shutil.which("clang") or shutil.which("cc")
if not cc:raise SystemExit("C compiler missing; cannot mark HOST PASS")
with tempfile.TemporaryDirectory() as tmp:
 exe=Path(tmp)/("s153diag.exe" if os.name=="nt" else "s153diag")
 a=subprocess.run([cc,"-std=c11","-Wall","-Wextra","-Werror","-I",
      str(root/"Core/Inc"),str(root/"tools/test_m8_s153_stall_diag.c"),
      "-o",str(exe)],capture_output=True,text=True)
 if a.returncode:raise SystemExit(a.stdout+a.stderr)
 b=subprocess.run([str(exe)],capture_output=True,text=True)
 if b.returncode:raise SystemExit(b.stdout+b.stderr)
 print(b.stdout)
print("S15.3D HOST/STATIC PASS only; MCU Build/Flash and hardware PASS separate.")
