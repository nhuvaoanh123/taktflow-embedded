param(
    [string]$PortName = "COM11",
    [int]$BaudRate = 115200,
    [int]$TimeoutSec = 7
)

$port = New-Object System.IO.Ports.SerialPort $PortName, $BaudRate, 'None', 8, 'One'
$port.ReadTimeout = 1000
$port.Open()

try {
    $buf = New-Object byte[] 1024
    $total = 0
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    while ($sw.ElapsedMilliseconds -lt ($TimeoutSec * 1000)) {
        try {
            $n = $port.Read($buf, $total, 1024 - $total)
            $total += $n
            if ($total -ge 1024) { break }
        } catch {
            # timeout, keep trying
        }
    }
    if ($total -gt 0) {
        $text = [System.Text.Encoding]::ASCII.GetString($buf, 0, $total)
        Write-Output "=== $total bytes from $PortName ==="
        Write-Output $text
        Write-Output "=== HEX (first 64 bytes) ==="
        $limit = [Math]::Min($total, 64)
        $hex = ($buf[0..($limit-1)] | ForEach-Object { '{0:X2}' -f $_ }) -join ' '
        Write-Output $hex
    } else {
        Write-Output "NO OUTPUT from $PortName - 0 bytes in $TimeoutSec seconds"
    }
} finally {
    $port.Close()
}
