# LỜI NHẮN KHỞI ĐẦU CHO HỘI THOẠI MỚI — TIẾP QUẢN TRỰC TIẾP TỪ GITHUB

> **Sao chép nguyên văn phần lời nhắn bên dưới vào một hội thoại mới.** Đây là hướng dẫn lấy ngữ cảnh, không thay thế `docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md` 386 dòng.

Tôi muốn bạn tiếp tục dự án **xe tăng cướp bóng — STM32F767ZI**, không bắt tôi kể lại toàn bộ lịch sử.

**Nguồn chính:** https://github.com/Spring-Mustache/control_motor

**Nhánh phải kiểm tra HEAD trước tiên:** `feature/m7-s15-ps2-controls-restoration-staged` (không mặc định là `main`). Nếu chỉ thị mới của tôi đổi nhánh, ưu tiên chỉ thị mới. SHA trong tài liệu là snapshot lịch sử, không phải HEAD.

**Không cần Release cho SHA mới nhất.** Theo chính sách hiện hành, GitHub Actions kiểm tra ngữ cảnh trên mỗi push; gói Release `context-<FULL_SHA>` chỉ được tạo khi chọn thủ công cho milestone/offline. Thiếu Release ở HEAD hoàn toàn bình thường. Không yêu cầu tôi tạo Release hoặc tải lại hồ sơ nếu bạn đọc được GitHub.

## Các bước phải thực hiện ngay bằng công cụ GitHub

1. **Xác minh repo và HEAD SHA của đúng nhánh** qua GitHub API/ref hoặc `git rev-parse`; ghi chính xác SHA đã kiểm tra. Đọc tất cả source/docs tiếp theo tại **cùng SHA**, tránh vô tình đọc `main` hoặc phiên bản cũ.
2. Đọc **mục 1 cập nhật hiện hành** của `CONTEXT_HANDOFF.md`, rồi đọc đầy đủ **17 mục 0–16 và các bổ sung lịch sử** (không coi phần đầu lịch sử cũ là sự thật hôm nay). Đọc `docs/PROJECT_MILESTONES.md`: phần trạng thái hiện hành ở đầu và các mục bổ sung mới nhất gần cuối; bảng có ngày 2026-10-17 là snapshot, không phải bảng hiện hành.
3. Đọc `docs/CONTEXT_RELEASE_POLICY.md`, `docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md` nguyên văn, README, tài liệu M7/M8 và code liên quan. Kiểm tra `git log -n 20`, diff commit mới nhất, GitHub Actions kiểm tra hồ sơ ở đúng SHA. Đừng coi Actions PASS là CubeIDE Build, Flash hoặc hardware PASS.
4. Tự tổng hợp: mục tiêu/yêu cầu và quyết định kỹ thuật đang có hiệu lực; phương án đã thử/loại và lý do; milestone/tiêu chí nghiệm thu; lỗi đã sửa và lỗi còn mở; SOURCE/HOST/CUBEIDE/FLASH/HARDWARE riêng biệt; tình trạng thử ở giá treo/mặt đất; việc đang dừng, hành động tiếp theo và đầu vào còn thiếu.
5. **Release là tài liệu tham khảo tùy chọn**. Nếu có `context-<FULL_SHA>` ở https://github.com/Spring-Mustache/control_motor/releases thì có thể kiểm tra tag target, mở `RELEASE_STATUS.md` và ZIP; nếu không có, **tiếp tục bình thường từ repository tại SHA**, tuyệt đối không dùng Release cũ để thay HEAD.
6. Nêu rõ phần nguồn đã đọc, phần thiếu và độ tin cậy; log riêng tư/CSV, file Windows, ảnh trong cuộc hội thoại khác có thể không hiện trong repository. Chỉ hỏi tôi đúng một thông tin thiết yếu nếu không thể tự lấy. Không tự tạo số liệu, commit SHA, kết quả đo, bằng chứng hay quyền hành động.

## Giữ đúng các quyết định và bảo vệ

- Điều khiển 4 motor bánh + 2 motor lái, PS2/VC-02, encoder/AS5600, NC/STOP. Kiểm tra trạng thái firmware **thực tế ở HEAD**, không mặc định theo milestone cũ.
- S14.2 Level1 và S14.3 Fast Start đã được chọn boot-on trong source trước đó; **S14.5 early RIGHT R_MIN catch-up là thử nghiệm boot-OFF** tại mốc gần nhất đang được theo dõi. Điều này cần kiểm tra lại bằng code hiện hành.
- FIRST-STOP RIGHT cấp điện khi CCR=0 **đã được người dùng xác nhận sửa**. R2 RIGHT code=9 là sự kiện độc lập. Không tự kích hoạt Level2, cấp nguồn động lực, hoặc coi stop/coast đã nghiệm thu khi chưa có dữ liệu.
- Không tự thay đổi firmware/giới hạn an toàn; dữ liệu code commit khác firmware đã CubeIDE Clean Build, flash và thử xe. Không đăng log phần cứng riêng tư lên repo public nếu chưa có quyền.

## Sau khi tiếp quản

Trả lời ngắn bằng tiếng Việt: **đúng repo/nhánh/SHA đã xác nhận, nguồn đã đọc, milestone đang làm, trạng thái bằng chứng, việc còn dở và hành động đầu tiên**. Nếu có dữ liệu/công cụ/quyền cho hành động đó thì tiếp tục thực hiện. Đừng chỉ trả lời “đã hiểu”.

Sau mỗi sửa mã hoặc kết quả thử quan trọng, cập nhật đúng 17 mục `CONTEXT_HANDOFF.md` theo **đặc tả gốc**, roadmap, tài liệu M7/M8 liên quan và commit; sau push kiểm tra Actions **PASS**. Không cần tạo Release cho mỗi commit. Nếu bản ghi mới mâu thuẫn với ghi chú cũ, giữ lịch sử và thêm kết luận cập nhật có nguồn, không xóa dữ kiện cũ.
