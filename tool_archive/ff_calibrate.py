import argparse
import csv
import math
from pathlib import Path


BEGIN_MARKERS = {"#WHEEL_PID_CSV_BEGIN", "#WHEEL_ANALOG_CSV_BEGIN"}
END_MARKERS = {"#WHEEL_PID_CSV_END", "#WHEEL_ANALOG_CSV_END"}


def extract_csv_lines(path: Path):
    text = path.read_text(encoding="utf-8", errors="ignore").splitlines()
    inside = False
    rows = []
    for raw in text:
        line = raw.strip()
        if not line:
            continue
        if line in BEGIN_MARKERS:
            inside = True
            continue
        if line in END_MARKERS:
            break
        if not inside or line.startswith("#"):
            continue
        rows.append(line)
    if not rows:
        raise RuntimeError("Không tìm thấy block CSV của wheel logger.")
    return rows


def read_log(path: Path):
    reader = csv.DictReader(extract_csv_lines(path))
    if not reader.fieldnames:
        raise RuntimeError("Không đọc được header CSV.")

    if "pid_target_mrpm" in reader.fieldnames:
        target_key = "pid_target_mrpm"
    elif "target_mrpm" in reader.fieldnames:
        target_key = "target_mrpm"
    else:
        raise RuntimeError("Log không có pid_target_mrpm hoặc target_mrpm.")

    wheels = ["ld", "ru", "rd"]
    if "lu_mrpm" in reader.fieldnames and "lu_pwm" in reader.fieldnames:
        wheels = ["lu", "ld", "ru", "rd"]

    required = ["time_ms", target_key]
    for wheel in wheels:
        required.extend([f"{wheel}_mrpm", f"{wheel}_pwm"])
    missing = [key for key in required if key not in reader.fieldnames]
    if missing:
        raise RuntimeError("Thiếu cột bắt buộc: " + ", ".join(missing))

    data = []
    for row in reader:
        item = {
            "t": float(row["time_ms"]) / 1000.0,
            "target": float(row[target_key]) / 1000.0,
        }
        for wheel in wheels:
            item[f"{wheel}_rpm"] = float(row[f"{wheel}_mrpm"]) / 1000.0
            item[f"{wheel}_pwm"] = float(row[f"{wheel}_pwm"])
        data.append(item)
    return data, wheels


def stable_segments(data, slope_limit=1.5, min_target=20.0,
                    max_target=90.0, min_duration=2.0, settle=1.0):
    """Find plateaus where target RPM is almost constant.

    slope_limit: maximum absolute target slope in rpm/s.
    min/max_target: calibration range; avoids low-speed dead-zone and saturation.
    settle: seconds ignored at the beginning of each plateau.
    """
    if len(data) < 3:
        return []

    stable = [False] * len(data)
    for i in range(1, len(data)):
        dt = data[i]["t"] - data[i - 1]["t"]
        if dt <= 0:
            continue
        slope = abs(data[i]["target"] - data[i - 1]["target"]) / dt
        mag = abs(data[i]["target"])
        stable[i] = (
            slope <= slope_limit
            and mag >= min_target
            and mag <= max_target
        )

    segs = []
    start = None
    for i, flag in enumerate(stable):
        if flag and start is None:
            start = i
        if (not flag or i == len(stable) - 1) and start is not None:
            end = i if not flag else i + 1
            if end - start >= 2:
                t0 = data[start]["t"]
                t1 = data[end - 1]["t"]
                if (t1 - t0) >= min_duration:
                    use_start_t = t0 + settle
                    use = [r for r in data[start:end] if r["t"] >= use_start_t]
                    if use:
                        segs.append(use)
            start = None
    return segs


def mean(vals):
    return sum(vals) / len(vals)


def summarize_segments(segs, wheels):
    summaries = []
    for seg in segs:
        s = {
            "target": mean([abs(r["target"]) for r in seg]),
            "duration": seg[-1]["t"] - seg[0]["t"],
        }
        for wheel in wheels:
            s[f"{wheel}_rpm"] = mean([abs(r[f"{wheel}_rpm"]) for r in seg])
            s[f"{wheel}_pwm"] = mean([abs(r[f"{wheel}_pwm"]) for r in seg])
        # Exclude plateaus where any actuator is too close to saturation.
        if max(s[f"{wheel}_pwm"] for wheel in wheels) < 950.0:
            summaries.append(s)
    return summaries


