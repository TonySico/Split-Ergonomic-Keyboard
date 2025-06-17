@REM Script to compile code with new mapping
@echo off

set Path=%Path%;%CD%\avr8-gnu-toolchain\bin
set Path=%Path%;%CD%\avr8-gnu-toolchain\avr\bin

echo Compiling and linking: 
echo.
firmware\avr8-gnu-toolchain\bin\avr-gcc.exe -mmcu=atmega32u4 -Wall -Os -o firmware\bin\main_master.elf firmware\src\main_master.c firmware\src\helper.c firmware\src\i2c.c firmware\src\usb.c firmware\src\mapping.c firmware\src\macros.c
echo. 
echo Successfully compiled and linked
echo. 
echo Generating .hex files:
echo. 
firmware\avr8-gnu-toolchain\bin\avr-objcopy -j .text -j .data -O ihex firmware\bin\main_master.elf firmware\bin\main_master.hex
echo. 
echo Successfully generated .hex files
echo. 
echo Flashing (reset master using onboard button):
echo.
firmware\avrdude\avrdude -p m32u4 -c avr109 -P COM4 -U flash:w:firmware\bin\main_master.hex
echo.
echo Successfully flashed board
echo.
echo All steps succeeded! You may close this window.