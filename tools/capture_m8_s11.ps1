# M8-S11: six-motor LIVE CSV capture, USART3/ST-LINK VCP to PC disk.
# Built-in Windows PowerShell 5.1+, no Python/packages.
# Firmware streaming is opt-in; no onboard Flash writing or sample history.
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s11.ps1 -Port COM5
[CmdletBinding()]
param(
    [string]$Port = "",
    [int]$Baud = 115200,
    [ValidateRange(10, 3600)]
    [int]$TimeoutSeconds = 240,
    [string]$OutputDirectory = ".\logs"
)

$ErrorActionPreference = 'Stop'
$expectedHeader = 'seq,ms,mode,ly,rx,dleft,dright,r1,r2,pid_en,pid_on,target_l_rpm10,target_r_rpm10,rpm_lu10,rpm_ld10,rpm_ru10,rpm_rd10,pwm_lu,pwm_ld,pwm_ru,pwm_rd,denc_lu,denc_ld,denc_ru,denc_rd,raw_l,raw_r,soft_goal_l,soft_goal_r,req_l,req_r,dir_l,dir_r,ccr_l,ccr_r,manual_state_l,manual_state_r,manual_result_l,manual_result_r,pos_state_l,pos_state_r,pos_result_l,pos_result_r,fine_l,fine_r,nc,trip,stop,spin,manual_active,position_active,pid_ticks,fb_lu10,fb_ld10,fb_ru10,fb_rd10,err_l,err_r,age_l_ms,age_r_ms'
$expectedFields = 60
$available = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
if ($available.Count -eq 0) { throw 'No COM ports. Connect the ST-LINK USB VCP.' }
if ([string]::IsNullOrWhiteSpace($Port)) {
    $stlink = @()
    try {
        $stlink = @(Get-CimInstance -ClassName Win32_PnPEntity -ErrorAction Stop |
            Where-Object { $_.Name -match 'ST.?LINK.*\(COM\d+\)' } |
            ForEach-Object { [regex]::Match($_.Name, 'COM\d+').Value } |
            Where-Object { $available -contains $_ } |
            Select-Object -Unique)
    } catch {
        # Fall back to manual selection.
    }
    if ($stlink.Count -eq 1) {
        $Port = $stlink[0]
    } elseif ($available.Count -eq 1) {
        $Port = $available[0]
    } else {
        Write-Host "Available: $($available -join ', ')"
        $Port = (Read-Host 'ST-LINK Virtual COM port (e.g. COM5)').Trim()
    }
}
if ($available -notcontains $Port) { throw "Port $Port not available: $($available -join ', ')" }

