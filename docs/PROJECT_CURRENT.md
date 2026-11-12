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
| Milestone hiện hành | M9 — Steering Active Position Hold |
| Nhánh phát triển được chỉ định | `feature/m9-steering-active-position-hold` |
| HEAD nhánh M9 đã xác minh | `7fd8ab82552efb59de12d12e6960eaeda8149d16` |
| Mốc source firmware M9 Profile 4 | `b1c1e9c4d8f7d191b21123a328ee1dea550595da` |
| Hồ sơ bàn giao | `docs/M9_SESSION_HANDOFF_2026_11_11.md` |
| Bộ điều khiển hiện hành | Giữ góc hai trục bằng PI vị trí + D vận tốc |
| Động học tổng thể | Chưa hoàn thành tích hợp |
| SELECT tự động | Tạm hoãn theo quyết định người dùng |

HEAD trên là ảnh chụp tại lần xác minh, không thay thế việc lấy HEAD mới từ GitHub mỗi khi tiếp quản.

## 3. Kết quả và bằng chứng

### Phần mềm

Firmware Profile 4 đã được triển khai trên nhánh M9.

GitHub Actions của mốc firmware:

https://github.com/Spring-Mustache/control_motor/actions/runs/42759998485

Kết quả: PASS các kiểm tra bắt buộc, bao gồm biên dịch và liên kết firmware STM32F767ZI.

GitHub Actions của commit tài liệu M9 ngày 11/11:

https://github.com/Spring-Mustache/control_motor/actions/runs/42980764616

Kết quả: PASS.

### Phần cứng

Ngày 11/11/2026, người vận hành xác nhận cơ cấu có phản lực chống tác động bên ngoài và không còn quay qua lại nhiều lần để dò góc trong lần thử đã thực hiện.

Mức chứng cứ: **USER-REPORTED trên xe**.

Lần xác nhận này chưa kèm log SHA firmware MCU, số đo lực, sai số góc, thời gian ổn định, dòng điện, nhiệt độ hoặc nghiệm thu đầy đủ từng trục và tải.

Không được tự nâng kết quả này thành HARDWARE PASS toàn diện.

## 4. Công việc còn mở

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
6. Kiểm tra lại tệp này sau commit.

Không chỉ thay tên milestone mà bỏ qua trạng thái nguồn và bằng chứng.

## 7. Giới hạn và an toàn

Luôn phân biệt SOURCE, HOST TEST, ARM/CUBEIDE BUILD, FLASHED, USER-REPORTED và HARDWARE PASS.

Không tự nhận firmware đã nạp hoặc phần cứng đã nghiệm thu chỉ dựa trên CI.

Không tự thay đổi PWM, STOP, NC, sensor, ownership hoặc giới hạn an toàn.

Không đăng dữ liệu UART/CSV riêng tư công khai khi chưa được phép.

Giữ nguyên đặc tả bàn giao gốc tại `docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md`.

**Nguyên tắc cuối cùng: xác định nhánh hiện hành trước, xác minh bằng chứng sau, rồi mới kết luận tiến độ dự án.**
