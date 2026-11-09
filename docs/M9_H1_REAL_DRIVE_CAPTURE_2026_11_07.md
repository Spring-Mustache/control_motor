# M9-H1 REAL DRIVE — Thu vận động motor thật để chỉnh PD (2026-11-07)

> **KẾT QUẢ MỚI 2026-11-07:** Đã nhận file UART private từ người dùng: 4/4 lệnh M8 DONE, 130 frame AS5600/CCR/u_shadow đầy đủ, lỗi cảm biến=0. H1 đánh giá được dấu và độ bão hòa trên động học thật; KHÔNG được kết luận cấp lực giữ hoặc phản ứng tải ngoài đã đạt. Xem bảng phân tích ở cuối file.

**MỤC TIÊU:** Người dùng muốn **một lượt thu dữ liệu xe thật**, không mô phỏng không Live Expressions và không tạo project STM32CubeIDE riêng. Bộ ghi RAM chỉ quan sát các lệnh position M8 vốn đã có, không cấp PWM mới, không đẩy bánh bằng tay. Tệp này là hướng dẫn bench & ranh giới kết luận, không phải báo cáo chạy xe đã diễn ra.

## Nguồn dữ liệu, thời điểm lấy mẫu

- Điều kiện bắt đầu: M8 owner đã **được cấp phép** lệnh R1 CENTER (PS2/VC02) hoặc Voice5..40. Không thu R2/Voice45 NC, debug test, SELECT calibration, manual servo.
- Trong quá trình M8 motor **đang chạy thật**: sau cập nhật M8/AS5600 của main loop, H1 recorder đọc **cache cảm biến đã có** và snapshot CCR TIM12 CH2 LEFT/TIM14 CH1 RIGHT. Không gọi thêm I2C, không thay PWM/DIR/owner; không gọi UART trong giai đoạn đang chạy.
- Lưu **80 frame tối đa**, mỗi frame cách ≥80ms. Chứa: `t` sau START (ms), `rawL/rawR`, `eL/eR` (RAW signed shortest), `vL/vR` (RAW/s lọc H1 window≥100ms), `uL/uR` (**shadow-only arbitrary units**), `ccrL/ccrR` (giá trị CCR **thực của M8** chỉ đọc), `ageL/ageR` ms của cached mẫu, NC mask và trạng thái từng trục. BEGIN có goalL/R, start/end L/R, result, thời gian, peak CCR, overflow/bad/stale/skipped/dropped.
- Đủ điều kiện M8 result2 thì H0/H1 **tiếp tục** quan sát tĩnh 3000ms như cũ. Khi cả steering/wheels đã idle, UART3 IRQ 115200 lần lượt phát H0/H1 logs ưu tiên, rồi `M9H1_REAL_BEGIN`, 0..80 `M9H1_REAL_FRAME`, `M9H1_REAL_END`. Nếu frame tràn, `overflow>0` — dữ liệu đã lấy vẫn dùng được nhưng không đại diện toàn bộ thời gian chuyển động.
- `M9H1_REAL_END` **không phải kết quả lực HOLD**, là dấu đã truyền xong snapshot. Script không gửi lệnh, không cấp dòng hay tạo chuyển động.

## Hướng dẫn một lần trên cùng project hiện tại

**Trước khi cập nhật mã:** user từng báo thay đổi cục bộ `.settings/language.settings.xml`, `.settings/stm32cubeide.project.prefs`, `Core/Startup/startup_stm32f767zitx.s` và file CSV bị xóa. **KHÔNG reset/restore/clean**; bảo vệ các thay đổi cần giữ trước khi `git pull`. Đặc biệt file startup nếu khác nhau phải so/merge trước khi Build. Xác nhận branch `feature/m9-steering-active-position-hold` và HEAD thật từ `git rev-parse HEAD`.

1. CI GitHub trên SHA mới phải PASS trước; Clean Build và Flash firmware đúng HEAD theo STM32CubeIDE, không chỉ tự tin vì Build cũ; trong Debug phải Resume (F8) để MCU chạy liên tục. Không cần Live Expressions hay sửa các flag.
2. Xe trên **giá cố định, bánh dẫn động không chạm đất**, bảo đảm NC/STOP và công tắc nguồn động lực độc lập đã kiểm tra. Không đứng trong vùng kẹp trục; không dùng tay đẩy bánh hoặc làm phép thử tải ngoài tại bước này.
3. Chạy PowerShell trong project hiện hành:
   ```powershell
   cd "D:\STM32 task\control_motor"
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\capture_m9_h1_real.ps1" -TimeoutSeconds 600 -MaxRuns 4 -OutputDirectory ".\logs"
   ```
