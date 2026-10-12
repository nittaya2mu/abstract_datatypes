@echo off
chcp 65001 >nul
cd /d "%~dp0"
echo กำลังเตรียมโปรแกรม...
gcc -std=c99 -Wall -Wextra main.c booking.c graph.c pqueue.c queue.c stack.c hashtable.c avltree.c data.c -o ferry -lm
if %errorlevel% neq 0 (
    echo คอมไพล์ไม่ผ่าน มี error ด้านบน
    pause
    exit /b 1
)
echo พร้อมแล้ว กำลังเปิดโปรแกรม...
ferry.exe
pause