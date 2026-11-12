# Xe tăng cướp bóng — STM32F767ZI

Firmware điều khiển 4 motor bánh + 2 motor đánh lái, PS2, VC-02, AS5600, NC/STOP.

## Bắt đầu đúng milestone trong hội thoại ChatGPT mới

**Nguồn chỉ định trạng thái hiện hành:** [docs/PROJECT_CURRENT.md](docs/PROJECT_CURRENT.md) trên nhánh main.

1. Đọc PROJECT_CURRENT, đối chiếu với chỉ thị gần nhất của người vận hành.
2. Liệt kê nhánh GitHub, lịch sử commit liên quan. Không tự chọn nhánh chỉ vì commit mới hơn.
3. Lấy HEAD thật của nhánh hiện hành được xác nhận. Đọc source và tài liệu ở cùng SHA.
4. Đọc CONTEXT_HANDOFF.md đủ 17 mục, docs/PROJECT_MILESTONES.md và hồ sơ milestone hiện hành.
5. Kiểm tra Actions đúng SHA; phân biệt SOURCE, HOST, ARM/CubeIDE, FLASHED và kết quả phần cứng.

Không suy ra tiến độ từ HEAD của main. Main giữ snapshot lịch sử và tệp điều hướng; firmware phát triển có thể nằm ở nhánh khác. Tên nhánh hoặc SHA ghi trong tài liệu cũ không phải trạng thái hiện hành vĩnh viễn.

Có thể bắt đầu chat bằng [lời nhắn bàn giao cho B](docs/CHAT_B_START_PROMPT.md), hoặc yêu cầu trực tiếp "tiếp tục dự án" khi ChatGPT có quyền truy cập repository.

## Tiếp quản và giới hạn

- GitHub ghi nhận mã/tài liệu **đã commit**, không tự nhận được file Windows, log riêng tư hoặc nội dung chat chưa lưu.
- Giữ nguyên [đặc tả 386 dòng](docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md).
- [CONTEXT_HANDOFF.md](CONTEXT_HANDOFF.md) và [roadmap](docs/PROJECT_MILESTONES.md) trong main bao gồm snapshot lịch sử; dùng nhánh đang hoạt động cho hồ sơ mới nhất.
- Không tự sửa firmware, tăng PWM hoặc tuyên bố PASS cơ khí khi không có bằng chứng.
- Mức nghiệm thu: SOURCE-STAGED, HOST TEST, ARM/CUBEIDE BUILD, FLASHED, USER-REPORTED, HARDWARE PASS là khác nhau.
- [Chính sách CI và phát hành](docs/CONTEXT_RELEASE_POLICY.md) là quy định hiện hành về kiểm tra và Release tùy chọn.

## CI và GitHub Releases

Workflow [Verify canonical project status](.github/workflows/verify-project-current.yml) kiểm tra nhánh, HEAD, baseline và handoff; chạy cả định kỳ để phát hiện trạng thái cũ.

Workflow [Verify context handoff; optional release](.github/workflows/context-handoff-release.yml) kiểm tra đặc tả 386 dòng và hồ sơ 17 mục trên push. **Push thông thường không phát hành Release**. Chỉ yêu cầu thủ công với publish_release=true mới xuất ZIP; tag có sẵn được giữ nguyên.

Release cũ là bản lưu lịch sử tại SHA riêng, không phải bằng chứng firmware đã flash và không thay cho HEAD của nhánh được chỉ định.
