# Passive M9-H0 post-DONE summaries; no serial commands, no motor writes.
[CmdletBinding()]param(
[string]$Port="",
[ValidateRange(10,3600)][int]$TimeoutSeconds=600,
[ValidateRange(1,30)][int]$MaxEvents=6,
[string]$OutputDirectory=".\logs"
)
$ErrorActionPreference="Stop"
$ports=@([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
if($ports.Count -eq 0){throw "Khong tim thay COM ST-LINK VCP."}
if([string]::IsNullOrWhiteSpace($Port)){
 $st=@()
 try{$st=@(Get-CimInstance Win32_PnPEntity -ErrorAction Stop |
 Where-Object {$_.Name -match 'ST.?LINK.*\(COM\d+\)'} |
 ForEach-Object {[regex]::Match($_.Name,'COM\d+').Value} |
 Where-Object {$ports -contains $_} | Select-Object -Unique)}catch{}
 if($st.Count -eq 1){$Port=$st[0]}
 elseif($ports.Count -eq 1){$Port=$ports[0]}
 else{$Port=(Read-Host "Chon COM ST-LINK: $($ports -join ', ')").Trim()}
}
if($ports -notcontains $Port){throw "COM khong hop le: $Port"}
$folder=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
[System.IO.Directory]::CreateDirectory($folder)|Out-Null
$file=Join-Path $folder ("M9H0_drift_"+(Get-Date -Format "yyyyMMdd_HHmmss")+".txt")
$serial=New-Object System.IO.Ports.SerialPort -ArgumentList @(
$Port,115200,[System.IO.Ports.Parity]::None,8,[System.IO.Ports.StopBits]::One)
$serial.Handshake=[System.IO.Ports.Handshake]::None
$serial.ReadTimeout=600
$serial.NewLine="`n"
$serial.Encoding=[System.Text.Encoding]::ASCII
$serial.DtrEnable=$false
$serial.RtsEnable=$false
$n=0;$fwSha="";$utf8=New-Object System.Text.UTF8Encoding($false)
$deadline=(Get-Date).AddSeconds($TimeoutSeconds)
try{
 $serial.Open()
 Write-Host "Nghe M9H0_DRIFT tren $Port. File: $file"
 while((Get-Date) -lt $deadline -and $n -lt $MaxEvents){
  try{$line=$serial.ReadLine().Trim()}catch [System.TimeoutException]{continue}
  if(-not $line.StartsWith("M9H0_DRIFT,")){continue}
  if($line -match '(^|,)sha=([0-9a-fA-F]{40})(,|$)'){
    $candidate=$Matches[2].ToLowerInvariant()
    if($fwSha -eq ""){$fwSha=$candidate}
    elseif($fwSha -ne $candidate){Write-Warning "Firmware SHA changed while capturing"}
  }else{Write-Warning "M9H0_DRIFT does not contain full MCU SHA"}
  [System.IO.File]::AppendAllText($file,$line+"`r`n",$utf8)
  $n++
  Write-Host "Captured $n/${MaxEvents}: $line"
 }
}finally{
 if($serial.IsOpen){$serial.Close()};$serial.Dispose()
}
Write-Host "Saved $n records: $file ; MCU SHA=$fwSha"
if($n -eq 0){throw "Khong co M9H0_DRIFT: kiem tra firmware/COM va lenh R1 hoac Voice5..40 DONE; ban ghi chi phat khi xe dung."}
