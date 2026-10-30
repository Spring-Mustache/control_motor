Hãy tạo một hồ sơ bàn giao ngữ cảnh đầy đủ từ cuộc hội thoại này để tôi chuyển sang một cuộc hội thoại mới và tiếp tục công việc một cách mạch lạc.

Gọi cuộc hội thoại hiện tại là A, cuộc hội thoại tiếp nhận là B.

MỤC TIÊU

Xuất một file Markdown tên CONTEXT_HANDOFF.md.

File phải giúp B hiểu:
- Chúng ta đang làm gì và vì sao.
- Mục tiêu, phạm vi, yêu cầu và các ràng buộc hiện hành.
- Toàn bộ milestone đã thống nhất và trạng thái từng milestone.
- Công việc đã làm, kết quả đạt được và mức độ kiểm chứng.
- Công việc đang dang dở và đúng điểm chúng ta đang dừng.
- Các quyết định đã chốt, lý do và phương án đã loại.
- Các vấn đề, rủi ro, phụ thuộc và câu hỏi còn mở.
- Phương án tiếp tục, thứ tự ưu tiên và hành động đầu tiên cần thực hiện.
- Cách phối hợp với tôi để tiếp tục tự nhiên như đang làm việc trong A.

Đây phải là hồ sơ đủ để tiếp quản công việc, không phải một bản tóm tắt chung chung.

NGUYÊN TẮC BẮT BUỘC

1. Rà soát toàn bộ nội dung hội thoại và tài liệu mà bạn thực sự còn truy cập được. Không chỉ dựa vào vài tin nhắn cuối.

2. Không giả định rằng bạn còn thấy đầy đủ lịch sử:
   - Nếu phần lịch sử cũ đã bị rút gọn, không còn truy cập được hoặc không thể kiểm tra, ghi rõ giới hạn đó.
   - Không tuyên bố “đã bao quát toàn bộ” khi không có căn cứ.
   - Không dựng lại chi tiết bị mất bằng suy đoán.

3. Phân biệt rõ:
   - Dữ kiện hoặc yêu cầu do người dùng cung cấp.
   - Quyết định đã được người dùng chấp thuận.
   - Đề xuất của assistant chưa được chấp thuận.
   - Việc đã thực hiện.
   - Kết quả đã kiểm chứng.
   - Kết quả mới được báo cáo nhưng chưa kiểm chứng.
   - Suy luận hoặc đánh giá mới.
   - Thông tin chưa biết.

   Một câu assistant từng nói “đã xong” không tự động chứng minh công việc đã thành công.

4. Khi thiếu dữ liệu, ghi “CHƯA CÓ DỮ LIỆU” hoặc “CHƯA KIỂM CHỨNG”, kèm ảnh hưởng của thiếu sót đó. Không tự điền số liệu, phần trăm tiến độ, thời hạn, phiên bản hoặc kết quả.

5. Khi yêu cầu hoặc quyết định thay đổi:
   - Ghi rõ trạng thái hiện hành.
   - Ghi quyết định cũ đã bị thay thế và lý do nếu biết.
   - Ưu tiên chỉ thị rõ ràng mới nhất của người dùng.
   - Nếu vẫn còn mâu thuẫn, giữ lại và đánh dấu cần làm rõ.

6. Giữ chính xác tên, thuật ngữ, thông số, đơn vị, công thức, đường dẫn, mã lỗi, phiên bản và nội dung đã chốt khi chúng có ảnh hưởng đến việc tiếp tục.

7. Không biến công việc dự kiến thành công việc đã hoàn thành. Không biến một ý tưởng đã thảo luận thành yêu cầu bắt buộc.

8. Có thể gộp nội dung lặp lại, nhưng phải giữ đầy đủ thông tin ảnh hưởng đến quyết định, triển khai, kiểm chứng và phối hợp.

9. Nếu hội thoại có nhiều dự án hoặc nhiều nhánh công việc, ghi rõ từng nhánh và xác định nhánh đang hoạt động. Không trộn trạng thái hoặc kết quả giữa các nhánh.

