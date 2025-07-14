@echo off
echo Compiling ESP32-C3 SimpleTest...
arduino-cli compile --fqbn esp32:esp32:esp32c3 "..\SimpleTest" > compile_output.txt 2>&1
echo Compilation complete. Checking output...
type compile_output.txt
echo.
echo Upload attempt...
arduino-cli upload --fqbn esp32:esp32:esp32c3 --port COM3 "..\SimpleTest" > upload_output.txt 2>&1
type upload_output.txt
pause
