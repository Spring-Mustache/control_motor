#!/usr/bin/env python3
"""M8-S11 source-only regression guard. Does not prove hardware or MCU build."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
MAIN = (ROOT / "Core/Src/main.c").read_text(encoding="utf-8")
IT = (ROOT / "Core/Src/stm32f7xx_it.c").read_text(encoding="utf-8")
ITH = (ROOT / "Core/Inc/stm32f7xx_it.h").read_text(encoding="utf-8")
PS = (ROOT / "tools/capture_m8_s11.ps1").read_text(encoding="utf-8")

def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)

header = re.search(r'"%s\\r\\n", "(seq,[^"]+)"', MAIN)
require(header is not None, "M8-S11 header missing")
columns = header.group(1).split(",")
require(len(columns) == 60 and len(set(columns)) == 60, "expected 60 unique CSV columns")

ps_hdr = re.search(r"\$expectedHeader = '([^']+)'", PS)
require(ps_hdr is not None and ps_hdr.group(1) == header.group(1),
        "PC capture header differs from MCU header")
require("$expectedFields = 60" in PS, "PowerShell field count mismatch")

sample = re.search(
    r'\n  n = snprintf\(\(char \*\)m8_s11_tx_line, sizeof\(m8_s11_tx_line\),\n'
    r'        "([^"]+)",\n([\s\S]*?)\);\n  m8_s11_last_tick',
    MAIN,
)
require(sample is not None, "M8-S11 sample formatter missing")
fmt = sample.group(1)
args = [x.strip() for x in sample.group(2).splitlines() if x.strip()]
require(fmt.endswith(r"\r\n"), "sample line framing broken")
require(len(re.findall(r"%(?:ld|lu|d|u)", fmt)) == 60, "printf 60 conversion mismatch")
require(len(args) == 60, "printf 60 argument mismatch")
require(len(header.group(1)) + 2 < 640, "CSV header exceeds asynchronous TX buffer")

stream_start = MAIN.index("static void M8S11_StreamUpdate(void)\n{")
stream_end = MAIN.index("static void M8S00_DumpUpdate(void)\n{", stream_start)
stream = MAIN[stream_start:stream_end]
for forbidden in ("HAL_UART_Transmit(&huart3", "HAL_Delay(", "HAL_FLASH_",
                  "HAL_GPIO_WritePin(", "__HAL_TIM_SET_COMPARE(",
                  "WheelControl_SetTargetPWM(", "Steering_PositionAxisUpdate(",
                  "AS5600_Read"):
    require(forbidden not in stream, "stream service must not contain " + forbidden)
require("HAL_UART_Transmit_IT(&huart3" in stream, "nonblocking UART TX missing")
require("m8_s11_stream_enable = 0U" in MAIN, "stream must default OFF")
require("if (period < 50U) period = 50U;" in stream, "minimum interval changed")
require("if (period > 250U) period = 250U;" in stream, "maximum interval changed")
require("HAL_NVIC_SetPriority(USART3_IRQn, 8, 0)" in MAIN, "USART3 low priority missing")
require("void USART3_IRQHandler(void)" in IT, "USART3 IRQ handler missing")
require("HAL_UART_IRQHandler(&huart3)" in IT, "USART3 HAL handler missing")
require("void USART3_IRQHandler(void);" in ITH, "USART3 declaration missing")
require("m8_s11_stream_state == 0U" in MAIN and
        "huart3.gState == HAL_UART_STATE_READY" in MAIN,
        "legacy blocking dump / async UART arbitration missing")

left = MAIN.index("static void Steering_ManualUpdate(void)\n{")
right = MAIN.index("\n}\n", left)
steer = MAIN[left:right]
require("int8_t left_direction = (int8_t)(-command);" in steer,
        "D-pad LEFT must map to L_MAX")
require("int8_t right_direction = command;" in steer,
        "D-pad LEFT must map to R_MIN")
require("(command < 0)" not in steer,
        "M8-S09 regression: stale PS2 sense used for LEFT hardware")
require("(left_direction < 0) ? GPIO_PIN_RESET : GPIO_PIN_SET" in steer,
        "LEFT hardware DIR no longer follows physical axis")
require("(right_direction < 0) ? GPIO_PIN_RESET : GPIO_PIN_SET" in steer,
        "RIGHT hardware DIR no longer follows physical axis")
require("Steering_SafetyOnLimitInterrupt(uint8_t bit)" in MAIN,
        "own-axis NC IRQ safety path missing")
require("#define STEER_POS_FINE_LEFT_FROM_MIN_RAW 10U" in MAIN,
        "R1 CENTER fine-entry baseline changed")

for intent, ldir, rdir, lgoal, rgoal in [
    (-1, 1, -1, "L_MAX", "R_MIN"),
    (1, -1, 1, "L_MIN", "R_MAX"),
]:
    actual_l = -intent
    actual_r = intent
    require((actual_l, actual_r) == (ldir, rdir),
            f"{lgoal}/{rgoal} mapping regression")

print("PASS M8-S11: 60-column CSV protocol, UART IRQ/guards, no actuation, PS2 mapping")
print("NOTE: Static inspection only. Full CubeIDE build and suspended hardware tests remain required.")
