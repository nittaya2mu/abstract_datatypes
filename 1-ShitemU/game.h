#ifndef GAME_H
#define GAME_H

// โครงสร้างข้อมูลสำหรับตั้งค่าโหมดเกม (ชื่อโหมด, เวลา, โบนัส, หักเวลา, จำนวนช่องว่าง, โควตาตอบผิด)
typedef struct {
    const char *name;   // ชื่อโหมดการเล่น
    int timeLimit;      // เวลาจำกัด (วินาที)
    int bonus;          // เวลาโบนัสเมื่อตอบถูก (วินาที)
    int penalty;        // เวลาที่ถูกหักเมื่อตอบผิด (วินาที)
    int emptyCells;     // จำนวนช่องว่างในโจทย์
    int maxMistakes;    // จำนวนครั้งที่ตอบผิดได้สูงสุด
} GameConfig;

// ฟังก์ชันดึงค่าการตั้งค่าเกมตามขนาดตารางและความยาก
GameConfig getGameConfig(int size, int difficulty);

// ฟังก์ชันหลักสำหรับดำเนินกระบวนการเล่นเกม Sudoku
void playGame(void);

#endif