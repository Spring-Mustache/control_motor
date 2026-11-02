# M8-S15.2: capture FOUR manual D-pad/RX M8S152 logs into ONE TXT.
# Built-in Windows PowerShell 5.1+ System.IO.Ports only. NO terminal app,
# Python, Live Expressions, external packages, serial apps or MCU PWM changes.
#
# powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\capture_m8_s152.ps1 -Port COM5
# Port is optional if exactly one ST-LINK VCP is automatically detected.
# Do not run capture_m8_s11.ps1 simultaneously: both use USART3 VCP.
[CmdletBinding()]
param(
    [string]$Port = '',
    [ValidateRange(1200, 115200)]
    [int]$Baud = 115200,
    [ValidateRange(1, 12)]
    [int]$ExpectedRuns = 4,
    [ValidateRange(30, 7200)]
    [int]$TimeoutSeconds = 1200,
    [ValidateRange(128, 1200)]
    [int]$MaxLinesPerRun = 750,
    [string]$OutputDirectory = '.\logs',
    [string]$ExpectedCommit = ''
)
$ErrorActionPreference = 'Stop'
# Expected Git HEAD is only an expectation, not proof of flashed firmware.
if ([string]::IsNullOrWhiteSpace($ExpectedCommit)) {
    $root = Split-Path -Parent $PSScriptRoot
    $gitResult = & git -C $root rev-parse --verify HEAD 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw 'Cannot determine Git HEAD. Pass -ExpectedCommit <40-hex-sha>.'
    }
    $ExpectedCommit = ($gitResult | Out-String).Trim()
}
$ExpectedCommit = $ExpectedCommit.Trim().ToLowerInvariant()
if ($ExpectedCommit -cnotmatch '^[0-9a-f]{40}$') {
    throw 'ExpectedCommit must be the exact 40-character Git SHA.'
}
Write-Host "Expected firmware SHA: $ExpectedCommit"
$available = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
if ($available.Count -eq 0) { throw 'No COM port. Check ST-LINK USB VCP cable and driver.' }
if ([string]::IsNullOrWhiteSpace($Port)) {
    $stlink = @()
    try {
        $stlink = @(Get-CimInstance -ClassName Win32_PnPEntity -ErrorAction Stop |
            Where-Object { $_.Name -match 'ST.?LINK.*\(COM\d+\)' } |
            ForEach-Object { [regex]::Match($_.Name, 'COM\d+').Value } |
            Where-Object { $available -contains $_ } | Select-Object -Unique)
    } catch { }
    if ($stlink.Count -eq 1) {
        $Port = $stlink[0]
    } elseif ($available.Count -eq 1) {
        $Port = $available[0]
    } else {
        throw "Multiple COM ports ($($available -join ', ')). Rerun with -Port COMx."
    }
}
if ($available -notcontains $Port) {
    throw "$Port not available. Ports: $($available -join ', ')"
}

$dir = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
[System.IO.Directory]::CreateDirectory($dir) | Out-Null
$stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
$file = Join-Path $dir "M8S152_four_tests_$stamp.txt"
$serial = New-Object System.IO.Ports.SerialPort -ArgumentList @(
    $Port, $Baud, [System.IO.Ports.Parity]::None, 8,
    [System.IO.Ports.StopBits]::One)
$serial.Handshake = [System.IO.Ports.Handshake]::None
$serial.ReadTimeout = 500
$serial.WriteTimeout = 1000
$serial.NewLine = [string][char]10
$serial.Encoding = [System.Text.Encoding]::ASCII
$serial.DtrEnable = $false
$serial.RtsEnable = $false

$utf8 = New-Object System.Text.UTF8Encoding -ArgumentList @($false)
$writer = $null
$complete = 0
$receiving = $false
$linesInRun = 0
$meta = ''
$firmwareLine = ''
$haveRows = $false
$haveFinalZero = $false
$badRuns = 0
$lastRunId = -1L
$readError = ''
$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$tests = @('D-pad LEFT', 'D-pad RIGHT', 'RX LEFT', 'RX RIGHT')
$expectedKinds = @(3, 3, 4, 4)
$expectedDirections = @(1, -1, 1, -1)

