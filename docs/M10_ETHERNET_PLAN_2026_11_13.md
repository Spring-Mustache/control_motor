# M10 — Ethernet STM32F767ZI ↔ Laptop (Windows trước, Ubuntu sau)

**Khởi tạo:** 13/11/2026. **Trạng thái:** M10-S00 — đã tạo nhánh riêng và lập kế hoạch; CHƯA có firmware Ethernet mới, CHƯA build/flash M10, CHƯA kiểm tra kết nối vật lý.

## 1. Mục tiêu và phạm vi

Thiết lập giao tiếp Ethernet có thể kiểm tra được giữa laptop và STM32F767ZI, tuần tự:
1. Windows: kết nối vật lý, IPv4 tĩnh, ICMP ping, gói UDP echo có nội dung.
2. Ubuntu: lặp lại cùng giao thức và dữ liệu trên phần cứng/firmware đã được xác minh.
3. Tích hợp dữ liệu trạng thái chỉ-đọc (nếu người dùng yêu cầu sau đó), không điều khiển động cơ qua Ethernet khi chưa có đặc tả và nghiệm thu riêng.

Không mở lại công việc M9 HOLD, động học toàn xe hoặc SELECT. Không thay đổi motor PWM/DIR, PI/PD, encoder, AS5600, NC/STOP, PS2, Voice, Rotate hay cơ chế phân quyền.

## 2. Branch và baseline bất biến

- Repo: `Spring-Mustache/control_motor`.
- Nhánh riêng: `feature/m10-ethernet-windows-ubuntu`.
- Được tách từ HEAD M9: `7fd8ab82552efb59de12d12e6960eaeda8149d16` (không phải `main`).
- Mốc **firmware M9 Profile 4** để bảo toàn: `b1c1e9c4d8f7d191b21123a328ee1dea550595da`.
- M9 nhánh `feature/m9-steering-active-position-hold` **không chỉnh sửa**.
- Chỉ đưa thay đổi trở lại M9/main sau quyết định tích hợp riêng; không tự merge/cherry-pick.

**Lưu ý:** cách ly Git không cách ly Flash của MCU. Khi nạp thử firmware M10 lên cùng board, chương trình trên MCU sẽ bị thay; phải bảo toàn ảnh firmware M9 để có đường quay lại, xác minh trạng thái motor an toàn rồi mới nạp.

## 3. Kiểm tra thực trạng mã nguồn tại baseline

- `control_motor.ioc`: ETH RMII; PHY ghi là LAN8742A, địa chỉ PHY 0; các tín hiệu RMII gán PA1, PA2, PC1, PA7, PC4, PC5, PG11, PG13, PB13.
- `Core/Src/main.c`: có `MX_ETH_Init()`, gọi `HAL_ETH_Init(&heth)` và cấu hình DMA descriptor/TxConfig.
- Chưa thấy `Core/Src/lwip.c`, `ethernetif.c` hoặc middleware giao thức TCP/IP trong cây mã hiện có. Vì vậy **chưa thể suy ra ICMP/UDP/TCP đã hoạt động**.
- Việc tiếp theo phải rà linker/DMA, vùng nhớ cache, bộ đệm Ethernet, cấu hình PHY/clock, ngắt, MAC và phiên bản HAL hiện dùng, nhất là STM32F7 với D-cache.
- Không tự đặt là board custom hay mặc định PHY và RJ45 thực tế đã hoạt động: xác nhận từ board đang dùng và thử link.
- `UM1974` và hồ sơ chân của dự án là tham chiếu; pin map Ethernet phải giữ nhất quán, không cướp chân motor.

## 4. Kiến trúc dự kiến

```text
Laptop Windows (sau đó Ubuntu)
    |  Ethernet RJ45 / switch hoặc cáp trực tiếp
    v
PHY LAN8742A <--> STM32F767ZI ETH MAC (RMII)
                              |
                    M10 TCP/IP stack (dự kiến LwIP)
                              |
                  ICMP + UDP echo (kiểm thử)
                              |
                     Read-only status (tùy chọn)
```

M10 chỉ được đọc dữ liệu trạng thái khi đã định nghĩa nguồn dữ liệu an toàn; **không** làm đường mạng có quyền ghi motor, đổi owner, unlock STOP/NC hoặc tăng PWM trong giai đoạn bring-up.

## 5. Phân đoạn / Acceptance gates

### M10-S00 — Audit và cách ly (hiện hành)

- Kiểm tra chính xác code HAL ETH, .ioc, linker và DMA/cache; không thay đổi firmware motor.
- Xác định phần cứng đang dùng có RJ45/PHY nào, mối nối thực tế và cách cấp nguồn phù hợp.
- Chốt cách thử **Ethernet-only**, outputs motor 0/không nhận lệnh kích hoạt, không cần chạy xe.
- Ghi baseline M9, trạng thái git local, và kế hoạch rollback trước flash.
- **PASS khi:** có cấu hình, sơ đồ thử, cách rollback và điểm kiểm an toàn có căn cứ. Hiện CHƯA PASS.

### M10-S01 — Giao tiếp Windows (sau S00)

- Thiết lập liên kết RJ45, xác minh link tại cả hai đầu; phân biệt lỗi link vật lý với lỗi IP.
- Dùng IPv4 tĩnh cho phép thử độc lập, ví dụ laptop `192.168.50.1/24`, STM32 `192.168.50.2/24` khi dải này không xung đột mạng đang dùng; chưa cần gateway/DHCP.
- Triển khai stack hoặc gói tin tối thiểu để phản hồi ARP/ICMP và UDP echo. `HAL_ETH_Init` một mình không cung cấp ping.
- Xác minh link, ARP, ping, echo dữ liệu hai chiều; lưu rõ SHA firmware, loại board, IP, payload, kết quả timeout/loss.
- **PASS khi:** Windows trao đổi các gói có nội dung với STM32 đúng chiều, có log và trường hợp mất/kết nối lại hoạt động theo thiết kế. Chưa PASS.

