import argparse
import csv
import math
from pathlib import Path

import matplotlib.pyplot as plt


# Chỉ dùng để làm mượt telemetry/đồ thị.
# KHÔNG tham gia vòng feedback PID trên STM32.
TELEMETRY_MA_SAMPLES = 5

BEGIN_MARKERS = {
    '#WHEEL_ANALOG_CSV_BEGIN',
    '#WHEEL_PID_CSV_BEGIN',
}
END_MARKERS = {
    '#WHEEL_ANALOG_CSV_END',
    '#WHEEL_PID_CSV_END',
}

RPM_X1000_COLUMNS = {
    'target_mrpm',
    'raw_target_mrpm',
    'command_target_mrpm',
    'pid_target_mrpm',
    'lu_mrpm', 'ld_mrpm', 'ru_mrpm', 'rd_mrpm',
    'lu_err_mrpm', 'ld_err_mrpm', 'ru_err_mrpm', 'rd_err_mrpm',
    'sync_spread_mrpm',
}


def moving_average_causal(values, window):
    """Moving average causal: mỗi điểm chỉ dùng mẫu hiện tại và các mẫu trước."""
    if window <= 1:
        return list(values)

    result = []
    acc = 0.0
    queue = []

    for value in values:
        queue.append(value)
        acc += value

        if len(queue) > window:
            acc -= queue.pop(0)

        result.append(acc / len(queue))

    return result


def extract_csv_lines(path: Path):
    text = path.read_text(encoding='utf-8', errors='ignore').splitlines()
    inside = False
    rows = []
    metadata = []
    begin_marker = None

    for raw in text:
        line = raw.strip()
        if not line:
            continue

        if line in BEGIN_MARKERS:
            inside = True
            begin_marker = line
            continue

        if line in END_MARKERS:
            break

        if not inside:
            continue

        if line.startswith('#'):
            metadata.append(line)
            continue

        rows.append(line)

    if not rows:
        raise RuntimeError(
            'Không tìm thấy block WHEEL_ANALOG/WHEEL_PID CSV trong file log.'
        )

    return rows, metadata, begin_marker


def require_columns(fieldnames, required):
    missing = [name for name in required if name not in fieldnames]
    if missing:
        raise RuntimeError('Thiếu cột bắt buộc: ' + ', '.join(missing))


def read_log(path: Path):
    lines, metadata, begin_marker = extract_csv_lines(path)
    reader = csv.DictReader(lines)

    if not reader.fieldnames:
        raise RuntimeError('Không đọc được header CSV.')

    require_columns(
        reader.fieldnames,
        ['time_ms', 'ld_mrpm', 'ru_mrpm', 'rd_mrpm',
         'ld_pwm', 'ru_pwm', 'rd_pwm']
    )

    wheels = ['ld', 'ru', 'rd']
    if 'lu_mrpm' in reader.fieldnames and 'lu_pwm' in reader.fieldnames:
        wheels = ['lu', 'ld', 'ru', 'rd']

    data = {k: [] for k in reader.fieldnames}

    for row in reader:
        if not row or row.get('time_ms') in (None, ''):
            continue
        for key in data:
            value = row.get(key, '')
            if value in (None, ''):
                data[key].append(float('nan'))
            else:
                data[key].append(float(value))

    if not data['time_ms']:
        raise RuntimeError('CSV không có mẫu dữ liệu.')

    # Các cột firmware gửi theo scale x1000 được đổi về đơn vị thật.
    for key in RPM_X1000_COLUMNS:
        if key in data:
            data[key] = [v / 1000.0 for v in data[key]]

    if 'ff_accel_pwm_x1000' in data:
        data['ff_accel_pwm'] = [v / 1000.0 for v in data['ff_accel_pwm_x1000']]

    if 'capture_ru_pwm_x1000' in data:
        data['capture_ru_pwm'] = [v / 1000.0 for v in data['capture_ru_pwm_x1000']]

    if 'zero_ff_taper_x1000' in data:
        data['zero_ff_taper_scale'] = [
            v / 1000.0 for v in data['zero_ff_taper_x1000']
        ]

    data['time_s'] = [v / 1000.0 for v in data['time_ms']]

    # Format analog mới có raw_target và pid_target.
    # Format bench cũ chỉ có target_mrpm; giữ tương thích tự động.
    if 'pid_target_mrpm' in data:
        data['pid_target_rpm'] = data['pid_target_mrpm']
    elif 'target_mrpm' in data:
        data['pid_target_rpm'] = data['target_mrpm']
    else:
        raise RuntimeError('Không có pid_target_mrpm hoặc target_mrpm.')

    if 'raw_target_mrpm' in data:
        data['raw_target_rpm'] = data['raw_target_mrpm']
    else:
        data['raw_target_rpm'] = list(data['pid_target_rpm'])

    if 'command_target_mrpm' in data:
        data['command_target_rpm'] = data['command_target_mrpm']
    else:
        # v2/cũ chưa có tầng hysteresis riêng.
        data['command_target_rpm'] = list(data['raw_target_rpm'])

    # Telemetry RPM raw + MA5 causal để hiển thị giống cách phân tích trước đây.
    for wheel in wheels:
        raw_key = f'{wheel}_mrpm'
        data[f'{wheel}_rpm'] = data[raw_key]
        data[f'{wheel}_rpm_filtered'] = moving_average_causal(
            data[raw_key], TELEMETRY_MA_SAMPLES
        )

    # Nếu logger không gửi error thì tính lại theo PID target thật.
    for wheel in wheels:
        err_key = f'{wheel}_err_mrpm'
        if err_key in data:
            data[f'{wheel}_err_rpm'] = data[err_key]
        else:
            data[f'{wheel}_err_rpm'] = [
                target - rpm
                for target, rpm in zip(data['pid_target_rpm'], data[f'{wheel}_rpm'])
            ]

    data['_wheels'] = wheels
    data['_begin_marker'] = begin_marker
    return data, metadata


