@echo off
setlocal
cd /d "%~dp0"

echo ============================================
echo   SmartBudget - Windows Desktop Builder
echo ============================================
echo.

echo [1/4] Installing dependencies...
call npm install
if errorlevel 1 goto :error

echo.
echo [2/4] Rebuilding native SQLite for Electron...
call npx electron-builder install-app-deps
if errorlevel 1 goto :error

echo.
echo [3/4] Building SmartBudget.exe...
call npm run dist
if errorlevel 1 goto :error

echo.
echo [4/4] DONE
echo.
echo EXE files are in the dist folder.
echo.
start "" explorer "%~dp0dist"
exit /b 0

:error
echo.
echo BUILD FAILED. Please check the error above.
pause
exit /b 1
