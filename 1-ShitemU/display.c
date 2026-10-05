#include <stdio.h>
#include "display.h"
#include "puzzle.h"

// แสดงผลตารางกระดาน Sudoku ขนาดต่างๆ (4x4 หรือ 9x9) ทางหน้าจอ
void printUniversalBoard(int b[N][N], int size) {
    // กำหนดขนาดของบล็อกย่อย (4x4 ใช้บล็อก 2x2, 9x9 ใช้บล็อก 3x3)
    int boxSize = (size == 4) ? 2 : 3;
    
    // พิมพ์หมายเลขคอลัมน์ด้านบน
    printf("\n    ");
    for (int j = 0; j < size; j++) {
        printf("%d ", j + 1);
        if (j < size - 1 && (j + 1) % boxSize == 0) printf("  "); // เว้นช่องระหว่างบล็อก
    }
    printf("(Col)\n");
    
    // พิมพ์เส้นกรอบด้านบน
    int lineLength = size * 2 + (size / boxSize) - 1;
    printf("  +");
    for (int k = 0; k < lineLength; k++) printf("-");
    printf("+\n");
    
    // พิมพ์ตารางแต่ละแถว
    for (int i = 0; i < size; i++) {
        printf("%d | ", i + 1); // พิมพ์หมายเลขแถวด้านซ้าย
        for (int j = 0; j < size; j++) {
            if (b[i][j] == 0) printf(". "); // ช่องว่างแสดงเป็นจุด .
            else printf("%d ", b[i][j]);    // ช่องที่มีค่าให้พิมพ์ตัวเลข
            if (j < size - 1 && (j + 1) % boxSize == 0) printf("| "); // เส้นแบ่งบล็อกแนวตั้ง
        }
        printf("|\n");
        // พิมพ์เส้นแบ่งบล็อกแนวนอนเมื่อจบกลุ่มแถว
        if (i < size - 1 && (i + 1) % boxSize == 0) {
            printf("  +");
            for (int k = 0; k < lineLength; k++) printf("-");
            printf("+\n");
        }
    }
    // พิมพ์เส้นกรอบด้านล่างสุด
    printf("  +");
    for (int k = 0; k < lineLength; k++) printf("-");
    printf("+\n");
}

// แสดงผลตาราง Sudoku พร้อมทำไฮไลต์สีแถวและคอลัมน์ที่ผู้เล่นเลือก (ด้วย ANSI Escape Sequences)
void printUniversalBoardHighlight(int b[N][N], int size, int hlRow, int hlCol) {
    int boxSize = (size == 4) ? 2 : 3;
    
    // พิมพ์หมายเลขคอลัมน์
    printf("\n    ");
    for (int j = 0; j < size; j++) {
        printf("%d ", j + 1);
        if (j < size - 1 && (j + 1) % boxSize == 0) printf("  ");
    }
    printf("(Col)\n");
    
    int lineLength = size * 2 + (size / boxSize) - 1;
    printf("  +");
    for (int k = 0; k < lineLength; k++) printf("-");
    printf("+\n");
    
    // พิมพ์ข้อมูลตารางพร้อมการไฮไลต์
    for (int i = 0; i < size; i++) {
        printf("%d | ", i + 1);
        for (int j = 0; j < size; j++) {
            // เช็คว่าตำแหน่งปัจจุบันอยู่ในแถวหรือคอลัมน์ที่ต้องการเน้นสีหรือไม่
            if (i == hlRow || j == hlCol) {
                if (b[i][j] == 0) printf("\033[46;30m. \033[0m"); // ไฮไลต์ช่องว่างด้วยพื้นหลังสีฟ้า
                else printf("\033[46;30m%d \033[0m", b[i][j]);    // ไฮไลต์ตัวเลขด้วยพื้นหลังสีฟ้า
            } else {
                if (b[i][j] == 0) printf(". ");
                else printf("%d ", b[i][j]);
            }
            if (j < size - 1 && (j + 1) % boxSize == 0) printf("| ");
        }
        printf("|\n");
        if (i < size - 1 && (i + 1) % boxSize == 0) {
            printf("  +");
            for (int k = 0; k < lineLength; k++) printf("-");
            printf("+\n");
        }
    }
    printf("  +");
    for (int k = 0; k < lineLength; k++) printf("-");
    printf("+\n");
}