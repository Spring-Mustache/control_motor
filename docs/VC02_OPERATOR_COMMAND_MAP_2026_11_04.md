# VC-02 — 49 khẩu lệnh, mã HEX và bit UART (ảnh USER 2026-11-04)

**Nguồn:** 4 ảnh chụp giao diện *Product management → Control details* mà người vận hành gửi trong hội thoại. Ảnh mô tả **cấu hình dự định tạo firmware để nạp VC-02**, không phải bản ghi UART nhận trên STM32, không chứng minh VC-02 đã build/flash. Bốn ảnh nguồn không được thêm thành image asset trong repo; bảng dưới đây là nội dung có thể đối chiếu.

Tất cả hàng có **Control type: UART1_TX**, **Action: Send it**. Cột Parameters trên VC-02 là byte dạng HEX. Lệnh TIME dùng **2 byte** liên tiếp `D0 XX`; các lệnh khác dùng **1 byte**. Nhị phân 8 bit bên dưới biểu diễn từng byte MSB→LSB, không phải chân GPIO.

## Bảng đủ 49 hàng từ ảnh

| STT | Command word | Behavior | Parameters (HEX) | UART bits (8-bit/byte) | Ghi chú |
|---:|---|---|---|---|---|
| 1 | Hello UniHi Uni (chữ mờ, cần đối chiếu) | `wakeup_uni` | `A1` | `1010 0001` |  |
| 2 | Go forward | `forward` | `A2` | `1010 0010` |  |
| 3 | Go backward | `backward` | `A3` | `1010 0011` |  |
| 4 | Rotate left | `rotateleft` | `A4` | `1010 0100` |  |
| 5 | Turn left | `turnleft` | `A5` | `1010 0101` |  |
| 6 | Turn right | `turnright` | `A6` | `1010 0110` |  |
| 7 | Zero degrees | `deg000` | `B0` | `1011 0000` |  |
| 8 | Five degrees | `deg005` | `B1` | `1011 0001` |  |
| 9 | Ten degrees | `deg010` | `B2` | `1011 0010` |  |
| 10 | Fifteen degrees | `deg015` | `B3` | `1011 0011` |  |
| 11 | Twenty degrees | `deg020` | `B4` | `1011 0100` |  |
| 12 | Twenty five degrees | `deg025` | `B5` | `1011 0101` |  |
| 13 | Thirty degrees | `deg030` | `B6` | `1011 0110` |  |
| 14 | Thirty five degrees | `deg035` | `B7` | `1011 0111` |  |
| 15 | Forty degrees | `deg040` | `B8` | `1011 1000` |  |
| 16 | Forty five degrees | `deg045` | `B9` | `1011 1001` |  |
| 17 | Very slow speed | `vslow` | `C1` | `1100 0001` |  |
| 18 | Slow speed | `slow` | `C2` | `1100 0010` |  |
| 19 | Medium speed | `medium` | `C3` | `1100 0011` |  |
| 20 | Fast speed | `fast` | `C4` | `1100 0100` |  |
| 21 | Full speed | `full` | `C5` | `1100 0101` |  |
| 22 | Five seconds | `sec005` | `D0 05` | `1101 0000 · 0000 0101` |  |
| 23 | Ten seconds | `sec010` | `D0 0A` | `1101 0000 · 0000 1010` |  |
| 24 | Fifteen seconds | `sec015` | `D0 0F` | `1101 0000 · 0000 1111` |  |
| 25 | Twenty seconds | `sec020` | `D0 14` | `1101 0000 · 0001 0100` |  |
| 26 | Twenty five seconds | `sec025` | `D0 19` | `1101 0000 · 0001 1001` |  |
| 27 | Thirty seconds | `sec030` | `D0 1E` | `1101 0000 · 0001 1110` |  |
| 28 | Thirty five seconds | `sec035` | `D0 23` | `1101 0000 · 0010 0011` |  |
| 29 | Forty seconds | `sec040` | `D0 28` | `1101 0000 · 0010 1000` |  |
| 30 | Forty five seconds | `sec045` | `D0 2D` | `1101 0000 · 0010 1101` |  |
| 31 | Fifty seconds | `sec050` | `D0 32` | `1101 0000 · 0011 0010` |  |
| 32 | Fifty five seconds | `sec055` | `D0 37` | `1101 0000 · 0011 0111` |  |
| 33 | Sixty seconds | `sec060` | `D0 3C` | `1101 0000 · 0011 1100` |  |
| 34 | Sixty five seconds | `sec065` | `D0 41` | `1101 0000 · 0100 0001` |  |
| 35 | Seventy seconds | `sec070` | `D0 46` | `1101 0000 · 0100 0110` |  |
| 36 | Seventy five seconds | `sec075` | `D0 4B` | `1101 0000 · 0100 1011` |  |
| 37 | Eighty seconds | `sec080` | `D0 50` | `1101 0000 · 0101 0000` |  |
| 38 | Eighty five seconds | `sec085` | `D0 55` | `1101 0000 · 0101 0101` |  |
| 39 | Ninety seconds | `sec090` | `D0 5A` | `1101 0000 · 0101 1010` |  |
| 40 | Ninety five seconds | `sec095` | `D0 5F` | `1101 0000 · 0101 1111` |  |
| 41 | One hundred seconds | `sec100` | `D0 64` | `1101 0000 · 0110 0100` |  |
| 42 | One hundred five seconds | `sec105` | `D0 69` | `1101 0000 · 0110 1001` |  |
| 43 | One hundred ten seconds | `sec110` | `D0 6E` | `1101 0000 · 0110 1110` |  |
| 44 | One hundred fifteen seconds | `sec115` | `D0 73` | `1101 0000 · 0111 0011` |  |
| 45 | One hundred twenty seconds | `sec120` | `D0 78` | `1101 0000 · 0111 1000` |  |
| 46 | Clear command | `clearcmd` | `F0` | `1111 0000` |  |
| 47 | Stop | `stop` | `FF` | `1111 1111` |  |
| 48 | center | `center` | `B0` | `1011 0000` | Alias hàng 7, **không tự đưa 2 motor về CENTER** trong STM32 hiện tại |
| 49 | Rotate right | `rotateright` | `A7` | `1010 0111` | Mã mới; STM32 decoder chưa hỗ trợ |

