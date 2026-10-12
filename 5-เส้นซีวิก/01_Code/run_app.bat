@echo off
pushd "%~dp0"
cls

g++ -std=c++17 -o fridge.exe main.cpp
if %errorlevel% neq 0 (
    echo Compilation failed!
    goto end
)

fridge.exe

:end
echo.
pause
popd
