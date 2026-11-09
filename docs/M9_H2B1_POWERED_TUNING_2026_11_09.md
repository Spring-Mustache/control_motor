# M9 H2-B1 — Thử giữ góc có cấp lực, hai trục liên tục (09/11/2026)

> **Cập nhật mới nhất:** **2026-11-09 — HOLD tự thử lại có giới hạn (SOURCE/HOST, CHƯA THỬ TRÊN XE):** USER xác nhận đẩy lệch rồi thả tay nhưng bánh không quay lại, chưa có số đo dòng; yêu cầu luôn cố gắng tạo lại phản lực và không muốn chờ đo dòng. Đã đổi riêng NO_PROGRESS11/EFFORT_BUDGET12 thành COOLDOWN6/active1: giữ nguyên hai goal, đưa cả hai PWM về0, hồi đủ ngân sách output rồi tính lại điều khiển từ góc mới và tự thử tiếp. Không tăng trần30/10CCR, không thêm hoặc tuyên bố có giới hạn dòng. STOP/NC/sensor/range/owner vẫn hủy goal, không tự khởi động lại. Đây là triển khai có giới hạn của yêu cầu; không phải cấp lực vô hạn hoặc đã sửa được phản ứng cơ khí. Xem docs/M9_H2B1_PERSISTENT_RETRY_2026_11_09.md. Các mục cũ nói11/12 hủy goal là lịch sử của firmware621a5ce.

> **2026-11-09 — First MCU powered-tuning register-output/cutoff evidence (NOT HOLD response PASS):** private UART M9H2B1_HOLD_20261109_172834.txt has3 complete rows, SHA621a5ce87385600314404bc8866662c0102f4df4 seq1/source2 CENTER/profile1. WAIT_DATA at37ms -> HOLDING at109ms -> FAULT reason11(NO_PROGRESS) at1868ms, active0/lease0/faultAxes1. LEFT peakCCR30, acknowledged nonzero output1007ms, final/peak error23RAW (goal1886,raw1909); RIGHT peakCCR/output-time0, peak error3RAW. Firmware cut both on its response guard, not a captured Voice STOP. Driver torque, release timing, current and thermal result are not established. No automatic PWM increase; next required observation is whether LEFT was released promptly or still externally held during the no-progress interval. Docs-only update: keep MCU621a5ce, do not reflash or repeat old static logs for this record.

## Quyết định hiện hành

USER yêu cầu test thực tế với STOP/NC hiện có, không thực hiện phép thử ngắt nguồn motor làm điều kiện tiên quyết. Yêu cầu này thay thế đề nghị kiểm tra ngắt nguồn của assistant. Không ghi việc người dùng từ chối phép thử thành một kết quả ngắt nguồn PASS hoặc thành giới hạn motor đã được xác nhận.

Sản phẩm vẫn là H2-B1 giữ hai mục tiêu liên tục sau DONE; không đổi sang chương trình một trục, các xung open-loop hoặc một phiên tự hết hạn3s. Bản này mở **profile1=TUNING có cấp lực**. Profile0=LOCKED; profile2=COMMISSIONED cần ghi nhận nghiệm thu vật lý riêng. POWER_COMMISSIONED vẫn0 vì chưa có dữ liệu dòng/nhiệt HOLD; số0 này **không còn đồng nghĩa PWM bị khóa khi profile=1**.

## Hành vi và cấu hình thử

- Boot/reset chưa có goal: HOLD OFF, không phát lực. Sau cả hai M8 DONE hợp lệ của PS2 R1/Voice CENTER hoặc Voice5..40 mới nhận hai goal. Không tự lấy góc khi nhả manual, không giữ ở NC R2/Voice45.
- Vùng yên: không ép motor ăn dòng thường trực. Sai số đạt8RAW mới chốt hiệu chỉnh; nhả chốt khi còn4RAW, deadband5RAW. Nếu cơ khí tự giữ trong vùng yên, PWM0 là đúng.
- Hai trục có output riêng, không có thời hạn phiên HOLD. STOP, owner lái mới, NC, lỗi cảm biến hoặc lỗi tuning cắt cả hai và hủy goal; không tự phục hồi từ goal lỗi.
- Luồng bánh thường và các lệnh M8 đã có được giữ nguyên. Chưa nghiệm thu phối hợp giữ/chạy dưới tải hoặc H3; không thêm lệnh Voice/PS2 mới.

