@echo off
rem Build both versions with MinGW gcc (run in this folder)
echo Building console version (laundry.exe)...
gcc -Wall -Wextra -o laundry.exe laundry_console.c laundry_core.c
if errorlevel 1 goto err
echo Building GUI version (SmartLaundry.exe)...
gcc -O2 -Wall -o SmartLaundry.exe laundry_gui.c laundry_core.c -mwindows -lcomctl32 -lgdi32 -lshell32 -lole32
if errorlevel 1 goto err
echo Done. Run SmartLaundry.exe (window) or laundry.exe (console menu).
goto end
:err
echo Build FAILED.
:end
pause
