Get-WmiObject Win32_PnPEntity | Where-Object { $_.Name -match 'COM[0-9]+' } | Select-Object Name, DeviceID | Format-Table -AutoSize