def merge_similar_levels(summaries, wheels, tol=3.0):
    """Average repeated plateaus around the same target so one long level
    does not dominate the linear fit."""
    groups = []
    for s in sorted(summaries, key=lambda x: x["target"]):
        placed = False
        for g in groups:
            if abs(s["target"] - g[0]["target"]) <= tol:
                g.append(s)
                placed = True
                break
        if not placed:
            groups.append([s])

    merged = []
    for g in groups:
        keys = ["target", "duration"]
        for wheel in wheels:
            keys.extend([f"{wheel}_rpm", f"{wheel}_pwm"])
        merged.append({key: mean([x[key] for x in g]) for key in keys})
    return merged


def linear_fit(points, wheel):
    x = [p[f"{wheel}_rpm"] for p in points]
    y = [p[f"{wheel}_pwm"] for p in points]

    if len(x) < 2:
        raise RuntimeError(f"{wheel.upper()}: cần ít nhất 2 mức tốc độ ổn định.")

    xbar = mean(x)
    ybar = mean(y)
    sxx = sum((v - xbar) ** 2 for v in x)
    if sxx < 1e-9:
        raise RuntimeError(f"{wheel.upper()}: các mức RPM quá giống nhau.")

    k = sum((a - xbar) * (b - ybar) for a, b in zip(x, y)) / sxx
    offset = ybar - k * xbar

    # Physical recommendation for this firmware: non-negative dead-zone offset.
    offset_rec = max(0.0, offset)

    yhat = [k * a + offset for a in x]
    ss_res = sum((b - yh) ** 2 for b, yh in zip(y, yhat))
    ss_tot = sum((b - ybar) ** 2 for b in y)
    r2 = 1.0 - ss_res / ss_tot if ss_tot > 1e-9 else 1.0

    return k, offset, offset_rec, r2


def main():
    ap = argparse.ArgumentParser(
        description="Ước lượng feedforward riêng từng bánh từ wheel PID/analog CSV log.")
    ap.add_argument("logfile")
    args = ap.parse_args()

    data, wheels = read_log(Path(args.logfile))
    segs = stable_segments(data)
    summaries = merge_similar_levels(summarize_segments(segs, wheels), wheels)

    print("=== Stable calibration plateaus ===")
    if not summaries:
        print("Không tìm thấy plateau phù hợp.")
        print("Hãy test 30 -> 50 -> 70 -> 85 rpm, giữ mỗi mức 4-5 s.")
        raise SystemExit(2)

    for i, s in enumerate(summaries, 1):
        detail = " | ".join(
            f"{w.upper()} {s[f'{w}_rpm']:.2f} rpm/{s[f'{w}_pwm']:.1f} PWM"
            for w in wheels
        )
        print(f"{i}: target~{s['target']:.1f} rpm | {detail}")

    if len(summaries) < 3:
        print("\nCảnh báo: chỉ có dưới 3 mức tốc độ khác nhau; fit vẫn chạy nhưng nên")
        print("lấy thêm plateau 30/50/70/85 rpm để hệ số đáng tin hơn.\n")

    print("\n=== Linear feedforward fit: PWM = Kff * RPM + offset ===")
    results = {}
    for w in wheels:
        k, off, off_rec, r2 = linear_fit(summaries, w)
        results[w] = (k, off_rec)
        print(
            f"{w.upper()}: Kff={k:.4f} PWM/rpm, "
            f"offset_raw={off:.2f}, offset_recommended={off_rec:.2f} PWM, "
            f"R^2={r2:.4f}"
        )

    print("\n=== Nhập vào Live Expressions khi xe đang dừng ===")
    for w in wheels:
        k, off = results[w]
        print(f"wheel_ff_k_{w} = {k:.4f}")
        print(f"wheel_ff_offset_{w} = {off:.2f}")

    print("\nSau khi nhập, test lại 40 -> 85 -> 40 -> 0 và so đồ thị.")
    print("Không sửa Kp/Ki/Kd trong lượt calibration này.")


if __name__ == "__main__":
    main()