4. Khi PowerShell báo `M9-H1 REAL: listening COM...`, sử dụng **4 lệnh Voice/PS2 đã từng thử an toàn trên chính giá**. Gợi ý phủ hai chiều: Voice 15° hướng trái, Voice 15° hướng phải, Voice 30° hướng trái/phải đã kiểm chứng, cuối cùng Voice CENTER. Dùng đúng cú pháp Voice sẵn có của xe, không gán lệnh giả; không yêu cầu nhất thiết phải đúng một thứ tự.
5. **Sau mỗi lệnh**, chờ DONE, ≥3s cửa sổ H0/H1, và thông báo `REAL_END 1/4`, `2/4`,... trước khi bắt đầu lệnh tiếp. Không thử 45°/NC/kháng lực. Nếu STOP/NC/trip, dừng test, giữ file TXT để chẩn đoán an toàn; không tìm cách vượt bảo vệ.
6. Sau `REAL_END 4/4`, gửi **file `M9H1_REAL_yyyymmdd_hhmmss.txt` riêng trong hội thoại**, không commit vào repo public. User không cần chụp Live Expressions. Đừng xóa log gốc, không cần chạy lại 4 static H0/H1 lượt cũ riêng.

**Nếu PowerShell báo không tìm thấy file:** working tree local đang ở commit trước, cập nhật source an toàn rồi kiểm tra `Test-Path .\tools\capture_m9_h1_real.ps1` phải True. Đừng ghi đè file startup local. Không copy một PowerShell script mới để sử dụng với firmware cũ vì nó không phát `M9H1_REAL_*`.

## Khi nhận log: phân tích để cải tiến

- Kiểm tra 4 cặp BEGIN/END theo SHA firmware, số frame, `result`, `overflow`/bad/age, timestamp theo mỗi mốc, quá độ đạt/chậm/kẹt theo từng trục.
- Vẽ `raw/goal`, error, `v`, candidate `u` theo thời gian và so actual `CCR` của M8 để đánh giá điểm đảo dấu/đường đi qua goal/overshoot/phần M8 fine. Phân biệt `CCR=0` vì M8 đã tắt/trục DONE và giá trị `u` shadow không đưa xuống driver.
- **Không hiệu chuẩn Kp/Kd bằng một số đơn vị SHADOW ≠ mô-men thực.** Đường cong vận động dưới PWM M8 giúp nhận dạng tốc độ cơ cấu, trễ vòng lặp, noise, braking time và độ vọt lố khi điều khiển M8. Nó chưa đo đáp ứng của servo giữ góc trước ngoại lực thực sau DONE.
- Nếu dữ liệu đủ chất lượng, quyết định **đề xuất** phương án H2 theo giới hạn điện áp, dòng/nhiệt và cơ chế STOP/NC; không tự bật H2. Bước thử có phản lực thực chỉ bàn sau đánh giá rủi ro và mục tiêu tải.

## Ranh giới bằng chứng

Source/CI PASS là kiểm tra phần mềm, **không phải** CubeIDE Clean Build/Flash/REAL ON-TARGET PASS. Chưa có file `M9H1_REAL` do người dùng gửi; bộ thu chỉ tạo khi xe thực sự chạy bốn lệnh. Không dùng bài unit test dữ liệu giả làm log đo xe. H0/H1 static 4/4 từ `00ccaeace` đã PASS giới hạn, vẫn lưu lịch sử. Không tạo release, không đụng original 386 dòng hoặc các giới hạn M8 NC/STOP.

## Exact source CI (2026-11-07)

Firmware source commit `93551551422559914052ec8b8671a521715714b4` passed GitHub Actions run `42368389841` (Linux GCC real trace C/wiring + no new motor writes and Windows PowerShell5.1 script syntax). Source milestone **PASS**. **Hardware real-data CAPTURE vẫn PENDING** đến khi STM32 được Clean Build/Flash đúng SHA và gửi file output của `capture_m9_h1_real.ps1`. Logs CSV/TXT cá nhân không xuất hiện trên GitHub.

## 2026-11-07 — M9-H1 REAL data: 4 lượt motor M8 thật, UART PRIVATE

**[USER-PROVIDED REAL MCU TXT]** File gốc `M9H1_REAL_20261107_183513.txt` gửi trong hội thoại; mọi dòng có `sha=3dbdfaf678b700f03f3517cf0e70664c5557f6ba`. **Chỉ lưu phân tích tổng hợp trong GitHub, KHÔNG commit 146 dòng TXT riêng tư.** Đây là số đo từ motor/AS5600 thật dưới bộ lái M8; thuật toán H1 chỉ tạo `u` SHADOW không cấp lực.