def finite(values):
    return [v for v in values if math.isfinite(v)]


def rmse(values):
    vals = finite(values)
    if not vals:
        return 0.0
    return math.sqrt(sum(v * v for v in vals) / len(vals))


def mae(values):
    vals = finite(values)
    if not vals:
        return 0.0
    return sum(abs(v) for v in vals) / len(vals)


def mean(values):
    vals = finite(values)
    if not vals:
        return 0.0
    return sum(vals) / len(vals)


def percentage_active(values):
    vals = finite(values)
    if not vals:
        return 0.0
    return 100.0 * sum(1 for v in vals if v >= 0.5) / len(vals)


def print_metrics(data, metadata):
    print('=== Metadata ===')
    for line in metadata:
        print(line)

    n = len(data['time_ms'])
    duration = data['time_s'][-1] - data['time_s'][0] if n >= 2 else 0.0
    print(f'\nSamples: {n} | Duration: {duration:.2f} s')

    if n >= 2:
        diffs = [
            data['time_ms'][i] - data['time_ms'][i - 1]
            for i in range(1, n)
            if data['time_ms'][i] > data['time_ms'][i - 1]
        ]
        if diffs:
            avg_period_ms = mean(diffs)
            print(
                f'Logger period: ~{avg_period_ms:.2f} ms | '
                f'MA{TELEMETRY_MA_SAMPLES} window: ~'
                f'{avg_period_ms * TELEMETRY_MA_SAMPLES:.0f} ms'
            )

    print('\n=== Tracking error vs PID target (RAW wheel RPM) ===')
    for wheel in data['_wheels']:
        err = data[f'{wheel}_err_rpm']
        vals = finite(err)
        max_abs = max((abs(v) for v in vals), default=0.0)
        print(
            f'{wheel.upper()}: MAE={mae(err):.3f} rpm, '
            f'RMSE={rmse(err):.3f} rpm, MaxAbs={max_abs:.3f} rpm'
        )

    raw_command_delta = [
        raw - cmd
        for raw, cmd in zip(data['raw_target_rpm'], data['command_target_rpm'])
    ]
    command_pid_delta = [
        cmd - pid
        for cmd, pid in zip(data['command_target_rpm'], data['pid_target_rpm'])
    ]
    print('\n=== Analog command -> hysteresis -> slew/PID target ===')
    print(
        f'|raw - accepted command|: mean={mae(raw_command_delta):.3f} rpm, '
        f'max={max((abs(v) for v in finite(raw_command_delta)), default=0.0):.3f} rpm'
    )
    print(
        f'|accepted command - pid target|: mean={mae(command_pid_delta):.3f} rpm, '
        f'max={max((abs(v) for v in finite(command_pid_delta)), default=0.0):.3f} rpm'
    )

    if 'integral_hold' in data:
        print(f'Integral hold active: {percentage_active(data["integral_hold"]):.1f}% samples')

    if 'soft_landing_active' in data:
        print(f'Same-direction soft landing active: {percentage_active(data["soft_landing_active"]):.1f}% samples')

    if 'zero_soft_landing_active' in data:
        print(f'Zero soft landing active: {percentage_active(data["zero_soft_landing_active"]):.1f}% samples')

    if 'zero_ff_taper_scale' in data:
        taper_vals = finite(data['zero_ff_taper_scale'])
        taper_active = [1.0 if v < 0.999 else 0.0 for v in taper_vals]
        print(
            f'Zero FF offset taper active: {percentage_active(taper_active):.1f}% samples | '
            f'min scale={min(taper_vals, default=1.0):.3f}'
        )

    if 'sync_active' in data:
        print(f'Sync active: {percentage_active(data["sync_active"]):.1f}% samples')

    hunt_keys = [key for key in ['hunt_lu', 'hunt_ld', 'hunt_ru', 'hunt_rd'] if key in data]
    if hunt_keys:
        print('Steady-hold active: ' + ', '.join(
            f'{key.split("_")[1].upper()}={percentage_active(data[key]):.1f}%'
            for key in hunt_keys
        ))

    if 'command_profile' in data:
        manual_pct = percentage_active(data['command_profile'])
        print(
            f'Command profile: MANUAL={manual_pct:.1f}% | '
            f'FIXED={100.0 - manual_pct:.1f}% samples'
        )

    if 'sync_spread_mrpm' in data:
        spread = data['sync_spread_mrpm']
        print(
            f'Sync spread: mean={mean(spread):.3f} rpm, '
            f'max={max(finite(spread), default=0.0):.3f} rpm'
        )


