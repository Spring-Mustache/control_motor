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

## 10. Windows S01.1 — báo cáo link vật lý 100 Mbps

**USER-REPORTED ngày 13/11/2026:** người vận hành kết nối RJ45 onboard NUCLEO-F767ZI với laptop Windows và chạy `Get-NetAdapter -Physical`. Adapter **Ethernet 2 (Intel(R) Ethernet Connection (22) I219-LM)** hiển thị **Status=Up; LinkSpeed=100 Mbps**. Đồng thời Wi-Fi đang Up. Trong bối cảnh người dùng đã xác nhận sử dụng RJ45 onboard, đây là bằng chứng link vật lý ở đầu Windows, **không** phải kết quả PHY register MCU, ARP, IP, ICMP hay UDP. Đánh dấu **M10-S01.1: USER-REPORTED PHY LINK OBSERVED (Windows)**, chờ quan sát IP với `Get-NetIPConfiguration -InterfaceAlias "Ethernet 2" | Format-List InterfaceAlias,IPv4Address,IPv4DefaultGateway`.

Chưa thay đổi adapter/IP/gateway/Wi-Fi, chưa phát gói ICMP, chưa nạp firmware mạng. Không suy ra M10-S01 PASS hoặc firmware STM32 có TCP/IP stack. Trước khi cấu hình IPv4 tĩnh phải đọc IPv4 hiện tại của Ethernet 2, tránh thay nhầm adapter hay gây xung đột mạng hiện dùng.

## 11. Windows S01.2 — đã đọc địa chỉ IPv4 của Ethernet 2

**USER-REPORTED, 13/11/2026:** kết quả lệnh `Get-NetIPConfiguration -InterfaceAlias "Ethernet 2" | Format-List InterfaceAlias,IPv4Address,IPv4DefaultGateway`:
- InterfaceAlias: `Ethernet 2`
- IPv4Address: `192.168.10.10`
- IPv4DefaultGateway: để trống.

**Quyết định:** GIỮ NGUYÊN IPv4 hiện có của Windows, không áp dụng đề xuất thử `192.168.50.1` trước đó. Đề xuất **STM32 `192.168.10.20`** cho cùng segment, **CHƯA THIẾT LẬP** và chưa xác minh địa chỉ này không trùng. Trước khi chọn subnet/IP STM32 cần đọc `PrefixLength` của adapter thực tế bằng lệnh chỉ đọc:
```powershell
Get-NetIPAddress -InterfaceAlias "Ethernet 2" -AddressFamily IPv4 | Format-Table IPAddress,PrefixLength,AddressState -AutoSize
```

Gateway trống không phải lỗi trong mô hình 2 thiết bị trực tiếp trên cùng subnet. `Ethernet 2` Up/100 Mbps là quan sát link ở đầu laptop, không chứng minh Nucleo có IP stack hoặc ping thành công. Chưa đổi IP máy tính, chưa sửa mã Ethernet, chưa build/flash firmware M10 mới, chưa nghiệm thu ARP/ICMP/UDP. Không làm gián đoạn Wi-Fi.

## 12. Windows S01.3 — đã xác minh subnet /24 và sẵn sàng chuyển sang firmware M10

**USER-REPORTED 13/11/2026:** `Get-NetIPAddress -InterfaceAlias "Ethernet 2" -AddressFamily IPv4` trả về `IPAddress=192.168.10.10`, `PrefixLength=24`, `AddressState=Preferred`. Kết hợp với link Windows `Up / 100 Mbps` và gateway trống đã báo trước đó.

**Kế hoạch địa chỉ chốt cho bench Ethernet trực tiếp, chưa được áp lên STM32:**
- Windows Ethernet 2: `192.168.10.10/24` (đã có thực tế, **không đổi**).
- STM32 M10: `192.168.10.20/24` (chọn làm IP tĩnh mục tiêu; **chưa có IP stack/firmware gán IP**, còn cần bảo đảm không trùng IP khi lên mạng thực).
- Gateway: không cần cho 2 thiết bị cùng subnet.
- Wi-Fi Windows: để nguyên.

