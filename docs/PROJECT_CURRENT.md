# PROJECT CURRENT — XE TĂNG CƯỚP BÓNG

## 1. Mục đích

Đây là tệp điều hướng trạng thái hiện hành của dự án STM32F767ZI.

Repository: https://github.com/Spring-Mustache/control_motor

Các phiên ChatGPT mới phải đọc tệp này để tìm nhánh phát triển đang được chỉ định, sau đó kiểm tra lại dữ liệu GitHub thực tế.

**Không được coi nhánh, SHA hoặc milestone ghi trong tệp này là bất biến.**

## 2. Trạng thái được ghi nhận

Ngày đối chiếu: **13/11/2026**

| Trường | Giá trị |
|---|---|
| Milestone hiện hành | M10 — Ethernet STM32F767ZI ↔ Laptop (Windows → Ubuntu) |
| Nhánh phát triển được chỉ định | `feature/m10-ethernet-windows-ubuntu` |
| HEAD nhánh M10 đã xác minh | `eed6297a1309f7b15fdab55bdddf159369455c24` |
| Mốc source firmware M10 (kế thừa M9 Profile 4) | `b1c1e9c4d8f7d191b21123a328ee1dea550595da` |
| Hồ sơ bàn giao | `docs/M10_SESSION_HANDOFF_2026_11_13.md` |
| Bộ điều khiển M9 được bảo toàn | PI vị trí + D vận tốc, không sửa thuật toán |
| Giai đoạn M10 | S00 audit/docs, chưa có Ethernet app/IP/UDP mới, chưa flash |
| Động học tổng thể | Chưa hoàn thành tích hợp |
| SELECT tự động | Tạm hoãn theo quyết định người dùng |

HEAD M10 trên là ảnh chụp lúc lập hồ sơ ngày 13/11/2026, không thay thế việc lấy HEAD mới từ GitHub khi tiếp quản hoặc sau mỗi commit trên M10.

**M9 được đóng băng ở nhánh `feature/m9-steering-active-position-hold` HEAD `7fd8ab82552efb59de12d12e6960eaeda8149d16`.** Mốc source Profile 4 `b1c1e9c4d8f7d191b21123a328ee1dea550595da`; CI ARM/HOST M9 và kết quả quan sát trên xe vẫn được bảo toàn. M10 được tách từ chính HEAD M9, không từ main. M10 chưa đổi firmware và không tự merge ngược M9.

## 3. Kết quả và bằng chứng

### M10 Ethernet — tình trạng ngày 13/11/2026

**Cập nhật host Windows:** `Ethernet 2` = `192.168.10.10/24`, `Preferred`, đã báo link Up/100 Mbps và không gateway. Giữ nguyên IP host; địa chỉ tĩnh mục tiêu STM32 = `192.168.10.20/24`, **chưa nạp/cấu hình firmware**. Chưa có ping/UDP. Audit source cho thấy cần tầng IP/ICMP/UDP và kiểm tra vị trí Ethernet DMA/linker/cache trước khi triển khai, không đụng M9.

**Windows Ethernet 2:** người vận hành báo IPv4 `192.168.10.10`, default gateway trống. Giữ nguyên IP Windows; `192.168.10.20` chỉ là ứng viên IP STM32, chờ quan sát PrefixLength và kiểm tra tránh trùng địa chỉ. Chưa sửa firmware mạng hoặc thử ping/UDP.

**Quyết định mới của người vận hành:** dùng cổng RJ45 tích hợp NUCLEO-F767ZI, không dùng PHY/module rời. Theo ST UM1974, Ethernet onboard dùng LAN8742A, CN14, RMII và yêu cầu JP6/JP7 ON. **Bằng chứng Windows S01.1 ngày 13/11:** người vận hành báo Ethernet 2 (Intel I219-LM) Up/100 Mbps. Đây là bằng chứng link ở đầu laptop, chưa xác minh PHY MCU, IPv4, ARP, ping, UDP hay flash. Bước tiếp theo đọc cấu hình IPv4 bằng lệnh chỉ đọc; không thay Wi-Fi.


Nhánh M10 đã được tạo và có hai tài liệu `docs/M10_ETHERNET_PLAN_2026_11_13.md`, `docs/M10_SESSION_HANDOFF_2026_11_13.md`. `CONTEXT_HANDOFF.md` đủ 17 mục, roadmap và Chat B entrypoint đã bổ sung chỉ thị M10 mà không xóa lịch sử M9.

Khảo sát source kế thừa: `.ioc` có ETH RMII/LAN8742A PHY=0 và `main.c` có `MX_ETH_Init()` / `HAL_ETH_Init`; chưa thấy middleware TCP/IP hay endpoint ứng dụng. **Chưa có firmware Ethernet mới, build/flash M10 hoặc kết quả link/ping/UDP trên Windows/Ubuntu.** CI tại HEAD M10 phải được kiểm tra riêng; trạng thái kiểm tra mã không phải kết quả phần cứng.

### M9 đã đóng băng — phần mềm

Firmware Profile 4 đã được triển khai trên nhánh M9.

GitHub Actions của mốc firmware:

https://github.com/Spring-Mustache/control_motor/actions/runs/42759998485

Kết quả: PASS các kiểm tra bắt buộc, bao gồm biên dịch và liên kết firmware STM32F767ZI.

