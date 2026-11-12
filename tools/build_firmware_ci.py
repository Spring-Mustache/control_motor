#!/usr/bin/env python3
"""Cross-compile and link exact committed STM32F767ZI source, independently of CubeIDE.
Requires gcc-arm-none-eabi and libnewlib-arm-none-eabi. Does not flash hardware.
"""
from pathlib import Path
import subprocess
import shutil
import json
import datetime
import hashlib
R=Path(__file__).resolve().parents[1]
cc=shutil.which('arm-none-eabi-gcc')
if not cc: raise SystemExit('arm-none-eabi-gcc required')
def run(args):
 subprocess.run(args,cwd=R,check=True)
def git(*args):
 return subprocess.check_output(['git',*args],cwd=R,text=True).strip()
sha=git('rev-parse','HEAD')
if git('status','--porcelain','--untracked-files=normal'):
 raise SystemExit('Refuse release binary from a dirty/untracked source tree')
out=R/'build-ci';out.mkdir(exist_ok=True)
header=R/'Core/Inc/firmware_build_id_generated.h'
header.write_text('#ifndef FIRMWARE_BUILD_ID_GENERATED_H\n#define FIRMWARE_BUILD_ID_GENERATED_H\n'
 f'#define FW_BUILD_GIT_SHA "{sha}"\n#define FW_BUILD_GIT_DIRTY 0U\n'
 '#define FW_BUILD_GIT_BRANCH "CI-exact-commit"\n'
 f'#define FW_BUILD_UTC "{datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")}"\n#endif\n')
flags=['-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard',
       '-DUSE_HAL_DRIVER','-DSTM32F767xx','-O0','-g3','-ffunction-sections','-fdata-sections',
       '-Wall','-Wextra','-Werror=implicit-function-declaration','-std=gnu11']
incs=['Core/Inc','Services/Inc','Drivers/STM32F7xx_HAL_Driver/Inc',
      'Drivers/STM32F7xx_HAL_Driver/Inc/Legacy','Drivers/CMSIS/Device/ST/STM32F7xx/Include','Drivers/CMSIS/Include']
flags+=['-I'+str(R/p) for p in incs]
sources=sorted((R/'Core/Src').glob('*.c'))+sorted((R/'Services/Src').glob('*.c'))+sorted((R/'Drivers/STM32F7xx_HAL_Driver/Src').glob('*.c'))
sources+=[R/'Core/Startup/startup_stm32f767zitx.s']
objects=[]
for src in sources:
 obj=out/(str(src.relative_to(R)).replace('/','_')+'.o');objects.append(obj)
 run([cc,*flags,'-c',str(src),'-o',str(obj)])
elf=out/'control_motor_M10_ethernet.elf'
run([cc,'-mcpu=cortex-m7','-mthumb','-mfpu=fpv5-d16','-mfloat-abi=hard',
     '--specs=nano.specs','--specs=nosys.specs','-T'+str(R/'STM32F767ZITX_FLASH.ld'),
     '-Wl,--gc-sections,-Map='+str(out/'control_motor_M10_ethernet.map'),
     *map(str,objects),'-Wl,--start-group','-lc','-lm','-Wl,--end-group','-o',str(elf)])
run(['arm-none-eabi-size',str(elf)])
for fmt,suffix in [('binary','bin'),('ihex','hex')]:
 run(['arm-none-eabi-objcopy','-O',fmt,str(elf),str(out/f'control_motor_M10_ethernet.{suffix}')])
manifest={'commit':sha,'target':'STM32F767ZI','validation':'ARM compile/link only; NOT hardware acceptance',
 'compiler':subprocess.check_output([cc,'--version'],text=True).splitlines()[0],
 'sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.glob('control_motor_M10_ethernet.*')}}
(out/'BUILD_MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('PASS exact-commit full Cortex-M7 firmware compile/link:',sha)
