# M8-S14.6 — Phân tích thụ động RIGHT về R_MIN ở PWM thấp

**Ngày:** 2026-10-31. **Phạm vi:** công cụ phân tích log và bằng chứng để thiết kế profile phù hợp; **không thay đổi firmware, PWM hay giới hạn an toàn**. Chỉ đánh dấu software analysis PASS nếu script chạy thành công; không đồng nghĩa cơ khí an toàn.

## Vấn đề và nguồn dữ liệu

- `M8S11_six_motor_20261031_153229.csv` / `.meta.txt`: S14.5 OFF, 1.356 mẫu, dữ liệu complete, không mất mẫu.
- `M8S11_six_motor_20261031_154708.csv` / `.meta.txt`: S14.5 ON, 1.596 mẫu, dữ liệu complete, không mất mẫu.
- `M8_S00_20261031_154858.txt`: `M8S00_S144,enable=1,biasFrames=350,rightPriority=80,peakBias=200`; thuật toán opt-in đã thực sự chạy nhưng không chứng minh rút ngắn arrival.
- Các file CSV/TXT trên người dùng đã gửi riêng trong ChatGPT. Không sao chép raw logs lên repository công khai.

### So sánh có chọn mẫu

Chỉ so lượt D-pad LEFT, `soft_goal_l=1454`, `soft_goal_r=216`, bắt đầu ở LEFT RAW ≥2100 và RIGHT RAW ≥900, cả hai chạm SOFT khi D-pad còn giữ. Cỡ mẫu mỗi nhóm **n=2**; lượt nhả trước đích được báo riêng, không gán thời gian giả.

| Chỉ số | OFF | ON |
|---|---:|---:|
| Lượt toàn hành trình hoàn tất | 2 | 2 |
| RIGHT trễ so LEFT, trung bình | 511,5ms | 546,5ms |
| LEFT 100 RAW cuối, trung bình | 274,5ms | 244,0ms |
| RIGHT 100 RAW cuối, trung bình | 542,0ms | 546,5ms |
| RIGHT chậm hơn (ON−OFF) | | +35,0ms |

Do khác điều kiện và cỡ mẫu chỉ 2, kết quả chỉ hỗ trợ quyết định **không bật S14.5 mặc định**, không chứng minh cơ chế gây chậm là duy nhất.

### Đặc tính PWM và vận tốc mẫu RIGHT (gộp các lượt đầy đủ và chưa hoàn tất)

| RIGHT cách R_MIN | OFF median CCR | OFF median AS5600 RAW/s | ON median CCR | ON median RAW/s |
|---|---:|---:|---:|---:|
| 100–180 RAW | 100 | 327,9 | 100 | 327,9 |
| 60–80 RAW | 84,5 | 270,5 | 84,0 | 270,5 |
| 20–40 RAW | 65,5 | 188,5 | 64,5 | 180,3 |
| 0–10 RAW | 52 | 114,8 | 53 | 114,8 |

Cách đo: `speed_RAW_s = (raw_R_previous - raw_R_current)*1000 / (board_ms_current-board_ms_previous)` trong lượt D-pad LEFT, trục RIGHT đang có CCR>0; loại khoảng thời gian <=0 hoặc >150ms, bước nhảy RAW sai chiều hoặc >150. UART trung bình 50–61ms, hai AS5600 đọc lệch thời điểm; đây **không** là tốc độ góc chuẩn tuyệt đối, không đo motor current/torque hoặc effective braking.

### Phát hiện từ firmware

`Steering_ManualAxisUpdate()` dùng đường giảm PWM 100 RAW cuối hướng tới `steer_manual_right_rx_min_pwm` (50 CCR). Khi sắp đến SOFT, duty giảm từ 100 về ~50, vận tốc giảm tương ứng. S14.5 chỉ bias góc ở >320 RAW nên không tác động vào đoạn chậm 100 RAW cuối. Tăng ngay 50→100 gần R_MIN mà không xác minh quãng dừng có thể gây vượt SOFT/nguy cơ chạm NC.

Các log có LEFT MIN RAW ~1429–1430 so SOFT1454, RIGHT MIN ~211 so SOFT216, RIGHT MAX ~1022–1023 so SOFT999. M8-S00 runtime ghi R_MAX NC RAW1029 trong hệ tọa độ trước đó; nếu hình học còn như cũ thì chỉ còn ~6–7 RAW, **cần xác minh lại thật sự** trước khi thử PWM cao. Không suy đây là phép đo clearance cơ khí độc lập.

