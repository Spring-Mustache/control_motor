# M8-S15 — Một đồng hồ thời gian, hai góc TARGET đánh lái độc lập

**Ngày 2026-10-31. Firmware branch:** `feature/m8-s15-dual-target-common-time`.

## Yêu cầu bất biến của milestone

LEFT và RIGHT có thể tới hai góc khác nhau (ví dụ LEFT=+35°, RIGHT=+30°), bắt đầu ở hai góc riêng, nhưng **đều phải bám một quỹ đạo thời gian chung và hoàn thành trong T**. Về sau bộ động học Ackermann sẽ tạo hai góc mục tiêu riêng theo thời gian; không ép hai góc bằng nhau, PWM bằng nhau, hoặc tốc độ góc bằng nhau. Nếu T không khả thi theo điều kiện motor/an toàn, cần báo chứ không vượt cap. Mốc thuật toán chưa chứng minh sẽ đạt T bằng vật lý.

## Nhánh và rollback

- Nhánh S15 được tạo từ chính HEAD S14.10 `0bdb4412a1b37c6397391eb5dd9802d921df32ff`; giữ đủ lịch sử PI/S149/S1410 và docs.
- Nhánh cũ `feature/m7-s15-ps2-controls-restoration-staged` theo yêu cầu người dùng phải quay về `bcf6c2a4e122de91ee8fdb864f5fe7cfd10cca5d`, parent của commit đầu tiên S14.7 `21e3f353d724a5d3f8544a942a8789c0c6775b09`.
- Việc di chuyển ref cũ cần force-with-lease sau khi S15 đã được bảo toàn, không xóa commits cũ khỏi nhánh S15.

## Những gì source mới thực sự triển khai

- `Core/Inc/steer_s15_common_time.h`: C thuần `M8S15_Init`, `M8S15_Step`; nhận `startL, goalL, startR, goalR, T` cdeg. (s(u)=3u^2-2u^3), (u\in[0,1]), **chung u(t)**, nhưng `thetaL = startL+(goalL-startL)*s`, `thetaR = startR+(goalR-startR)*s`. Mỗi trục có vận tốc mục tiêu và PWM feedback riêng.
- Auto T dựa trên biên vận tốc mô hình 6500 cdeg/s và 150ms dự phòng; T trong [1200,4200]ms. T yêu cầu ngắn hơn thời gian tối thiểu mô hình bị từ chối. Mô hình này CHƯA được hiệu chuẩn để đảm bảo cả hai motor thật đến đúng T. Đặt `steer_s15_common_time_ms=0` để auto T (khuyến nghị cho pilot).
- Bên motor nhanh có thể được giảm PWM để đồng bộ đồng hồ; bên chậm được tăng trong dư trần đã có: cap L450/R200 Level2, +12 CCR tăng mỗi vòng, **không có tăng thêm dưới 180RAW**, giữ nguyên giảm tốc owner trong 100RAW cuối và không hồi sinh PWM0. Bộ điều khiển S15 trực tiếp quyết định candidate; **KHÔNG SUM** S147 PI + S148 Hybrid + S149 early lead + S1410 shadow.
- `steer_s15_common_time_enable=0` lúc boot, latch chỉ cho **lệnh D-pad LEFT** Level2 vừa bấm sau neutral, khi tay lái đã được arm an toàn bằng quy trình cũ. Cơ chế per-axis `Steering_ManualAxisUpdate` vẫn có riêng sensor/direction/NC/SOFT/stall; S15 lấy các góc từ `Steering_AngleFromRaw`, tốc độ lọc theo timestamp riêng đã có; chạy **trước lớp atomic late-IRQ/CCR** không thay đổi. Bộ thống kê `M8S00_S15` ghi ref/err/candidate/applied và T thực.
- **Giới hạn triển khai hiện tại:** Bộ lập quỹ đạo C hỗ trợ các góc khác nhau và dấu khác nhau, nhưng **đường chủ động phần cứng mới chỉ dùng hai SOFT TARGET hiện có của D-pad LEFT**. Chưa đấu đầu vào góc bất kỳ do động học vào owner để dừng ở các góc trung gian; không được tuyên bố tính năng đó đã hardware-ready. R1/R2/Voice/RX, các mode còn lại KHÔNG thay đổi.
- Safety: steering motion permit/manual arm boot0, PS2 neutral, NC/EXTI, STOP, SOFT, stall, driver independent supply cutoff and test fixture all still required. Không cần PI OFF test: S15 mode chỉ test controller ON. Không được thay đổi giới hạn điện/cơ theo suy đoán.
- Telemetry: `M8S00_S15` chỉ dump sau disarm, dùng `M8S05/M8S06/M8S11` để so sánh từng trục đến SOFT, matched angle phase, timing and faults. HOST unit test `tools/test_m8_s15_common_time.c` validates 35°/30° with same clock, opposite signs, reject unrealistic T, no positive terminal boost, cap, dead owner; `tools/verify_m8_s15_common_time.py` and CI workflow enforce integration. CI PASS ≠ CubeIDE clean target build, flash, motor gate/current/coast/NC clearance or physical common-arrival PASS.

## Kết quả pilot ON 19:34–19:35 ngày 2026-10-31

