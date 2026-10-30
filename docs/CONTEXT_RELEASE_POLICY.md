# Quy tắc hồ sơ ngữ cảnh và GitHub Releases

## Mục tiêu

Chuyển giao dự án `Spring-Mustache/control_motor` giữa nhiều hội thoại mà không lệ thuộc vào khả năng ChatGPT còn nhớ hội thoại trước. GitHub là bản gốc bền vững cho mã nguồn, lịch sử commit và các tài liệu **được commit**. Mỗi lần **push một commit mới lên nhánh có workflow**, GitHub Actions tạo một **prerelease bàn giao** tương ứng `context-<full SHA>`. Bản phát hành gồm `RELEASE_STATUS.md`, file ZIP `context-handoff-<short SHA>.zip`, và GitHub tự cung cấp archive mã nguồn tại tag đó.

> Đây là bản phát hành **ngữ cảnh**, không phải phát hành firmware đã build hay chứng nhận an toàn. Workflow không Flash MCU, không kiểm tra cơ khí/điện và không tự truy cập dữ liệu ChatGPT.

## Nguồn và thứ tự ưu tiên

1. **`RELEASE_STATUS.md` trong release mới**: xác nhận SHA, nhánh, danh sách file đổi tại commit và lịch sử 20 commit; tự tạo sau mỗi push.
2. **`CONTEXT_HANDOFF.md`**: bản bàn giao hội thoại A tại thời điểm chốt 2026-10-31; giữ lịch sử 0–16 đầy đủ. **Không tự viết lại theo nội dung hội thoại mới**. Các SHA và trạng thái trong đó là snapshot, không thay thế trạng thái thực tế của commit mới.
3. **`docs/PROJECT_MILESTONES.md` và `docs/M8_*.md`, `docs/M7_*.md`**: hồ sơ milestone và chi tiết kỹ thuật do người thực hiện cập nhật/commit. Các mục mới nhất phải ưu tiên hơn ghi chú lịch sử cũ.
4. **Mã nguồn tại commit của release và các log thực tế đã được duyệt đăng**: quyết định cuối cùng cần đối chiếu trạng thái MCU/build với bằng chứng đó.

**Các thay đổi trong chat, kết quả thử nghiệm chỉ được người dùng gửi qua chat và chưa commit sẽ KHÔNG tự xuất hiện trong release.** Sau những thay đổi ấy cần cập nhật `.md` và commit. Không lầm `git push` với việc firmware đã được Flash; không suy từ unit test sang PASS cơ khí.

## Đặc tả bàn giao NGUYÊN VĂN của người vận hành — bắt buộc

Đây là **hai tài liệu khác nhau**, không thay thế hoặc viết lại cho nhau:
- `docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md`: **toàn bộ văn bản gốc 386 dòng do người vận hành cung cấp**, giữ nguyên byte và ngữ nghĩa, SHA-256: `13d8976ec0d74057af94bf84c8445bddbb48715b308964e22eb6684711f708a4`. Đây là **yêu cầu bắt buộc áp dụng mỗi khi xuất hồ sơ bàn giao mới**, không phải bản tóm tắt và không phải lời nhắn cho B.
- `docs/CHAT_B_START_PROMPT.md`: lời nhắn giúp AI B truy cập đúng nhánh, commit và Release. Có thể biên tập hướng dẫn này khi cách bàn giao thay đổi, **nhưng không được thay đổi yêu cầu gốc theo nó**.

**Mỗi khi có phiên bản code mới:** phải áp dụng NGUYÊN VĂN đặc tả gốc để rà soát các nguồn còn truy cập được, tạo/cập nhật hồ sơ `CONTEXT_HANDOFF.md` **đủ 17 mục 0–16, không bỏ hoặc viết gọn thông tin có giá trị để tiếp tục**, cập nhật `docs/PROJECT_MILESTONES.md`, rồi mới commit/push cùng code. Phân biệt thông tin mới từ chat, nguồn thực sự đọc được và phần chưa kiểm chứng. Khi lịch sử chat bị rút gọn, ghi rõ thay vì tự dựng.

GitHub Actions thực thi `tools/verify_context_handoff_contract.py` trước khi tạo Release:
- Kiểm tra văn bản gốc còn **đúng từng byte** và hồ sơ có các mục 0–16 cùng dòng kết thúc bắt buộc.
- Nếu lần push có thay đổi mã firmware/source/scripts, **bắt buộc** có cả `CONTEXT_HANDOFF.md` và `docs/PROJECT_MILESTONES.md` thay đổi trong lần push; nếu thiếu thì job **FAIL, không phát hành Release**. Đây là kiểm soát tự động để tránh bàn giao file cũ như file mới.
- Kiểm tra máy không thể tự chứng minh tất cả 17 mục đã đủ ý nghĩa/đúng sự thật. Người cập nhật/AI có quyền truy cập nguồn phải thực sự làm đủ theo văn bản nguyên gốc; GitHub Actions **không thể tự đọc hội thoại ChatGPT, hồi phục lịch sử đã mất hoặc tự viết trung thực nội dung chỉ có trong chat**.
- Workflow có thể đánh dấu thất bại sau khi push; nó không tự chặn Git commit hay ghi lại hội thoại. Khi thất bại, sửa hồ sơ và push commit bổ sung có nội dung phù hợp, chạy lại workflow. Đối chiếu các commit đã thiếu tài liệu trong lịch sử, không xóa đi.

