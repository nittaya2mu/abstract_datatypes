@echo off
pushd "%~dp0"
cls
echo ====================================================================
echo   TIMING EVALUATION - TEENOI SUKI (BST SEARCH)
echo   Course 01204212: Abstract Data Types and Problem Solving
echo ====================================================================
echo.

if not exist timing.exe (
    echo [Compiling] gcc -O2 timing_template.c -o timing.exe
    gcc -O2 timing_template.c -o timing.exe
)

echo --------------------------------------------------------------------
echo [1/3] Testing Dataset Size n = 1,000...
echo --------------------------------------------------------------------
timing.exe data_1000.txt targets_1000.txt
echo.

echo --------------------------------------------------------------------
echo [2/3] Testing Dataset Size n = 10,000...
echo --------------------------------------------------------------------
timing.exe data_10000.txt targets_10000.txt
echo.

echo --------------------------------------------------------------------
echo [3/3] Testing Dataset Size n = 100,000...
echo --------------------------------------------------------------------
timing.exe data_100000.txt targets_100000.txt
echo.

echo ====================================================================
echo   [SUCCESS] All 3 benchmark runs completed!
echo ====================================================================
echo.
pause
popd
