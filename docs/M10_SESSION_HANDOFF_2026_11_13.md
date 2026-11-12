**CẬP NHẬT 13/11/2026 — WINDOWS ETHERNET 2 IPv4 `192.168.10.10`.** Người vận hành chạy `Get-NetIPConfiguration`: Ethernet 2 có `192.168.10.10`, default gateway trống. Đây là quan sát cấu hình Windows, không phải ping/UDP. Giữ nguyên IP laptop thay vì dùng dải 192.168.50.x ví dụ trước đó; IP STM32 `192.168.10.20` chỉ là ứng viên, chưa cài. Bước tiếp theo đọc PrefixLength bằng `Get-NetIPAddress -InterfaceAlias "Ethernet 2" -AddressFamily IPv4 | Format-Table IPAddress,PrefixLength,AddressState -AutoSize` để xác nhận subnet, sau đó mới đề xuất cấu hình firmware mạng trên nhánh M10 và kiểm tra trùng địa chỉ. Không thay Wi-Fi/M9/flash.


**CẬP NHẬT 13/11/2026 — M10-S01.1 WINDOWS LINK 100 Mbps (USER-REPORTED).** Người vận hành cung cấp kết quả PowerShell: `Get-NetAdapter -Physical` hiển thị **Ethernet 2 / Intel I219-LM / Up / 100 Mbps**; Wi-Fi Up. Với kế hoạch kết nối RJ45 onboard NUCLEO-F767ZI, ghi nhận link vật lý tại NIC laptop. Chưa có kiểm tra MDIO PHY trên MCU, IP, ARP, ping, UDP echo hoặc flash M10; chưa nghiệm thu S01. Bước quan sát tiếp theo duy nhất: đọc IPv4 hiện tại của `Ethernet 2` bằng `Get-NetIPConfiguration` trước khi thay IP. Không tự thay Wi-Fi. M9 bất biến.

**BỔ SUNG 13/11/2026 — USER CONFIRMED RJ45 TÍCH HỢP.** Người vận hành cho biết dùng cổng RJ45 ngay trên NUCLEO-F767ZI. Đây là **xác nhận phương án phần cứng**, chưa phải kết quả link hoặc truyền dữ liệu. ST UM1974 xác định cổng CN14, PHY U9 LAN8742A, RMII và JP6/JP7 ON để chạy Ethernet. Kế hoạch S01 ưu tiên thử đường dây và trạng thái NIC Windows bằng kiểm tra chỉ đọc; chưa tự chỉnh IP hoặc flash, chưa có ICMP/UDP. M9 không đổi.

# M10 CONTEXT HANDOFF — Ethernet Windows → Ubuntu

Ngày bắt đầu 13/11/2026, giờ Việt Nam. Đọc cùng `CONTEXT_HANDOFF.md` và `docs/M10_ETHERNET_PLAN_2026_11_13.md` tại **cùng HEAD nhánh M10**. Không biến các mốc SHA lịch sử thành HEAD cố định.

## 0. Thông tin hồ sơ và phạm vi
Tạo M10 riêng cho giao tiếp Ethernet laptop–STM32F767ZI. Dự án kế thừa toàn bộ M6–M9 nhưng không mở lại hoặc sửa thuật toán M9. Hồ sơ toàn bộ 17 mục và lịch sử M9 được giữ tại `CONTEXT_HANDOFF.md`.

## 1. Đọc mục này để tiếp tục ngay
Repo `Spring-Mustache/control_motor`. Nhánh **`feature/m10-ethernet-windows-ubuntu`** được tạo từ `7fd8ab82552efb59de12d12e6960eaeda8149d16` (HEAD M9 xác minh ngày 13/11). Firmware M9 Profile4 nguồn `b1c1e9c4d8f7d191b21123a328ee1dea550595da`, đã có CI ARM/HOST PASS và quan sát giữ góc định tính ngày 11/11. M10-S00 đã có tài liệu thiết kế; chưa có chương trình Ethernet ứng dụng, build/flash/test M10.
**Mục tiêu:** thử Ethernet trên Windows trước, sau đó Ubuntu bằng cùng giao thức. Giữ M9 nhánh và mã nguồn không thay đổi.

## 2. Bối cảnh, mục tiêu, phạm vi
STM32F767ZI dùng 4 motor truyền động, 2 motor lái, AS5600 và cầu H; dự án sử dụng PS2/VC-02, NC/STOP. M10 thêm đường truyền Ethernet laptop↔STM32. Ban đầu ưu tiên Ethernet-only và dữ liệu echo, không điều khiển motor qua mạng.

## 3. Yêu cầu người dùng và quy tắc làm việc
Người dùng yêu cầu chính thức tách M10 để không ảnh hưởng M9; Windows là giai đoạn đầu, Ubuntu là giai đoạn sau. Không thay đổi PWM/limit/cảm biến, không flash hoặc thao tác cơ cấu khi chưa có điều kiện an toàn phù hợp. Cập nhật docs, phân biệt SOURCE/HOST/ARM/FLASHED/USER/HARDWARE.

