# M9-H1 REAL DRIVE — Thu vận động motor thật để chỉnh PD (2026-11-07)

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