## Công cụ đo lặp không cần sửa MCU

Chạy từ `D:\\STM32 task\\control_motor` khi giữ hai CSV và metadata trong `logs`:

```powershell
python tools/analyze_m8_right_low_pwm.py --off .\\logs\\M8S11_six_motor_20261031_153229.csv --on .\\logs\\M8S11_six_motor_20261031_154708.csv --output-json .\\logs\\M8S14_6_right_low_pwm_AB.json
```

Script kiểm tra `complete=True`, `captured_rows`, `invalid_lines=0`, `sequence_missing=0`, `missed=0` và `tx_errors=0`; tính cụ thể từng lượt và gộp theo vùng. JSON viết chỉ vào máy người vận hành, không tự commit/đăng dữ liệu. Dùng `python tools/analyze_m8_right_low_pwm.py --self-test` để thử bảng giả lập hoàn tất/nhả trước/mốc SOFT. GitHub Actions gọi self-test sau mỗi push.

## Cổng kiểm chứng trước khi chỉnh profile RIGHT

1. Xác nhận cắt nguồn động lực độc lập và STOP/NC đúng cơ khí khi motor không cấp điện; tránh tự chạy vào NC để kiểm tra.
2. Trên giá treo chắc chắn, đo/ví dụ quan sát khoảng hở cơ khí thực tế còn lại tại RIGHT MIN/MAX sau lệnh STOP và sau coast (không chỉ trừ số RAW). Ghi lại nếu NC từng bị chạm hoặc công tắc còn rất gần.
3. Nếu có dụng cụ điện và người đo đủ năng lực, ghi dòng/đỉnh dòng và nhiệt driver ở các vùng PWM 50/65/85/100; không thực hiện thao tác làm mất bảo vệ an toàn.
4. Khi có căn cứ bảo đảm dừng an toàn, thử phiên bản **opt-in mặc định OFF** của profile RIGHT được xây từ dữ liệu và so A/B cùng điều kiện; không tự kích hoạt ở đất hoặc Level2.

**Đang chờ:** kết quả đo quãng trôi thực, cắt nguồn và dòng điện. Không sửa `STEER_MANUAL_APPROACH_WINDOW_RAW`, sàn/trần PWM hay SOFT cho tới khi cổng này có bằng chứng.

## Bổ sung M8-S00 16:09 và CSV 16:07 — sự kiện R1 độc lập (2026-10-31)

M8-S00 ghi một **FAULT LEFT R1 code8** tick67140, trước CSV start board_ms78246, nên CSV này không thể tái dựng chính xác diễn biến ngay trước lỗi. Snapshot manual STOP tick66899: L1740/R510 CCR342/125; snapshot R1 FAULT tick67140 L1652/R471; LEFT cần tăng từ1740 về center1873 nhưng hai snapshot giảm88RAW. Lỗi8 trong code là nhóm wrong-way/no-progress/stall; không suy kết luận nguyên nhân duy nhất, không đánh dấu fixed.

CSV 1031 row complete, 60.567s, không mất mẫu. R1 từ MAX trên CSV bị **hủy do RX non-neutral** (128→82→0 ở board ms114460), M8-S00 history run12 `stop9`/LEFT reason9, RIGHT reason2; đây không phải FAULT8. R1 từ MIN tiếp theo run18 BOTH success, LEFT1758ms RIGHT2149ms skew391ms.

Ba D-pad LEFT full-travel RIGHT late 547/548/607ms (mean567.3), S14.5 OFF theo M8-S00; Lmin1430, Rmin209, Rmax1019 so soft1454/216/999; không có NC/trip/stop được log trong CSV, nhưng chưa có đo clearance/cutoff thực. Chỉ47 row trong CSV có bánh commanded nonzero. **Không sửa gần-SOFT PWM** hay tự diễn giải nghiệm thu chỉ từ UART.

Bước kiểm tra R1 cần thực hiện trước khi thiết kế patch: giữ RX trung tính, chờ cơ cấu hoàn toàn ngừng trôi sau manual, kiểm tra nguồn ngắt độc lập/STOP/NC và bật capture trước R1 (event8 đã xảy ra trước CSV lần này). Không cần phát hành firmware/build/flash mới chỉ để đọc bổ sung tài liệu này.