10. Khi nêu bằng chứng, dùng nguồn thực sự có: tên tệp, đoạn hội thoại, nội dung kiểm thử, kết quả công cụ hoặc đường dẫn. Không tạo ID tin nhắn, liên kết hoặc nguồn trích dẫn giả.

11. Không hỏi tôi xác nhận trước khi lập hồ sơ. Hãy xuất bản tốt nhất từ dữ liệu đang có và ghi rõ những khoảng trống.

CẤU TRÚC FILE

# CONTEXT HANDOFF

## 0. Thông tin hồ sơ và phạm vi bao phủ

Ghi:
- Tên dự án hoặc chủ đề.
- Phiên bản hồ sơ bàn giao.
- Thời điểm chốt trạng thái, nếu xác định được.
- Nguồn đã dùng để lập hồ sơ.
- Phạm vi lịch sử thực sự có thể rà soát.
- Những tài liệu hoặc phần hội thoại không truy cập được.
- Giới hạn của hồ sơ.

Phân biệt thời điểm tạo hồ sơ với thời điểm gần nhất trạng thái dự án được xác nhận.

## 1. Đọc mục này để tiếp tục ngay

Viết một bản định hướng ngắn nhưng cụ thể:
- Mục tiêu hiện hành.
- Yêu cầu gần nhất của tôi chưa được hoàn tất.
- Milestone và nhiệm vụ đang thực hiện.
- Bước cuối cùng đã thực hiện.
- Kết quả của bước đó và mức độ kiểm chứng.
- Phần đang dang dở.
- Vướng mắc hiện tại.
- Quyết định quan trọng phải giữ.
- Hành động đầu tiên B nên thực hiện.
- Dữ liệu hoặc tệp cần có để thực hiện hành động đó.
- Tiêu chí để biết hành động đó đã hoàn thành.

Mục này phải chỉ đúng điểm tiếp tục, không viết chung chung như “tiếp tục phát triển dự án”.

## 2. Bối cảnh, mục tiêu và phạm vi

Trình bày:
- Vấn đề cần giải quyết.
- Lý do thực hiện dự án.
- Đối tượng sử dụng hoặc bên liên quan.
- Mục tiêu cuối cùng.
- Các đầu ra cần bàn giao.
- Tiêu chí thành công và nghiệm thu.
- Phạm vi đã thống nhất.
- Các hạng mục ngoài phạm vi hoặc chủ động hoãn.
- Ràng buộc về thời gian, nguồn lực, công cụ, chi phí hoặc chất lượng đã được nêu.

Nếu mục tiêu đã thay đổi, ghi quá trình thay đổi và mục tiêu đang có hiệu lực.

## 3. Yêu cầu hiện hành và cách phối hợp với người dùng

Ghi đầy đủ:
- Yêu cầu chức năng và phi chức năng, nếu có.
- Yêu cầu về nội dung, thiết kế, định dạng hoặc cách bàn giao.
- Các thông số và quy ước đã chốt.
- Điều bắt buộc giữ nguyên.
- Điều tôi đã yêu cầu tránh hoặc không thực hiện.
- Những việc tôi đã cho phép thực hiện.
- Những việc còn cần tôi quyết định.
- Phong cách giao tiếp, ngôn ngữ và mức chi tiết tôi mong muốn.
- Cách tôi muốn bạn chủ động, đặt câu hỏi và báo tiến độ.
- Các lỗi hiểu sai mà tôi đã sửa cho assistant.

Phân biệt sở thích được tôi nói rõ với sở thích bạn suy ra.

## 4. Roadmap và milestone

Lập bảng:

ID | Milestone | Đầu ra | Tiêu chí nghiệm thu | Trạng thái | Bằng chứng tiến độ | Việc còn lại | Phụ thuộc | Hạn đã thống nhất

Sử dụng trạng thái rõ ràng, chẳng hạn:
- Chưa bắt đầu.
- Đang thực hiện.
- Bị chặn.
- Đã thực hiện, chờ kiểm chứng.
- Đã hoàn thành và kiểm chứng.
- Tạm hoãn.
- Đã hủy hoặc bị thay thế.