GitHub Actions của commit tài liệu M9 ngày 11/11:

https://github.com/Spring-Mustache/control_motor/actions/runs/42980764616

Kết quả: PASS.

### M9 — kết quả phần cứng

Ngày 11/11/2026, người vận hành xác nhận cơ cấu có phản lực chống tác động bên ngoài và không còn quay qua lại nhiều lần để dò góc trong lần thử đã thực hiện.

Mức chứng cứ: **USER-REPORTED trên xe**.

Lần xác nhận này chưa kèm log SHA firmware MCU, số đo lực, sai số góc, thời gian ổn định, dòng điện, nhiệt độ hoặc nghiệm thu đầy đủ từng trục và tải.

Không được tự nâng kết quả này thành HARDWARE PASS toàn diện.

## 4. Công việc còn mở

**M10 (ưu tiên hiện hành):** S00 audit cấu hình Ethernet/PHY, HAL, DMA/cache/linker và chuẩn bị test an toàn; S01 Windows link/ARP/ICMP/UDP echo; S02 Ubuntu lặp cùng giao thức; S03 hồi quy và bàn giao. Chỉ mở quyền điều khiển qua Ethernet nếu có yêu cầu và đặc tả riêng. Chưa có ca nào được xác nhận PASS phần cứng.

**M9 (đóng băng, giữ lịch sử):**

- Hoàn thiện động học phối hợp chuyển động của toàn xe và tích hợp khi được yêu cầu.
- Nghiệm thu hệ thống dưới tải thực tế khi có điều kiện phù hợp.
- Chưa có bộ số liệu định lượng đầy đủ về lực giữ, sai số góc, thời gian ổn định, dòng và nhiệt.
- SELECT tự hiệu chuẩn đang tạm hoãn; không tự khởi động lại.
- Giữ nguyên những chức năng M6, M7, M8 và các sửa lỗi Rotate đã được xác nhận, trừ khi có yêu cầu thay đổi mới.

Không tự sửa thuật toán M9 chỉ vì gặp ghi chú lỗi thuộc các phiên bản lịch sử.

## 5. Quy trình bắt buộc cho phiên ChatGPT mới

Khi người dùng yêu cầu "bắt đầu" hoặc "tiếp tục dự án":

1. Đọc chỉ thị mới nhất của người dùng.
2. Đọc tệp `docs/PROJECT_CURRENT.md` từ nhánh `main`.
3. Liệt kê và kiểm tra các nhánh liên quan, commit và lịch sử cập nhật trên GitHub.
4. Đối chiếu nhánh được chỉ định ở đây với các chỉ thị và hồ sơ mới nhất.
5. Nếu nhất quán, lấy HEAD SHA hiện tại của nhánh được chỉ định.
6. Đọc source và hồ sơ tại đúng SHA đó:
   - `CONTEXT_HANDOFF.md`, đủ 17 mục (0–16).
   - `docs/PROJECT_MILESTONES.md`.
   - `docs/CHAT_B_START_PROMPT.md`.
   - Tài liệu bàn giao của milestone hiện hành.
7. Kiểm tra GitHub Actions tại đúng SHA.
8. Báo cáo milestone, tiến độ, kết quả kiểm chứng, vấn đề còn mở và bước tiếp theo.

Nếu tài liệu này chưa được cập nhật nhưng GitHub hoặc chỉ thị mới của người dùng cho thấy dự án đã chuyển milestone, **không được kết luận dự án vẫn ở milestone cũ**.

Phải làm rõ mâu thuẫn bằng bằng chứng và cập nhật tệp chỉ dẫn sau khi xác nhận.

Không tự chọn nhánh chỉ vì nó có commit mới nhất.

## 6. Quy tắc chuyển milestone

Khi người dùng quyết định chuyển sang milestone mới, ví dụ M10:

1. Xác nhận tên milestone và nhánh phát triển được lựa chọn.
2. Cập nhật mục trạng thái hiện hành trong tệp này.
3. Cập nhật hồ sơ bàn giao và roadmap tương ứng.
4. Ghi rõ source baseline, HEAD đã kiểm tra và trạng thái CI.
5. Giữ milestone cũ trong lịch sử; không xóa kết quả nghiệm thu.
6. Kiểm tra lại tệp này sau commit. Khi tiếp tục commit M10, cập nhật snapshot HEAD trong tệp này và đối chiếu CI; không để tài liệu trỏ tới SHA M10 đã lỗi thời.

Không chỉ thay tên milestone mà bỏ qua trạng thái nguồn và bằng chứng.

## 7. Giới hạn và an toàn

Luôn phân biệt SOURCE, HOST TEST, ARM/CUBEIDE BUILD, FLASHED, USER-REPORTED và HARDWARE PASS.

Không tự nhận firmware đã nạp hoặc phần cứng đã nghiệm thu chỉ dựa trên CI.

Không tự thay đổi PWM, STOP, NC, sensor, ownership hoặc giới hạn an toàn.

Không đăng dữ liệu UART/CSV riêng tư công khai khi chưa được phép.

Giữ nguyên đặc tả bàn giao gốc tại `docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md`.

**Nguyên tắc cuối cùng: xác định nhánh hiện hành trước, xác minh bằng chứng sau, rồi mới kết luận tiến độ dự án.**