def plot_speed(data, out_dir: Path, show_raw=False):
    plt.figure(figsize=(11, 6))

    if 'raw_target_mrpm' in data:
        plt.plot(
            data['time_s'], data['raw_target_rpm'],
            linestyle='--', linewidth=1.0, label='Raw analog target'
        )

    plt.plot(
        data['time_s'], data['pid_target_rpm'],
        linewidth=1.5, label='PID target after slew'
    )

    for wheel in data['_wheels']:
        plt.plot(
            data['time_s'], data[f'{wheel}_rpm_filtered'],
            label=f'{wheel.upper()} RPM MA5'
        )

    if show_raw:
        for wheel in data['_wheels']:
            plt.plot(
                data['time_s'], data[f'{wheel}_rpm'],
                linewidth=0.7, alpha=0.25, label=f'{wheel.upper()} raw'
            )

    plt.xlabel('Time (s)')
    plt.ylabel('Speed (rpm)')
    plt.title('Analog command and wheel-speed response')
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / 'pid_speed_response.png', dpi=160)


def plot_command(data, out_dir: Path):
    if 'raw_target_mrpm' not in data:
        return

    plt.figure(figsize=(11, 5))
    plt.plot(data['time_s'], data['raw_target_rpm'], label='Raw analog target')
    if 'command_target_mrpm' in data:
        plt.plot(
            data['time_s'], data['command_target_rpm'],
            linewidth=1.2, label='Accepted target after hysteresis'
        )
    plt.plot(data['time_s'], data['pid_target_rpm'], label='PID target after slew')
    plt.xlabel('Time (s)')
    plt.ylabel('Target speed (rpm)')
    plt.title('Analog target -> hysteresis -> slew-limited PID target')
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / 'pid_target_chain.png', dpi=160)


def plot_pwm(data, out_dir: Path):
    plt.figure(figsize=(11, 5))
    for wheel in data['_wheels']:
        plt.plot(data['time_s'], data[f'{wheel}_pwm'], label=f'{wheel.upper()} PWM')

    if 'ff_accel_pwm' in data:
        plt.plot(
            data['time_s'], data['ff_accel_pwm'],
            linestyle='--', linewidth=1.0, label='Dynamic FF accel PWM'
        )

    plt.xlabel('Time (s)')
    plt.ylabel('PWM')
    plt.title('Motor command / control effort')
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / 'pid_pwm.png', dpi=160)


