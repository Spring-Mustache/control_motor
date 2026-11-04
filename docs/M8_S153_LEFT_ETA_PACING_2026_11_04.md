# S15.3 — điều tiết LEFT theo ETA đến SOFT (04/11/2026)

## Nguồn kết quả
Log M8S152_four_tests_20261104_131625.txt ghi bản f2137556 dirty1. D-pad LEFT L2006/R2150, +144ms (7,18%); D-pad RIGHT L1886/R2018, +132ms (7,00%); RX LEFT L1742/R2078, +336ms (19,29%); RX RIGHT L1826/R2090, +264ms (14,46%). Cả 4 hành trình đến SOFT, faults0. Chưa đạt yêu cầu dưới 2% từng lượt.

Hai lượt RIGHT->RMIN: từ còn 180RAW tới SOFT, LEFT mất 468/468ms, RIGHT mất 852/864ms. Hai lượt RIGHT->RMAX: LEFT mất 672/612ms, RIGHT mất 744/816ms. Đó là số đo quan sát giới hạn từ 4 hành trình, không phải hằng số động lực đã được chứng nhận.

## Giải pháp thử, không thay đổi PWM mặc định
Thư viện thuần C mới Core/Inc/steer_s153_left_pace.h, hàm M8S153_LeftPacePlan, đọc tốc độ S15.2 AS5600 hiện có (q-milli/s), chuyển về RAW/s bằng chiều dài hành trình đã hiệu chuẩn rồi ước lượng thời gian còn lại từ phần đường cách SOFT >180RAW cộng thời gian đoạn 180RAW cuối của từng hướng. Khi dự báo RIGHT về trễ hơn LEFT >60ms, đề xuất giảm LEFT: min((etaLag-60)/5,72CCR, ownerL-floorL). Không tác động RIGHT.

Cổng an toàn: hai trục MOVING, valid AS5600 cả hai trục và timestamp mới <=100ms, hướng tốc độ đúng, span700..1000RAW, qspeed>=600/s, rawSpeed>=150/s, NC0, không STOP/trip. Nếu một trục còn <=320RAW tới SOFT, đề xuất về0; giảm dần từ72CCR khi còn >=464RAW xuống0 tại320RAW. Không hạ LEFT dưới nền 300CCR, không sửa bộ owner cuối, NC/STOP/atomic, STALL650, RMIN195/SOFT245 hay 50CCR terminal RIGHT.

Mặc định: steer_s153_left_pace_shadow_enable=1U, steer_s153_left_pace_enable=0U. Cả hai cấu hình này dùng để quan sát, KHÔNG làm thay đổi PWM. Nếu có bản thử kích hoạt sau này, chốt latch chỉ ở đầu một hành trình Level2 RIGHT->RMIN mode1 mới, không kích hoạt đồng thời far trim RIGHT hoặc ETA. Với hướng RIGHT->RMAX ở giai đoạn này chỉ ghi nhận shadow, không cho tác động.

Dòng báo sau DISARM: M8S153_PACE,shadowEnable,applyEnable,applyLast,frames,eligible,proposed,actuationRequests,peakReduction,lastLagMs,peakLagMs,lastOwnerL,lastCandidateL. proposed là tính toán giả định, không phải CCR đã được ghi hoặc bằng chứng tăng chất lượng động cơ.

## Nghiệm thu và giới hạn
Kiểm thử source+GCC C qua tools/verify_m8_s153_left_pace.py không phải STM32CubeIDE target build, không phải flash đúng firmware, không chứng minh tốc độ, moment xoắn, quán tính/cutoff và không thể tuyên bố đã đạt 2%. Cần kiểm tra lại cutoff nguồn motor độc lập, dòng driver và khoảng trôi so với NC, không nâng ga RIGHT sát NC. Sau đó mới quan sát bốn hành trình được phép có log, so sánh ETA ước tính và thời điểm thật; nếu hợp lý mới tiến đến A/B chủ động có kiểm soát. Giữ PS2 visual mapping cho sau, chức năng lái hiện tại không đổi; S15.3 chưa PASS, S15.4/VC02 chưa mở.