Yêu cầu:
- Giữ milestone đã thống nhất, kể cả milestone hoàn thành hoặc bị hủy.
- Ghi rõ milestone nào mới là đề xuất.
- Chỉ ghi phần trăm tiến độ khi có căn cứ.
- Phân biệt hạn cam kết với thời gian dự kiến.
- Nêu nguyên nhân bị chặn và điều kiện để tiếp tục.
- Nếu roadmap đã thay đổi, ghi bản hiện hành và thay đổi đáng chú ý.

Có thể tạo ID quản lý trong hồ sơ, nhưng phải nói rõ đó là ID mới nếu hội thoại chưa có.

## 5. Công việc đã thực hiện và kết quả hiện tại

Theo từng hạng mục, ghi:
- Việc đã làm.
- Đầu ra tạo ra.
- Trạng thái của đầu ra.
- Kết quả đạt được.
- Bằng chứng hoặc nguồn.
- Nội dung đã kiểm tra và kết quả kiểm tra.
- Nội dung chưa kiểm tra.
- Sai lệch so với yêu cầu.
- Phần còn thiếu.

Phân biệt rõ:
- Đã đề xuất.
- Đã viết hoặc tạo.
- Đã chạy hoặc áp dụng.
- Đã kiểm thử.
- Đã được người dùng nghiệm thu.
- Đã triển khai hoặc phát hành, nếu có.

Không gộp các trạng thái này thành một chữ “xong”.

## 6. Công việc đang dang dở và điểm dừng chính xác

Mô tả đủ để B tiếp quản:
- Nhiệm vụ đang làm và mục đích.
- Các bước đã hoàn tất trong nhiệm vụ.
- Bước đang dừng.
- Tệp, phần nội dung, chức năng hoặc dữ liệu đang xử lý.
- Thao tác hoặc thử nghiệm gần nhất.
- Kết quả gần nhất.
- Lỗi hoặc trở ngại hiện tại.
- Giả thuyết đang được kiểm tra.
- Kế hoạch tiếp tục đã nói với tôi.
- Điều tôi đang chờ nhận.
- Thông tin nào cần kiểm tra lại trước khi tiếp tục.

Nếu có nhiều việc dang dở, xếp theo ưu tiên và ghi quan hệ phụ thuộc.

## 7. Tệp, tài nguyên và phiên bản cần chuyển sang B

Lập danh mục:

Tài nguyên | Vai trò | Tên hoặc đường dẫn | Phiên bản hiện hành | Trạng thái | B có cần được cung cấp lại? | Ghi chú

Bao gồm các tài nguyên có liên quan:
- Tệp tôi đã tải lên.
- Tệp assistant đã tạo.
- Tài liệu yêu cầu.
- Mã nguồn, repository, nhánh, commit hoặc PR.
- Dataset, bảng tính, hình ảnh, sơ đồ và báo cáo.
- URL hoặc nguồn tham khảo.
- Công cụ, môi trường và phiên bản đã dùng.

Ghi rõ:
- Bản nào hiện hành.
- Bản nào đã lỗi thời.
- Tài nguyên nào chỉ được nhắc đến nhưng chưa được đọc.
- Tài nguyên nào không còn truy cập được.
- Tệp nào cần tôi đính kèm lại.

Không giả định B có thể truy cập tệp, đường dẫn cục bộ, phiên công cụ hoặc liên kết tải xuống của A.

Với nội dung quan trọng để tiếp tục, đưa phần cần thiết vào phụ lục nếu bạn thực sự có nội dung đó.

## 8. Lịch sử quyết định và các phương án đã cân nhắc

Lập bảng:

Quyết định | Trạng thái hiện hành | Phương án đã chọn | Lý do | Đánh đổi | Phương án đã loại | Điều kiện xem xét lại | Nguồn

Bao gồm:
- Quyết định đã chốt.
- Quyết định còn mở.
- Quyết định đã bị thay thế.
- Phương án đã thử và không phù hợp.
- Phương án bị loại trước khi thử.
- Những lựa chọn cần giữ để tránh làm lại công việc cũ.

Không dựng thêm lý do nếu hội thoại chưa nêu. Có thể ghi “chưa ghi nhận lý do”.

