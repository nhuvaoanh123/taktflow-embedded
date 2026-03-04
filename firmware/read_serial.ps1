$port = New-Object System.IO.Ports.SerialPort 'COM8', 115200, 'None', 8, 'One'
$port.ReadTimeout = 3000
$port.Open()
$output = ''
for ($i = 0; $i -lt 40; $i++) {
    try {
        $line = $port.ReadLine()
        $output += $line + "`n"
        if ($line -match 'CAN TX test') { break }
    } catch {
        Start-Sleep -Milliseconds 100
    }
}
$port.Close()
Write-Output $output
