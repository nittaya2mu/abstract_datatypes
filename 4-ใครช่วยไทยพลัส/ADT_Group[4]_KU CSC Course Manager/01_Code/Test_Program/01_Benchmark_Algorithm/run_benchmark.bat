@echo off
title Benchmark Search Algorithms - 01204212
echo =======================================================
echo   Benchmarking Search Algorithms: Sequential vs Binary vs AVL
echo =======================================================
echo.

if not exist timing.exe (
    echo Compiling timing_template.c ...
    gcc timing_template.c -o timing.exe -O3
)

echo [1/3] Running benchmark with N = 1,000 (sorted) ...
timing.exe data_1000_sorted.txt targets_1000.txt
echo.

echo [2/3] Running benchmark with N = 10,000 (sorted) ...
timing.exe data_10000_sorted.txt targets_10000.txt
echo.

echo [3/3] Running benchmark with N = 100,000 (sorted) ...
timing.exe data_100000_sorted.txt targets_100000.txt
echo.

echo =======================================================
echo Benchmark completed!
echo =======================================================
pause