## 9. Vấn đề, lỗi và bài học từ những lần thử trước

Theo từng vấn đề:
- Hiện tượng.
- Điều kiện xảy ra.
- Thông báo lỗi hoặc dữ liệu quan trọng.
- Nguyên nhân đã xác nhận hoặc giả thuyết.
- Những cách đã thử.
- Kết quả từng cách.
- Cách đã giải quyết, nếu có.
- Cách chưa thử.
- Điều cần tránh lặp lại.
- Bài học ảnh hưởng đến bước tiếp theo.

Giữ nguyên mã lỗi, lệnh hoặc thông số cần thiết để tái hiện và kiểm tra.

## 10. Rủi ro, giả định và phụ thuộc

Lập bảng:

Rủi ro | Căn cứ | Khả năng xảy ra | Mức ảnh hưởng | Dấu hiệu cảnh báo | Biện pháp giảm thiểu | Phương án dự phòng | Quyết định cần có

Yêu cầu:
- Phân biệt rủi ro từng được thảo luận với đánh giá mới khi bàn giao.
- Giải thích cơ sở của mức đánh giá.
- Không tạo xác suất chính xác nếu thiếu dữ liệu.
- Gắn rủi ro với milestone hoặc công việc bị ảnh hưởng.

Liệt kê riêng:
- Giả định đang dùng nhưng chưa xác nhận.
- Phụ thuộc vào người, tệp, công cụ hoặc quyết định.
- Thông tin có thể đã thay đổi và cần kiểm tra lại.
- Những thiếu sót ngữ cảnh có thể khiến B tiếp tục sai hướng.

## 11. Kế hoạch tiếp tục có thể thực hiện

Lập kế hoạch theo thứ tự ưu tiên:

Ưu tiên | Hành động cụ thể | Mục đích | Đầu vào cần có | Đầu ra mong đợi | Tiêu chí hoàn thành | Phụ thuộc

Chia thành:
- Hành động đầu tiên khi sang B.
- Các bước để hoàn thành nhiệm vụ đang dang dở.
- Các bước để hoàn thành milestone hiện tại.
- Công việc tiếp theo sau milestone đó.

Phân biệt:
- Kế hoạch đã được tôi chấp thuận.
- Đề xuất mới của bạn.

Nếu có nhiều hướng tiếp tục:
- Nêu các hướng thực sự liên quan.
- So sánh lợi ích, hạn chế và rủi ro.
- Đề xuất hướng ưu tiên với lý do.
- Ghi điều kiện chuyển sang phương án khác.

Không đưa kế hoạch chung chung thiếu đầu ra hoặc tiêu chí hoàn thành.

## 12. Câu hỏi còn mở và thông tin cần bổ sung

Lập bảng:

Câu hỏi hoặc dữ liệu thiếu | Vì sao cần | Ảnh hưởng nếu chưa có | Có thể tiếp tục phần nào? | Nguồn có thể bổ sung | Mức ưu tiên

Phân biệt:
- Thông tin bắt buộc phải có trước bước tiếp theo.
- Thông tin có thể bổ sung sau.
- Thông tin có thể xác minh từ tài nguyên sẵn có.

Không hỏi lại những điều đã được trả lời trong hội thoại.

## 13. Dòng thời gian và diễn biến quan trọng

Ghi theo thứ tự:
- Các giai đoạn chính.
- Những lần đổi mục tiêu hoặc phạm vi.
- Những lần chốt hoặc đổi phương án.
- Kết quả quan trọng.
- Phản hồi và điều chỉnh của tôi.
- Các trở ngại và cách xử lý.
- Điểm chuyển sang trạng thái hiện tại.

Chỉ ghi ngày giờ nếu biết. Nếu không, dùng thứ tự tương đối.

Giữ đủ diễn biến để B hiểu vì sao dự án đi tới trạng thái hiện tại.

## 14. Phụ lục nội dung cần giữ chính xác

