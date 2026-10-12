@echo off
pushd "%~dp0"
cls

gcc timing_template.c -o timing.exe
if %errorlevel% neq 0 (
    echo Compilation failed!
    goto end
)

timing.exe data_1000.txt targets_1000.txt
echo.
timing.exe data_10000.txt targets_10000.txt
echo.
timing.exe data_100000.txt targets_100000.txt

:end
echo.
pause
popd
