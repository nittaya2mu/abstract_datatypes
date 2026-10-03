@echo off
cd /d "%~dp0"
where gcc >nul 2>nul
if errorlevel 1 (
  echo Install GCC / MinGW-w64 with Windows headers. See README.
  pause
  exit /b 1
)
gcc -std=c11 -O2 -Wall -Wextra -DLAUNDRY=0 c\main.c c\adt.c c\benchmark.c -o SmartParkingC.exe -lm
if errorlevel 1 exit /b 1
gcc -std=c11 -O2 -Wall -Wextra -DLAUNDRY=0 c\ui_windows.c c\adt.c c\benchmark.c -mwindows -lcomctl32 -lgdi32 -luser32 -lm -o SmartParking.exe
if errorlevel 1 exit /b 1
SmartParkingC.exe --self-test
pause
