@echo off
title KU CSC Course Manager - Performance Test
set PATH=C:\msys64\ucrt64\bin;%PATH%
set TERMINFO=C:\msys64\ucrt64\share\terminfo
set TERM=xterm-256color
cd /d "%~dp0"
main_perf_test.exe
pause
