# PS2 key-layout audit — 2026-11-04

## User-observed discrepancy, not yet proven in raw SPI
Physical L2 -> Live Expressions PS2_Pad.R2; R2 -> PS2_Pad.L1; L1 -> PS2_Pad.R1; R1 -> PS2_Pad.Triangle. That is a consistent **one BYTE debugger field displacement** if the live view reads the previous uint8_t for each requested field. These actual struct fields are adjacent uint8_t at offsets 10..14. Prior AS5600 Live Expressions also displayed displaced narrow data, so a narrow-expression presentation error is plausible. This is NOT proof of an actual PS2 SPI frame bit shift, of wrong physical controller wiring, or that the receiver sends the standard bit order.

Existing Core/Src/ps2.c already matches conventional PS2 payload:
- BYTE3 bits0..7 = Select,L3,R3,Start,Up,Right,Down,Left
- BYTE4 bits0..7 = L2,R2,L1,R1,Triangle,Circle,Cross,Square
- Active LOW. PS2 packet header requires mode 0x41/0x73 and 0x5A; SPI1 config high polarity, second clock edge, LSB first.
Existing main.c L1/L2 spin branch permits wheel movement ONLY with exactly LEFT_MIN+RIGHT_MIN NC mask 0x06 and no MAX NC; blocked attempts increment ps2_spin_nc_block_frames. It is NOT evidence of a key decode problem if wheel motor does not spin at center. Never bypass or spoof the NC condition.

## Passive, safe 32-bit diagnosis
Two new source-only readouts, updated with every valid PS2 sample:
- debug_ps2_frame_buttons32: 16-bit active-low raw packet bitmask widened to uint32_t **before** decoder field assignment; 0 when packet invalid or SPI failure.
- debug_ps2_button_mask32: computed from decoded PS2_Pad fields in original all-input observer (needs Mode 0x73), widened to uint32_t.
- debug_ps2_button_seen_mask32: cumulative OR of decoded masks since reboot, for short presses too.

All 16 bits have same positions. For single physical button:
L2=256/0x100; R2=512/0x200; L1=1024/0x400; R1=2048/0x800; Triangle=4096/0x1000. Before TEST, keep motor power isolated. Observe one held button at a time in Live Expressions. Raw32=decoded32 expected bit but PS2_Pad field label wrong -> likely debugger/ELF/watch symbol issue. Both32 report one position forward -> actual radio/controller/SPI protocol mismatch, collect packet data before modifying mapping. Raw32 correct, decoded32 wrong -> actual decoder/ABI bug to investigate. If raw32 always0 check packet Connected/Mode, RF pairing, SPI and flash/ELF identity. Never use these diagnostic values to enable powered wheel-spin before independent NC/STOP/cutoff validation.

## Validation layers
New actual C-source HOST regression executes real Core/Src/ps2.c with HAL mock for 16 single-key packets, invalid header and injected SPI failure; static checks ensure read-only mirrors and physical spin NC veto. Only after Actions exact SHA: user's motor-power-isolated CubeIDE clean target build, flash, and readback checks. The user's actual controller/receiver has not been independently sampled, so do not claim real key mapping PASS or change safety control based on debugger labels. Existing source path/caps/NC, R1/R2, and S15.3 active code unchanged.
