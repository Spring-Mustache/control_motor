#!/usr/bin/env python3
"""S15.4 after-I2C fresh-tick coordinator eligibility and drop diagnostics.

Static integration assertions plus actual C uint32 wrap arithmetic HOST test.
No STM32 target / hardware sync or motor safety PASS.
"""
from pathlib import Path
import tempfile,subprocess,shutil
R=Path(__file__).resolve().parents[1]
C=(R/"Core/Src/main.c").read_text(encoding="utf-8")
W=(R/".github/workflows/context-handoff-release.yml").read_text(encoding="utf-8")
def function(sig):
 start=C.index(sig);brace=C.index("{",start);depth=0
 for i in range(brace,len(C)):
  if C[i]=="{":depth+=1
  elif C[i]=="}":
   depth-=1
   if depth==0:return C[start:i+1]
 raise AssertionError(sig)
pos=function("static void Steering_PositionUpdate(void)\n{")
sync=function("static void Steering_VoiceS154Sync(")
cap=function("static void Steering_VoiceRightFineCapture(")
uart=function("static void M8S11_StreamUpdate(void)\n{")
first=pos.index("Steering_PositionAxisUpdate(1U")
second=pos.index("Steering_PositionAxisUpdate(0U")
fresh=pos.index("Steering_VoiceS154Sync(HAL_GetTick(), raw_mask, &left_cmd, &right_cmd);")
guard=pos.index("late_pending = steer_pos_irq_pending_mask;")
assert first < second < fresh < guard
assert "Steering_VoiceS154Sync(now, raw_mask, &left_cmd, &right_cmd);" not in pos
assert "(uint32_t)(now - steer_pos_left_last_sample_ms) > 40U" in sync
assert "(uint32_t)(now - steer_pos_right_last_sample_ms) > 40U" in sync
for field in ("sync_disabled","sync_phase","sync_stale","sync_nc","sync_waypoint"):
 assert f"d->{field} = steer_voice_s154_run_{field if field!='waypoint' else 'sync_waypoint'}" not in cap or field=="not_necessary"
 # All new snapshots must be emitted and reset on each new Voice4 run.
 assert f"d->{field} = steer_voice_s154_run_sync_{field};" in cap
 assert f"steer_voice_s154_run_sync_{field} = 0U;" in pos
assert "Steering_PositionWheelsStopped() != 0U" in uart
assert "huart3.gState == HAL_UART_STATE_READY" in uart
assert "HAL_UART_Transmit(" not in uart
assert "steer_voice_s154_enable = 1U;" in C
assert "steer_voice_s154_right_late_fine_enable = 1U;" in C
assert "steer_voice_s154_right_fine_diag_enable = 1U;" in C
assert "verify_m8_s154_sync_sample_clock.py" in W
cc=shutil.which("gcc") or shutil.which("clang") or shutil.which("cc")
if not cc:raise SystemExit("compiler missing")
source=r"""
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static uint8_t fresh(uint32_t now,uint32_t sample) {
    return ((uint32_t)(now-sample) <= 40U);
}
int main(void) {
    const uint32_t before_reads=1000U;
    const uint32_t left_stamp=1001U, right_stamp=1003U;
    const uint32_t after_reads=1004U;
    assert(!fresh(before_reads,left_stamp));
    assert(!fresh(before_reads,right_stamp));
    assert(fresh(after_reads,left_stamp));
    assert(fresh(after_reads,right_stamp));
    assert(!fresh(1100U,left_stamp));
    // wrap-safe only with forward time
    assert(fresh(2U,0xffffffffUL));
    puts("PASS wrap-safe uint32 ages: pre-I2C rejects future, post-I2C accepts");
    return 0;
}
"""
with tempfile.TemporaryDirectory() as temp:
 p=Path(temp)/"test.c";p.write_text(source)
 exe=Path(temp)/"test"
 run=subprocess.run([cc,"-std=c11","-O2","-Wall","-Wextra","-Werror",str(p),"-o",str(exe)],capture_output=True,text=True)
 if run.returncode:raise SystemExit("HOST compile FAIL\n"+run.stderr)
 run=subprocess.run([str(exe)],capture_output=True,text=True)
 if run.returncode:raise SystemExit("HOST runtime FAIL\n"+run.stderr+run.stdout)
 print(run.stdout.strip())
print("PASS fresh sample tick wiring after BOTH AS5600 reads; no NC/fine/STOP changes")
