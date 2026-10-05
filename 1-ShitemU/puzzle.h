#ifndef PUZZLE_H
#define PUZZLE_H

// ขนาดกระดานสูงสุด (9x9) ใช้เป็นขนาดของ Array
enum { N = 9 };

extern int currentN;            // ขนาดกระดานที่กำลังเล่นอยู่ปัจจุบัน (4 หรือ 9)
extern int board[N][N];         // ตารางกระดานที่ผู้เล่นกำลังใส่อยู่
extern int startBoard[N][N];    // ตารางกระดานเริ่มต้น (ใช้ล็อกช่องห้ามแก้ไข)
extern int solution[N][N];      // ตารางกระดานเฉลยที่ถูกต้อง

// ฟังก์ชันสำหรับสร้างโจทย์ใหม่
void makeNewPuzzle(int emptyCells);
void makeNewPuzzle4x4(int b[N][N], int startB[N][N]);

// ฟังก์ชันนับจำนวนคำตอบของกระดาน (จะหยุดนับเมื่อถึงขีดจำกัด limit)
int countSolutions(int b[N][N], int size, int limit);

// ฟังก์ชันสำหรับตรวจสอบกฎกติกาและสถานะของตาราง
int isValidMove(int b[N][N], int r, int c, int val, int size);
int didMoveCompleteBox(int b[N][N], int r, int c, int size);
int didMoveCompleteLine(int b[N][N], int r, int c, int size);
int isBoardFull(int b[N][N], int size);

#endif