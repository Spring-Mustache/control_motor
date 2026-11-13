# M10 — CubeMX-safe source patch (13/11/2026)

## Phạm vi

Người vận hành đã cho phép sửa mã nguồn M10. Bản vá chỉ thuộc nhánh `feature/m10-ethernet-windows-ubuntu`, **không sửa nhánh M9**, không tác động workspace Windows `D:\STM32 task\control_motor` (local đang M9, SHA `b1c1e9c4`, có hai tệp `.settings` modified). **Chưa chạy CubeMX Generate Code, chưa flash, chưa kiểm tra Ethernet trên board.**

## Sửa mã và phân tích

1. `Core/Src/main.c`: `M10_ETH_ConfigMPU()` được chuyển vào `USER CODE BEGIN SysInit` và vẫn nằm trước `MX_ETH_Init()`. `memcpy(MACAddr, m10_eth_mac, sizeof(MACAddr))` được chuyển vào `USER CODE BEGIN MACADDRESS` và vẫn trước `HAL_ETH_Init()`. Khi CubeMX tạo các phép gán MAC mặc định `00:80:E1:00:00:00`, vùng USER CODE sẽ ghi đè thành MAC M10 `02:6D:31:30:00:01` trước khi cấu hình HAL. ARP/ICMP/UDP dùng cùng MAC do `m10_eth_protocol.c` định nghĩa.
2. Hai lời gọi mã M9 nằm ngoài USER CODE (`Steering_QuarantineParkPins()` và `Steering_DebugRetainWatchSymbols()`) cũng được chuyển vào `USER CODE BEGIN 2`. Guard parking vẫn chạy trước khi bắt đầu `HAL_TIM_PWM_Start()`, nhưng vị trí chuyển từ sau TIM14/trước init USART6/I2C sang đầu USER CODE 2, tức sau các init này. Đây là thay đổi thứ tự cần rà trên phần cứng khi được phép kiểm thử; không tự tuyên bố tương đương tuyệt đối. Không thay đổi chính thuật toán PI vị trí + D vận tốc, hệ số, giới hạn hoặc điều kiện cho phép motor.
3. Giữ `M10_ETH_Init(&heth)` trong `USER CODE 2`, `M10_ETH_Poll()` trong `USER CODE WHILE`.
4. Di chuyển bốn tệp M10 do người dùng viết `m10_eth_{bringup,protocol}.c/.h` từ `Core` sang `Services/Src` và `Services/Inc`, vốn đã được CubeIDE `.cproject` và build CI bao gồm. Nội dung các tệp giữ nguyên. Thay đổi đường dẫn host regression trong workflow.
5. Trên `control_motor.ioc` **chỉ nhánh M10**, `KeepUserCode=true`, `BackupPrevious=true`, `DeletePrevious=false`, `LastFirmware=false` và `FirmwarePackage=STM32Cube FW_F7 V1.17.4`. Các chân RMII không đổi.
6. Linker Flash `STM32F767ZITX_FLASH.ld` giữ nguyên và có một bản chuẩn độc lập `config/M10_FLASH_LINKER_BASELINE.ld`. STM32F767ZI SRAM2 là 16 KiB từ `0x2007C000`; kiểu `ETH_DMADescTypeDef` trong HAL đang dùng gồm 10 DWORD = 40 bytes, 4 RX descriptor chiếm 160 bytes nên bảng TX được bố trí tại `0x2007C0A0` đúng ảnh CubeMX. DMA descriptors và RX/TX payload trong `ETHRAM`.
7. Thêm cổng kiểm tra **chỉ đọc** `python tools/verify_m10_cubemx_safe.py` trong GitHub Actions. Cổng kiểm tra vị trí USER CODE, MAC trước HAL, MPU trước ETH, M9 park trước PWM, modules Services, `.ioc`, `.cproject` và linker khớp bản chuẩn. Cổng không tự khôi phục hay ghi đè nội dung.

## Giới hạn kiểm chứng và các bước sau

- Kết quả SOURCE-STAGED không tương đương Generate Code PASS, CubeIDE build PASS, FLASHED hoặc HARDWARE PASS. CI tại exact SHA mới phải hoàn tất trước khi công bố HOST/ARM PASS.
- Khi muốn Generate Code, cần bản làm việc **M10** cách ly và bản sao an toàn source/linker, sau đó so diff, chạy guard và build kiểm lại. Không thực hiện trên local M9 hiện có. Nếu CubeMX thay linker, **đối chiếu sai khác trước khi phục hồi**, không tự động ghi đè.
- CI chỉ bảo vệ các bất biến đã liệt kê; vẫn cần kiểm chứng thực tế rằng CubeMX phiên bản cài trên Windows giữ các vùng USER CODE và các tệp riêng, và rà toàn bộ diff trước khi công bố an toàn.
- Giữ mọi kiểm tra motor/PHY/ARP/ICMP/UDP và flash cho đợt được cho phép riêng, kèm đường rollback M9. Không mở điều khiển động cơ bằng mạng.

## Tham chiếu

M10 `docs/M10_ETHERNET_PLAN_2026_11_13.md` mục 17 (9 bước CubeMX đã xong), `docs/M10_SESSION_HANDOFF_2026_11_13.md`, nguồn M9 đóng băng và GitHub Actions exact SHA.