## Kết luận ánh xạ đã chốt

- **A4 = Rotate left**; **A7 = Rotate right**, KHÔNG dùng A8; **A5/A6** vẫn dành cho Turn left/Turn right.
- **B0** có hai cách nói *Zero degrees* (hàng 7) và *center* (hàng 48), cả hai cùng gửi `1011 0000`. **B1..B9** là 5..45° theo bước 5°.
- **C1..C5** là 5 mức tốc độ. **D0 05..D0 78** là các duration 5..120s, bước 5s (giá trị giây trong byte thứ hai dạng hex, không phải BCD). **F0** clear, **FF** STOP.
- Chữ chính xác của wake phrase ở hàng 1 không đủ rõ trong ảnh; giữ trạng thái chưa xác nhận, không dùng làm tiêu chí điều khiển.

## KHOẢNG TRỐNG PHẦN MỀM STM32 (chưa sửa trong commit này)

1. `Core/Src/vc02.c` chỉ giải mã A1..A6, B0..B9, C1..C5, D0+duration, F0, FF. **A7 chưa được giải mã**, do đó không thể coi Rotate right đã chạy.
2. A4 hiện được đưa vào `VC02_EVENT_ROTATE_IN_PLACE` **không mang chiều xoay**. Phải cập nhật STM32 trước khi A4 có nghĩa Rotate left trong đường thực thi.
3. B0 hiện gọi `Voice_SetStraight()` để xóa TURN/ANGLE trong chuỗi đang chờ. **B0 chưa phải lệnh vị trí R1/CENTER**; hai khẩu lệnh Zero degrees và center hiện là alias ở lớp UART thôi.
4. `Voice_FinalizeCommand` nhận TURN+ANGLE+TIME, nhưng `Voice_IsOpenLoopCommandSupported` chỉ thực thi FORWARD/BACKWARD thẳng; `Voice_ExecuteMotorControl` gọi `Turn_Motors_Stop()`. Chưa có đường motor cho TURN/ANGLE/ROTATE qua Voice.
5. PS2 L1/L2 đã có spin trái/phải bằng `WheelControl_SetSpinDirection(+1/-1,...)`, nhưng yêu cầu **cả NC L_MIN và R_MIN thực sự tác động, cả MAX phải không tác động**; STOP/PS2 arbitration, steering safety, independent cutoff vẫn áp dụng nếu tích hợp Voice sau này.

## Trạng thái và bước tiếp theo

- **Đã chốt:** dữ liệu cấu hình 49 mục VC-02 theo ảnh, dùng cho giai đoạn S15.4 và các bài thử lệnh **theo giá trị góc từ VC-02**.
- **Chưa kiểm chứng:** firmware VC-02 đã tạo/nạp; byte UART sau nạp; STM32 đã build/flash version tích hợp; chuyển động thực của 2 motor lái theo góc.
- Khi người vận hành báo đã nạp VC-02, trước hết so thực tế UART A4/A7/B0/B9/D0 05/FF qua debug event, giữ nguồn động lực an toàn, sau đó mới thiết kế đường S15.4 dựa trên owner vị trí hiện có. Không tự đồng nhất góc phần mềm B0..B9 với góc bánh thực tế chưa có động học.

## 2026-11-04 — Quy tắc sử dụng command (USER cập nhật sau flash VC-02)
Theo quyết định người vận hành, 4 lệnh wheel A2/A3/A4/A7 **bắt buộc có D0 XX** để có hiệu lực; A5/A6 + B0..B9 là lệnh hai trục lái hoàn chỉnh **không cần TIME**, B0 độc lập là CENTER. Hai owner Voice wheel/steer có thể chạy song song, FF STOP hủy tất cả, PS2 input đầu tiên hủy toàn bộ Voice và chỉ thao tác MỚI sau khi PS2 valid neutral mới thực thi. Speed C1..C5 dành cho wheel; steering speed suy ra từ 2 target riêng. Xem hợp đồng thiết kế `docs/M8_S154_VC02_INDEPENDENT_COMMAND_CONTRACT_2026_11_04.md`. Đây là **yêu cầu cho firmware STM32 tiếp theo**; parser/executor hiện tại chưa đáp ứng, đừng tuyên bố đã hoàn thành.