## 4. Roadmap
M10-S00 audit phần cứng/IOC/HAL/linker/cache và an toàn; M10-S01 Windows link/IP/ping/UDP echo; M10-S02 Ubuntu chạy cùng protocol; M10-S03 regression, tài liệu, rollback, tích hợp chỉ khi được yêu cầu. Không tự khởi động SELECT/động học hoặc thay sửa M9.

## 5. Kết quả đã thực hiện
Đã tạo nhánh mới từ SHA M9; `control_motor.ioc` có ETH RMII, địa chỉ PHY 0, LAN8742A, chân PA1/PA2/PC1/PA7/PC4/PC5/PG11/PG13/PB13. `main.c` có `MX_ETH_Init()` và `HAL_ETH_Init`, chưa tìm thấy middleware LwIP hay UDP/TCP endpoint trong source. **Không suy từ cấu hình này rằng liên kết mạng đã chạy.**

## 6. Điểm dừng chính xác
M10 ở bước lập hồ sơ M10-S00, trước bất kỳ sửa firmware Ethernet, build/flash hoặc thử gói tin. M9 nguyên vẹn. Lúc cập nhật lần tiếp theo phải kiểm tra HEAD thật trên GitHub và GitHub Actions tại SHA ấy.

## 7. Tài nguyên
`docs/M10_ETHERNET_PLAN_2026_11_13.md`, `control_motor.ioc`, `Core/Src/main.c`, `Core/Src/stm32f7xx_hal_msp.c`, linker scripts, UM1974, `docs/M9_SESSION_HANDOFF_2026_11_11.md`, `CONTEXT_HANDOFF.md`, `docs/PROJECT_MILESTONES.md`. Main có `docs/PROJECT_CURRENT.md` là nguồn chỉ định nhánh hiện hành.

## 8. Quyết định đã chốt
Chọn branch M10 chuyên biệt từ M9 HEAD; không chuyển sang `main` và không tạo nhánh từ firmware M7/M8. Windows trước Ubuntu. Dự kiến IP tĩnh + ICMP/UDP echo trước ứng dụng phức tạp. Chưa chốt TCP/telemetry/điều khiển từ xa.

## 9. Vấn đề và bài học
Lịch sử M9 có lệnh điều khiển và giới hạn an toàn đã được thử; lỗi lịch sử không đồng nghĩa tồn tại ở baseline. ETH HAL init không phải IP/UDP stack. Git branch cách ly mã, nhưng nạp M10 sẽ thay ảnh firmware đang chạy trên MCU chung.

## 10. Rủi ro/giả định
Chưa xác minh board thực tế có jack RJ45/PHY/clock phù hợp, dây nối, nguồn, IP laptop, linker DMA và D-cache coherency. Cần kiểm tra MAC, PHY link, IRQ và cách ly motor trước test. Các IP thí dụ là giá trị thiết kế, không phải thiết lập thực tế. Không publish log hay thông số mạng riêng tư.

## 11. Kế hoạch tiếp tục
Rà phần cứng hiện có, HAL ETH init và mapping DMA/linker; thiết kế gói M10 chỉ trao đổi dữ liệu không ra lệnh motor; đưa lên nhánh M10 kèm kiểm tra HOST/ARM. Khi có firmware và điều kiện bench phù hợp, thử link rồi ICMP/UDP trên Windows. Chỉ sau khi Windows PASS chuyển sang Ubuntu mà không đổi giao thức.

## 12. Câu hỏi còn mở
Loại board/jack RJ45 đang dùng và cổng Ethernet laptop sẽ ảnh hưởng cách nối; chỉ hỏi khi sắp thực hiện kết nối thật. Đặc tả TCP/UDP sản phẩm và hệ thống điều khiển mạng chưa được người dùng giao, tạm dùng UDP echo làm mục tiêu bring-up.

## 13. Dòng thời gian
11/11/2026: M9 Profile4 có xác nhận định tính trên xe. 13/11/2026: xác minh HEAD M9 `7fd8ab8...`, tạo nhánh M10, lập kế hoạch Ethernet Windows→Ubuntu. Mốc tạo nhánh không chứng minh tác vụ Ethernet đã hoạt động.

## 14. Phụ lục nguyên tắc
IP thí dụ mạng thử riêng: Windows `192.168.50.1/24`, STM32 `192.168.50.2/24`; cần tránh xung đột trước cấu hình. Bài đo tối thiểu gồm Ethernet link, ARP, ICMP, UDP echo payload/sequence, kiểm tra mất kết nối và số lỗi. Không gán PORT hay mật khẩu nếu ứng dụng thực tế chưa quyết định.

## 15. Chỉ dẫn cho chat tiếp theo
Đọc `docs/PROJECT_CURRENT.md` trên main; xác minh HEAD đúng M10; đọc đủ 17 mục tích lũy và kế hoạch M10 tại chính SHA; kiểm tra CI. Không mặc định tài liệu M9 cũ là chỉ thị hiện hành. Không đưa firmware M10 vào M9/main nếu chưa có yêu cầu.

## 16. Đánh giá chất lượng và giới hạn chứng cứ
Đã có bằng chứng Git branch và lập tài liệu; chưa có đo link, ping, UDP, build firmware mới hoặc flash. Bảo toàn kết quả M9 và hồ sơ 386 dòng; báo cáo mọi trạng thái chính xác, không đồng nhất CI với test trên board.

END OF M10 HANDOFF
