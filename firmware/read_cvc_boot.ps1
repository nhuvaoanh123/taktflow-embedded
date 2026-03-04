# Open CVC serial port first
$port = New-Object System.IO.Ports.SerialPort 'COM7', 115200, 'None', 8, 'One'
$port.ReadTimeout = 6000
$port.Open()

# Flush any old data
Start-Sleep -Milliseconds 200
$port.DiscardInBuffer()

# Reset CVC via ST-LINK (index=1)
Start-Process -NoNewWindow -FilePath "C:\Program Files (x86)\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" -ArgumentList "--connect port=SWD index=1 mode=UR --start 0x8000000"

# Wait for reset
Start-Sleep -Milliseconds 1500

# Read fresh output
for ($i = 0; $i -lt 12; $i++) {
    try {
        $line = $port.ReadLine()
        Write-Host $line
    } catch {
        Start-Sleep -Milliseconds 500
    }
}
$port.Close()