$folder = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
[System.IO.Directory]::CreateDirectory($folder) | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$csvFile = Join-Path $folder "M8S11_six_motor_$stamp.csv"
$metaFile = Join-Path $folder "M8S11_six_motor_$stamp.meta.txt"
$serial = New-Object System.IO.Ports.SerialPort -ArgumentList @(
    $Port, $Baud, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$serial.Handshake = [System.IO.Ports.Handshake]::None
$serial.ReadTimeout = 500
$serial.WriteTimeout = 1000
$serial.NewLine = [string][char]10
$serial.Encoding = [System.Text.Encoding]::ASCII
$serial.DtrEnable = $false
$serial.RtsEnable = $false

$started = $false
$done = $false
$rows = 0
$badRows = 0
$lostSeq = 0L
$lastSeq = 0L
$lastMs = -1L
$beginLine = ''
$endLine = ''
$readError = ''
$headerLine = ''
$writer = $null
$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$utf8 = [System.Text.UTF8Encoding]::new($false)

try {
    $serial.Open()
    $serial.DiscardInBuffer()
    Write-Host "Listening $Port, $Baud baud; saving to $csvFile"
    Write-Host 'NOW set Live Expressions m8_s11_stream_enable=1.'
    Write-Host 'Run suspended-chassis tests. Set m8_s11_stream_enable=0 to close CSV.'
    Write-Host 'Do not request the legacy USER CSV/M8S00 dump during this stream.'
    while ((Get-Date) -lt $deadline -and -not $done) {
        $line = ''
        try {
            $line = $serial.ReadLine().Trim()
        } catch [System.TimeoutException] {
            continue
        }
        if (-not $started) {
            if ($line -notmatch '^#M8S11_BEGIN,1,period_ms=(\d+)$') { continue }
            $beginLine = $line
            $started = $true
            $writer = [System.IO.StreamWriter]::new($csvFile, $false, $utf8)
            $writer.AutoFlush = $true
            Write-Host "START: $line"
            continue
        }
        if ($line -match '^#M8S11_END,') {
            $endLine = $line
            $done = $true
            continue
        }
        if ($line.StartsWith('#')) { continue }
        if (-not $headerLine) {
            if ($line -ne $expectedHeader) {
                throw "Unexpected header; expected $expectedFields fields, got: $line"
            }
            $headerLine = $line
            $writer.WriteLine($line)
            continue
        }
        $cells = @($line.Split(','))
        if ($cells.Count -ne $expectedFields) {
            $badRows++
            continue
        }
        $seq = 0L
        $ms = 0L
        if (-not [long]::TryParse($cells[0], [ref]$seq) -or
            -not [long]::TryParse($cells[1], [ref]$ms)) {
            $badRows++
            continue
        }
        if ($seq -le $lastSeq -or $ms -lt $lastMs) {
            $badRows++
            continue
        }
        # Every remaining field is an integer in this protocol.
        # Reject corrupt payloads rather than silently changing motor evidence.
        $numeric = $true
        for ($j = 2; $j -lt $expectedFields; $j++) {
            $value = 0L
            if (-not [long]::TryParse($cells[$j], [ref]$value)) {
                $numeric = $false
                break
            }
        }
        if (-not $numeric) {
            $badRows++
            continue
        }
        if ($seq -gt ($lastSeq + 1L)) {
            $lostSeq += $seq - $lastSeq - 1L
        }
        $lastSeq = $seq
        $lastMs = $ms
        $writer.WriteLine($line)
        $rows++
        if (($rows % 100) -eq 0) {
            Write-Host "rows=$rows ms=$lastMs seq_gaps=$lostSeq bad=$badRows"
        }
    }
} catch {
    $readError = $_.Exception.Message
} finally {
    if ($null -ne $writer) { $writer.Flush(); $writer.Dispose() }
    if ($serial.IsOpen) { $serial.Close() }
    $serial.Dispose()
    $meta = @(
        "begin=$beginLine",
        "end=$endLine",
        "captured_rows=$rows",
        "invalid_lines=$badRows",
        "sequence_missing=$lostSeq",
        "last_board_ms=$lastMs",
        "complete=$done",
        "port=$Port",
        "baud=$Baud",
        "error=$readError",
        "csv=$csvFile"
    )
    [System.IO.File]::WriteAllLines($metaFile, [string[]]$meta, $utf8)
}
Write-Host "CSV: $csvFile"
Write-Host "Metadata: $metaFile"
if (-not $started) { throw "No M8S11 BEGIN marker received; $readError" }
if ($done -and -not $headerLine) { throw 'END received without CSV header; inspect metadata.' }
if ($done -and $endLine -match '^#M8S11_END,rows=(\d+),missed=(\d+),busy=(\d+),tx_errors=(\d+)
if (-not $done) {
    Write-Warning 'PARTIAL CSV SAVED: END missing. Disable m8_s11_stream_enable.'
}
if ($badRows -ne 0 -or $lostSeq -ne 0) {
    Write-Warning 'Malformed rows or lost sequence numbers; see metadata.'
}
if ($readError) { throw "Serial capture error: $readError" }
) {
    $boardRows = [long]$Matches[1]
    $boardMissed = [long]$Matches[2]
    $boardBusy = [long]$Matches[3]
    $boardTxErrors = [long]$Matches[4]
    Write-Host "Board rows=$boardRows, missed_periods=$boardMissed, busy=$boardBusy, tx_errors=$boardTxErrors"
    if ($boardRows -ne $rows -or $boardMissed -ne 0 -or $boardTxErrors -ne 0) {
        Write-Warning 'MCU/PC sample counters do not match or stream had missed periods/errors.'
    }
}
if ($done -and $rows -eq 0) { Write-Warning 'Stream ended without data rows.' }
if (-not $done) {
    Write-Warning 'PARTIAL CSV SAVED: END missing. Disable m8_s11_stream_enable.'
}
if ($badRows -ne 0 -or $lostSeq -ne 0) {
    Write-Warning 'Malformed rows or lost sequence numbers; see metadata.'
}
if ($readError) { throw "Serial capture error: $readError" }