**M10-S01 Windows adapter configured: USER-REPORTED HOST READY.** KHÔNG đánh dấu end-to-end IP, ICMP, UDP hoặc HARDWARE PASS. Không có ích khi yêu cầu ping `192.168.10.20` trước khi firmware STM32 thực sự xử lý ARP/ICMP.

### Source audit bổ sung tại M10 baseline (read-only)

- `Core/Src/main.c` có `MX_ETH_Init()`/`HAL_ETH_Init()` và biến DMA descriptor ở các section `.RxDecripSection` và `.TxDecripSection`, nhưng **không thấy** `HAL_ETH_Start`, `HAL_ETH_ReadData` hoặc protocol handler trong source `main.c`.
- `STM32F767ZITX_FLASH.ld` hiện khai báo `RAM ORIGIN=0x20000000 LENGTH=512K` và không thấy vị trí **tường minh** cho section ETH DMA descriptor. Điều này **chưa chứng minh** DMA hỏng, nhưng phải rà **ELF map/linker** để biết vị trí thực tế; với STM32F7, DMA ETH phải dùng vùng bộ nhớ truy cập được, xét thêm MPU/D-cache.
- Đừng sao chép địa chỉ D2 SRAM `0x300xxxxx` của STM32H7 sang STM32F7. Thiết kế cần dựa trên sơ đồ nhớ chính xác của F767ZI và HAL ETH đang dùng.

**Gate tiếp theo:** chuẩn bị riêng mã mạng M10 (ưu tiên stack LwIP phù hợp thư viện hiện có, ARP+ICMP+UDP echo, IPv4 tĩnh), phân vùng ETH DMA/linker và bộ đệm an toàn; HOST/ARM CI exact SHA. Chỉ flash trên board ở bench đã cô lập motor, có cách khôi phục binary M9 và đã kiểm đường cấp nguồn. Sau khi MCU đã có IP stack mới chạy ping/UDP Windows. Giai đoạn Ubuntu dùng **cùng firmware/cùng protocol**. Không tự thêm lệnh vận hành motor qua Ethernet.

## 13. SOURCE-IMPLEMENTED candidate — raw ARP + ICMP + UDP echo (chưa flash)

**Triển khai trên NHÁNH M10, không thay M9:** lựa chọn tạm thời **giao thức tối thiểu trên Ethernet HAL, không thêm LwIP** để giảm bề mặt thay đổi và không làm chậm milestone. Đây KHÔNG phải TCP/IP stack đầy đủ, không hỗ trợ TCP, DHCP, IPv4 fragments hay điều khiển motor.