def plot_error(data, out_dir: Path):
    plt.figure(figsize=(11, 5))
    for wheel in data['_wheels']:
        plt.plot(
            data['time_s'], data[f'{wheel}_err_rpm'],
            label=f'{wheel.upper()} error'
        )

    plt.axhline(0.0, linewidth=1)
    plt.xlabel('Time (s)')
    plt.ylabel('PID target - wheel RPM (rpm)')
    plt.title('Tracking error relative to slew-limited PID target')
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / 'pid_error.png', dpi=160)


def plot_states(data, out_dir: Path):
    if not any(key in data for key in ['integral_hold', 'soft_landing_active', 'zero_soft_landing_active', 'zero_ff_taper_scale', 'sync_active', 'hunt_lu', 'hunt_ld', 'hunt_ru', 'hunt_rd', 'capture_ru_active', 'sync_spread_mrpm', 'command_profile']):
        return

    plt.figure(figsize=(11, 5))

    if 'sync_spread_mrpm' in data:
        plt.plot(data['time_s'], data['sync_spread_mrpm'], label='Sync spread (rpm)')

    if 'integral_hold' in data:
        plt.step(
            data['time_s'], data['integral_hold'],
            where='post', linewidth=1.0, label='Integral hold (0/1)'
        )

    if 'soft_landing_active' in data:
        plt.step(
            data['time_s'], data['soft_landing_active'],
            where='post', linewidth=1.0, label='Same-dir soft landing (0/1)'
        )

    if 'zero_soft_landing_active' in data:
        plt.step(
            data['time_s'], data['zero_soft_landing_active'],
            where='post', linewidth=1.0, label='Zero soft landing (0/1)'
        )

    if 'zero_ff_taper_scale' in data:
        plt.plot(
            data['time_s'], data['zero_ff_taper_scale'],
            linewidth=1.1, label='Zero FF offset scale (0..1)'
        )

    if 'sync_active' in data:
        plt.step(
            data['time_s'], data['sync_active'],
            where='post', linewidth=1.0, label='Sync active (0/1)'
        )

    for key in ['hunt_lu', 'hunt_ld', 'hunt_ru', 'hunt_rd']:
        if key in data:
            plt.step(
                data['time_s'], data[key],
                where='post', linewidth=0.9,
                label=f'Steady hold {key.split("_")[1].upper()} (0/1)'
            )

    if 'command_profile' in data:
        plt.step(
            data['time_s'], data['command_profile'],
            where='post', linewidth=1.0, label='Command profile (0=fixed, 1=manual)'
        )

    plt.xlabel('Time (s)')
    plt.ylabel('State / spread')
    plt.title('Command-shaper / integral-hold / wheel-sync states')
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(out_dir / 'pid_states.png', dpi=160)


def main():
    parser = argparse.ArgumentParser(
        description=(
            'Vẽ và phân tích log wheel controller 3/4 bánh. '
            'Hỗ trợ WHEEL_ANALOG steady-hold/v5/v4/v3/v2 và log WHEEL_PID cũ.'
        )
    )
    parser.add_argument('logfile', help='File CSV/text đã capture từ STM32')
    parser.add_argument('--out', default='pid_plots', help='Thư mục lưu PNG')
    parser.add_argument(
        '--show-raw',
        action='store_true',
        help='Vẽ thêm RPM encoder raw mờ phía sau đường MA5'
    )
    parser.add_argument(
        '--no-show',
        action='store_true',
        help='Chỉ lưu PNG, không mở cửa sổ matplotlib'
    )
    args = parser.parse_args()

    log_path = Path(args.logfile)
    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    data, metadata = read_log(log_path)
    print_metrics(data, metadata)
    plot_speed(data, out_dir, show_raw=args.show_raw)
    plot_command(data, out_dir)
    plot_pwm(data, out_dir)
    plot_error(data, out_dir)
    plot_states(data, out_dir)

    print(f'\nĐã lưu biểu đồ vào: {out_dir.resolve()}')
    if not args.no_show:
        plt.show()


if __name__ == '__main__':
    main()
