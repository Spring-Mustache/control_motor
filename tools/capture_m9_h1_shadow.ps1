# Passive M9-H1 shadow and H0 logs from ST-LINK VCP; NEVER SENDS COMMANDS.
[CmdletBinding()]param(
[string]$Port="",
[ValidateRange(10,3600)][int]$TimeoutSeconds=600,
[ValidateRange(1,30)][int]$MaxEvents=4,
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
$file=Join-Path $folder ("M9H1_shadow_"+(Get-Date -Format "yyyyMMdd_HHmmss")+".txt")
$serial=New-Object System.IO.Ports.SerialPort -ArgumentList @(
$Port,115200,[System.IO.Ports.Parity]::None,8,[System.IO.Ports.StopBits]::One)
$serial.Handshake=[System.IO.Ports.Handshake]::None
$serial.ReadTimeout=600
$serial.NewLine="`n"
$serial.Encoding=[System.Text.Encoding]::ASCII
$serial.DtrEnable=$false
$serial.RtsEnable=$false
$shadowEvents=0;$fwSha="";$utf8=New-Object System.Text.UTF8Encoding($false)
$deadline=(Get-Date).AddSeconds($TimeoutSeconds)
try{
 $serial.Open()
 Write-Host "Nghe M9H0_DRIFT + M9H1_SHADOW tren $Port. File: $file"
 while((Get-Date) -lt $deadline -and $shadowEvents -lt $MaxEvents){
  try{$line=$serial.ReadLine().Trim()}catch [System.TimeoutException]{continue}
  if(-not ($line.StartsWith("M9H0_DRIFT,") -or $line.StartsWith("M9H1_SHADOW,"))){continue}
  if($line -match '(^|,)sha=([0-9a-fA-F]{40})(,|$)'){
    $candidate=$Matches[2].ToLowerInvariant()
    if($fwSha -eq ""){$fwSha=$candidate}
    elseif($fwSha -ne $candidate){Write-Warning "Firmware SHA changed while capturing"}
  }else{Write-Warning "M9 line missing full MCU SHA"}
  [System.IO.File]::AppendAllText($file,$line+"`r`n",$utf8)
  if($line.StartsWith("M9H1_SHADOW,")){$shadowEvents++}
  Write-Host "Shadow $shadowEvents/${MaxEvents}: $line"
 }
}finally{
 if($serial.IsOpen){$serial.Close()};$serial.Dispose()
}
Write-Host "Saved $shadowEvents M9H1_SHADOW records: $file ; MCU SHA=$fwSha"
if($shadowEvents -eq 0){throw "No M9H1_SHADOW after successful R1 or Voice 5..40 DONE. Check COM, MCU SHA and motor idle."}