Đưa vào đây những nội dung mà chỉ tóm tắt sẽ dễ gây sai:
- Đặc tả hoặc yêu cầu đã chốt.
- Đoạn nội dung cuối cùng đã được duyệt.
- Công thức, thông số, cấu trúc dữ liệu hoặc quy ước.
- Đoạn mã, lệnh, cấu hình hoặc kết quả quan trọng.
- Checklist nghiệm thu.
- Ví dụ và phản ví dụ người dùng đã đưa.
- Định nghĩa thuật ngữ riêng của dự án.

Ghi nguồn và trạng thái của từng nội dung.

Không đưa nhiều bản cũ mà không đánh dấu rõ bản hiện hành.

## 15. Chỉ dẫn để hội thoại B tiếp quản

Viết một đoạn chỉ dẫn trực tiếp cho B:
- Dùng hồ sơ này để tái dựng ngữ cảnh làm việc.
- Giữ các yêu cầu và quyết định hiện hành.
- Đối chiếu trạng thái với tài nguyên thực tế khi cần.
- Tiếp tục từ đúng điểm dừng.
- Không làm lại công việc đã hoàn thành khi chưa có lý do.
- Không tái đề xuất phương án đã loại mà không xét lý do loại.
- Không coi đề xuất hoặc giả định là quyết định đã chốt.
- Không tuyên bố nhớ hoặc đã kiểm tra nội dung không được cung cấp.
- Chỉ hỏi những thông tin thực sự thiếu và ảnh hưởng đến bước tiếp theo.
- Tiếp tục phần công việc đủ dữ liệu và đã được cho phép.
- Khi tôi cung cấp cập nhật mới, điều chỉnh hồ sơ theo cập nhật đó.

## 16. Kiểm tra chất lượng và khoảng trống bàn giao

Trước khi xuất, rà soát:
- Milestone có khớp với tiến độ và kế hoạch tiếp tục không?
- Mục “đã hoàn thành” có bằng chứng tương ứng không?
- Quyết định đã bị thay thế có bị trình bày như còn hiệu lực không?
- Yêu cầu gần nhất chưa hoàn tất đã được giữ lại chưa?
- Có phân biệt bản hiện hành và bản cũ không?
- B có biết hành động đầu tiên, đầu vào và tiêu chí hoàn thành không?
- Có ghi tài nguyên cần cung cấp lại không?
- Có phần nào bị bỏ vì không còn truy cập được không?

Cuối file ghi:
- Các khoảng trống còn lại.
- Ảnh hưởng của từng khoảng trống.
- Những phần chưa thể kiểm chứng.
- Các mục không áp dụng và lý do.
- Trạng thái tài liệu: hoàn chỉnh trong phạm vi nguồn hiện có hoặc còn thiếu các phần cụ thể.

Kết thúc file bằng dòng:
END OF CONTEXT HANDOFF

YÊU CẦU XUẤT

- Viết bằng tiếng Việt, giữ nguyên tên và thuật ngữ cần thiết.
- Ưu tiên đầy đủ thông tin có giá trị cho việc tiếp tục; không giới hạn độ dài tùy tiện.
- Đặt bản định hướng ngắn ở đầu, hồ sơ chi tiết phía sau.
- Các mục phải có nội dung hoặc giải thích vì sao chưa có dữ liệu.
- Không dùng “...” để thay nội dung cần bàn giao.
- Không dừng ở việc đưa ra đề cương.
- Nếu có công cụ tạo tệp, hãy thực sự tạo và đính kèm CONTEXT_HANDOFF.md để tôi tải xuống.
- Nếu không thể tạo tệp, xuất toàn bộ nội dung Markdown trong khối mã để tôi lưu thành CONTEXT_HANDOFF.md; nói rõ đây là nội dung để lưu, không phải tệp đã được tạo.
- Nếu nội dung vượt giới hạn đầu ra, ưu tiên tạo toàn bộ vào tệp bằng công cụ. Nếu vẫn buộc phải chia phần, đánh số rõ, ghi phần đã xuất và phần còn thiếu, không âm thầm cắt nội dung hoặc tuyên bố đã hoàn chỉnh.
- Ngoài tệp, chỉ trả lời ngắn về tên tệp, phạm vi bao phủ và những tài nguyên quan trọng tôi cần mang sang B.

Hãy thực hiện ngay từ dữ liệu thực sự đang có.