| seq | source | goal L/R RAW | M8 result | M8 elapsed ms | REAL frames | peak v L/R RAW/s | final M8 error L/R RAW | H0 post-DONE reason, ms |
|---|---|---|---|---:|---:|---|---|---|
| 1 | 3 Voice position | 1716 / 507 | 2 DONE | 1550 | 19 | 277 / 240 | −2 / −4 | 1, 3000 |
| 2 | 3 Voice position | 2160 / 906 | 2 DONE | 3098 | 37 | 324 / 296 | +5 / +3 | 1, 3003 |
| 3 | 3 Voice position | 1431 / 247 | 2 DONE | 2954 | 36 | 370 / 324 | +2 / −3 | **3, 1180** |
| 4 | 2 Voice CENTER | 1886 / 662 | 2 DONE | 3110 | 38 | 407 / 333 | +4 / +5 | 1, 3010 |

### Data quality / movement observations

- **4/4** REAL_BEGIN/REAL_END `result=2`; **130/130** FRAME present (19+37+36+38), every `i` contiguous, uniform 84ms spacing, every frame `ageL/ageR=0..1ms`, `nc=0`, `overflow=0`, `bad=0`, `stale=0`, `skipped=0`, `dropped=0`; `shadowOnly=1`. No measured lost frames. Max stored 38, far below 80.
- M8 measured CCR peak L/R = **300/100** on all 4 runs; this is *drive output*, not H1 output. Some later `ccr=0` samples have position state MOVING/quiet, followed by brief fine corrections, not necessarily completed DONE. Example seq2 LEFT t=2858ms `ccrL=175` while `eL=+6`; seq4 LEFT t=2606/2858 `ccrL=175` while eL≈+7/+6. The 84ms recorder cannot resolve exact fine-pulse width; do not tune pulse duration from these sparse frames.
- Position error after M8 DONE is within **±5 RAW** both sides for every run; peak filtered movement speeds ranged L=277..407 and R=240..333 RAW/s (different path lengths/conditions, not stand-alone motor datasheet ratings). No persistent large final overshoot is observed. Seq3 LEFT crosses goal by roughly **2 RAW** at sampled t=2690ms; modest overshoot is observed, not proof of perfect transient control.
- **H1 SHADOW on REAL motion is nonzero**: `peakCmdL=40, peakCmdR=40` for all four runs; 69/130 frames LEFT and 72/130 RIGHT reach ±40 arbitrary shadow-unit saturation. This diagnoses **a controller design ceiling, not an over-current measurement**.
- **Critical: damping term dominates far from goal on real motor movement.** Seq2 at t=1178ms: LEFT e=+150 RAW, v=+305 RAW/s, `uL=-7` although actual M8 `ccrL=300`; RIGHT e=+116,v=+268,uR=-23,ccrR=88. Seq4 t=1010ms LEFT e=+180,v=+361,uL=-5 while `ccrL=300`. H1 candidate proposes an **opposite sign of position error** while M8 is still driving normally. With `Kp_q8=256` and `Kd_q8=128` (effective Kd/Kp 0.5s), sign flip expected near `e≈0.5v` counts; **do not map current u_shadow to real PWM/DIR or claim it is already tuned for H2**.
- The per-axis `brakeL/R` 59..88 and `reversalL/R` 1..3 reported in REAL_END are *per-sensor-feed, calculated SHADOW diagnostics*, not 59..88 actual motor brake pulses. Actual motor DIR was not logged. Velocity data on M8 move indicates response under active drive, not passive response to external force after DONE.
- Post-DONE H0/H1 `peakCmd=0`, `peakVel=0` for four logs, and H0 drift max 0..1RAW. **seq3 H0/H1 `reason=3` after 1180ms**: observer aborted on a STOP/safety-neutral/STOP-mode/monitor-disable condition, not on a 3000ms complete window. Other three reason1. This field by itself **does not determine whether physical STOP was pressed or the exact trigger**, so do not label seq3 H0 static complete or a confirmed NC fault. Its REAL movement `result=2` remains valid.

### Evidence-based decisions (no new actuator code)

1. **H1 REAL DYNAMIC DATA CAPTURE: PASS 4/4**; H1 source/host CI and earlier static sensor-filter also PASS. We have demonstrated PD SHADOW calculation and filtered velocity with real AS5600 movement, **not** H1 force/hold active PASS.
2. **H1 gains/cap are uncalibrated for physical H2.** Current D component / 40-unit saturation would propose early sign reversal during transit. Segregate transit M8 from any prospective post-DONE holding controller; do not replace M8 owner, disable existing NC/STOP or route shadow candidates directly to H-bridge.
3. To design actual position hold, still need **external disturbance/release under physically safe conditions**, motor current and thermal/stall constraints, driver PWM=0 coast/brake behavior, DIR polarity and PWM→torque relation. These cannot be inferred from the 130 normal-motion samples. Changing gains based solely on them risks unsafe actuation.
4. **Next engineering improvement without requesting repeat of these four runs:** analyze M8 free-coast/quiet fine-pulse segments and motor response asymmetry offline; require explicit safety gates for owner/NC/STOP and bounded torque/current before any H2. An H1 read-only reason-bitmask for abort3 and finer post-DONE observation may help only if a specific new test is later justified.

