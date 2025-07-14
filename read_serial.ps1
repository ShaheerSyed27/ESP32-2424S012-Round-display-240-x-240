# PowerShell script to read from COM3
# Simple serial port reader

try {
    $port = new-Object System.IO.Ports.SerialPort COM3,9600,None,8,one
    $port.Open()
    Write-Host "Serial port COM3 opened successfully at 9600 baud"
    Write-Host "Listening for data... Press Ctrl+C to stop"
    Write-Host "----------------------------------------"
    
    $timeout = 0
    while ($timeout -lt 300) {  # 30 second timeout
        if ($port.BytesToRead -gt 0) {
            $data = $port.ReadExisting()
            Write-Host $data -NoNewline
            $timeout = 0  # Reset timeout if we got data
        } else {
            Start-Sleep -Milliseconds 100
            $timeout++
        }
    }
    
    if ($timeout -ge 300) {
        Write-Host "`nNo data received after 30 seconds"
        Write-Host "Try physically pressing the reset button on the ESP32-C3"
    }
    
    $port.Close()
    Write-Host "`nSerial port closed"
} catch {
    Write-Host "Error: $($_.Exception.Message)"
    Write-Host "Make sure COM3 is available and not in use by another program"
}
