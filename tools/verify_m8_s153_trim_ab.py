#!/usr/bin/env python3
"""S15.3 mode B source isolation, no hardware claim."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/"Core/Src/main.c").read_text()
a=s.index("    else if(steer_s153_trim_only_latched!=0U)")
b=s.index("    else if (steer_s142_latched != 0U)",a)
x=s[a:b]
checks={
"mode B enabled only on fresh eligible Level2 RIGHT RMIN":
 "steer_s153_trim_only_enable=1U;" in s and
 "steer_s153_trim_only_latched=((steer_s153_trim_only_enable==1U)" in s and
 "(steer_s142_latched==2U) && (right_direction<0)" in s,
"mode A / C and ETA boot OFF remain":
 "steer_s153_right_only_enable=0U;" in s and
 "else if(steer_s153_latched!=0U)" in s,
"original left positive correction and separate right temporary":
 "uint16_t baseline_l=la,discarded_right=ra;" in x and
 "&la,&discarded_right);" in x and
 "if(la<baseline_l)la=baseline_l;" in x and
 "ra=discarded_right" not in x,
"other active pilots excluded": "(steer_s153_trim_only_latched==0U)" in s,
"no ETA call or output-register write in mode B":
 "Steering_S153RightApply" not in x and
 "__HAL_TIM_SET_COMPARE" not in x and
 "HAL_GPIO_WritePin" not in x,
"STOP NC and independent owner retained":
 "primask = __get_PRIMASK();\n  __disable_irq();\n  mask = Steering_TestRawLimitMask();" in s and
 "__HAL_TIM_SET_COMPARE(&htim14, TIM_CHANNEL_1, ra);" in s,
"terminal 50CCR, RMIN195 and STALL650 unchanged":
 "steer_manual_right_rx_min_pwm = 50U;" in s and
 "steer_cal_right_min_runtime = 195U;" in s and
 "#define STEER_MANUAL_STALL_MS          650U" in s,
"post-DISARM mode identification": "M8S153_MODE,mode=%u,isoEnable=%u" in s and
 "(M8S153_Dump() == 0U)" in s
}
for k,v in checks.items(): print(("PASS" if v else "FAIL")+": "+k)
assert all(checks.values()),"Mode B guard failed"
print("HOST STATIC PASS only; CubeIDE and physical 4/4 not tested")
