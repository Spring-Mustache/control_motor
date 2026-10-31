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

## Sau khi CI

1. Kiểm tra GitHub HEAD chính xác của **cả hai nhánh** và CI S15 đúng SHA.
2. Trên Windows lấy nhánh M8-S15 mới (không `git merge --ff-only` vào nhánh cũ đã rewind).
3. STM32CubeIDE Clean Build và xác minh SHA/ELF/flash thực tế.
4. Chỉ vận hành D-pad LEFT ON trên giá khi có cắt nguồn motor độc lập, NC/STOP, đo current, coast, clearance; theo dõi `steer_s15_last` và `M8S00_S15`.
5. Kết quả PASS cần cả hai tới đúng góc và cùng T trong dung sai đo được, không chỉ skew nhỏ. Nếu không đủ thời gian do PWM cap/nguồn/cơ khí phải giữ lỗi báo cáo, không tự nâng trần.
