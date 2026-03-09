param(
    [string]$PortName = "COM11",
    [int]$BaudRate = 115200
)

$port = New-Object System.IO.Ports.SerialPort $PortName, $BaudRate, 'None', 8, 'One'
$port.ReadTimeout = 1000
$port.DtrEnable = $true
$port.RtsEnable = $true
$port.Open()
Write-Output "Port $PortName opened at $BaudRate baud"
Start-Sleep -Milliseconds 200
try { $port.ReadExisting() | Out-Null } catch {}

Write-Output "Flashing..."
$flashOut = & "C:/ti/ccs2041/ccs/ccs_base/DebugServer/bin/DSLite.exe" load -c "$PSScriptRoot/TMS570LC43xx_XDS110.ccxml" -f "$PSScriptRoot/build/tms570/sc.out" 2>&1
$flashOk = $LASTEXITCODE -eq 0
if ($flashOk) { Write-Output "Flash: OK" } else { Write-Output "Flash: FAIL"; Write-Output $flashOut }

Write-Output "Reading UART for 12 seconds..."
$buf = New-Object byte[] 4096
$total = 0
$sw = [System.Diagnostics.Stopwatch]::StartNew()
while ($sw.ElapsedMilliseconds -lt 12000) {
    try {
        $n = $port.Read($buf, $total, 4096 - $total)
        $total += $n
        if ($total -ge 4096) { break }
    } catch {}
}
$port.Close()

if ($total -gt 0) {
    $text = [System.Text.Encoding]::ASCII.GetString($buf, 0, $total)
    Write-Output "=== Got $total bytes ==="
    Write-Output $text
    Write-Output "=== HEX (first 64 bytes) ==="
    $limit = [Math]::Min($total, 64)
    $hex = ($buf[0..($limit-1)] | ForEach-Object { '{0:X2}' -f $_ }) -join ' '
    Write-Output $hex
} else {
    Write-Output "NO OUTPUT - 0 bytes in 12 seconds"
}
