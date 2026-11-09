# Persistent M9-H2B1 status receiver. NO serial commands or motor activation.
[CmdletBinding()]param(
 [string]$Port="",
 [ValidateRange(30,3600)][int]$TimeoutSeconds=300,
 [ValidateRange(1,100)][int]$MaxEvents=4,
 [switch]$UntilStopped,
 [string]$OutputDirectory=".\logs"
)
$ErrorActionPreference="Stop"
$ports=@([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
if($ports.Count -eq 0){throw "Khong co COM ST-LINK VCP."}
if([string]::IsNullOrWhiteSpace($Port)){
 $st=@()
 try{
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
$file=Join-Path $folder ("M9H2B1_HOLD_"+(Get-Date -Format "yyyyMMdd_HHmmss")+".txt")
$serial=New-Object System.IO.Ports.SerialPort -ArgumentList @(
 $Port,115200,[System.IO.Ports.Parity]::None,8,[System.IO.Ports.StopBits]::One)
$serial.Handshake=[System.IO.Ports.Handshake]::None
$serial.ReadTimeout=500
$serial.NewLine=[string][char]10
$serial.Encoding=[System.Text.Encoding]::ASCII
$serial.DtrEnable=$false
$serial.RtsEnable=$false
$utf8=New-Object System.Text.UTF8Encoding($false)
$events=0;$sha="";$deadline=(Get-Date).AddSeconds($TimeoutSeconds)
try{
 $serial.Open()
 Write-Host "H2-B1 listening $Port, NO COMMANDS. $file"
 Write-Host "Collector timeout/end does NOT stop the HOLD controller."
 while((Get-Date) -lt $deadline -and $events -lt $MaxEvents){
  try{$line=$serial.ReadLine().Trim()}catch [System.TimeoutException]{continue}
  if(-not $line.StartsWith("M9H2B1_HOLD,")){continue}
  [System.IO.File]::AppendAllText($file,$line+[char]13+[char]10,$utf8)
  if($line -match '(^|,)sha=([0-9a-fA-F]{40})(,|$)'){
   $candidate=$Matches[2].ToLowerInvariant()
   if($sha -eq ""){$sha=$candidate}
   elseif($sha -ne $candidate){Write-Warning "Mixed MCU firmware SHA."}
  }else{Write-Warning "Full MCU SHA absent; preserve record."}
  $events++
  Write-Host "HOLD $($events)/$($MaxEvents): $line"
  if($line -match '(^|,)state=6(,|$)' -and $line -match '(^|,)active=1(,|$)'){
   Write-Host "COOLDOWN: target retained, outputs zero; automatic retry after budget recovery."
  }
  if($line -match '(^|,)profile=1(,|$)'){
   Write-Warning "TUNING profile CAN POWER BOTH MOTORS; commissioned=0 means thermal/current acceptance is pending."
  }elseif($line -match '(^|,)commissioned=0(,|$)'){
   Write-Warning "Physical power UNCOMMISSIONED. This record is NOT powered HOLD validation."
  }
  if($UntilStopped -and $line -match '(^|,)active=0(,|$)' -and
     $line -match '(^|,)state=(4|5)(,|$)'){
   Write-Host "Terminal STOP/revoke/fault captured; inspect reason/everApplied/appliedLms/appliedRms."
   break
  }
 }
}finally{
 if($serial.IsOpen){$serial.Close()}
 $serial.Dispose()
}
Write-Host "H2-B1 records=$events; firmware=$sha; file=$file"
if($events -eq 0){throw "No H2B1 status; check flash SHA, COM, MCU Resume and eligible completed target."}

