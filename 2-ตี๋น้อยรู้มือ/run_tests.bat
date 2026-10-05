@echo off
pushd "%~dp0"
cls
echo ========================================================
echo   TEENOI SUKI - AUTOMATED TEST AND BENCHMARK SUITE
echo ========================================================
echo.

if not exist teenoi.exe (
    echo Error: teenoi.exe not found!
    pause
    popd
    exit /b 1
)

echo [1/3] Running Unit Checks (Self-Test)...
teenoi.exe --test
if errorlevel 1 (
    echo [FAIL] Self-test failed!
    pause
    popd
    exit /b 1
)
echo.

echo [2/3] Running Big-O Empirical Benchmark (100, 1000, 10000 items)...
teenoi.exe --benchmark
echo.

echo [3/3] Running End-to-End Scenario Simulation...
copy /y mock_data_small.txt teenoi_data.txt > nul
powershell -NoProfile -Command "Write-Host '>> Scenario 1 [Happy Path - Small DB 100 items]:'; Measure-Command { Get-Content scenario_happy.txt | .\teenoi.exe } | Select-Object TotalMilliseconds"

copy /y mock_data_medium.txt teenoi_data.txt > nul
powershell -NoProfile -Command "Write-Host '>> Scenario 2 [Stress Waitlist - Medium DB 1,000 items]:'; Measure-Command { Get-Content scenario_stress_waitlist.txt | .\teenoi.exe } | Select-Object TotalMilliseconds"

copy /y mock_data_large.txt teenoi_data.txt > nul
powershell -NoProfile -Command "Write-Host '>> Scenario 3 [Promo Admin - Large DB 10,000 items]:'; Measure-Command { Get-Content scenario_promo_admin.txt | .\teenoi.exe } | Select-Object TotalMilliseconds"

echo.
echo ========================================================
echo   [SUCCESS] All checks and benchmarks completed 100%%!
echo ========================================================
echo.
pause
popd
