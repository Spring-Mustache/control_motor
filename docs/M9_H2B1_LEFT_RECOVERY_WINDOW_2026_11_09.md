# M9 H2-B1 — Cửa sổ kéo về LEFT300CCR, tối đa200ms (09/11/2026)

## Quan sát và lý do chỉnh

Log184953 trênb89b4d5 cho LEFT12burst,peak175CCR,sai số đo89->83->78->78RAW. USER xác nhận đĩa lái đứng yên hoàn toàn,có tiếng rít;ERR và iOVER đều tắt,RUN nhấp nháy. Đây là quan sát của USER,không phải đo dòng hoặc mô-men. Không suy RUNblink bằng mứcPWM hay ERRoff loại trừ mọi giới hạn bên trong driver. ACCE/DECCE có trong manual nhưng setting trên xe chưa biết;không đổi biến trở hoặc reset cờ driver trong lần này.

Ghi chú M8 trong main.c đã nêu trường hợp LEFT tám xung175CCR/30–45ms không dịch cảm biến gần center. Assistant đã bỏ sót ghi chú này khi chọn trợ lực175/30 đầu tiên. Điều kiện góc của M8 khác lượt HOLD,nhưng cùng với phản hồi hiện tại đủ để không tiếp tục lặp cấu hình xung yếu đó. Không coi thiếu lực/thời gian là nguyên nhân duy nhất đã chứng minh.

## Candidate hiện hành

| Thông số | LEFT mới | RIGHT |
|---|---:|---:|
| Trần trợ lực | 300CCR=30% | 85CCR=8,5% |
| Cửa sổ tối đa | 200ms | 12ms |
| PD nền ngoài trợ lực | 30CCR | 10CCR |

300CCR bằng trần chạy CENTER trái hiện có trong source,không phải100%duty hoặc giá trị suy từ dòng định mức driver.200ms là cửa sổ thử do assistant chọn nhằm cho motor thời gian khởi động;chưa được hiệu chuẩn hoặc nghiệm thu dòng/nhiệt. RIGHT chưa có powered response trong log nên chưa tăng cấu hình của nó.

Đây vẫn là controller HOLD hai goal liên tục,có feedback AS5600. Chỉ trợ lực khi điều kiện đứng yên/dữ liệu mới/dấu kéo về hợp lệ. Khi goal đã đạt,vượt qua goal,PD yêu cầu hãm/đổi chiều hoặc có safety fault,cửa sổ được kết thúc sớm;không bắt buộc phát đủ200ms. SysTick cutter dùng deadline mới và không gia hạn bằng các lần ghi lặp. Không đổi cơ chế cutoff hoặc ownership đã kiểm ở bản trước.

Giữ ngưỡng8/4/deadband5,vận tốc đứng yên<=20RAW/s trong200ms,settle PWM0>=200ms và mẫu mới,<=4cửa sổ mỗi trục/vòng. No-progress11/ngân sách12 có thể cho nghỉ trước đủ4cửa sổ. Nghỉ13 hoặc11/12 vẫn giữ goal,phải >=3000ms và cả hai budget hồi0 mới retry. Giữ1500ms ngân sách output/10s recovery model;không gọi đó là giới hạn dòng. Sai số32RAW không quay lại;ngoài hành trình và STOP/NC/sensor/service/port/newowner vẫn hủy goal.

## Bằng chứng phần mềm

Native-C profile thực kiểm actualmock300/85,giới hạn200/12ms,cửa sổ/settle/round/cooldown và các hồi quy trước. Thêm ca mẫu goal mới cắt cửa sổ LEFT300/200 sớm và không tạo trợ lực khi đã yên. Fixture main/HAL/SysTick thật kiểm cùng earlygoal disarm,main không service225ms vẫn bị tick cắt ởdeadline,không rearm hết hạn,STOP/NC,tickwrap và owner mới không bị deadline cũ cắt. Compile/runtime bác bỏ301CCR và201ms. ASan/UBSan cùng CI exactSHA tách khỏi target/hardware.

Không có ARM cross-build,flash hoặc log config mới từ môi trường này. MCU gần nhất vẫnb89b4d5,không gán kết quả đó cho300/200. Source mới tăng khả năng thử lực nhưng chưa chứng minh motor đủ kéo đĩa. Bộ cắt tick chỉ cắt lệnhPWM khi tick/IRQ hoạt động,không phải ngắt dòng độc lập;trễ driver/cơ khí chưa đo.

## Cập nhật và thử

Sau Actions đúng commit PASS:

```powershell
cd "D:\STM32 task\control_motor"
git status --short
git fetch origin
git merge --ff-only origin/feature/m9-steering-active-position-hold
git rev-parse HEAD
```

Giữ .settings/startup và log đang có;không reset/clean. CubeIDE Refresh ->Clean Build ->Flash ->Resume F8. Không kích Live Expressions hoặc đổi biến trở driver.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\capture_m9_h2b1_hold.ps1" -TimeoutSeconds 300 -MaxEvents 100 -UntilStopped -OutputDirectory ".\logs"
```

Trên giá đang dùng,mở collector trước CENTER,không phát lệnh chạy bánh. Đẩy nhẹ LEFT rồi thả,quan sát motor có kéo đĩa về và có rung/giật/đảo chiều bất thường không;dùng STOP kết thúc. Không ghì cơ cấu hoặc halt/step CPU lúc cấp lực. Không yêu cầu phép thử ngắt nguồn đã bị USER loại. Gửi TXT và quan sát;không cần đo dòng để báo kết quả lượt này,nhưng nghiệm thu dòng/nhiệt vẫn chưa có.

ĐỉnhLEFT có thể đạt300,kickL tăng,goal được giữ. Chỉ kết luận phản lực/return có hiệu quả khi góc thật giảm sai số và ổn định,kèm quan sát cơ khí. Nếu vẫn không quay,không tự tiếp tục nâng PWM/thời gian hoặc tuyên bố lỗi đã giải quyết.
