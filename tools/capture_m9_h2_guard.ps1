# M9-H2A passive HOLD supervisor UART3 listener, 115200 8N1.
# NO commands transmitted. Does NOT activate HOLD/current/PWM.
[CmdletBinding()]param(
  [string]$Port="",
  [ValidateRange(30,3600)][int]$TimeoutSeconds=600,
  [ValidateRange(1,20)][int]$MaxEvents=4,
  [string]$OutputDirectory=".\logs"
)
$ErrorActionPreference="Stop"
$ports=@([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
if($ports.Count -eq 0){throw "Khong co COM ST-LINK VCP."}
if([string]::IsNullOrWhiteSpace($Port)){
  $st=@()
  try {
    $st=@(Get-CimInstance Win32_PnPEntity -ErrorAction Stop |
       Where-Object {$_.Name -match 'ST.?LINK.*\(COM\d+\)'} |
       ForEach-Object {[regex]::Match($_.Name,'COM\d+').Value} |
       Where-Object {$ports -contains $_} | Select-Object -Unique)
  }catch{}
  if($st.Count -eq 1){$Port=$st[0]}
  elseif($ports.Count -eq 1){$Port=$ports[0]}
  else{$Port=(Read-Host "Chon COM ST-LINK: $($ports -join ', ')").Trim()}
}
if($ports -notcontains $Port){throw "COM khong hop le: $Port"}
$folder=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
[System.IO.Directory]::CreateDirectory($folder) | Out-Null
$file=Join-Path $folder ("M9H2_GUARD_"+(Get-Date -Format "yyyyMMdd_HHmmss")+".txt")
$serial=New-Object System.IO.Ports.SerialPort -ArgumentList @(
 $Port,115200,[System.IO.Ports.Parity]::None,8,[System.IO.Ports.StopBits]::One)
$serial.Handshake=[System.IO.Ports.Handshake]::None
$serial.ReadTimeout=500
$serial.NewLine="`n"
$serial.Encoding=[System.Text.Encoding]::ASCII
$serial.DtrEnable=$false
$serial.RtsEnable=$false
$utf8=New-Object System.Text.UTF8Encoding($false)
$events=0;$sha="";$deadline=(Get-Date).AddSeconds($TimeoutSeconds)
try {
  $serial.Open()
  Write-Host "H2A GUARD: listening $Port, 115200, NO MOTOR OUTPUT. $file"
  while((Get-Date) -lt $deadline -and $events -lt $MaxEvents){
    try{$line=$serial.ReadLine().Trim()}catch [System.TimeoutException]{continue}
    if(-not ($line.StartsWith("M9H2_GUARD,") -or
             $line.StartsWith("M9H0_DRIFT,") -or
             $line.StartsWith("M9H1_SHADOW,"))){continue}
    if($line -match '(^|,)sha=([0-9a-fA-F]{40})(,|$)'){
      $candidate=$Matches[2].ToLowerInvariant()
      if($sha -eq ""){$sha=$candidate}
      elseif($sha -ne $candidate){Write-Warning "Mixed MCU SHA: verify firmware."}
    }else{Write-Warning "Full MCU SHA absent on record."}
    # Preserve incomplete serial evidence, but NEVER count it as a valid
    # completed H0/H1 record. A serial line may be truncated/concatenated.
    if($line.StartsWith("M9H0_DRIFT,") -and
       (($line -notmatch ',samplesL=') -or
        ($line -notmatch ',samplesR=') -or
        ($line -notmatch ',dropped='))){
      Write-Warning "Incomplete/concatenated M9H0_DRIFT line; preserve raw text, do not score H0 PASS."
    }
    [System.IO.File]::AppendAllText($file,$line+"`r`n",$utf8)
    if($line.StartsWith("M9H2_GUARD,")){
      $events++
      if(($line -notmatch 'powered=0') -or ($line -notmatch 'dutyL=0,dutyR=0')){
        Write-Warning "UNEXPECTED NONZERO ACTIVE OUTPUT CLAIM. STOP test and review firmware."
        break
      }
      Write-Host "GUARD $events/${MaxEvents}: $line"
    }else{Write-Host "Read-only H0/H1 observer record received"}
  }
}finally {
  if($serial.IsOpen){$serial.Close()};$serial.Dispose()
}
Write-Host "H2A records=$events; firmware=$sha; file=$file"
if($events -eq 0){throw "Khong nhan M9H2_GUARD; kiem tra Flash SHA, COM, MCU Resume."}
if($events -lt $MaxEvents){Write-Warning "Partial file useful; no need repeat already logged runs."}