| Thông số | LEFT | RIGHT |
|---|---:|---:|
| Trần PWM thử | 30CCR =3% | 10CCR =1% |
| Kp (CCR/RAW) | 2 | 0,75 |
| Kd (CCR/(RAW/s)) | 0,125 | 0,046875 |
| Tốc độ tăng output | 60CCR/s | 20CCR/s |
| Khoảng PWM0 khi đổi chiều | 200ms | 200ms |

Đây là lựa chọn kỹ thuật ban đầu cho việc hiệu chỉnh, **không phải thông số motor hoặc cấu hình đã nghiệm thu**. Trần thấp hơn CCR đỉnh300/100 từng ghi trong H1/M8; quan hệ này không chứng minh dòng/nhiệt an toàn khi đứng giữ. 200ms là khoảng chặn phần mềm, không phải số đo driver đã dừng dòng hay đã phanh. Không chuyển score SHADOW H1 thành PWM.

### Bảo vệ profile thử

- Khi đang có PWM thực sự được adapter ghi nhận và sai số còn ngoài4RAW: nếu sai số không giảm thêm ít nhất2RAW trong1000ms, cắt cả hai, reason11=NO_PROGRESS. Nghỉ PWM/đảo chiều bắt đầu lại cửa sổ tiến triển; bộ tích lũy effort dưới đây vẫn giữ lịch sử.
- Mỗi trục có ngân sách1500ms output khác0. Khi output0, ngân sách hồi dần theo tỷ lệ1500/8500; hồi hết sau8500ms nghỉ nếu đã dùng đủ. Không reset ngân sách tại ranh giới thời gian cố định. Hết ngân sách thì cắt cả hai, reason12=EFFORT_BUDGET. Việc phát hiện phụ thuộc nhịp service phần mềm; đây không phải ngắt phần cứng đúng tới từng ms.
- Các ngưỡng trên là giới hạn hoạt động thử, không phải giới hạn ampere, I2t hoặc cảm biến nhiệt. Không có ADC dòng/nhiệt được thêm vào firmware.
- Sai số trên32RAW, dữ liệu stale/jump, mất quyền, STOP/NC/pending EXTI và các điều kiện cũ vẫn thu hồi quyền. Không tự tăng PWM khi không đủ lực. Compile/runtime từ chối tăng trần thử quá30/10, bỏ guard hoặc chọn profile2 khi chưa có chốt nghiệm thu.

Vì thế HOLD có thể tồn tại nhiều phút khi hợp lệ; một tình trạng cấp lực liên tục nhưng không sửa được góc hoặc dùng hết ngân sách sẽ được coi là lỗi cần đánh giá. Đây không phải timer tự kết thúc mọi phiên giữ.

## Cập nhật và nạp

Giữ project D:\STM32 task\control_motor; không reset/clean Git, không tạo worktree, không áp lại stash, không ghi đè các thay đổi .settings/startup đang có. Sau khi Actions trên đúng commit PASS:

    cd "D:\STM32 task\control_motor"
    git status --short
    git fetch origin
    git merge --ff-only origin/feature/m9-steering-active-position-hold
    git rev-parse HEAD

Nếu Git báo xung đột hoặc sẽ ghi đè file, giữ nguyên và xử lý đúng thông báo; không dùng force/reset để vượt qua. Trong CubeIDE: Refresh → Clean Build → Debug/Flash → Resume F8. Ghi nhớ SHA vừa nạp để so với UART. Không Pause/step CPU khi đang cấp lực.

## Một lượt thử thật

1. Cố định xe trên giá đang dùng, để bánh dẫn động không chạm đất; giữ PS2 kết nối và thao tác STOP hiện có trong tầm sử dụng. Không phát lệnh chạy bánh ở lượt hiệu chỉnh đầu. **Không yêu cầu thử tắt nguồn motor.**
2. Mở bộ thu trước khi phát lệnh:

        powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\capture_m9_h2b1_hold.ps1" -TimeoutSeconds 300 -MaxEvents 100 -UntilStopped -OutputDirectory ".\logs"