- **Phiên bản nguồn:** HEAD GitHub nhánh S15 khi nhận log là `cee4d8bce562d6c77c36dbe0991caab28b4d8d2b`; Actions run `41200962485` PASS cùng SHA, gồm contract 386 dòng và S15 GCC/static tests. Chưa có CubeIDE Clean Build log hoặc ELF/Flash SHA; UART cho thấy firmware có tính năng S15 đã chạy trên MCU nhưng không chứng minh chính commit này đã nạp.
- **File riêng tư, không đưa raw lên GitHub:** `logs/M8S11_six_motor_20261031_193425.csv` và `.meta.txt`, `logs/M8_S00_20261031_193556.txt` trên máy người dùng. CSV 1071×60, board ms 192384–250923 (58,539 s); metadata `complete=True`, captured_rows/rows=1071, missed/busy/tx_errors/invalid_lines/sequence_missing=0. M8-S00: `M8S00_S15 enable=1,last=0,started=7,frames=844,skip=7,rejectedT=0,T=1200`. `last=0` do lệnh khác xảy ra sau lệnh S15; counters chứng minh S15 đã thực thi. T=1200 chỉ là session cuối được lưu, không phải T từng stroke.
- **Kết quả D-pad LEFT:** 7 stroke trong CSV, 6 đạt `manual_result_l=manual_result_r=11` tại SOFT, 1 nhả sớm. Hai stroke đủ MAX→MIN từ L~2232–2237/R~1006–1007 có thời gian từ mẫu đầu lệnh đến SOFT theo CSV như sau:

  | Board ms bắt đầu | LEFT tới SOFT | RIGHT tới SOFT | RIGHT trễ LEFT |
  |---:|---:|---:|---:|
  | 220532 | 1995 ms | 2443 ms | 448 ms |
  | 227216 | 1938 ms | 2607 ms | 669 ms |

  M8-S06 `id=11` cho lượt thứ hai ghi LEFT1960/RIGHT2588ms, skew628ms theo timer firmware; CSV lấy mẫu ~50–57ms nên chênh hàng chục ms. Lượt `id=15` CENTER→MIN ghi LEFT1117/RIGHT1766ms, skew649ms; không trộn với MAX→MIN để tính trung bình đối chứng.
- **Phân rã vùng cuối từ chính hai stroke MAX→MIN:** tại mốc mỗi trục còn 180 RAW, RIGHT mới trễ LEFT **114/57ms**. Đoạn 180→SOFT mất LEFT **570/513ms**, RIGHT **904/1125ms**; tại SOFT lệch thành **448/669ms**. RIGHT CCR ở 100–180 RAW đều bằng 100 trong các mẫu này, còn 0–40 RAW trung bình **58/56,6CCR**. Đây là phép lấy mẫu UART trên các mốc RAW, không phải phép đo dòng, lực hoặc quãng trôi sau CCR0. Source S15 cấm tăng PWM trong 180 RAW cuối, vì vậy độ lệch chủ yếu phát sinh ở vùng S15 không thể boost; không tự nới taper khi thiếu clearance/current.
- **Diễn giải:** planner cùng đồng hồ đã chạy nhưng các đích SOFT **không đạt đồng thời** ở các lượt có dữ liệu. M8-S00 không lưu T/ref/error riêng mỗi stroke và M8-S11 không có cột S15 per-row, nên chưa tách được sai số lập lịch, thiếu headroom, taper terminal và động học cơ khí. S14.10 ON trước đó báo skew khoảng255ms trong ba lượt MAX→MIN; hai lượt S15 trên gợi ý kết quả kém hơn, nhưng khác phiên và thiếu đo điện/cơ nên chưa kết luận quan hệ nhân quả. Không có thử điều khiển hai góc target bất kỳ trên phần cứng; 35°/30° chỉ là HOST test.
- **Bảo vệ quan sát được và giới hạn:** M8-S00 `FAULT` rỗng; trong CSV `trip=0`, `stop=0`. `nc=4/6` chỉ xuất hiện trong R2, không thuộc D-pad LEFT; không gọi đó là S15 fault. Bốn bánh có target khác 0 ở 31/1071 hàng nhưng không trong bảy stroke LEFT. Không có dòng điện, nhiệt, quãng trôi sau CCR0, khoảng hở cơ khí tại NC hoặc xác minh nguồn ngắt độc lập. Vì thế không đóng Gate A/B và không chỉnh firmware/PWM/T chỉ từ kết quả này.

## Sau khi CI

1. Kiểm tra lại HEAD chính xác của **cả hai nhánh** và CI S15 đúng SHA; bảo toàn các thay đổi CubeIDE và log cục bộ khi đồng bộ Windows.
2. Đối chiếu CubeIDE Clean Build, ELF và SHA đã flash nếu có bằng chứng; log UART chỉ xác nhận tính năng S15 đang chạy, không định danh commit đã nạp.
3. Phân tích RAW/CCR/timing của log hiện có. Để quyết định sửa planner hay duty, cần thêm T/ref/error theo từng stroke hoặc phép đo tương đương, cùng chứng cứ nguồn ngắt độc lập, current, coast và NC clearance. Không tự lặp test powered khi gate này chưa xác minh.
4. Khi đủ gate, chỉ thử controller ON trên giá với lệnh và vị trí đầu được ghi rõ; PASS cần cả hai tới đúng góc và cùng T trong dung sai được xác lập, không chỉ mỗi bên chạm SOFT. Nếu không đủ thời gian do PWM cap/nguồn/cơ khí phải giữ kết quả chưa đạt, không tự nâng trần.
