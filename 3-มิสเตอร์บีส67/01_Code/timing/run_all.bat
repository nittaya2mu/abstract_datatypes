@echo off
setlocal
REM รันจากโฟลเดอร์ timing/ โดยใช้คำสั่งรูปแบบเดียวกับชุดข้อมูลกลางของรายวิชา
REM Usage: run_all.bat "C:\path\to\ชุดข้อมูลทดสอบ_01204212"
if "%~1"=="" (
  echo Usage: run_all.bat "C:\path\to\ชุดข้อมูลทดสอบ_01204212"
  exit /b 1
)
set "DATA_DIR=%~1"
cd /d "%~dp0"

echo [1/2] Compiling timing_avl.c ...
gcc timing_avl.c -o timing_avl.exe
if errorlevel 1 (
  echo Compile failed.
  exit /b 1
)

echo.
echo [2/2] Running n=1,000 ...
timing_avl.exe "%DATA_DIR%\data_1000.txt" "%DATA_DIR%\targets_1000.txt"
if errorlevel 1 exit /b 1

echo.
echo [2/2] Running n=10,000 ...
timing_avl.exe "%DATA_DIR%\data_10000.txt" "%DATA_DIR%\targets_10000.txt"
if errorlevel 1 exit /b 1

echo.
echo [2/2] Running n=100,000 ...
timing_avl.exe "%DATA_DIR%\data_100000.txt" "%DATA_DIR%\targets_100000.txt"
if errorlevel 1 exit /b 1

echo.
echo All timing runs completed successfully.
endlocal
