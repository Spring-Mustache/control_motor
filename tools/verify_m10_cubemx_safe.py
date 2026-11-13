#!/usr/bin/env python3
"""Read-only protection gate for M10 STM32CubeMX regeneration."""
from pathlib import Path
import sys

R = Path(__file__).resolve().parents[1]


def check(test, message):
    if not test:
        raise RuntimeError("M10 CubeMX guard FAIL: " + message)


def read(path):
    return (R / path).read_text(encoding="utf-8")


def user_zone(source, label):
    begin = "/* USER CODE BEGIN " + label + " */"
    end = "/* USER CODE END " + label + " */"
    check(source.count(begin) == 1 and source.count(end) == 1,
          "missing or duplicate USER CODE markers: " + label)
    a = source.index(begin) + len(begin)
    b = source.index(end)
    check(a < b, "reversed USER CODE markers: " + label)
    return source[a:b]


def verify():
    main = read("Core/Src/main.c")
    for label, statement in (
        ("Includes", '#include "m10_eth_bringup.h"'),
        ("Includes", '#include "m10_eth_protocol.h"'),
        ("SysInit", "M10_ETH_ConfigMPU();"),
        ("MACADDRESS", "memcpy(MACAddr, m10_eth_mac, sizeof(MACAddr));"),
        ("2", "Steering_QuarantineParkPins();"),
        ("2", "Steering_DebugRetainWatchSymbols();"),
        ("2", "M10_ETH_Init(&heth);"),
        ("WHILE", "M10_ETH_Poll();"),
    ):
        check(statement in user_zone(main, label), "lost USER CODE: " + statement)
    for statement in (
        "M10_ETH_ConfigMPU();",
        "memcpy(MACAddr, m10_eth_mac, sizeof(MACAddr));",
        "Steering_DebugRetainWatchSymbols();",
        "M10_ETH_Init(&heth);",
        "M10_ETH_Poll();",
    ):
        check(main.count(statement) == 1, "missing/duplicated call: " + statement)
    check(main.index("M10_ETH_ConfigMPU();") < main.index("MX_ETH_Init();"),
          "MPU must precede ETH initialization")
    check(main.index("memcpy(MACAddr, m10_eth_mac, sizeof(MACAddr));") <
          main.index("if (HAL_ETH_Init(&heth) != HAL_OK)"),
          "MAC override must precede HAL ETH initialization")
    park_zone = main.index("/* USER CODE BEGIN 2 */")
    check(main.index("Steering_QuarantineParkPins();", park_zone) <
          main.index("if (HAL_TIM_PWM_Start(&htim12"),
          "M9 parking must happen before steering PWM starts")
    check("STEER_SAFETY_INCIDENT_LOCKOUT" in main,
          "M9 steering safety lockout definition missing")

    ioc = read("control_motor.ioc").splitlines()
    for setting in (
        "ProjectManager.KeepUserCode=true",
        "ProjectManager.BackupPrevious=true",
        "ProjectManager.DeletePrevious=false",
        "ProjectManager.LastFirmware=false",
        "ProjectManager.FirmwarePackage=STM32Cube FW_F7 V1.17.4",
        "ETH.MediaInterface=HAL_ETH_RMII_MODE",
        "ETH.PhyAddress=0",
    ):
        check(setting in ioc, "unexpected IOC setting: " + setting)
    for module in ("m10_eth_bringup", "m10_eth_protocol"):
        for folder, suffix in (("Services/Src/", ".c"), ("Services/Inc/", ".h")):
            check((R / (folder + module + suffix)).is_file(),
                  "missing user-owned M10 module: " + folder + module + suffix)
        for folder, suffix in (("Core/Src/", ".c"), ("Core/Inc/", ".h")):
            check(not (R / (folder + module + suffix)).exists(),
                  "duplicate file in generator-managed Core: " + module + suffix)

    flash = read("STM32F767ZITX_FLASH.ld")
    preserved = read("config/M10_FLASH_LINKER_BASELINE.ld")
    check(flash == preserved,
          "linker changed; compare against config/M10_FLASH_LINKER_BASELINE.ld")
    for item in ("ORIGIN = 0x2007C000", "LENGTH = 16K",
                 "KEEP(*(.RxDecripSection))", "KEEP(*(.TxDecripSection))",
                 "KEEP(*(.m10_eth_dma))", "} >ETHRAM"):
        check(item in flash, "missing SRAM2 DMA mapping: " + item)
    project = read(".cproject")
    check("Services/Inc" in project and "Services" in project,
          "CubeIDE may have dropped Services source/include tree")
    check("STM32F767ZITX_FLASH.ld" in project,
          "CubeIDE might not be using the expected FLASH linker")
    print("PASS: CubeMX-safe M10 code, MAC/MPU, M9 safety hook, IOC, Services and linker")


if __name__ == "__main__":
    try:
        verify()
    except (OSError, RuntimeError) as exc:
        print(exc, file=sys.stderr)
        sys.exit(1)
