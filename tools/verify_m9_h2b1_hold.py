#!/usr/bin/env python3
"""Full H2-B1 production C and main ownership/actuator HOST regression.

Synthetic test gains are NOT physical calibration or MCU/load PASS.
No serial hardware or motor output in these tests. The production tuning
profile authorizes bounded power; physical commissioning stays0.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import shutil
import subprocess
import ast
import re

ROOT=Path(__file__).resolve().parents[1]
MAIN=(ROOT/'Core/Src/main.c').read_text(encoding='utf-8')
PORT=(ROOT/'Core/Src/m9_h2b1_main.inc').read_text(encoding='utf-8')
CORE=(ROOT/'Core/Src/m9_h2b1_hold.c').read_text(encoding='utf-8')
CFG=(ROOT/'Core/Inc/m9_h2b1_commissioning.h').read_text(encoding='utf-8')
WORKFLOW=(ROOT/'.github/workflows/context-handoff-release.yml').read_text(encoding='utf-8')

def extract(signature):
    start=MAIN.index(signature+'\n{')
    begin=MAIN.index('{',start)
    depth=0
    for end in range(begin,len(MAIN)):
        if MAIN[end]=='{': depth+=1
        elif MAIN[end]=='}':
            depth-=1
            if not depth: return MAIN[start:end+1]
    raise AssertionError(signature)

assert '#define M9H2B1_POWER_COMMISSIONED 0U' in CFG
assert '#define M9H2B1_POWER_MODE        1U' in CFG
assert '#define M9H2B1_LEFT_MAX_PWM      30U' in CFG
assert '#define M9H2B1_RIGHT_MAX_PWM     10U' in CFG
assert 'tuning_guard' in CORE and 'M9B1_EFFORT_BUDGET' in CORE
assert 'M9B1_NO_PROGRESS' in CORE and 'fault_axes' in CORE
assert '3000' not in CORE and 'MAX_POLICY_MS' not in CORE
assert 'wheels_stopped' not in PORT.replace('No requirement for wheels_stopped here.','')
assert 'HAL_I2C' not in PORT and 'AS5600_Read' not in PORT
assert 'm9_h2b1_nc_epoch!=m9_h2b1_seen_nc_epoch' in PORT
assert 'EXTI->PR' in PORT and 'M9B1_Cut' in PORT
assert MAIN.index('    M9B1_PreOwnerService();')<MAIN.index('    Control_Arbitrate();')
assert MAIN.index('    M9H2_IdleUpdate();')<MAIN.index('    M9B1_Update();')
finish=extract('static void Steering_PositionFinish(uint8_t result)')
assert finish.index('Steering_PositionLeftOff();')<finish.index('M9B1_StartAfterDone(result);')
assert 'M9B1_KeepOutput()' in extract('static void Control_Arbitrate(void)')
assert 'M9B1_KeepOutput()' in extract('static void PS2_ApplyManualControl(void)')
assert '++m9_h2b1_nc_epoch' in extract('static void Steering_SafetyOnLimitInterrupt(uint8_t bit)')
packet='appliedMs=%lu,everApplied=%u,updates=%lu,dropped=%lu,retryCount=%lu'
assert packet+r'\r\n' in MAIN and packet+r'\\r\\n' not in MAIN
assert 'M9H2B1_HOLD' in MAIN
assert 'commissioned=%u,profile=%u' in MAIN
assert '(unsigned)M9H2B1_POWER_COMMISSIONED' in MAIN
assert 'peakErrL=%u,peakErrR=%u' in MAIN
assert 'appliedLms=%lu,appliedRms=%lu' in MAIN
assert 'M9B1_COOLDOWN' in CORE and 'retryCount=%lu' in MAIN
assert 'python tools/verify_m9_h2b1_hold.py' in WORKFLOW
packet_start=MAIN.index('"M9H2B1_HOLD,sha=')
packet_end=MAIN.index('FW_BUILD_GIT_SHA',packet_start)
format_string=''.join(ast.literal_eval(s) for s in re.findall(r'"(?:[^"\\]|\\.)*"',MAIN[packet_start:packet_end]))
worst=re.sub(r'%lu|%ld|%u|%d|%s',lambda m:{'%lu':'4294967295','%ld':'-2147483648','%u':'65535','%d':'-32768','%s':'f'*40}[m[0]],format_string)
assert len(worst.encode('ascii'))<640, 'HOLD status must fit the actual shared UART buffer'
assert format_string.endswith('\r\n')

cc=shutil.which('gcc') or shutil.which('cc')
if not cc: raise SystemExit('FAIL: C compiler is required, not SKIP')
names=['Steering_PositionLeftOff','Steering_PositionRightOff',
       'Steering_ManualLeftOff','Steering_ManualRightOff',
       'Steering_CalLeftOff','Steering_CalRightOff','Turn_Motors_Stop']
functions=[extract('static void '+n+'(void)') for n in names]
functions += [finish,extract('static void Steering_SafetyOnLimitInterrupt(uint8_t bit)')]
with TemporaryDirectory() as temp:
    d=Path(temp)
    # Compile rejection of unaccepted production mode, missing gains,
    # raised test ceilings and missing effort/response guards.
    for old,new,message in [
        ('M9H2B1_POWER_MODE        1U','M9H2B1_POWER_MODE        2U','requires physical acceptance'),
        ('M9H2B1_LEFT_MAX_PWM      30U','M9H2B1_LEFT_MAX_PWM      0U','requires valid'),
        ('M9H2B1_LEFT_MAX_PWM      30U','M9H2B1_LEFT_MAX_PWM      31U','tuning requires bounded'),
        ('M9H2B1_NO_PROGRESS_MS   1000U','M9H2B1_NO_PROGRESS_MS   0U','tuning requires bounded')]:
        assert old in CFG
        (d/'bad_commissioning.h').write_text(CFG.replace(old,new),encoding='utf-8')
        (d/'bad.c').write_text('#include "bad_commissioning.h"\nint main(void){return 0;}\n',encoding='utf-8')
        rejected=subprocess.run([cc,'-std=c11','-I'+str(ROOT/'Core/Inc'),'-c',str(d/'bad.c'),'-o',str(d/'bad.o')],capture_output=True,text=True)
        assert rejected.returncode!=0 and message in rejected.stderr,rejected.stderr
    fixture=(ROOT/'tools/m9_h2b1_main_host_test.c').read_text(encoding='utf-8')
    assert '/* PRODUCTION_FUNCTIONS */' in fixture
    (d/'integration.c').write_text(fixture.replace('/* PRODUCTION_FUNCTIONS */','\n'.join(functions)),encoding='utf-8')
    for source,name,extra in [(ROOT/'tools/m9_h2b1_host_test.c','controller',[]),
                              (d/'integration.c','main_hooks',[]),
                              (d/'integration.c','production_tuning_hooks',['-DM9B1_USE_TUNING_PROFILE'])]:
        executable=d/name
        cmd=[cc,'-std=c11','-O2','-Wall','-Wextra','-Werror',
             '-I'+str(ROOT/'Core/Inc'),'-I'+str(ROOT/'Core/Src'),
             str(ROOT/'Core/Src/m9_h2b1_hold.c'),str(source),'-o',str(executable)]
        subprocess.run(cmd+extra,check=True)
        subprocess.run([str(executable)],check=True)
    # Compile the actual production snprintf invocation, including its
    # argument types/count and the distinction between profile1 and
    # commissioned0. This does not need to mock the entire UART service.
    a=MAIN.index('    const M9B1Hold *b=&m9_h2b1_hold;',packet_start-150)
    b=MAIN.index('    if (M8S11_SendLine(n)',a)
    formatter='''#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "m9_h2b1_commissioning.h"
#define FW_BUILD_GIT_SHA "ffffffffffffffffffffffffffffffffffffffff"
static M9B1Hold m9_h2b1_hold;
static unsigned char m8_s11_tx_line[640];
static uint32_t m9_h2b1_dropped;
static uint32_t HAL_GetTick(void){return 99U;}
int main(void){int n;
'''+MAIN[a:b]+'''
assert(n>0 && n<(int)sizeof(m8_s11_tx_line));
assert(strlen((char *)m8_s11_tx_line)==(size_t)n);
assert(strstr((char *)m8_s11_tx_line,"commissioned=0,profile=1,"));
assert(m8_s11_tx_line[n-2]=='\\r' && m8_s11_tx_line[n-1]=='\\n');
puts("PASS actual production UART formatter and powered-tuning/commissioning labels");
return 0;}
'''
    (d/'formatter.c').write_text(formatter,encoding='utf-8')
    subprocess.run([cc,'-std=c11','-O2','-Wall','-Wextra','-Werror','-Wformat=2',
        '-I'+str(ROOT/'Core/Inc'),str(d/'formatter.c'),'-o',str(d/'formatter')],check=True)
    subprocess.run([str(d/'formatter')],check=True)
print('PASS H2-B1 persistent dual-axis controller + production tuning/effort/progress and actual main/HAL-port HOST; NOT physical acceptance')
