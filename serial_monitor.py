import serial
import time

try:
    # Open serial port at 9600 baud
    ser = serial.Serial('COM3', 9600, timeout=1)
    print("Serial monitor started - listening on COM3 at 9600 baud...")
    print("Press Ctrl+C to stop")
    print("-" * 50)
    
    while True:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8').rstrip()
            if line:
                print(line)
        time.sleep(0.1)
        
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
except KeyboardInterrupt:
    print("\nSerial monitor stopped")
finally:
    if 'ser' in locals():
        ser.close()
