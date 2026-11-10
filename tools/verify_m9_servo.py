#!/usr/bin/env python3
"""Verify the M9 requirement: continuous disturbance rejection, not timed recovery."""
from pathlib import Path
import subprocess,tempfile,shutil
R=Path(__file__).resolve().parents[1];cc=shutil.which('gcc')
if not cc:raise SystemExit('GCC required')
cfg=(R/'Core/Inc/m9_h2b1_commissioning.h').read_text()
assert '#define M9H2B1_POWER_MODE        4U' in cfg
assert '#define M9H2B1_POWER_COMMISSIONED 0U' in cfg
with tempfile.TemporaryDirectory() as temp:
 d=Path(temp)
 for old,new in [('MODE        4U','MODE        3U'),('LEFT_MAX_PWM    300U','LEFT_MAX_PWM    301U'),('RIGHT_MAX_PWM   100U','RIGHT_MAX_PWM   101U'),('LEFT_MAX_PWM    300U','LEFT_MAX_PWM    0U')]:
  assert old in cfg
  (d/'bad.h').write_text(cfg.replace(old,new));(d/'bad.c').write_text('#include "bad.h"\nint main(void){return 0;}\n')
  p=subprocess.run([cc,'-std=c11','-I'+str(R/'Core/Inc'),'-c',str(d/'bad.c'),'-o',str(d/'bad.o')],capture_output=True)
  assert p.returncode and b'#error' in p.stderr
 for extra,name in [([], 'plain'),(['-fsanitize=undefined','-fno-sanitize-recover=all'],'ubsan')]:
  exe=d/name
  subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-I'+str(R/'Core/Inc'),str(R/'Core/Src/m9_h2b1_hold.c'),str(R/'tools/m9_servo_host_test.c'),'-lm','-o',str(exe)]+extra,check=True)
  subprocess.run([str(exe)],check=True)
with tempfile.TemporaryDirectory() as temp:
 for flags,name in [([], 'servo'),(['-DM9B1_TEST_RECOVERY_BASELINE'],'published_recovery'),
                    (['-fsanitize=undefined','-fno-sanitize-recover=all'],'servo_ubsan')]:
  exe=Path(temp)/name
  subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror',
      '-I'+str(R/'Core/Inc'),'-I'+str(R/'tools'),str(R/'Core/Src/m9_h2b1_hold.c'),
      str(R/'tools/m9_servo_release_host_test.c'),'-lm','-o',str(exe)]+flags,check=True)
  subprocess.run([str(exe)],check=True)
print('PASS continuous load compensation, configuration guards and UBSan; physical torque/thermal capacity UNMEASURED')