Thành phần dự kiến cùng một commit:
- \`Core/Inc/m10_eth_protocol.h\`, \`Core/Src/m10_eth_protocol.c\`: bộ xử lý frame thuần C, MAC cục bộ \`02:6d:31:30:00:01\`, IPv4 \`192.168.10.20/24\`, ARP reply, ICMP echo reply, UDP echo cổng \`5005\` (payload <= 512 bytes), kiểm tra độ dài/chksum, bỏ frame khác.
- \`Core/Inc/m10_eth_bringup.h\`, \`Core/Src/m10_eth_bringup.c\`: driver dùng HAL ETH polling; đọc link PHY0/LAN8742A, cấu hình MAC tốc độ/duplex theo PHY, không phát lệnh motor. Xử lý tối đa 1 frame mỗi lượt, không dùng heap; MDIO timeout ngắn 2ms, TX timeout 2ms và khóa reuse TX buffer khi lỗi timeout.
- \`STM32F767ZITX_FLASH.ld\`: tách SRAM2 16 KiB \`0x2007C000\` làm \`ETHRAM\`, descriptors/rx/tx đặt ở đó; hạ vùng RAM chung từ 512K xuống 496K. \`M10_ETH_ConfigMPU\` đánh dấu SRAM2 noncacheable trước \`MX_ETH_Init\`, chống incoherence ETH DMA trên Cortex-M7.
- \`Core/Src/main.c\`: chỉ thêm MPU before ETH, init/poll dịch vụ M10, MAC mới; không sửa control motor/owner/STOP/NC/PWM.
- \`tools/test_m10_eth_protocol.c\`: native C tests ARP, ping, UDP echo, checksum, wrong-port/wrong-IP/malformed; chạy ở GitHub Actions.
- \`tools/m10_udp_echo_test.ps1\`: phép thử UDP Windows PS5.1 (read-only, không chỉnh network adapter).

**Điều kiện kỹ thuật:** HOST test và ARM compile/link ở exact SHA CI sau commit. STM32 **chưa flashed**, Windows ping/UDP echo **chưa thử** cho đến khi người vận hành lưu lại firmware M9, cấp nguồn Nucleo riêng, ngắt nguồn motor/actuator và theo hướng dẫn đã duyệt. Đặc biệt link Up/100 Mbps mới là trạng thái ở Windows và chưa chứng minh ETH DMA/PHY STM32 đang hoạt động.

**Windows sau khi flash M10 và bảo đảm an toàn:** \`ping.exe -n 4 192.168.10.20\`, kiểm ARP \`Get-NetNeighbor -InterfaceAlias 'Ethernet 2' -AddressFamily IPv4\`, và \`powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\m10_udp_echo_test.ps1\` ở root repo. Cả ping và UDP phải PASS mới coi M10-S01 trao đổi gói tin thành công. Nếu ping fail phân biệt PHY, MAC/DMA/MPU, ARP và ICMP; không tăng PWM.

## 14. Review PHY MDIO và CI firmware M10

**CI firmware candidate \`a6951d4559b27f0025c23e66e024698f42a420e7\` đã PASS** 3 job (host C regression, full ARM compile/link, Windows PowerShell syntax) tại https://github.com/Spring-Mustache/control_motor/actions/runs/43313309315 ; artifact exact SHA đã được GitHub tạo. Đây vẫn chỉ là SOFTWARE/SOURCE/HOST/ARM, không FLASHED.

**Rà lại ST HAL Ethernet:** bản HAL của dự án có \`HAL_ETH_SetMDIOClockRange\` nhưng **\`HAL_ETH_Init()\` không tự gọi nó**. M10 cần gọi \`HAL_ETH_SetMDIOClockRange(eth)\` trong \`M10_ETH_Init\` trước lần đọc MDIO đầu tiên; nếu thiếu, software build PASS có thể không đọc LAN8742A. Bản fix phải chạy lại CI exact SHA trước khi sử dụng. Workflow M10 đổi tên artifact thành \`M10-ethernet-<SHA>\` để không nhầm với M9; binary nội bộ vẫn có tên di sản \`control_motor_M9_final.*\` từ build script chung nhưng build manifest lưu exact SHA của M10. Thêm kiểm tra cú pháp PowerShell file echo trong CI.

## 15. Định danh binary M10 và CI

Bản \`752569d6759fb29e33d09c1d762f463f83a522d3\` đã được xác minh **3 job CI PASS** tại https://github.com/Spring-Mustache/control_motor/actions/runs/43313758582 (HOST protocol regression, ARM compiler/linker, PowerShell 5.1 syntax). Không có kết quả Flash/Hardware của commit này.

Trước khi bàn giao binary cho người vận hành, sửa tên output **trên nhánh M10 duy nhất** từ tên cũ di sản \`control_motor_M9_final.{elf,hex,bin,map}\` sang \`control_motor_M10_ethernet.{elf,hex,bin,map}\` trong script build CI và danh sách upload artifact. Build manifest ghi hash của chính các file mới. Thao tác này chỉ đổi tên artefact M10, không sửa M9 firmware/control, và phải được CI exact-SHA kiểm lại.

**Lưu ý CubeIDE:** script CI không phải pre-build CubeIDE. Khi build trên Windows bằng STM32CubeIDE cần xác nhận Git SHA nhúng từ \`tools/stamp_firmware_build.ps1\`; không tự lấy binary M9 chỉ vì tên thư mục Debug giống nhau.
