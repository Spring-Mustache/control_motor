# M9 H2-B1 — Trợ lực ngắn để thắng ma sát (09/11/2026)

> **2026-11-09 — MCU log184953: trợ lực LEFT đã phát, chưa về center:** 13dòng hợp lệ trênb89b4d5c45c6173f5d726b9b1d9f7f755af05fe1. Seq3 LEFT kick0→4→8→12,peak175CCR,sai số tại các nhóm−89→−83→−78→−78RAW;cuối23053ms state6/reason13/active1,retry3,appliedLms3916. Xác nhận đường software/adapter burst hoạt động và budget4đợt nghỉ có giữ goal; không chứng minh mô-men,thời lượng điện hoặc tự về center. RIGHT sai số<=2RAW,output/kick0,chưa thử lực. Seq4 là CENTER mới,gần0sai số rồi state4/reason1=NEW_OWNER;không dùng làm bằng chứng HOLD tự hồi hoặc STOP2. Cần quan sát cơ khí bên trái nhích/giật theo burst hay đứng yên trước chỉnh tiếp. Docs-only:giữ MCUb89b4d5,không flash/lặp phép thu chỉ để chứng minh kick đã kích;không tự nâng lực từ file.

## Mục tiêu và căn cứ

Người dùng yêu cầu thực hiện bước tạo phản lực sau log181422. MCU32520fe đã giữ goal khi lệch149RAW,tự retry4lần nhưng LEFT cuối vẫn lệch138RAW dù adapter ghi5166ms output,đỉnh30CCR. RIGHT chưa phát lực. Kết quả không chứng minh nguyên nhân duy nhất là ma sát; PWM thấp chưa tạo hiệu quả kéo về là điều cần thử cải thiện. Không đăng log gốc riêng tư.

Bản này thêm trợ lực thắng ma sát bên trong owner HOLD hai trục liên tục. Không đổi sang chương trình một trục hoặc chuỗi xung không có feedback. Mức và thời lượng lấy từ source tinh chỉnh CENTER hiện có: LEFT175CCR/30ms,RIGHT85CCR/12ms. Đó là cơ sở chọn thử,không phải chứng nhận motor chịu HOLD liên tục hoặc dòng/nhiệt đã an toàn. Người dùng không chọn cụ thể các con số; đây là lựa chọn kỹ thuật của assistant theo yêu cầu triển khai.

## Cấu hình và hành vi

| Tham số | LEFT | RIGHT |
|---|---:|---:|
| Trần PD nền | 30CCR | 10CCR |
| Trợ lực ngắn | 175CCR=17,5% | 85CCR=8,5% |
| Thời lượng burst | 30ms | 12ms |
| Đứng yên trước trợ lực | 200ms,abs(v)<=20RAW/s | 200ms,abs(v)<=20RAW/s |
| Nghỉ quan sát sau burst | PWM0>=200ms và có mẫu mới | PWM0>=200ms và có mẫu mới |
| Số burst mỗi vòng | <=4 | <=4 |

Chỉ trợ lực khi sai số>=8RAW,PD đang yêu cầu kéo đúng về mục tiêu,không trong đổi chiều và có mẫu góc mới sau cửa sổ đứng yên. Không trợ lực ở boot,chưa BOTH-DONE,trong vùng yên,đang được chủ khác điều khiển hoặc có lỗi an toàn. Khi cần damping,vượt qua goal hoặc sai số vào vùng ra4RAW,burst được kết thúc sớm. Các khoảng đổi chiều PWM0>=200ms vẫn giữ.

Sau tối đa4lần trợ lực thực sự được adapter ghi nhận,một yêu cầu trợ lực tiếp theo đưa cả hai trục vào state6/reason13=ASSIST_BUDGET. Giữ goal và nghỉ ít nhất3000ms,đồng thời phải hồi hết ngân sách output của cả hai trục mới retry. Không bỏ goal hoặc tự tăng biên độ/thời lượng khi chưa thành công. Khi góc đang chuyển động,điều khiển PD nền vẫn làm phần việc của nó; không ép burst vào mọi bước.

## Cắt burst và quyền actuator

M9B1_HAL_Pwm trả lại giá trị thực đã ghi. Output trên cap PD chỉ được chấp nhận khi core đang ở pha trợ lực đúng trục/đúng biên độ. Lần ghi đầu tiên mới arm deadline; những lần ghi lặp không gia hạn. SysTick_Handler thật gọi M9B1_PulseTick sau HAL_IncTick: khi hết hạn chỉ ghi CCR0 cho kênh được arm,không ghi DIR,không đọc I2C,không gửi UART và không tự cấp lực.

Nếu vòng main đang bận nhưng tick/IRQ còn chạy,burst vẫn được cắt theo deadline. Cờ expired chặn ghi lại mức cao cho tới khi core đi qua pha nghỉ. STOP/NC thu hồi output và disarm; deadline cũ không được cắt PWM của owner mới. Kiểm cả tick wrap bằng HOST.