## Bước duy trì ngữ cảnh khi có thay đổi

- Trước khi commit tính năng: cập nhật phần trạng thái mới nhất và điều còn thiếu trong `docs/PROJECT_MILESTONES.md`; thêm/cập nhật tài liệu riêng cho milestone đang làm.
- Sau khi có CSV/TXT mới: ghi tên log, điều kiện đo, kết quả và mức kiểm chứng; chỉ commit log gốc khi người dùng cho phép đăng công khai. Repo hiện là **public**.
- Khi người dùng đổi yêu cầu (ví dụ FIRST-STOP được xác nhận đã fix): ghi quyết định thay thế và nguồn vào tài liệu; không tiếp tục sao chép cảnh báo cũ thành lỗi đang mở.
- Commit và push code + docs cùng nhau, không đợi chat quá dài. Release tự động gắn SHA tương ứng; xác minh tại tab **Actions** và **Releases**.
- Khi chuyển chat, B mở release `context-<sha>` mới nhất của **nhánh đang làm**, đọc `RELEASE_STATUS.md` trước, sau đó mở `CONTEXT_HANDOFF.md` trong ZIP và roadmap. Không chỉ chọn release dựa vào giờ tạo nếu các nhánh khác nhau.

## Phạm vi kích hoạt và giới hạn của GitHub Actions

Workflow `context-handoff-release.yml` theo dõi **push tới tất cả nhánh chứa workflow**, gồm `main` và nhánh `feature/m7-s15-ps2-controls-restoration-staged` tại thời điểm thiết lập. Một lần push nhiều commit chỉ phát hành cho **SHA đầu nhánh của lần push** (không tự phát hành từng commit trung gian). Nhánh mới không mang workflow sẽ không chạy workflow; hãy merge/copy workflow trước khi muốn nó tự phát hành.

Release được đánh dấu `prerelease` với `--latest=false` để không giả làm phiên bản firmware chính thức. Tên tag `context-<full SHA>` tránh trùng giữa các nhánh, không có lệnh push commit từ workflow nên không tạo vòng lặp vô hạn. Re-run workflow cập nhật ZIP cùng tag. Repository cần cho phép GitHub Actions và `GITHUB_TOKEN` có quyền `contents: write`; nếu chính sách repository chặn, Action sẽ thất bại và **không có bản phát hành**: kiểm tra log, quyền ở Settings → Actions và chạy lại. Không ghi secret/token vào tài liệu.

## Lời nhắn chuyển tiếp dùng trong mọi cuộc hội thoại mới

Dùng [`docs/CHAT_B_START_PROMPT.md`](CHAT_B_START_PROMPT.md) làm lời nhắn đầu tiên trong cuộc hội thoại B (có thể sao chép nguyên văn). Lời nhắn bắt đầu bằng việc xác minh HEAD của nhánh đang làm, tìm `context-<SHA>` đúng với nhánh và mở `RELEASE_STATUS.md`; sau đó đọc **`CONTEXT_HANDOFF.md` tại thư mục gốc** cùng roadmap. Không yêu cầu người dùng phải đính kèm lại file tĩnh khi có thể truy cập release. Workflow cũng đóng gói lời nhắn này vào ZIP để dùng ở các cuộc hội thoại C/D tiếp theo.

## Checklist của hội thoại B

1. Xác nhận SHA **đúng nhánh** từ `RELEASE_STATUS.md` và GitHub; xác nhận mã trên Windows trùng phiên bản dự kiến.
2. Đọc milestone cuối và mã/diff thực tế; tách `SOURCE-STAGED`, `HOST TEST PASS`, `CUBEIDE BUILD PASS`, `FLASHED`, `HARDWARE PASS`.
3. Không hỏi lại kết quả user đã chốt, đặc biệt FIRST-STOP; nếu muốn xác minh bằng hồ sơ firmware thì yêu cầu bằng chứng cụ thể, không coi lỗi đã tái diễn.
4. Tiếp tục công việc M8-S14.2 nếu trạng thái chưa đổi: build/flash, kiểm tra an toàn thực tế, thử level 1 trước level 2 trên giá treo và phân tích log đồng bộ.
5. Bất kỳ thay đổi mới nào ảnh hưởng mục tiêu hoặc milestone phải được commit vào GitHub để gói bàn giao tiếp theo không bị tụt hậu so với chat.