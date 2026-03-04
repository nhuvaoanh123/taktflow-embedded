# Open serial port first
$port = New-Object System.IO.Ports.SerialPort 'COM8', 115200, 'None', 8, 'One'
$port.ReadTimeout = 2000
$port.Open()

# Reset the board via ST-LINK (background job)
Start-Process -NoNewWindow -FilePath "C:\Program Files (x86)\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" -ArgumentList "--connect port=SWD index=2 mode=UR --start 0x8000000"

# Wait a bit for reset to take effect
Start-Sleep -Milliseconds 1000

# Read boot output
for ($i = 0; $i -lt 80; $i++) {
    try {
        $line = $port.ReadLine()
        Write-Host $line
        if ($line -match 'CAN-DIAG') {
            # Read a few more lines after
            for ($j = 0; $j -lt 3; $j++) {
                try {
                    $line = $port.ReadLine()
                    Write-Host $line
                } catch { break }
            }
            break
        }
    } catch {
        Start-Sleep -Milliseconds 200
    }
}
$port.Close()
