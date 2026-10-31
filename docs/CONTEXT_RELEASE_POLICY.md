# Chính sách ngữ cảnh và phát hành tùy chọn — Xe tăng cướp bóng STM32F767ZI

## Mục tiêu bất biến

**Ưu tiên:** đúng và đầy đủ ngữ cảnh → tiếp quản được ở hội thoại mới → kiểm chứng tin cậy → tiết kiệm thao tác. Repository [Spring-Mustache/control_motor](https://github.com/Spring-Mustache/control_motor) là bản chính của **những gì đã commit**, KHÔNG tự đồng bộ nội dung chat riêng, thử nghiệm chỉ gửi qua chat, hoặc file Windows chưa commit.

**Quy tắc hoạt động từ thay đổi chính sách 2026-10-31:** một lần push chạy CI kiểm tra hồ sơ và source regression, **không tạo Release**. Chỉ `workflow_dispatch` với `publish_release=true` mới đóng gói và xuất bản prerelease sau khi kiểm tra đạt. Các Release/tag cũ được giữ nguyên, không xóa lịch sử.

## Ba lớp trách nhiệm

| Lớp | Vai trò bắt buộc | Điều lớp này không thể chứng minh |
|---|---|---|
| 1. Git commits + source/docs tại đúng SHA | Lưu quyết định hiện hành, lịch sử, thay đổi source, milestone, bằng chứng đã ghi; có thể đọc trực tiếp và dùng `git log`/`git diff` | Không tự biết nội dung chat chưa được commit hay firmware nào đã Flash |
| 2. GitHub Actions trên mỗi push | Kiểm tra nguyên văn 386 dòng, SHA-256, cấu trúc 0–16, cập nhật handoff/roadmap cùng code, các verifier/test C; xuất trạng thái CI tại SHA | Không thay việc đánh giá ngữ nghĩa, không tự Clean Build CubeIDE, Flash hoặc nghiệm thu phần cứng |
| 3. GitHub Release (theo mốc/chủ động) | ZIP và `RELEASE_STATUS.md`, bản bàn giao ngoại tuyến, tag bất biến gắn đúng SHA; thích hợp milestone quan trọng hoặc không truy cập GitHub trực tiếp | Không phải nguồn cập nhật hơn HEAD, không phải điều kiện để chat mới tiếp tục |

### Vì sao Release không còn là phụ thuộc

Git commit đã lưu chính xác từng file theo SHA, lịch sử và diff. Release hiện chỉ sao chép các tài liệu này vào ZIP và thêm trạng thái sinh tự động; không tự thu được các CSV/TXT riêng tư, trạng thái flashed hoặc quyết định chưa commit. Một Release cũ **không được** chọn làm HEAD chỉ vì đứng đầu danh sách.

**Ưu điểm của Release cần giữ:** gói offline tại SHA, URL và assets tập trung, chốt milestone có ý nghĩa. Khi có yêu cầu, vẫn tạo đúng ZIP, `RELEASE_STATUS.md`, pre-release `context-<FULL_SHA>` với `--latest=false`. Việc tạo lại cùng tag không xóa hoặc ghi đè bản cũ.

## Đặc tả NGUYÊN VĂN và hai lớp nội dung của hồ sơ

**KHÔNG THAY ĐỔI** `docs/CONTEXT_HANDOFF_REQUIREMENTS_ORIGINAL.md`: đúng 386 dòng, byte và SHA-256 `13d8976ec0d74057af94bf84c8445bddbb48715b308964e22eb6684711f708a4`. Đây là yêu cầu người vận hành cho **mỗi lần bàn giao**. Không thay thế bằng README, prompt, workflow hay một bản tóm tắt.

`CONTEXT_HANDOFF.md` có **hai mặt bổ sung nhau**, không phải hai tài liệu loại trừ nhau:

1. **Snapshot lịch sử A và toàn bộ sự kiện/giải pháp đã thử:** phải giữ nguyên sự thật ở thời điểm đó, kể cả lỗi cũ, giả thuyết đã loại, trạng thái source cũ và bằng chứng. Có thể đánh dấu `LỊCH SỬ`, `ĐÃ THAY THẾ`; không xóa để làm tài liệu có vẻ hiện đại.
2. **Lớp hiện hành đang cập nhật:** mục 1 ở đầu phải định hướng đúng vấn đề và hành động đang chờ, các cập nhật liên quan trong mục 0–16 và phụ lục phải được rà soát sau code/decision/log mới. **Không** sửa snapshot cũ thành chứng cứ hiện tại; ghi cập nhật mới có nguồn và thời điểm. Nội dung phần 0–16 phải bảo toàn yêu cầu, lịch sử quyết định, các vấn đề đã giải quyết/chưa giải quyết và trạng thái kiểm chứng.

`docs/PROJECT_MILESTONES.md` cũng gồm snapshot lịch sử và cập nhật tích lũy. Bảng ở đầu vốn từ 2026-10-17, không phải bảng trạng thái hiện hành; phải đọc phần **định hướng hiện hành** và những mục cập nhật mới gần cuối. Dùng `git log`, `git diff` và các tài liệu milestone để đối chiếu tình trạng mới nhất. Không suy luận commit mới đã build/flash.

**Khi code, scripts hoặc workflow thay đổi:** phải thực sự rà soát các nguồn còn đọc được, cập nhật hồ sơ **đầy đủ 17 mục (0–16)** theo đặc tả gốc, giữ `END OF CONTEXT HANDOFF`, đồng thời cập nhật `docs/PROJECT_MILESTONES.md` trong **cùng push/commit**. Không chỉ chèn câu trống để qua kiểm tra. Khi chỉ có thay đổi yêu cầu, quyết định hay kết quả phần cứng, cũng phải cập nhật tài liệu tương ứng và commit sớm, dù không có thay đổi code. Chỉ công khai các dữ liệu/log được người dùng cho phép đăng (repository public). Không dựng lại phần chat đã mất.

### Mức độ chứng cứ

Luôn tách `SOURCE-STAGED`, `HOST TEST PASS`, `CUBEIDE BUILD PASS`, `FLASHED`, `HARDWARE PASS trên giá treo`, `HARDWARE PASS có tải mặt đất`, cùng `USER-REPORTED`, `CHƯA KIỂM CHỨNG`. GitHub Actions PASS kiểm tra source/hồ sơ, **không** chứng minh firmware đã được Flash hay NC/STOP/cơ khí an toàn. FIRST-STOP RIGHT cấp điện khi CCR=0 **đã được người dùng xác nhận fix**; lịch sử cảnh báo cũ không phải lỗi hiện hành. R2 RIGHT code=9 là vấn đề khác.

## Workflow, quyền và kiểm tra

Tệp `.github/workflows/context-handoff-release.yml` được giữ nguyên đường dẫn để không làm hỏng bookmark. Mỗi push tới nhánh mang workflow sẽ chạy **job `verify-handoff`** với quyền `contents: read`; mỗi `workflow_dispatch` cũng kiểm tra. Trình tự:

1. Checkout đúng `github.sha`, đối chiếu `git rev-parse HEAD`, dùng `fetch-depth: 0`.
2. `tools/verify_context_handoff_contract.py` bảo vệ nguyên văn SHA-256/386 dòng, đủ 0–16/footer, và kiểm tra file code/tools/workflow đổi thì cả handoff và roadmap đều đổi trong diff. Trên push dùng `github.event.before`, trên dispatch không có before dùng parent của SHA; không âm thầm bỏ bước update-gate.
3. `tools/test_context_handoff_process.py` tự thử kịch bản commit chỉ đổi code bị chặn, commit code+docs được nhận, spec bị thay bị chặn, và khôi phục tài liệu chỉ từ commit không có Release.
4. Chạy S14.3 và S14.2/S14.5 verifier + C/GCC unit tests, giữ cơ chế bảo vệ cũ. Bất kỳ bước FAIL nào là CI FAIL. Tài liệu không được nhận tự động là đúng về mặt ngữ nghĩa; người/assistant vẫn phải rà soát nội dung.

Workflow không tự chặn `git push` nếu branch không bắt buộc status checks; CI FAIL là tín hiệu cần sửa. Chỉ yêu cầu branch protection có **quyết định và cấu hình GitHub riêng**, không mặc định đã bật. Manual Release phải chờ `verify-handoff` PASS rồi mới mở quyền `contents: write` ở job phát hành. Thiếu quyền chỉ làm job phát hành lỗi, không thay đổi kết quả kiểm tra code/hồ sơ.

### Cách tạo gói ZIP theo yêu cầu

Khi cần milestone, snapshot độc lập hoặc bàn giao offline: mở Actions → workflow **Verify context handoff; publish ZIP only on explicit request** → **Run workflow**, chọn **đúng nhánh** đã kiểm tra HEAD và đánh dấu `publish_release=true`. Hoặc (GitHub CLI, nếu đã xác thực):

```powershell
gh workflow run context-handoff-release.yml --ref "feature/m8-s15-dual-target-common-time" -f publish_release=true
```

Workflow chỉ publish khi kiểm tra PASS. Package đọc dữ liệu từ Git tree **đúng SHA đã checkout**, không lấy file working copy có thể chưa commit. Output có `RELEASE_STATUS.md`, ZIP tài liệu, README, 386-line spec, handoff 0–16, roadmap, tài liệu M7/M8, các verifier/test được chọn; không mặc định đính kèm log riêng tư. Nếu Release/tag SHA đã tồn tại, giữ nguyên; không ghi đè assets. Nếu không có Release tương ứng HEAD, đó là bình thường, không phải lỗi đồng bộ.

## Cách tiếp quản trực tiếp từ repository (không cần Release)

1. Từ `README.md` và `docs/CHAT_B_START_PROMPT.md`, dùng nhánh M8-S15 đang làm hoặc nhánh người dùng chỉ định mới và xác minh **HEAD SHA thật bằng API/Git**; **KHÔNG** lấy HEAD của `main`, nhánh M7-S15 cũ hoặc một SHA đã ghi trong snapshot làm HEAD M8-S15.
2. Mọi file đọc theo **cùng SHA cố định**, trước tiên mục 1 `CONTEXT_HANDOFF.md` để định hướng, rồi đọc **toàn bộ mục 0–16 và cập nhật/phụ lục**, `docs/PROJECT_MILESTONES.md` (đầu và cuối), `docs/CONTEXT_RELEASE_POLICY.md`, bản gốc 386 dòng, tài liệu M7/M8 và code liên quan. Xem `git log -n 20`, commit diff mới nhất; xác minh CI PASS trên SHA đó.
3. Tổng hợp quyết định hiện hành, các phương án đã loại, sự kiện và nguồn; tách USER/REPO/LOG/HOST, hành động và tiêu chí hoàn tất, điểm thiếu và mức kiểm chứng. Nếu hồ sơ cũ mâu thuẫn với mã/log mới, đối chiếu thời điểm; không tùy tiện sửa sự thật cũ.
4. **Release tùy chọn:** nếu có `context-<FULL_SHA>`, kiểm tra tag/target SHA và đọc `RELEASE_STATUS.md` hoặc ZIP để tiện; nếu không có, **tiếp tục bình thường bằng repository**. Không dùng Release của SHA cũ thay cho HEAD mới.
5. Nếu không có GitHub connector, thử link GitHub trực tiếp; chỉ yêu cầu file tối thiểu khi không có cách đọc. Nếu phát hiện dữ liệu chỉ nằm trong hội thoại riêng, nói rõ thiếu gì thay vì tự tạo dữ kiện.

## Kiểm chứng quy trình mới và giới hạn

**Case push:** thay code mà không đổi cả `CONTEXT_HANDOFF.md` + roadmap phải làm CI FAIL; thay code và cập nhật hai tài liệu phải PASS, nhưng chất lượng nội dung phải rà soát thủ công. Workflow chạy source regression ở cùng SHA và **không xuất Release** khi push.

**Case chat mới:** lấy HEAD của nhánh, đọc đúng tài liệu và commit/diff/CI trên cùng SHA, không tìm hoặc đòi Release. Nếu không có Release, vẫn xác định được mốc hiện hành, hành động tiếp theo và mức đã kiểm chứng. Bài test tự động mô phỏng clone/checkout không có Release; thao tác thật ở một hội thoại ChatGPT hoàn toàn mới không thể được giả lập/khẳng định chỉ bằng Python.

**Rủi ro còn giữ:** tài liệu quên cập nhật dù code không đổi, prose đổi tượng trưng nhưng thiếu thông tin, GitHub mất truy cập, private log không trong repo, workflow không có trên nhánh/không được chạy. CI không chứng minh nội dung hội thoại đã được ghi đủ; người thực hiện phải kiểm tra và commit.
