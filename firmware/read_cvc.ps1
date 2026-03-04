# Read CVC serial output (COM7)
$port = New-Object System.IO.Ports.SerialPort 'COM7', 115200, 'None', 8, 'One'
$port.ReadTimeout = 6000
$port.Open()
for ($i = 0; $i -lt 8; $i++) {
    try {
        $line = $port.ReadLine()
        Write-Host $line
    } catch {
        Start-Sleep -Milliseconds 500
    }
}
$port.Close()
