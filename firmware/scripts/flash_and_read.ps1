param(
    [string]$PortName = "COM11",
    [int]$BaudRate = 115200
)

# Open serial port first
$port = New-Object System.IO.Ports.SerialPort $PortName, $BaudRate, 'None', 8, 'One'
$port.ReadTimeout = 1000
$port.DtrEnable = $true
$port.RtsEnable = $true
$port.Open()
Write-Output "Port $PortName opened at $BaudRate baud"

# Flush any stale data
Start-Sleep -Milliseconds 200
try { $port.ReadExisting() | Out-Null } catch {}

# Now flash - this resets the TMS570
Write-Output "Flashing firmware..."
$flashOut = & "C:/ti/ccs2041/ccs/ccs_base/DebugServer/bin/DSLite.exe" load -c "H:/VS-Taktflow-Systems/Taktflow-Systems/taktflow-embedded/firmware/sc/TMS570LC43xx_XDS110.ccxml" -f "H:/VS-Taktflow-Systems/Taktflow-Systems/taktflow-embedded/firmware/build/tms570/sc.elf" 2>&1
$flashSuccess = $LASTEXITCODE -eq 0
Write-Output "Flash result: $(if ($flashSuccess) {'SUCCESS'} else {'FAILED'})"

# Read for 10 seconds to catch boot messages + heartbeat
Write-Output "Reading UART for 10 seconds..."
$buf = New-Object byte[] 2048
$total = 0
$sw = [System.Diagnostics.Stopwatch]::StartNew()
while ($sw.ElapsedMilliseconds -lt 10000) {
    try {
        $n = $port.Read($buf, $total, 2048 - $total)
        $total += $n
        if ($total -ge 2048) { break }
    } catch {
        # timeout
    }
}

$port.Close()

if ($total -gt 0) {
    $text = [System.Text.Encoding]::ASCII.GetString($buf, 0, $total)
    Write-Output "=== Got $total bytes ==="
    Write-Output $text
    Write-Output "=== HEX (first 128 bytes) ==="
    $limit = [Math]::Min($total, 128)
    $hex = ($buf[0..($limit-1)] | ForEach-Object { '{0:X2}' -f $_ }) -join ' '
    Write-Output $hex
} else {
    Write-Output "NO OUTPUT - 0 bytes in 10 seconds"
}
