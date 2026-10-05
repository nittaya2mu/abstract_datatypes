#include <stdlib.h>   // ใช้ rand()
#include "puzzle.h"

// ตัวแปรส่วนกลางสำหรับจัดการโจทย์และกระดานเกม (ถูกอ้างอิง extern ใน puzzle.h)
int currentN = 9;
int board[N][N];
int startBoard[N][N];
int solution[N][N];

/* ------------------------------------------------------------
   ส่วนนับจำนวนเฉลย (ใช้การค้นหาแบบย้อนรอย Backtracking)
   ------------------------------------------------------------ */

// หาช่องว่างที่มีตัวเลือกตัวเลขที่สามารถลงได้น้อยที่สุด แล้วทดลองลงทีละค่า
// คืนค่าจำนวนเฉลยที่ค้นพบ และจะหยุดทำงานเมื่อจำนวนเฉลยถึงค่า limit
static int solveCount(int b[N][N], int size, int limit) {
    int bestR = -1, bestC = -1, bestCnt = size + 1;

    // วนลูปหาช่องว่างที่มีตัวเลือกตัวเลขน้อยที่สุด
    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            if (b[r][c] != 0) continue;

            int cnt = 0;
            for (int v = 1; v <= size; v++)
                if (isValidMove(b, r, c, v, size)) cnt++;

            if (cnt == 0) return 0;   // เจอช่องทางตันที่ไม่สามารถใส่เลขอะไรได้เลย
            if (cnt < bestCnt) { bestCnt = cnt; bestR = r; bestC = c; }
        }
    }

    // หากไม่เหลือช่องว่าง แสดงว่าพบคำตอบที่ถูกต้อง 1 ชุด
    if (bestR == -1) return 1;

    // ลองใส่ตัวเลขทีละค่าในช่องที่เลือกไว้
    int total = 0;
    for (int v = 1; v <= size; v++) {
        if (isValidMove(b, bestR, bestC, v, size)) {
            b[bestR][bestC] = v;
            total += solveCount(b, size, limit - total);
            b[bestR][bestC] = 0;   // ถอยกลับ (Backtrack)
            if (total >= limit) break;
        }
    }
    return total;
}

// ฟังก์ชันคัดลอกตารางไปชุดชั่วคราวก่อนคำนวณ เพื่อป้องกันไม่ให้กระทบกระดานจริง
int countSolutions(int b[N][N], int size, int limit) {
    int tmp[N][N];
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) tmp[i][j] = b[i][j];
    return solveCount(tmp, size, limit);
}

/* ------------------------------------------------------------
   ส่วนการเจาะช่องว่างในกระดาน
   ------------------------------------------------------------ */

// สุ่มลำดับตำแหน่งของช่อง แล้วทดลองลบทีละช่อง
// จะลบออกได้ต่อเมื่อกระดานยังมีเฉลยเพียงชุดเดียวเท่านั้น (Unique Solution)
static void digHoles(int b[N][N], int size, int emptyCells) {
    int total = size * size;
    int order[N * N];

    // สุ่มสลับลำดับการเจาะช่องด้วยวิธี Fisher-Yates Shuffle
    for (int i = 0; i < total; i++) order[i] = i;
    for (int i = total - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int t = order[i]; order[i] = order[j]; order[j] = t;
    }

    int removed = 0;
    for (int k = 0; k < total && removed < emptyCells; k++) {
        int r = order[k] / size, c = order[k] % size;
        int saved = b[r][c];
        b[r][c] = 0;
        // ตรวจสอบว่าหลังจากลบแล้ว คำตอบยังเป็นแบบเดียว (countSolutions == 1) หรือไม่
        if (countSolutions(b, size, 2) == 1) removed++;
        else b[r][c] = saved;   // ถ้าคำตอบไม่ unique ให้คืนค่าเดิมกลับมา
    }
}

/* ------------------------------------------------------------
   ส่วนสร้างโจทย์เกม
   ------------------------------------------------------------ */

// สร้างโจทย์ 9x9 จากกระดานตั้งต้น แล้วทำการสุ่มสลับรูปแบบเพื่อให้ได้โจทย์ที่ไม่ซ้ำกัน
void makeNewPuzzle(int emptyCells) {
    // กระดานตั้งต้นที่ถูกต้อง 1 ชุด
    int baseGrid[9][9] = {
        {5,3,4,6,7,8,9,1,2},
        {6,7,2,1,9,5,3,4,8},
        {1,9,8,3,4,2,5,6,7},
        {8,5,9,7,6,1,4,2,3},
        {4,2,6,8,5,3,7,9,1},
        {7,1,3,9,2,4,8,5,6},
        {9,6,1,5,3,7,2,8,4},
        {2,8,7,4,1,9,6,3,5},
        {3,4,5,2,8,6,1,7,9}
    };

    // คัดลอกตารางไปยังกระดานชั่วคราว
    int tempGrid[9][9];
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            tempGrid[i][j] = baseGrid[i][j];
        }
    }

    // สุ่มสลับการจับคู่ตัวเลข 1-9 (เช่น เปลี่ยนตัวเลข 5 ทั้งหมดเป็นเลข 2)
    int map[10];
    for (int i = 1; i <= 9; i++) map[i] = i;
    for (int i = 1; i <= 9; i++) {
        int j = (rand() % 9) + 1;
        int t = map[i];
        map[i] = map[j];
        map[j] = t;
    }
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            tempGrid[i][j] = map[tempGrid[i][j]];
        }
    }

    // สุ่มสลับแถวภายในกลุ่มบล็อกแนวนอนเดียวกัน
    for (int block = 0; block < 3; block++) {
        int r1 = block * 3 + (rand() % 3);
        int r2 = block * 3 + (rand() % 3);
        for (int c = 0; c < 9; c++) {
            int t = tempGrid[r1][c];
            tempGrid[r1][c] = tempGrid[r2][c];
            tempGrid[r2][c] = t;
        }
    }

    // สุ่มสลับคอลัมน์ภายในกลุ่มบล็อกแนวตั้งเดียวกัน
    for (int block = 0; block < 3; block++) {
        int c1 = block * 3 + (rand() % 3);
        int c2 = block * 3 + (rand() % 3);
        for (int r = 0; r < 9; r++) {
            int t = tempGrid[r][c1];
            tempGrid[r][c1] = tempGrid[r][c2];
            tempGrid[r][c2] = t;
        }
    }

    // บันทึกคำตอบที่สมบูรณ์ลงในเฉลย และเตรียมตารางสำหรับการเล่น
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            solution[i][j] = tempGrid[i][j];
            board[i][j] = tempGrid[i][j];
        }
    }

    // เจาะช่องว่างตามจำนวนที่กำหนดเพื่อสร้างเป็นตัวโจทย์
    digHoles(board, 9, emptyCells);

    // บันทึกสถานะกระดานเริ่มต้นเอาไว้ใช้ตรวจสอบช่องห้ามแก้ไข
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            startBoard[i][j] = board[i][j];
}

