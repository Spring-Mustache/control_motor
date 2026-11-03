# M8-S15.3 — chế độ B cô lập hãm RIGHT, 03/11/2026

Trong log 20:36 firmware 004d04a (ETA OFF), D-pad LEFT STALL RIGHT tại RAW257/CCR56 và RX LEFT STALL RIGHT RAW255/CCR55, đều còn 12/10RAW tới SOFT245, tiến độ mất661ms, NC0. RIGHT->MAX hai lượt hoàn tất. EncoderDelta trong snapshot là tích lũy cả stroke. Không có dữ liệu điện/dòng xác định motor thực sự tạo mô-men tại 55CCR.

Bản thử mới phân biệt ba chế độ trên mỗi lệnh Level2 RIGHT->RMIN MỚI: mode0 legacy S14.2; mode1 bỏ riêng RIGHT trim của S14.2, vẫn giữ toàn bộ LEFT positive boosts và RIGHT owner; mode2 code ETA S15.3 cũ trên RIGHT trim-free owner. **Bản này boot mode1 ON, ETA boot OFF**, không chuyển chế độ giữa stroke. D-pad RIGHT/RX RIGHT và R1/R2/SELECT vẫn dùng mã cũ.

Quan sát sau DISARM: M8S153_MODE,mode=1,isoEnable=1,isoLast=1,etaEnable=0,isoFrames>0 và M8S153 requestFrames=0 đúng thiết kế. No PWM added in mode1: correct CCR is independent RIGHT owner. Không đổi RIGHT terminal floor50CCR, RMIN195/SOFT245, STOP/NC/AS5600, 650ms STALL, anti-escape, LEFT cap450/RIGHT200. S15.3 ETA mode2 chưa được phép thử khi mode1 còn STALL.

**SOURCE/HOST CI không phải MCU/hardware PASS.** Trước powered test phải xác minh nguồn động lực cắt độc lập, công tắc NC theo chiều, cơ cấu tự do, dòng driver và coast an toàn; không cố tạo STALL. Khi an toàn, chạy PowerShell native 4 lượt sau DISARM + USER, xác nhận 4/4 reached3 finishValid1 faults0, không FAULT13/STOP5/NC, so sánh lỗi quan hệ góc normalized và thời gian RIGHT/LEFT so với baseline. Nếu mode1 còn STALL quanh RAW255/257 thì ngừng phép thử, đo driver/cơ khí; không tăng floor, đổi SOFT hoặc timeout để gọi PASS. Chưa có ngưỡng chênh góc do người vận hành phê duyệt, nên S15.3 chưa PASS.
