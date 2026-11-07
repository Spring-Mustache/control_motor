# M9-H1 real-data flight-recorder UART3 collector (READ ONLY).
# Stores REAL AS5600 / timer CCR captured DURING existing M8 motions;
# DOES NOT issue any command or change MCU/power state.
[CmdletBinding()]param(
[string]$Port="",
[ValidateRange(20,3600)][int]$TimeoutSeconds=600,
[ValidateRange(1,16)][int]$MaxRuns=4,
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
 else{$Port=(Read-Host "Nhap COM ST-LINK: $($ports -join ', ')").Trim()}
}
if($ports -notcontains $Port){throw "COM khong ton tai: $Port"}
$folder=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory)
[System.IO.Directory]::CreateDirectory($folder)|Out-Null
$file=Join-Path $folder ("M9H1_REAL_"+(Get-Date -Format "yyyyMMdd_HHmmss")+".txt")
$serial=New-Object System.IO.Ports.SerialPort -ArgumentList @(
 $Port,115200,[System.IO.Ports.Parity]::None,8,[System.IO.Ports.StopBits]::One)
$serial.Handshake=[System.IO.Ports.Handshake]::None
$serial.ReadTimeout=500
$serial.NewLine="`n"
$serial.Encoding=[System.Text.Encoding]::ASCII
$serial.DtrEnable=$false
$serial.RtsEnable=$false
$utf8=New-Object System.Text.UTF8Encoding($false)
$finished=0;$begins=0;$frames=0;$sha=""
$deadline=(Get-Date).AddSeconds($TimeoutSeconds)
try{
 $serial.Open()
 Write-Host "M9-H1 REAL: listening $Port at 115200. File: $file"
 Write-Host "Run 4 known-safe REAL steering commands, wait for REAL_END after each; no simulated input."
 while((Get-Date) -lt $deadline -and $finished -lt $MaxRuns){
  try{$line=$serial.ReadLine().Trim()}catch [System.TimeoutException]{continue}
  if(-not ($line.StartsWith("M9H1_REAL_") -or
           $line.StartsWith("M9H0_DRIFT,") -or
           $line.StartsWith("M9H1_SHADOW,"))){continue}
  if($line -match '(^|,)sha=([0-9a-fA-F]{40})(,|$)'){
   $found=$Matches[2].ToLowerInvariant()
   if($sha -eq ""){$sha=$found}
   elseif($sha -ne $found){Write-Warning "MCU SHA changed. Logs need independent review."}
  }else{Write-Warning "Incomplete MCU SHA on line."}
  [System.IO.File]::AppendAllText($file,($line+"`r`n"),$utf8)
  if($line.StartsWith("M9H1_REAL_BEGIN,")){
   $begins++
   Write-Host "REAL_BEGIN #$begins"
  }elseif($line.StartsWith("M9H1_REAL_FRAME,")){
   $frames++
  }elseif($line.StartsWith("M9H1_REAL_END,")){
   $finished++
   Write-Host "REAL_END $finished/$MaxRuns; $frames motion frames saved"
  }elseif($line.StartsWith("M9H0_DRIFT,")){
   Write-Host "H0 drift record captured"
  }elseif($line.StartsWith("M9H1_SHADOW,")){
   Write-Host "H1 idle shadow record captured"
  }
 }
}finally{
 if($serial.IsOpen){$serial.Close()};$serial.Dispose()
}
Write-Host "RESULT: completed=$finished, begin=$begins, frames=$frames; MCU SHA=$sha"
Write-Host "PRIVATE TXT: $file"
if($finished -eq 0){throw "No M9H1_REAL_END received. Verify new H1 REAL firmware, MCU Resume, COM and wait at safe idle."}
if($finished -lt $MaxRuns){Write-Warning "Fewer runs than requested. Partial file is still useful: $file"}
