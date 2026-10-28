# M8-S00 STM32 first-stop diagnostic capture over ST-Link Virtual COM.
# Built-in Windows PowerShell only; no Tera Term, Python or serial packages.
# Run in the control_motor repo after flashing the matching firmware:
# powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s00.ps1
# Or specify: -Port COM5 -TimeoutSeconds 180 -OutputDirectory .\logs
[CmdletBinding()]
param(
    [string]$Port = "",
    [int]$Baud = 115200,
    [ValidateRange(10, 3600)]
    [int]$TimeoutSeconds = 180,
    [string]$OutputDirectory = "."
)

$ErrorActionPreference = "Stop"
$available = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
if ($available.Count -eq 0) {
    throw "Khong tim thay cong COM. Kiem tra USB ST-Link/driver va cap USB."
}

if ([string]::IsNullOrWhiteSpace($Port)) {
    # Prefer an identified ST-Link VCP when there is exactly one.
    $stlink = @()
    try {
        $stlink = @(Get-CimInstance -ClassName Win32_PnPEntity -ErrorAction Stop |
            Where-Object { $_.Name -match 'ST.?LINK.*\(COM\d+\)' } |
            ForEach-Object { [regex]::Match($_.Name, 'COM\d+').Value } |
            Where-Object { $available -contains $_ } |
            Select-Object -Unique)
    } catch {
        # CIM detection may be unavailable; manual or single-port fallback below.
    }
    if ($stlink.Count -eq 1) {
        $Port = $stlink[0]
    } elseif ($stlink.Count -eq 0 -and $available.Count -eq 1) {
        $Port = $available[0]
    } else {
        Write-Host "Cac cong COM dang co: $($available -join ', ')"
        $Port = (Read-Host "Nhap cong ST-Link Virtual COM (vi du COM5)").Trim()
    }
}
if ($available -notcontains $Port) {
    throw "Cong $Port khong co san. Danh sach: $($available -join ', ')"
}

$folder = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
[System.IO.Directory]::CreateDirectory($folder) | Out-Null
$file = Join-Path $folder ("M8_S00_" + (Get-Date -Format "yyyyMMdd_HHmmss") + ".txt")

$serial = New-Object System.IO.Ports.SerialPort -ArgumentList @(
    $Port, $Baud, [System.IO.Ports.Parity]::None, 8,
    [System.IO.Ports.StopBits]::One
)
$serial.Handshake = [System.IO.Ports.Handshake]::None
$serial.ReadTimeout = 500
$serial.WriteTimeout = 1000
$serial.NewLine = "`n"
$serial.Encoding = [System.Text.Encoding]::ASCII
$serial.DtrEnable = $false
$serial.RtsEnable = $false

$lines = New-Object 'System.Collections.Generic.List[string]'
$capturing = $false
$complete = $false
$readError = $null
try {
    $serial.Open()
    $serial.DiscardInBuffer()
    Write-Host ""
    Write-Host "Dang nghe $Port ($Baud baud, 8N1)..."
    Write-Host "Sau khi hoan thanh test: nha tay cam, disarm START, doi xe dung han."
    Write-Host "BAM-NHA ngan USER MOT LAN (< 3 giay) de xuat M8-S00."
    Write-Host "Khong reset/tat board truoc khi thay '#M8S00_END'."
    Write-Host ""
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)

    while ((Get-Date) -lt $deadline -and -not $complete) {
        try {
            $line = $serial.ReadLine().Trim()
        } catch [System.TimeoutException] {
            continue
        }
        if (-not $capturing) {
            if ($line -ne '#M8S00_BEGIN,1') {
                continue
            }
            $capturing = $true
        }
        $lines.Add($line)
        Write-Host $line
        if ($line -eq '#M8S00_END') {
            $complete = $true
        }
        if ($lines.Count -gt 40) {
            throw "Qua 40 dong khi chua thay marker ket thuc; dung de tranh ghi log sai."
        }
    }
} catch {
    $readError = $_.Exception.Message
} finally {
    if ($serial.IsOpen) {
        $serial.Close()
    }
    $serial.Dispose()
    if ($lines.Count -gt 0) {
        [System.IO.File]::WriteAllLines(
            $file, $lines.ToArray(), (New-Object System.Text.UTF8Encoding($false))
        )
    }
}

if ($complete) {
    $all = $lines -join "`n"
    if ($all -notmatch 'M8S00_META,STOP|M8S00_EMPTY,STOP') {
        Write-Warning "Da thay marker END, nhung chua duoc STOP hop le. Gui file de kiem tra."
    }
    if ($all -notmatch 'M8S00_META,FAULT|M8S00_EMPTY,FAULT') {
        Write-Warning "Khong thay phan FAULT. Co the log thieu dong."
    }
    Write-Host ""
    Write-Host "HOAN TAT - da luu: $file"
    Write-Host "Gui file TXT nay de phan tich M8-S00."
} elseif ($lines.Count -gt 0) {
    Write-Warning "CHI NHAN DUOC MOT PHAN, da luu file: $file"
    throw "Khong nhan duoc #M8S00_END. $readError"
} elseif ($readError) {
    throw "Loi serial: $readError"
} else {
    throw "Het $TimeoutSeconds giay ma chua nhan #M8S00_BEGIN,1. Kiem tra COM, USER, m8_s00_dump_status va da disarm START."
}