3. Cho xe thực hiện Voice CENTER đã kiểm chứng. Chờ tới khi báo H2B1 profile1/state3/active1 và motor đã yên. Trong vùng yên PWM0/everApplied0 là bình thường; chưa có bằng chứng cấp lực giữ.
4. Tạo nhiễu nhẹ, từ từ ở phần lốp có thể tiếp cận trên giá để có độ lệch góc rồi nhả. Tránh thanh liên kết, bánh răng và vùng kẹp; không nắm trục motor hoặc cố ghì cơ cấu không nhúc nhích. Cả hai vòng HOLD luôn hoạt động; có thể quan sát phản ứng từng bên trong cùng một phiên. Nếu cơ cấu không cho dịch chuyển nhẹ, không tăng lực để ép.
5. Quan sát phản lực giữ và khả năng trở về góc, hiện tượng rung/hunting, tự quay hoặc không về. Nếu báo FAULT, lực không phù hợp hoặc phản ứng bất thường, dùng STOP và kết thúc lượt; không tự tăng cap/bỏ guard. Trần khởi đầu có thể chưa thắng ma sát, reason11 không tự chứng minh lỗi cảm biến.
6. Sau khi quan sát hai bên, dùng STOP hiện có. Bộ thu thoát khi nhận terminal state4/5. Không phát ngay lệnh mới trước khi nhận dòng kết thúc vì trạng thái chờ có thể bị thay thế. Hết thời gian hoặc đóng PowerShell **không dừng HOLD**.
7. Gửi TXT riêng trong hội thoại. Kèm dòng cao nhất đồng hồ đã đọc được ở dây driver→motor, bên đã đo và quan sát rung/trở về. Chỉ ghi giá trị quan sát, không gọi đó là dòng RMS/peak đã xác nhận. Nếu thiết bị đo đã mắc sẵn thì giữ nguyên; không thay đổi dây đo khi đang phát lực. Nhiệt độ chỉ ghi khi thực sự đo được.

Không cần lặp H1, H2-B0 hoặc log chưa cấp lực4d004757 đã đạt. Nếu lượt này chưa có nhiễu đủ để PWM khác0, ghi nhận chưa kích hoạt lực; không gọi đó là HOLD powered PASS.

## Đọc kết quả

| Trường | Ý nghĩa |
|---|---|
| profile1, commissioned0 | Được phép cấp lực theo cấu hình thử; chưa nghiệm thu dòng/nhiệt |
| state3/active1 | Phiên HOLD còn hiệu lực |
| peakErrL/R | Sai số đỉnh kể từ khi nhận goal, đơn vị RAW |
| peakPwmL/R | Đỉnh yêu cầu PWM; không phải số đo mô-men/dòng |
| appliedLms/Rms, appliedMs, everApplied | Thời gian/việc adapter đã ghi output khác0; không chứng minh driver/motor thật đã tạo mô-men |
| effortL/R | Mức ngân sách thời gian output đang sử dụng, có hồi dần khi PWM0 |
| faultAxes | Bit0 LEFT, bit1 RIGHT: trục đầu tiên gây lỗi tuning được báo |
| state4/reason2/active0 | STOP đã thu hồi phiên ở phía phần mềm |
| state5/reason11 hoặc12 | Không tiến triển hoặc hết ngân sách thử; cả hai bị cắt, cần goal mới |

Reason3=NC,4=SENSOR,5=RANGE,9=SERVICE_GAP,10=PORT_FAULT vẫn giữ nghĩa. UART chỉ gửi khi CCR hai bên và bánh đã idle; trong lúc đang phát lực có thể chưa có dòng mới. Các thống kê tích lũy được xuất khi output yên hoặc sau STOP/fault. M9H2_GUARD là observer H2-A/B0 riêng; powered0 trong bản ghi đó không mô tả quyền cấp lực H2-B1 mới.

