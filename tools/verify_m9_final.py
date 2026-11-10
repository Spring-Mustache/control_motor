#!/usr/bin/env python3
"""Final recovery profile: actual C closed-loop and compiler checks; no MCU claim."""
from pathlib import Path
import subprocess
import tempfile
import shutil
R=Path(__file__).resolve().parents[1]
cc=shutil.which('gcc')
if not cc: raise SystemExit('GCC required')
cfg=(R/'tools/m9_h2b1_recovery_profile.h').read_text()
main=(R/'Core/Src/main.c').read_text()
assert 'steer_pos_left_pwm_min = 175U;' in main and 'steer_pos_left_pwm_max = 300U;' in main
assert 'steer_pos_right_pwm_min = 50U;' in main and 'steer_pos_right_pwm_max = 100U;' in main
assert '#define M9H2B1_POWER_COMMISSIONED 0U' in cfg
assert '|| m9_h2b1_hold.active)' in main
with tempfile.TemporaryDirectory() as temp:
 d=Path(temp)
 for old,new in [('MODE        3U','MODE        2U'),('LEFT_MAX_PWM    300U','LEFT_MAX_PWM    301U'),
                 ('RIGHT_MAX_PWM   100U','RIGHT_MAX_PWM   101U'),('LEFT_MIN_PWM    175U','LEFT_MIN_PWM    0U'),
                 ('NO_PROGRESS_MS 1000U','NO_PROGRESS_MS 0U'),('RECOVERY_MS    5000U','RECOVERY_MS    5001U'),
                 ('REST_MS        3000U','REST_MS        2999U')]:
  assert old in cfg
  (d/'bad.h').write_text(cfg.replace(old,new))
  (d/'bad.c').write_text('#include "bad.h"\nint main(void){return 0;}\n')
  bad=subprocess.run([cc,'-std=c11','-I'+str(R/'Core/Inc'),'-c',str(d/'bad.c'),'-o',str(d/'bad.o')],capture_output=True)
  assert bad.returncode and b'#error' in bad.stderr
 for extra,name in [([], 'normal'),(['-fsanitize=undefined','-fno-sanitize-recover=all'],'ubsan')]:
  exe=d/name
  subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(R/'Core/Inc'),
      str(R/'Core/Src/m9_h2b1_hold.c'),str(R/'tools/m9_final_host_test.c'),'-lm','-o',str(exe)]+extra,check=True)
  subprocess.run([str(exe)],check=True)
print('PASS final profile compile rejection, actual C synthetic plants and undefined-behavior sanitizer; physical result UNVERIFIED')
