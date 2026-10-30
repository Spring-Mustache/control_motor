# LỜI NHẮN KHỞI ĐẦU CHO HỘI THOẠI B — TIẾP QUẢN DỰ ÁN TỪ GITHUB

> **Dành để sao chép nguyên văn vào cuộc hội thoại mới (B).** Bản này thay thế cách cũ chỉ đính kèm một file `CONTEXT_HANDOFF.md` tĩnh.

Tôi đang chuyển việc của dự án **xe tăng cướp bóng — firmware STM32F767ZI** từ hội thoại A sang hội thoại B này. Đừng yêu cầu tôi kể lại toàn bộ lịch sử. **GitHub là nguồn bàn giao mã và tài liệu đã commit**; hồ sơ hội thoại A chỉ là bản chụp lịch sử.

## Địa chỉ dự án phải truy cập

- Repository: https://github.com/Spring-Mustache/control_motor
- Nhánh đang làm việc tại thời điểm tạo lời nhắn: `feature/m7-s15-ps2-controls-restoration-staged` (không phải `main`).
- Danh sách các bản phát hành bàn giao: https://github.com/Spring-Mustache/control_motor/releases
- Tài liệu định hướng ở nhánh: `README.md` và `docs/CONTEXT_RELEASE_POLICY.md`.

**Đừng chọn bản Release chỉ vì nó đứng đầu trang**: GitHub Releases chứa cả bản của `main` và các nhánh khác. Trước tiên, kiểm tra **HEAD SHA hiện tại của đúng nhánh** từ repository, sau đó tìm prerelease có tag `context-<FULL_HEAD_SHA>`. Nếu chưa tồn tại bản trùng SHA, kiểm tra GitHub Actions và báo rõ là Release chưa đồng bộ. Khi đó được đọc mã và tài liệu trực tiếp tại HEAD; không tự giả định Release cũ là hiện hành.

## Thứ tự bắt buộc để tiếp nhận

1. **Kiểm tra repository, nhánh và HEAD SHA thực tế.** Ghi rõ SHA và link commit/branch đã xác minh, không lấy SHA trong hồ sơ cũ làm HEAD mặc định.
2. **Mở đúng Release `context-<FULL_HEAD_SHA>`** (nếu đã phát hành). Đọc asset `RELEASE_STATUS.md`: xác nhận nhánh, SHA, file đổi, 20 commit gần nhất, giới hạn kiểm chứng. Tải ZIP `context-handoff-<SHORT_SHA>.zip` nếu có thể.
3. **Đọc đầy đủ hồ sơ `CONTEXT_HANDOFF.md` ở thư mục gốc** (trực tiếp từ repo tại SHA hoặc trong ZIP), gồm cả phụ lục, các mục 0–16. Đây là hồ sơ hội thoại A, **không tự thay đổi khi chat B có thông tin mới**.
4. **Đọc `docs/PROJECT_MILESTONES.md`**, đặc biệt những mục cập nhật cuối tài liệu; bảng trạng thái ở đầu có thể cũ. Đọc `docs/CONTEXT_RELEASE_POLICY.md` và tài liệu `docs/M8_*.md`, `docs/M7_*.md` liên quan đến milestone đang làm. Đối chiếu code ở cùng SHA trước khi đề xuất sửa.
5. **Đọc thay đổi kể từ thời điểm chốt hồ sơ A**, dựa trên `RELEASE_STATUS.md`, commit history và các quyết định đã được cập nhật trong tài liệu. Nếu tài liệu mâu thuẫn, làm rõ nguồn/thời điểm; không tự chọn thông tin thuận tiện.
6. **Nếu không thể truy cập một tài nguyên**, nói chính xác đã đọc được phần nào và thiếu gì; thử link GitHub/asset phù hợp trước khi đề nghị tôi tải lại. Không khẳng định đã đọc đủ khi chỉ thấy README hoặc tóm tắt.

## Nội dung phải xác định sau khi đọc