Đây là bảo vệ thời gian bằng phần mềm,tùy thuộc tick/IRQ hoạt động. Nó không phải ngắt nguồn độc lập,không đo dòng/nhiệt và không bảo đảm khi CPU bị halt hoặc IRQ bị mask kéo dài. Không Pause/step CPU lúc motor đang được cấp lực. Không thêm ADC hoặc gọi ngân sách thời gian là giới hạn dòng.

## Các bảo vệ và thống kê

Ngân sách1500ms output và hồi theo1500/8500 vẫn được giữ. Thống kê exposure là khoảng output được phần mềm/adapter ghi nhận; nếu tick cắt giữa hai lần service,phần mềm có thể tính dư khoảng tới service kế tiếp. Không dùng nó làm phép đo dòng hoặc thời lượng điện chính xác trên motor.

No-progress11 giờ tích lũy thời gian có output được acknowledge,giữ lịch sử qua các khoảng PWM0; không để burst/settle xóa lịch sử không tiến triển. Sai số giảm ít nhất2RAW,kể cả trong lúc coast,sẽ bắt đầu lại đếm không tiến triển. Các lý do11/12/13 đều nghỉ có giữ goal; STOP/NC/sensor/range/service/port/new-owner vẫn hủy goal và không tự hồi. Giới hạn sai số32RAW không quay lại; góc đo và goal vẫn qua hành trình hiệu chuẩn.

UART thêm kickL/kickR,đếm lần bắt đầu burst mà adapter đã ghi output khác0. peakPwm có thể lên175/85. Hai thống kê này không chứng minh mô-men hoặc góc thật đã quay về. Trong pha settle hoặc cooldown,PWM0 là dự kiến. Collector vẫn UntilStopped; dừng thu không dừng HOLD.

## Kiểm chứng

HOST native-C chạy cả profile mới và hồi quy profile nền cũ riêng biệt:175/85 actual mock writes,pulse30/12ms,settle,số đợt,nghỉ,retry,vùng yên,STOP và no-progress qua khoảng0. Fixture production main/HAL biên dịch chính SysTick_Handler từ file interrupt và mô phỏng tick1ms/main10ms: thời lượng burst,main ngừng service100ms,expired không rearm,STOP giữa output cao,NC,tickwrap và deadline không can thiệp owner mới. Compiler/runtime từ chối tăng cap burst,thời lượng hoặc giảm nghỉ trái cấu hình. Formatter UART thật kiểm buffer640bytes,kickL/R và toàn bộ argument types.

ASan/UBSan và hồi quy source/native workflow được kiểm riêng; CI đúng commit được xác minh sau push. HOST dùng mẫu tổng hợp và PWM mô phỏng,không phải phản lực/motor PASS. Chưa cross-build CubeIDE,flash hoặc có log mới cho burst. POWER_COMMISSIONED vẫn0,profile1 là thử có lực. MCU cuối xác nhận vẫn32520fe.

## Cập nhật và một lượt thử

Sau Actions đúng SHA PASS:

```powershell
cd "D:\STM32 task\control_motor"
git status --short
git fetch origin
git merge --ff-only origin/feature/m9-steering-active-position-hold
git rev-parse HEAD
```

Không reset/clean hoặc ghi đè .settings/startup đang sửa. CubeIDE Refresh ->Clean Build ->Debug/Flash ->Resume F8. Không kích biến Live Expressions.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File ".\tools\capture_m9_h2b1_hold.ps1" -TimeoutSeconds 300 -MaxEvents 100 -UntilStopped -OutputDirectory ".\logs"
```

Trên giá đang dùng,không phát lệnh chạy bánh ở lượt đầu,mở collector trước CENTER. Chờ motor yên,tác động nhẹ ở phần lốp tiếp cận được rồi thả; quan sát lực kéo về,rung/giật hoặc quay sai chiều. Không ghì cơ cấu để ép thử. Kết thúc bằng STOP hiện có; gửi TXT và quan sát. Không yêu cầu phép thử ngắt nguồn đã bị loại,không chặn thử source mới bằng việc hỏi lại số đo chưa có. Nếu phản ứng bất thường thì STOP,không tăng PWM hoặc kéo dài burst bằng debugger.

Tiêu chí tiếp theo:kickL/R tăng khi bên tương ứng đủ điều kiện,đỉnh output phù hợp,góc thật giảm sai số và ổn định,rồi STOP hủy hoàn toàn. Cần quan sát mỗi bên; RIGHT chưa được kích lực trong log181422. Chỉ kết luận khả năng kéo lại sau khi có bằng chứng trên xe; dữ liệu điện/nhiệt và H3 có tải còn chưa nghiệm thu.
