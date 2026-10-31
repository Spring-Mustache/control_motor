# M8-S14.8 — Level2 mặc định và phối hợp PI/P+D boost-only

**2026-10-31 — SOURCE-STAGED.** Người vận hành yêu cầu `steer_s142_trial_level=2` mặc định. Chưa có kiểm chứng CubeIDE Clean Build, Flash đúng SHA, đo dòng, quãng trôi/clearance, nguồn ngắt độc lập hoặc Level2 HARDWARE PASS. Không công khai raw logs.

## Vì sao cần bản mới

Các lượt A/B riêng tư S14.7 Level1 PI ON/OFF cho thấy giảm chênh lệch LEFT–RIGHT không đồng nghĩa RIGHT đến đích nhanh hơn: LEFT cũng chậm đi. Code tại S14.7 thay hẳn S14.2 P+D bằng PI, có thể bỏ phần boost cần thiết của S14.2. Không xem đây là nguyên nhân phần cứng duy nhất đã được xác nhận.

## Quyết định và giá trị chính xác

| Thuộc tính | Level1 tùy chọn | Level2 mặc định |
|---|---:|---:|
| Owner baseline LEFT / RIGHT | 330 / 125 CCR | 405 / 175 CCR |
| Trần LEFT / RIGHT | 375 / 150 CCR | 450 / 200 CCR |
| Trần tăng thêm qua coordinator | 12 CCR / pass | 12 CCR / pass |
| Bắt đầu fade trần tại SOFT | 320 RAW | 320 RAW |
| Coordinator không thêm PWM | khi một bên <=180 RAW | khi một bên <=180 RAW |
| Owner taper cuối | 100 RAW | 100 RAW |

- `steer_s142_trial_level=2` **khi boot**, nhưng `steer_manual_enable=0`, `steer_ps2_manual_armed=0` và STOP/NC/START+neutral tiếp tục chặn chuyển động khi chưa arm. Sử dụng Level2 sau START+neutral là thay đổi hiệu lực thực, chưa an toàn trên phần cứng nếu chưa có gate.
- `steer_s147_velocity_pi_enable=0` **mặc định OFF**. ON chỉ latch trong lệnh manual mới (sau neutral), D-pad LEFT -> RIGHT R_MIN, S14.5 OFF, tại Level1 hoặc Level2. Không tự đưa PI vào R1/R2/RX/D-pad RIGHT hoặc chọn Level2 trong khi đang chạy.
- M8-S14.8: P+D S14.2 và PI S14.7 tính trên **cùng PWM gốc owner**, sau đó `M8S148_Merge` chọn `max(owner, owner+P+D_boost, PI_candidate)` **cho từng trục**; KHÔNG cộng boost. P+D trim giảm LEFT/RIGHT bị bỏ khỏi nhánh hybrid để không làm giảm motor nhanh nhằm làm đẹp chênh lệch. PI vẫn là hai PI riêng, antiwindup và dữ liệu AS5600 từng timestamp.
- Output hybrid vẫn chịu cap mỗi trục, rise tối đa +12CCR vượt owner, không cấp lại PWM=0, không sửa bất kỳ chủ trục STOP/NC/SOFT/late-EXTI/atomic guard nào. 180RAW cuối không chạy coordinator và 100RAW cuối owner giảm tốc như cũ. S14.5 ON sẽ loại trừ PI, sử dụng nhánh cũ; S14.5 boot-OFF giữ nguyên.
- `M8S00_S147` là số khung PI tạo candidate boost, **không phải output cuối**. Thêm `M8S00_S148,frames,pdLf,pdRf,piLf,piRf` để biết ứng viên nào đóng góp output > owner. Vẫn phải dùng M8-S11 đo thời gian RIGHT đến SOFT thật, không chỉ so skew.

## Kiểm chứng source

`tools/verify_m8_s148_level2_hybrid.py` và `tools/test_m8_s148_level2_hybrid.c` kiểm tra helper C sản xuất: caps Level1/Level2, lựa chọn MAX không cộng, không trim, không bật motor CCR=0, giới hạn extra +12, arming/NC/STOP/180RAW và telemetries. CI cũng chạy S14.2/S14.3/S14.5/S14.7, handoff 17 mục/đặc tả nguyên văn 386 dòng. Chỉ ghi HOST PASS khi GitHub Actions đúng SHA PASS.

## Cổng an toàn trước khi Flash/cấp động lực

Level2 450/200 đã được mã hóa nhưng chưa có clearance/coast/current/temperature độc lập nên **chưa có cơ sở coi firmware boot Level2 là an toàn vận hành**. Trước test motor: chuẩn bị ngắt nguồn động lực độc lập, NC/STOP hoạt động thực, kiểm tra quãng trôi và khoảng hở cơ khí khi dừng tại hai đầu, dòng/tản nhiệt driver dưới điều kiện thích hợp; xác nhận CubeIDE Clean Build+ELF+Flash đúng SHA. Không chạy ngay toàn hành trình, không thử trên mặt đất có tải. Với điều kiện đạt, xung rất ngắn giữa hành trình Level2 PI OFF trước, rồi Level2 PI ON đối chứng cùng điều kiện; thu CSV complete, M8S00, tính riêng RIGHT arrival, LEFT arrival, last100RAW, NC, current/coast; sau đó kiểm tra hồi quy R1/R2/STOP/NC/wheel. Nếu bất thường thì cắt nguồn động lực và không tiếp tục.

**Không có kết quả phần cứng cho M8-S14.8 ở thời điểm commit.** Không biến lỗi R1 LEFT code8 do vật cản thành defect firmware.


## Bản mới M8-S14.9 (2026-10-31)

Đây là tài liệu lịch sử S14.8. Khi có M8-S14.9, giữ Level2 boot default và PI/P+D MAX, thêm pilot RIGHT forecast terminal opt-in riêng, cắt ở 180RAW và telemetry bốn bin. User yêu cầu chỉ thử controller ON. Xem `docs/M8_S14_9_TERMINAL_LEAD_ON_ONLY.md`; chưa build/flash/hardware PASS.