try {
    $serial.Open()
    $serial.DiscardInBuffer()
    $writer = New-Object System.IO.StreamWriter -ArgumentList @($file, $false, $utf8)
    $writer.AutoFlush = $true
    Write-Host "M8-S15.2 PowerShell-only capture: $Port at $Baud 8N1."
    Write-Host "Recording $ExpectedRuns complete M8S00 frames into ONE TXT: $file"
    Write-Host 'No terminal app, Live Expressions or S11 stream required.'
    Write-Host 'For EACH test: ensure physical cutoff/NC safety, arm PS2 START,'
    Write-Host 'perform short manual steering, release, press START to DISARM,'
    Write-Host 'wait until all motors fully stopped, then SHORT press USER once.'
    Write-Host 'After every #M8S00_END, arm again and perform the next test.'
    Write-Host 'Do not reset the MCU or disconnect USB while capture is running.'
    if ($ExpectedRuns -eq 4) {
        Write-Host 'Order: 1 D-pad LEFT, 2 D-pad RIGHT, 3 RX LEFT, 4 RX RIGHT.'
    }
    Write-Host ''
    while ((Get-Date) -lt $deadline -and $complete -lt $ExpectedRuns) {
        $line = ''
        try {
            $line = $serial.ReadLine().Trim()
        } catch [System.TimeoutException] {
            continue
        }
        if (-not $receiving) {
            if ($line -ne '#M8S00_BEGIN,1') { continue }
            $receiving = $true
            $linesInRun = 0
            $meta = ''
            $firmwareLine = ''
            $haveRows = $false
            $haveFinalZero = $false
        }
        $writer.WriteLine($line)
        $linesInRun++
        if ($linesInRun -gt $MaxLinesPerRun) {
            throw "M8S00 frame exceeded $MaxLinesPerRun lines without END."
        }
        if ($line -match '^M8S00_FW,') {
            $firmwareLine = $line
        } elseif ($line -match '^M8S152_META,') {
            $meta = $line
        } elseif ($line -match '^M8S152_A,i=') {
            $haveRows = $true
        } elseif (($line -match '^M8S152_B,i=') -and
                  ($line -match ',pwmL=0,pwmR=0,') -and
                  $line.EndsWith(',final=1')) {
            $haveFinalZero = $true
        }
        if ($line -ne '#M8S00_END') { continue }
        $receiving = $false
        $complete++
        $ok = $true
        $sourceSha = ''
        $sourceDirty = -1
        if ($firmwareLine -match '^M8S00_FW,proto=1,sha=([0-9a-f]{40}),dirty=([01]),') {
            $sourceSha = $Matches[1]
            $sourceDirty = [int]$Matches[2]
            if ($sourceSha -cne $ExpectedCommit) {
                $ok = $false
                Write-Warning "FW MISMATCH frame $complete: MCU=$sourceSha expected=$ExpectedCommit."
            }
            if ($sourceDirty -ne 0) {
                $ok = $false
                Write-Warning "FW DIRTY frame $complete: local changes existed at compilation."
            }
        } else {
            $ok = $false
            Write-Warning "FW UNKNOWN frame $complete: missing/invalid MCU firmware ID."
        }
        $kind = -1
        $dirL = 0
        $runId = -1L
        if ($meta -match '^M8S152_META,run=(\d+),kind=(\d+),dirL=(-?\d+),') {
            $runId = [long]::Parse($Matches[1])
            $kind = [int]::Parse($Matches[2])
            $dirL = [int]::Parse($Matches[3])
        } else {
            $ok = $false
            Write-Warning "Frame $complete missing valid M8S152_META; check firmware SHA."
        }
        if (-not $haveRows -or -not $haveFinalZero) {
            $ok = $false
            Write-Warning "Frame $complete missing M8S152 samples or final PWM=0 marker."
        }
        if ($runId -le $lastRunId) {
            $ok = $false
            Write-Warning "Frame $complete duplicate/nonincreasing run=$runId."
        }
        $lastRunId = $runId
        if ($complete -le 4 -and $ExpectedRuns -eq 4) {
            if ($kind -ne $expectedKinds[$complete - 1] -or
                $dirL -ne $expectedDirections[$complete - 1]) {
                $ok = $false
                Write-Warning "Frame $complete expected $($tests[$complete - 1]), got kind=$kind dirL=$dirL."
            }
        }
        if (-not $ok) { $badRuns++ }
        Write-Host (("Captured {0}/{1}: run={2}, kind={3}, dirL={4}, " +
            "lines={5}, valid={6}, fw={7}, dirty={8}") -f $complete, $ExpectedRuns, $runId,
            $kind, $dirL, $linesInRun, $ok, $sourceSha, $sourceDirty)
        if ($complete -lt $ExpectedRuns) {
            Write-Host "NEXT: perform test $($complete + 1) then DISARM + short USER."
        }
        Write-Host ''
    }
} catch {
    $readError = $_.Exception.Message
} finally {
    if ($null -ne $writer) { $writer.Flush(); $writer.Dispose() }
    if ($serial.IsOpen) { $serial.Close() }
    $serial.Dispose()
}
Write-Host "Saved all received frames to: $file"
Write-Host "Received $complete/$ExpectedRuns complete runs; invalid=$badRuns."
if ($readError) {
    throw "Capture error: $readError. Partial data preserved in $file"
}
if ($complete -ne $ExpectedRuns) {
    throw "Timeout: only $complete/$ExpectedRuns complete frames. Partial data preserved in $file"
}
if ($badRuns -ne 0) {
    throw "$badRuns invalid frame(s): firmware identity mismatch/unknown/dirty or invalid samples. TXT preserved at $file."
} else {
    Write-Host 'SUCCESS: exact expected STM32 firmware SHA verified for every frame.'
}
