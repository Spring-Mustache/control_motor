<#
M10 Windows PowerShell 5.1 UDP echo test.
Read-only network probe: sends ASCII payload to STM32; cannot control motors.
Run only after flashing isolated M10 firmware and verifying safe bench power.
#>
[CmdletBinding()]
param(
    [string]$TargetIP = "192.168.10.20",
    [int]$Port = 5005
)
$ErrorActionPreference = "Stop"
$bytes = [System.Text.Encoding]::ASCII.GetBytes("M10-HELLO-" + [Guid]::NewGuid().ToString("N").Substring(0,8))
$endpoint = [System.Net.IPEndPoint]::new([System.Net.IPAddress]::Parse($TargetIP), $Port)
$client = New-Object System.Net.Sockets.UdpClient
try {
    $client.Client.ReceiveTimeout = 2000
    $sent = $client.Send($bytes, $bytes.Length, $endpoint)
    $remote = [System.Net.IPEndPoint]::new([System.Net.IPAddress]::Any, 0)
    $reply = $client.Receive([ref]$remote)
    if ($remote.Address.ToString() -ne $TargetIP -or $remote.Port -ne $Port) {
        throw "Unexpected UDP response peer: $remote"
    }
    if ($reply.Length -ne $bytes.Length) {
        throw "Length mismatch: sent $($bytes.Length), received $($reply.Length)"
    }
    for ($i=0; $i -lt $bytes.Length; $i++) {
        if ($reply[$i] -ne $bytes[$i]) {
            throw "Payload mismatch at byte $i"
        }
    }
    Write-Host ("PASS M10 Windows UDP echo: {0} bytes from {1}:{2}" -f $sent,$TargetIP,$Port)
} finally {
    $client.Close()
}