Để kết luận giữ có lực đạt phải phối hợp log ghi output, góc thật trở về/ổn định, quan sát cơ khí và số đo điện/nhiệt phù hợp. Không suy torque hoặc giới hạn motor chỉ từ CCR. [Maxon: bảo vệ motor và quá tải](https://support.maxongroup.com/hc/en-us/articles/360019707620-Why-can-the-usage-of-a-motor-controller-be-an-advantage-even-for-a-DC-motor) và [đo dòng khi dùng PWM](https://support.maxongroup.com/hc/en-us/articles/360012273994-Motor-current-measurement-in-case-of-a-PWM-power-amplifier) được tham khảo về nguyên tắc; không gán thông số motor Maxon cho DC775 trên xe.

## Bằng chứng phần mềm và giới hạn hiện tại

Local strict GCC PASS: core hai trục liên tục120s, HAL/main hooks60s; **chính profile30/10** đi qua adapter/main thật, phát hai output có giới hạn và cắt không tiến triển; budget recovery/tick wrap, invalid config, compiler rejection và formatter UART thật PASS. ASan/UBSan core PASS với leak detector tắt vì hạn chế môi trường. H0/H1/H1REAL/H2-A/B0 và Voice executor/FF/PS2/auto-ARM regressions PASS.

Packet H2B1 worst-case545bytes trong buffer640bytes. Không thêm I2C, UART blocking hoặc ghi motor vào host tests. GitHub Actions phải được so đúng commit chứa thay đổi; Windows parser kiểm script mới. Máy thực hiện không có arm-none-eabi-gcc; **chưa cross-build CubeIDE, flash hay chạy motor bằng bản này**. File UART4d004757 trước đó chỉ xác nhận hai phiên chưa cấp lực; chưa có H2-B1 powered HARDWARE PASS. Động học chưa hoàn thành để tích hợp/nạp và SELECT tiếp tục hoãn.

## 2026-11-09 — Lượt MCU đầu tiên của profile cấp lực thử

Nguồn riêng tư: M9H2B1_HOLD_20261109_172834.txt; chỉ công khai tổng hợp, không đưa TXT gốc lên repo. Đã kiểm tra đủ3dòng cùng firmware621a5ce87385600314404bc8866662c0102f4df4, seq1/source2 CENTER/profile1; CRLF thật3, không có literal-backslash separator, dropped0. Quan hệ goal-raw=error khớp cả hai bên.

| Mốc | elapsedMs | state/reason | active/lease |
|---|---:|---|---|
| Khởi tạo vận tốc | 37 | 1/0 WAIT_DATA | 1/0 |
| Cho phép HOLD | 109 | 3/0 HOLDING | 1/1 |
| Cắt bảo vệ | 1868 | 5/11 NO_PROGRESS | 0/0 |

| Dữ liệu | LEFT | RIGHT |
|---|---:|---:|
| Goal RAW | 1886 | 662 |
| RAW báo cuối | 1909 | 660 |
| Sai số cuối RAW | -23 | +2 |
| Sai số đỉnh RAW | 23 | 3 |
| Đỉnh yêu cầu PWM CCR | 30 | 0 |
| Thời gian adapter ghi nhận output khác0 | 1007ms | 0ms |
| PWM báo cuối | 0 | 0 |

everApplied1, appliedMs1007, effortL/R1008/0, faultAxes1=LEFT. RIGHT chưa vượt ngưỡng kích hoạt8RAW nên output0 phù hợp; chưa có bằng chứng phản lực giữ RIGHT. LEFT được ghi CCR khác0 và đạt cap30; guard không thấy mức giảm sai số ít nhất2RAW trong cửa sổ1000ms khi còn phát lực nên cắt cả hai. Đây là xác nhận đường output/guard ở phía MCU của một lượt, **không chứng minh driver đã tạo mô-men hoặc lực giữ đã đạt**.

File kết thúc bởi FAULT11, không phải terminal STOP2. Không suy rằng khẩu lệnh Stop hỏng nếu nó được nói sau khi collector đã nhận FAULT và thoát. Không có trace vị trí dày theo thời gian hoặc thông tin nhả tay, dòng/nhiệt trong TXT này.

**Điểm cần làm rõ trước thay đổi PWM/gain/timeout:** người vận hành có nhả tác động lên bánh LEFT ngay sau khi tạo lệch, hay còn giữ nó trong khoảng guard đang đo không tiến triển? Nếu đã nhả mà không có hồi phục thì mức lực hữu hiệu/cấu hình cần được đánh giá với quan sát dòng; nếu vẫn đang giữ thì chưa phải phép đo hồi phục sau nhả tải. Log tự nó chưa phân biệt hai tình huống và không xác nhận một nguyên nhân vật lý duy nhất.

**Kết luận:** log hợp lệ, có MCU register-output LEFT và cắt NO_PROGRESS; H2-B1 HOLD response/current/thermal vẫn chưa PASS, RIGHT chưa được kích hoạt lực. Không tự nâng cap hoặc bỏ bảo vệ chỉ từ NO_PROGRESS. Bản ghi này không thay source, không yêu cầu nạp lại hoặc thu lại H1/H2-B0/unpowered B1 đã xác nhận.