// สร้างโจทย์ขนาด 4x4 โดยการสุ่มสลับจากแม่แบบ 3 แบบ
void makeNewPuzzle4x4(int b[N][N], int startB[N][N]) {
    // ล้างกระดานให้อยู่ในสถานะว่าง (0) ทั้งหมด
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            b[i][j] = 0;
            startB[i][j] = 0;
        }
    }

    // ชุดแม่แบบกระดานขนาด 4x4
    int baseTemplates[3][4][4] = {
        {{1, 2, 3, 4}, {3, 4, 1, 2}, {2, 1, 4, 3}, {4, 3, 2, 1}},
        {{2, 1, 4, 3}, {4, 3, 2, 1}, {1, 2, 3, 4}, {3, 4, 1, 2}},
        {{3, 4, 1, 2}, {1, 2, 3, 4}, {4, 3, 2, 1}, {2, 1, 4, 3}}
    };

    // สุ่มเลือกแม่แบบมา 1 ชุด
    int randTemplate = rand() % 3;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            b[i][j] = baseTemplates[randTemplate][i][j];
        }
    }

    // บันทึกคำตอบลงในเฉลย
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            solution[i][j] = b[i][j];

    // เจาะช่องว่างจำนวน 7 ช่อง
    digHoles(b, 4, 7);

    // บันทึกสถานะกระดานเริ่มต้น
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            startB[i][j] = b[i][j];
}

/* ------------------------------------------------------------
   ส่วนตรวจสอบกติกา
   ------------------------------------------------------------ */

// ตรวจสอบว่าสามารถลงตัวเลข val ในตำแหน่ง (r, c) ได้หรือไม่ (ต้องไม่ซ้ำในแถว คอลัมน์ และบล็อกย่อย)
int isValidMove(int b[N][N], int r, int c, int val, int size) {
    // ตรวจสอบความซ้ำซ้อนในแถวเดียวกัน
    for (int j = 0; j < size; j++) {
        if (b[r][j] == val) return 0;
    }

    // ตรวจสอบความซ้ำซ้อนในคอลัมน์เดียวกัน
    for (int i = 0; i < size; i++) {
        if (b[i][c] == val) return 0;
    }

    // ตรวจสอบความซ้ำซ้อนในบล็อกย่อย (2x2 สำหรับขนาด 4x4, 3x3 สำหรับขนาด 9x9)
    int boxSize = (size == 4) ? 2 : 3;
    int startRow = r - r % boxSize;
    int startCol = c - c % boxSize;
    for (int i = 0; i < boxSize; i++) {
        for (int j = 0; j < boxSize; j++) {
            if (b[startRow + i][startCol + j] == val) return 0;
        }
    }
    return 1;
}

// ตรวจสอบว่าการลงตัวเลขในช่อง (r, c) ทำให้บล็อกย่อยนั้นถูกเติมเต็มจนครบหรือไม่
int didMoveCompleteBox(int b[N][N], int r, int c, int size) {
    int boxSize = (size == 4) ? 2 : 3;
    int startRow = r - r % boxSize;
    int startCol = c - c % boxSize;
    int zeroCount = 0;
    for (int i = 0; i < boxSize; i++) {
        for (int j = 0; j < boxSize; j++) {
            if (b[startRow + i][startCol + j] == 0) zeroCount++;
        }
    }
    return (zeroCount == 0);
}

// ตรวจสอบว่าการลงตัวเลขในช่อง (r, c) ทำให้แถวหรือคอลัมน์นั้นถูกเติมเต็มจนครบหรือไม่
int didMoveCompleteLine(int b[N][N], int r, int c, int size) {
    int rowZeros = 0, colZeros = 0;
    for (int j = 0; j < size; j++) {
        if (b[r][j] == 0) rowZeros++;
    }
    for (int i = 0; i < size; i++) {
        if (b[i][c] == 0) colZeros++;
    }
    return (rowZeros == 0 || colZeros == 0);
}

// ตรวจสอบว่ากระดานถูกเติมเต็มครบทุกช่องแล้วหรือยัง (คืนค่า 1 หากไม่มีช่องว่างเหลือ)
int isBoardFull(int b[N][N], int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (b[i][j] == 0) return 0;
        }
    }
    return 1;
}