- Mục tiêu, yêu cầu và ràng buộc **đang có hiệu lực**, tách khỏi yêu cầu cũ bị thay thế.
- Milestone đang thực hiện, kết quả đã có, bằng chứng và mức kiểm chứng: `SOURCE-STAGED`, `HOST TEST`, `CUBEIDE BUILD`, `FLASHED`, `HARDWARE PASS` là các trạng thái khác nhau.
- Công việc gần nhất chưa hoàn tất, **đúng điểm dừng**, các tệp/code/log cần dùng, việc đầu tiên có thể làm ngay, tiêu chí hoàn thành.
- Những quyết định phải giữ, phương án đã loại và lý do, vấn đề hoặc rủi ro thực sự còn mở.
- Phân biệt thông tin do tôi báo, quyết định tôi đã chấp thuận, đề xuất của assistant, việc đã commit, việc đã kiểm tra, suy luận và `CHƯA KIỂM CHỨNG`.

**Lưu ý lịch sử kỹ thuật:** M8-S14.2 là milestone gần nhất được ghi nhận lúc lập hồ sơ A: chế độ thử nghiệm tăng trần PWM manual LEFT/RIGHT đến **450/200 CCR**, nấc trung gian **375/150**, đồng bộ góc–vận tốc dựa AS5600 với ưu tiên **tăng PWM motor chậm trước**, chỉ hãm motor nhanh khi phía chậm đã bão hòa; mặc định OFF. Trạng thái lúc đó là **đã commit mã, chưa xác minh Clean Build/Flash và vận hành phần cứng với trần cao**. Đây là thông tin thời điểm A, **không tự coi vẫn là trạng thái mới nhất nếu GitHub có cập nhật sau đó**.

**Lỗi FIRST-STOP RIGHT vẫn được cấp điện khi CCR=0 đã được tôi xác nhận FIX.** Không lặp lại nhận định lỗi này còn tồn tại chỉ vì tài liệu lịch sử có cảnh báo cũ. Lỗi R2 RIGHT `code=9` từng được ghi nhận là vấn đề khác; đừng trộn hai sự kiện.

## Cách làm việc với tôi trong B

- Dùng tiếng Việt; đi thẳng vào mục tiêu, chủ động kiểm tra repo/log bằng công cụ được phép, không bắt tôi tự chẩn đoán hoặc trả lời lại những điều đã có.
- Không bắt đầu lại dự án, không tùy tiện đổi phương án/giới hạn an toàn hoặc tự nâng trạng thái nghiệm thu. Nếu có bằng chứng mới làm thay đổi quyết định cũ, nêu rõ.
- Chỉ hỏi một thông tin thực sự quyết định bước tiếp theo nếu không thể tự lấy từ nguồn sẵn có. Vẫn tiến hành các phần đủ dữ liệu và quyền thực hiện.
- Không mặc định file, phiên công cụ, đường dẫn máy Windows hoặc lịch sử chat A còn dùng được trong B. Khi cần thao tác trên firmware, kiểm tra GitHub SHA, workspace và quyền trước.
- Sau mỗi thay đổi mã, kết quả thử xe hoặc quyết định quan trọng: **cập nhật tài liệu milestone tương ứng cùng commit, push lên GitHub và xác minh Release `context-<SHA>`**. Workflow không tự đọc nội dung ChatGPT. Không tự đăng dữ liệu nhạy cảm lên repo công khai.

## Phản hồi đầu tiên của bạn

Viết ngắn gọn: **đã mở nguồn nào, xác minh nhánh/SHA nào, hiểu dự án đang ở đâu, còn thiếu bằng chứng gì và hành động đầu tiên là gì**. Nếu có đủ dữ liệu/công cụ/quyền, **tiến hành ngay hành động đầu tiên** thay vì chỉ trả lời “đã hiểu”. Nếu nguồn không truy cập được, nói rõ và đưa một cách cung cấp link/tệp tối thiểu; không tự nhận nhớ cuộc hội thoại A.

**Bắt đầu bằng việc mở repository và xác định HEAD của nhánh được chỉ định, rồi tìm đúng bản phát hành ngữ cảnh gắn SHA đó.**
