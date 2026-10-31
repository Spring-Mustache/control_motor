#!/usr/bin/env python3
"""S14.3 fast-start source guards and optional C host test."""
from pathlib import Path
import shutil,subprocess,tempfile,os,re
root=Path(__file__).resolve().parents[1]
s=(root/'Core/Src/main.c').read_text(encoding='utf-8')
h=(root/'Core/Inc/steer_s143_faststart.h').read_text(encoding='utf-8')
checks={
 'opt-in boot OFF':'steer_s143_fast_start_enable = 0U;' in s,
 'alternate idle poll before quiet observe':
     s.index('Steering_PositionTelemetryIdleUpdate();') < s.index('Steering_S143NeutralObserve();'),
 'idle cadence matches cache windows':
     '#define STEER_POS_IDLE_POLL_MS      50U' in s and
     '#define M8S143_MAX_GAP_MS 180U' in h and
     '#define M8S143_MAX_AGE_MS 140U' in h,
 'both idle observers':'M8S143_Ready(&steer_s143_quiet_l,now,' in s and 'M8S143_Ready(&steer_s143_quiet_r,now,' in s,
 'new post-command sample':'dt >= 10U' in s and 'axis->fast_reference_sample_ms' in s,
 'newer clock after blocking grant read':(
   'uint32_t grant_now_ms = HAL_GetTick();' in s and
   'M8S143_Ready(&steer_s143_quiet_l,grant_now_ms,' in s and
   'M8S143_Ready(&steer_s143_quiet_r,grant_now_ms,' in s
 ),
 'newer clock after axis read':(
   'uint32_t sample_check_now = HAL_GetTick();' in s and
   '((uint32_t)(sample_check_now-sample_ms) <=' in s
 ),
 'diagnostic gate reasons':'M8S00_S143_GATE,missing=0x%04x' in s,
  'legacy fallback':(
    bool(re.search(r'^#define[ \t]+STEER_MANUAL_START_LEFT_MIN_MS[ \t]+100U[ \t]*$',s,re.M)) and
    bool(re.search(r'^#define[ \t]+STEER_MANUAL_START_RIGHT_MIN_MS[ \t]+160U[ \t]*$',s,re.M)) and
    bool(re.search(r'^#define[ \t]+STEER_MANUAL_START_QUIET_MS[ \t]+100U[ \t]*$',s,re.M)) and
    'STEER_MANUAL_START_SETTLE_MAX_MS' in s and
    'if (fast_now ||' in s and
    '((settle_ms >= min_ms) &&' in s and
    '((uint32_t)(now-axis->settle_quiet_since_ms) >=' in s
 ),
 'NC/soft checks retained':'Steering_TestRawLimitMask() == 0U' in s and 'STEER_MANUAL_RESULT_SOFT' in s,
 'no I2C or motor IO in helper':'HAL_' not in h and 'Steering_PositionReadOne' not in h,
 'diagnostic':'M8S00_S143,enable=%u,grants=%lu' in s,
 'caps unchanged':'375U:150U' in (root/'Core/Inc/steer_s142_speed.h').read_text(encoding='utf-8')
}
for name,ok in checks.items():print(('PASS' if ok else 'FAIL')+': '+name)
if not all(checks.values()):raise SystemExit(1)
cc=next((p for n in ('gcc','clang','cc') if (p:=shutil.which(n))),None)
if cc is None:print('SKIP native C tests: no compiler')
else:
 with tempfile.TemporaryDirectory() as tmp:
  exe=Path(tmp)/('test_s143.exe' if os.name=='nt' else 'test_s143')
  p=subprocess.run([cc,'-std=c11','-Wall','-Wextra','-Werror','-I',str(root/'Core/Inc'),str(root/'tools/test_m8_s143_faststart.c'),'-o',str(exe)],capture_output=True,text=True)
  if p.returncode:raise SystemExit(p.stderr)
  q=subprocess.run([str(exe)],capture_output=True,text=True)
  if q.returncode:raise SystemExit(q.stdout+'\n'+q.stderr)
  print(q.stdout)
print('STM32 target build/Flash/hardware NOT tested.')
