# LỜI NHẮN CHO PHIÊN CHATGPT MỚI — TIẾP QUẢN DỰ ÁN XE TĂNG CƯỚP BÓNG

Tôi muốn tiếp tục dự án STM32F767ZI, 4 motor bánh + 2 motor đánh lái, từ GitHub:
https://github.com/Spring-Mustache/control_motor

**Trước tiên phải đọc docs/PROJECT_CURRENT.md trên main** để biết milestone và nhánh được chỉ định. Sau đó kiểm tra chỉ thị mới nhất của tôi, liệt kê nhánh liên quan, xác minh HEAD thật và lịch sử commit. Không dùng HEAD main, SHA trong tài liệu bàn giao cũ hoặc nhánh có commit mới nhất để tự kết luận nhánh hiện hành.

Nếu PROJECT_CURRENT không tồn tại hoặc không nhất quán với chỉ thị mới / GitHub, phải đối chiếu bằng chứng, chỉ hỏi điều quyết định nếu vẫn không phân biệt được. Không mặc định M7, M8, M9 hay milestone nào sẽ giữ nguyên mãi mãi.

## Quy trình đọc nguồn ở đúng SHA

1. Chọn nhánh đang hoạt động có căn cứ và cố định SHA hiện tại từ GitHub.
2. Đọc CONTEXT_HANDOFF.md đầy đủ 17 mục (0–16), cả phụ lục; docs/PROJECT_MILESTONES.md, README, hồ sơ milestone và tài liệu liên quan tại SHA đã chọn.
3. Đọc lịch sử 20 commit gần đây và thay đổi đối với baseline; kiểm tra GitHub Actions đúng SHA.
4. Phân biệt việc đã commit, HOST test, ARM/CubeIDE build, firmware đã flash, quan sát người vận hành và nghiệm thu định lượng.
5. Tóm tắt công việc đã hoàn thành, lỗi đã sửa, việc còn mở, điểm dừng chính xác và bước tiếp theo. Tiến hành việc đủ quyền/dữ liệu thay vì yêu cầu kể lại lịch sử.

## Quy tắc bảo toàn

Giữ nguyên docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md, đặc tả nguyên văn 386 dòng, SHA-256 13d8976ec0d74057af94bf84c8445bddbb48715b308964e22eb6684711f708a4.

Không tự thay đổi firmware, PWM, NC/STOP/owner, Flash hoặc làm thí nghiệm điện/cơ khi chưa có căn cứ và quyền phù hợp. Không công khai UART/CSV riêng tư chưa được phép. Giữ lịch sử điều chỉnh, không trình bày giả thuyết là kết quả thực nghiệm.

## CI/Release

Bắt đầu trực tiếp từ GitHub commit/docs, **không bắt buộc Release**. Mọi push chạy kiểm tra hồ sơ; ZIP Release chỉ tạo khi người dùng yêu cầu rõ và workflow_dispatch.publish_release=true. Một Release cũ không chứng minh nó là phiên bản hiện hành hoặc phần cứng đã PASS.

Nếu có thay đổi mới, cập nhật handoff/roadmap đúng nội dung và xác minh CI. Trước khi báo "dự án đang ở Mx", phải xác minh nhánh/SHA/tài liệu nguồn phù hợp, nếu không thì nêu rõ điều chưa xác minh.
