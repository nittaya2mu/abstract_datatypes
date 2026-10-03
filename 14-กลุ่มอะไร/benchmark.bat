@echo off
cd /d "%~dp0"
if not exist results mkdir results
SmartParkingC.exe --benchmark data/instructor results/classroom_latest.csv
if errorlevel 1 (
 echo Benchmark failed. Do not submit incomplete values.
) else (
 echo Results: results\classroom_latest.csv
)
pause
