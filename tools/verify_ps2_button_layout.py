#!/usr/bin/env python3
"""Check real PS2 decoder/field layout on GCC HAL stub and passive 32-bit mirrors.
SOURCE/HOST-only. Not an STM32 CubeIDE build, flashed ABI or physical-input proof.
"""
from pathlib import Path
from tempfile import TemporaryDirectory
import subprocess,shutil
root=Path(__file__).resolve().parents[1]
ps=(root/"Core/Src/ps2.c").read_text(encoding="utf-8")
main=(root/"Core/Src/main.c").read_text(encoding="utf-8")
head=(root/"Core/Inc/ps2.h").read_text(encoding="utf-8")
checks={
 "raw byte3/4 32bit active-low mirror, no remap":
    "debug_ps2_frame_buttons32 =" in ps and
    "(uint32_t)(uint8_t)(~ps2_data[3]) |" in ps and
    "((uint32_t)(uint8_t)(~ps2_data[4]) << 8)" in ps,
 "32-bit decoded mirror and seen latch":
    "debug_ps2_button_mask32 = (uint32_t)mask;" in main and
    "debug_ps2_button_seen_mask32 |= (uint32_t)mask;" in main,
 "PS2 read then input-only observer":
    "PS2_Read_Data();\n    Steering_PS2ManualArmUpdate();" in main and
    "PS2_DebugObserveAllInputs();" in main,
 "L1/L2 spin requires BOTH physical MIN switches":
    "STEER_LIMIT_L_MIN_MASK | STEER_LIMIT_R_MIN_MASK" in main and
    "ps2_spin_nc_block_frames++;" in main and
    "WheelControl_SetSpinDirection(1, MOTOR_DPAD_PWM);" in main and
    "WheelControl_SetSpinDirection(-1, MOTOR_DPAD_PWM);" in main,
 "not changing throttle, NC, STOP, PWM route":
    "static void PS2_ApplyManualControl(void)" in main and
    "Steering_TestRawLimitMask();" in main,
 "PS2 fields declared volatile and externally visible":
    "extern volatile PS2_ControllerTypeDef PS2_Pad;" in head and
    "extern volatile uint32_t debug_ps2_frame_buttons32;" in head
}
for name,ok in checks.items():print(("PASS" if ok else "FAIL")+": "+name)
if not all(checks.values()):raise SystemExit("PS2 input static guards FAILED")
cc=shutil.which("gcc") or shutil.which("cc")
if not cc:raise SystemExit("Cannot verify C decoder: no GCC")
hal=r"""
#ifndef STM32F7XX_HAL_H
#define STM32F7XX_HAL_H
#include <stdint.h>
typedef struct { int placeholder; } SPI_HandleTypeDef;
typedef struct { int placeholder; } GPIO_TypeDef;
typedef enum { HAL_OK=0, HAL_ERROR=1 } HAL_StatusTypeDef;
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
void HAL_GPIO_WritePin(GPIO_TypeDef*,uint16_t,int);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef*,uint8_t*,uint8_t*,uint16_t,uint32_t);
#endif
"""
test=r"""
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "ps2.h"
_Static_assert(offsetof(PS2_ControllerTypeDef,L2)==10,"L2 wrong offset");
_Static_assert(offsetof(PS2_ControllerTypeDef,R2)==11,"R2 wrong offset");
_Static_assert(offsetof(PS2_ControllerTypeDef,L1)==12,"L1 wrong offset");
_Static_assert(offsetof(PS2_ControllerTypeDef,R1)==13,"R1 wrong offset");
_Static_assert(offsetof(PS2_ControllerTypeDef,Triangle)==14,"TRI wrong offset");
static uint8_t rx_frame[9],index_byte;
static unsigned fail;
void HAL_GPIO_WritePin(GPIO_TypeDef *p,uint16_t pin,int val)
{ (void)p;(void)pin;if(val==GPIO_PIN_RESET)index_byte=0; }
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *s,uint8_t *tx,
  uint8_t *rx,uint16_t n,uint32_t timeout)
{
 (void)s;(void)tx;(void)timeout;assert(n==1U);
 if(fail)return HAL_ERROR;
 assert(index_byte<9);
 *rx=rx_frame[index_byte++];
 return HAL_OK;
}
static void neutral(void)
{
 memset(rx_frame,0xFF,sizeof(rx_frame));
 rx_frame[1]=0x73;rx_frame[2]=0x5A;
 rx_frame[3]=0xFF;rx_frame[4]=0xFF;
 rx_frame[5]=128;rx_frame[6]=128;rx_frame[7]=128;rx_frame[8]=128;
}
int main(void)
{
 SPI_HandleTypeDef spi={0};GPIO_TypeDef port={0};unsigned k,j;
 PS2_Init(&spi,&port,1);
 for(k=0;k<16;k++)
 {
  volatile uint8_t *fld[]={
    &PS2_Pad.Select,&PS2_Pad.L3,&PS2_Pad.R3,&PS2_Pad.Start,
    &PS2_Pad.Up,&PS2_Pad.Right,&PS2_Pad.Down,&PS2_Pad.Left,
    &PS2_Pad.L2,&PS2_Pad.R2,&PS2_Pad.L1,&PS2_Pad.R1,
    &PS2_Pad.Triangle,&PS2_Pad.Circle,&PS2_Pad.Cross,&PS2_Pad.Square
  };
  neutral();
  if(k<8)rx_frame[3]&=(uint8_t)~(1U<<k);
  else rx_frame[4]&=(uint8_t)~(1U<<(k-8));
  PS2_Read_Data();
  assert(PS2_Pad.Connected==1 && PS2_Pad.Mode==0x73);
  assert(debug_ps2_frame_buttons32==(1UL<<k));
  for(j=0;j<16;j++)assert(*fld[j]==(j==k));
 }
 neutral();rx_frame[2]=0x00;PS2_Read_Data();
 assert(PS2_Pad.Connected==0 && debug_ps2_frame_buttons32==0);
 neutral();fail=1U;PS2_Read_Data();
 assert(PS2_Pad.Connected==0 && debug_ps2_frame_buttons32==0);
 puts("PASS host C actual PS2 decoder 16 keys, 5 field offsets, invalid/SPI clears");
 return 0;
}
"""
with TemporaryDirectory() as tmp:
 d=Path(tmp)
 (d/"stm32f7xx_hal.h").write_text(hal)
 (d/"test.c").write_text(test)
 exe=d/"ps2_test"
 build=subprocess.run([cc,"-std=c11","-Wall","-Wextra","-Werror",
    "-I",str(d),"-I",str(root/"Core/Inc"),
    str(root/"Core/Src/ps2.c"),str(d/"test.c"),"-o",str(exe)],
    capture_output=True,text=True)
 if build.returncode:raise SystemExit(build.stdout+build.stderr)
 run=subprocess.run([str(exe)],capture_output=True,text=True)
 if run.returncode:raise SystemExit(run.stdout+run.stderr)
 print(run.stdout.strip())
print("NOT proven: MCU ELF/Live Expressions accuracy or physical button mapping.")
