# CreoOS — Windows-side UART listener (PowerShell)
# Watches serial COM ports for the CreoOS banner and logs to CreoOS\serial.log.
# Works for a USB-C UART jig (best effort). Command:
#   powershell -ExecutionPolicy Bypass -File .\listen.ps1 -COM "COM5"   (specific)
#   powershell -ExecutionPolicy Bypass -File .\listen.ps1               (scan)
param(
    [string]$COM = "",
    [int]$BAUD = 115200,
    [string]$LogFile = "$env:USERPROFILE\Documents\CreoOS-boot\serial.log"
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path (Split-Path $LogFile) | Out-Null

Write-Host "== CreoOS listener (log: $LogFile)"

$ports = @()
if ($COM -ne "") { $ports += $COM }
else {
    $ports = @(Get-CimInstance -Class Win32_SerialPort | ForEach-Object { $_.DeviceID })
    Write-Host ("Serial ports found: " + ($ports -join ", "))
}

Add-Type -AssemblyName System.IO.Ports
foreach ($p in $ports) {
    try {
        $sp = New-Object System.IO.Ports.SerialPort($p, $BAUD, 'None', 8, 'One')
        $sp.ReadTimeout = 500
        $sp.NewLine = "`n"
        $sp.open()
        Write-Host "listening on $p ..."
        while ($sp.IsOpen) {
            try {
                $line = $sp.ReadExisting()
                if ($line -and $line.Length -gt 0) {
                    Add-Content -Path $LogFile -Value $line
                    Write-Host -NoNewline $line
                }
            } catch { }
            Start-Sleep -Milliseconds 100
        }
    } catch {
        Write-Host "  (skip $p: $($_.Exception.Message))"
    }
}
Write-Host "done — output saved in $LogFile (tell opener to read it)"
