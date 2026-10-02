@echo off
cd /d "%~dp0"
py -3 -m pip install pyinstaller==6.22.3
if errorlevel 1 goto failed
py -3 -m PyInstaller --noconfirm --onefile --windowed --name SmartParking10 --add-data "data;data" app.py
if errorlevel 1 goto failed
echo Built: dist\SmartParking10.exe
pause
exit /b 0
:failed
echo Build failed. Check your Python installation includes working Tkinter.
pause
exit /b 1