### M10-S02 — Ubuntu (chỉ sau Windows)

- Dùng **cùng firmware M10 và cùng giao thức UDP echo**; cấu hình NIC bằng NetworkManager/`nmcli` hoặc công cụ Linux phù hợp.
- Lặp lại kiểm tra link, ARP, ping, echo với payload tương đương; so sánh Windows/Ubuntu.
- **PASS khi:** Ubuntu đạt các ca tương đương, không phải thay firmware do đổi hệ điều hành máy chủ. Chưa PASS.

### M10-S03 — Kiểm thử không hồi quy và bàn giao

- Đối chiếu source M10 với baseline M9 để khoanh vùng mọi thay đổi.
- HOST tests + full ARM build GitHub Actions ở exact SHA sau sửa; bench-only Ethernet ở phần cứng khi đã được cho phép.
- Xác minh Ethernet không tự kích hoạt motor, không đổi STOP/NC/owner, không làm treo/tắc tác vụ M9; nếu nạp M10, ghi đúng SHA MCU.
- Đánh giá bộ đệm, timeout, việc mất kết nối, D-cache/ETH DMA và khả năng chạy lâu.
- Bàn giao cách dùng cho hai OS và rollback M9. Không tự merge.

## 6. Giao thức ban đầu

- Đường kiểm thử tối thiểu: **ICMP ping** (kiểm tra lớp IP) và **UDP echo** (kiểm tra dữ liệu ứng dụng).
- Dữ liệu thử là chuỗi ngắn ASCII/UTF-8, sequence number, không phải lệnh xe.
- TCP server/client, telemetry liên tục hay điều khiển từ laptop đều **chưa được yêu cầu/chưa chốt**.
- Nếu Windows không có cổng Ethernet, có thể dùng USB-to-Ethernet; xác nhận trên máy thực tế khi thực hiện.

## 7. Ranh giới an toàn, quyền và chứng cứ

- Nghiêm cấm tự tăng PWM, thay bảo vệ NC/STOP, phát lệnh motor qua Ethernet, hoặc flash firmware chỉ vì M10 đã có nhánh.
- M10 có thể build riêng nhưng muốn kiểm tra trên một board đang dùng M9 phải xác minh ảnh nạp, ảnh backup và trạng thái an toàn thực tế.
- Kết quả CI = HOST/ARM build, không đồng nghĩa FLASHED/HARDWARE PASS. Kiểm tra Windows và Ubuntu phải ghi riêng.
- Không đưa UART/CSV hoặc dữ liệu nhận dạng mạng riêng tư lên repo công khai.
- Giữ toàn bộ hồ sơ 17 mục và đặc tả gốc 386 dòng trên nhánh; cập nhật hồ sơ và roadmap sau mỗi thay đổi firmware.

## 8. Điểm dừng và hành động tiếp theo

**Đã thực hiện:** tạo nhánh M10 từ SHA M9 chính xác và kiểm tra cấu hình ETH/khung source liên quan.

**CHƯA thực hiện:** chỉnh mã TCP/IP, tạo socket, build M10, nạp board, chạy ping/UDP trên Windows hoặc Ubuntu.

**Tiếp theo:** audit code/linker/cache/PHY, chốt sơ đồ test Ethernet-only, lập bản thay đổi M10-S01 riêng. Chỉ tiến hành bước flash/đấu nối sau khi phần cứng và điều kiện an toàn đã xác định.

**Nguồn tham khảo:** `CONTEXT_HANDOFF.md`, `docs/PROJECT_MILESTONES.md`, `docs/M9_SESSION_HANDOFF_2026_11_11.md`, `control_motor.ioc`, `Core/Src/main.c`, UM1974.

## 9. Xác nhận phần cứng của người vận hành — 13/11/2026

**USER-CONFIRMED:** dùng **RJ45 tích hợp trên bo NUCLEO-F767ZI** (CN14), không dùng Ethernet shield/module PHY rời. Tài liệu ST UM1974 ghi PHY LAN8742A trên U9, RMII, Ethernet 10/100 Mb/s; jumper JP6 và JP7 phải ON khi dùng Ethernet. Cấu hình .ioc kế thừa khớp RMII/PHY address 0 nhưng phần cứng thực tế và link vẫn CHƯA THỬ.

Phép thử Windows S01 thứ tự mới: (1) xác minh board/cổng RJ45, JP6/JP7 và cấp nguồn board qua ST-LINK USB; (2) kết nối dây RJ45 trực tiếp vào cổng Ethernet laptop hoặc adapter Ethernet được nhận diện; (3) kiểm tra trạng thái adapter Windows bằng lệnh chỉ đọc `Get-NetAdapter -Physical | Format-Table Name,InterfaceDescription,Status,LinkSpeed`; (4) nếu đã lên link mới thiết lập IPv4 tĩnh trên **đúng adapter** và triển khai/flash firmware IP-only được kiểm tra; (5) thử ARP, ICMP ping, UDP echo.

**Không coi đèn LED hoặc Windows báo Connected là ping/IP PASS.** Ping chưa thể thành công chỉ với `HAL_ETH_Init`; cần tầng ARP/IPv4/ICMP hoặc TCP/IP stack. Không thay đổi IP của adapter chưa xác định; không dùng bridge/ICS trong bước đầu. Không dùng cổng USB ST-LINK như Ethernet.
