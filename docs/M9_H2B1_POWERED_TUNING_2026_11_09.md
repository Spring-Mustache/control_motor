# M9 H2-B1 — Thử giữ góc có cấp lực, hai trục liên tục (09/11/2026)

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
