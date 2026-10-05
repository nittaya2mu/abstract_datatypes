@echo off
pushd "%~dp0"
cls
echo ====================================================================
echo   TIMING EVALUATION (5 RUNS + TRIMMED MEAN) - TEENOI SUKI (BST)
echo   Course 01204212: Abstract Data Types and Problem Solving
echo ====================================================================
echo.

if not exist timing_5runs.exe (
    echo [Compiling] gcc -O2 timing_5runs.c -o timing_5runs.exe
    gcc -O2 timing_5runs.c -o timing_5runs.exe
)

echo --------------------------------------------------------------------
echo [1/3] Testing Dataset Size n = 1,000 (5 Runs)...
echo --------------------------------------------------------------------
timing_5runs.exe data_1000.txt targets_1000.txt
echo.

echo --------------------------------------------------------------------
echo [2/3] Testing Dataset Size n = 10,000 (5 Runs)...
echo --------------------------------------------------------------------
timing_5runs.exe data_10000.txt targets_10000.txt
echo.

echo --------------------------------------------------------------------
echo [3/3] Testing Dataset Size n = 100,000 (5 Runs)...
echo --------------------------------------------------------------------
timing_5runs.exe data_100000.txt targets_100000.txt
echo.

echo ====================================================================
echo   [SUCCESS] All 5-run benchmark evaluations completed!
echo ====================================================================
echo.
pause
popd
