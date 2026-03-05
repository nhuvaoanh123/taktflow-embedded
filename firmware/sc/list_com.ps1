Get-WmiObject Win32_SerialPort | Select-Object DeviceID, Caption, Description | Format-Table -AutoSize
Write-Host ""
Write-Host "--- PnP Serial Devices ---"
Get-WmiObject Win32_PnPEntity | Where-Object { $_.Caption -match 'COM\d' -or $_.Caption -match 'UART' -or $_.Caption -match 'Serial' -or $_.Caption -match 'XDS' } | Select-Object Caption, Status | Format-Table -AutoSize
