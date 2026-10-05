#include "game.h"
#include "adt.h"
#include "puzzle.h"
#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// รองรับฟังก์ชันอ่านแป้นพิมพ์แบบ Non-blocking ทั้งใน Windows และ Linux/macOS
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
static int my_kbhit(void) { return _kbhit(); }
static int my_getch(void) { return _getch(); }
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#define sleep_ms(ms) usleep((ms) * 1000)

// ฟังก์ชันตรวจสอบว่ามีการกดคีย์บอร์ดหรือไม่ (สำหรับระบบ POSIX)
static int my_kbhit(void) {
    struct termios oldt, newt;
    int ch, oldf;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }
    return 0;
}

// ฟังก์ชันอ่านตัวอักษร 1 ตัวจากคีย์บอร์ดโดยไม่ต้องกด Enter (สำหรับระบบ POSIX)
static int my_getch(void) {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}
#endif

// คืนค่าโครงสร้าง GameConfig ที่กำหนดเวลา, คะแนนโบนัส, การลงโทษ และจำนวนช่องว่างตามโหมดที่เลือก
GameConfig getGameConfig(int size, int difficulty) {
    GameConfig cfg;
    cfg.maxMistakes = 3;
    if (size == 4) {
        cfg.name = "4x4";
        cfg.timeLimit = 3 * 60;
        cfg.bonus = 10;
        cfg.penalty = 5;
        cfg.emptyCells = 7;
    } else if (difficulty == 1) {
        cfg.name = "9x9 Easy";
        cfg.timeLimit = 8 * 60;
        cfg.bonus = 8;
        cfg.penalty = 5;
        cfg.emptyCells = 32;
    } else if (difficulty == 3) {
        cfg.name = "9x9 Hard";
        cfg.timeLimit = 5 * 60;
        cfg.bonus = 5;
        cfg.penalty = 7;
        cfg.emptyCells = 50;
    } else {
        cfg.name = "9x9 Medium";
        cfg.timeLimit = 6 * 60;
        cfg.bonus = 7;
        cfg.penalty = 6;
        cfg.emptyCells = 40;
    }
    return cfg;
}

// คำนวณเวลาที่เหลืออยู่เป็นวินาที
static int remainingSeconds(time_t endTime) {
    int rem = (int)difftime(endTime, time(NULL));
    return rem < 0 ? 0 : rem;
}

// แสดงแถบสถานะด้านบน (โหมด, เวลาที่เหลือ, จำนวนครั้งที่ผิด)
static void printStatusLine(const GameConfig *cfg, time_t endTime, int mistakes) {
    int rem = remainingSeconds(endTime);
    printf("%s[ %s | Time: %02d:%02d | Mistakes: %d/%d ]\033[0m\033[K",
           rem <= 30 ? "\033[31m" : "\033[33m", // เตือนสีแดงหากเวลาน้อยกว่า 30 วินาที
           cfg->name, rem / 60, rem % 60, mistakes, cfg->maxMistakes);
}

// แสดงเมนูและคำสั่งที่ใช้งานได้ในโหมดเล่นเกม
static void printPlayMenu(void) {
    printf("\n\033[36m[ Play Mode (%dx%d) - Welcome to sudoku naja ]\033[0m\n", currentN, currentN);
    printf(" - \033[32mMove\033[0m     : [Row 1-%d] [Col 1-%d] [Val 1-%d] (ex: 2 3 4)\n", currentN, currentN, currentN);
    printf(" - \033[33mHighlight\033[0m: hl [Row] [Col]                   (ex: hl 2 2)\n");
    printf(" - \033[35mhistory\033[0m  : Show all previous moves\n");
    printf(" - \033[36mreset\033[0m    : Generate a new random board\n");
    printf(" - \033[31m0\033[0m        : Back to Main Menu\n");
    printf("---------------------------------------\n");
    printf("Action > ");
}

// แสดงผลหน้าจอทั้งหมดใหม่ (ล้างจอ, แถบสถานะ, ข้อความแจ้งเตือน, ตารางกระดาน, และเมนูคำสั่ง)
static void renderScreen(const GameConfig *cfg, time_t endTime, int mistakes, int hlRow, int hlCol, const char *inputBuf, const char *msgBuf, int msgIsError) {
    printf("\033[H\033[J"); // ล้างหน้าจอ Console

    printStatusLine(cfg, endTime, mistakes);
    printf("\n");

    // พิมพ์ข้อความแจ้งเตือน (ถ้ามี)
    if (msgBuf && strlen(msgBuf) > 0) {
        if (msgIsError) {
            printf("\033[31m%s\033[0m\n", msgBuf); // ตัวอักษรสีแดงสำหรับข้อผิดพลาด
        } else {
            printf("\033[32m%s\033[0m\n", msgBuf); // ตัวอักษรสีเขียวสำหรับข้อความสำเร็จ
        }
    }

    // วาดตารางกระดานตามสถานะไฮไลต์
    if (hlRow != -1 && hlCol != -1) {
        printUniversalBoardHighlight(board, currentN, hlRow, hlCol);
    } else {
        printUniversalBoard(board, currentN);
    }

    printPlayMenu();
    printf("%s", inputBuf);
    fflush(stdout);
}

// ให้ผู้เล่นเลือกขนาดกระดานและความยากของเกม
static int chooseMode(GameConfig *cfg) {
    char sizeInput[50];
    
    // วนลูปรับขนาดกระดาน (4x4 หรือ 9x9)
    while (1) {
        printf("\n\033[35mSelect Board Size:\033[0m\n");
        printf("  - \033[32m4\033[0m : Play 4x4 Sudoku\n");
        printf("  - \033[32m9\033[0m : Play 9x9 Sudoku\n");
        printf("Choose size (4 or 9) > ");

        if (fgets(sizeInput, sizeof(sizeInput), stdin) == NULL) {
            return 0;
        }

        sizeInput[strcspn(sizeInput, "\r\n")] = 0;

        if (strlen(sizeInput) == 0) {
            printf("\n\033[31m[!] Please enter a size (4 or 9)!\033[0m\n");
            continue;
        }

        if (sscanf(sizeInput, "%d", &currentN) != 1 || (currentN != 4 && currentN != 9)) {
            printf("\n\033[31m[!] Invalid size! Please choose 4 or 9.\033[0m\n");
            continue;
        }
        break;
    }

    // หากเลือกขนาด 9x9 ให้เลือกความยากเพิ่มเติม
    int difficulty = 2;
    if (currentN == 9) {
        char diffInput[50];
        while (1) {
            printf("\n\033[35mSelect Difficulty (9x9):\033[0m\n");
            printf("  - \033[32m1 : Easy\033[0m   (8:00 | +8s / -5s)\n");
            printf("  - \033[33m2 : Medium\033[0m (6:00 | +7s / -6s)\n");
            printf("  - \033[31m3 : Hard\033[0m   (5:00 | +5s / -7s)\n");
            printf("Choose difficulty (1-3) > ");
            
            if (fgets(diffInput, sizeof(diffInput), stdin) == NULL) {
                return 0;
            }

            diffInput[strcspn(diffInput, "\r\n")] = 0;

            if (strlen(diffInput) == 0) {
                printf("\n\033[31m[!] Please enter a difficulty (1-3)!\033[0m\n");
                continue;
            }

            if (sscanf(diffInput, "%d", &difficulty) != 1 || difficulty < 1 || difficulty > 3) {
                printf("\n\033[31m[!] Invalid difficulty! Please choose between 1 and 3.\033[0m\n");
                continue;
            }
            break;
        }
    }

    *cfg = getGameConfig(currentN, difficulty);
    return 1;
}

// ฟังก์ชันหลักที่ควบคุมวงรอบการเล่นเกม (Game Loop) และการรับคำสั่ง
void playGame(void) {
    GameConfig cfg;
    if (!chooseMode(&cfg)) {
        return;
    }

    Queue historyQueue; // Queue สำหรับเก็บประวัติการลงตัวเลข

    // สร้างโจทย์ใหม่ตามขนาดตารางที่เลือก
    if (currentN == 4) {
        makeNewPuzzle4x4(board, startBoard);
    } else {
        makeNewPuzzle(cfg.emptyCells);
    }

    time_t endTime = time(NULL) + cfg.timeLimit; // เวลาสิ้นสุดการเล่น
    int mistakes = 0;                            // จำนวนครั้งที่ตอบผิด
    int rewarded[9][9] = {{0}};                  // บันทึกช่องที่เคยได้รับเวลาโบนัสแล้ว

    initQueue(&historyQueue);

    int hlRow = -1;
    int hlCol = -1;

    char inputBuf[100] = "";
    int inputLen = 0;
    char msgBuf[2048] = "";
    int msgIsError = 0;

    time_t lastTime = time(NULL);

    renderScreen(&cfg, endTime, mistakes, hlRow, hlCol, inputBuf, msgBuf, msgIsError);

    // วนลูปหลักของเกม
    while (1) {
        time_t now = time(NULL);

        // ตรวจสอบว่าหมดเวลาหรือไม่
        if (difftime(endTime, now) <= 0) {
            renderScreen(&cfg, endTime, mistakes, hlRow, hlCol, inputBuf, "\n*** TIME'S UP! You lose. ***", 1);
            break;
        }

        // อัปเดตตัวนับเวลาบนแถบสถานะแบบ Real-time ทุก 1 วินาที
        if (now != lastTime) {
            lastTime = now;
            printf("\033[s");       // บันทึกตำแหน่งเคอร์เซอร์
            printf("\033[1;1H");     // ย้ายเคอร์เซอร์ไปมุมซ้ายบน
            printStatusLine(&cfg, endTime, mistakes); 
            printf("\033[u");       // คืนตำแหน่งเคอร์เซอร์กลับที่เดิม
            fflush(stdout);
        }

        // ตรวจสอบการกดปุ่มจากคีย์บอร์ด
        if (my_kbhit()) {
            int ch = my_getch();

            // เมื่อผู้เล่นกด Enter เพื่อส่งคำสั่ง
            if (ch == '\r' || ch == '\n') {
                inputBuf[inputLen] = '\0';
                if (inputLen > 0) {
                    char subCmd[50] = "";
                    int r = 0, c = 0, val = 0;
                    sscanf(inputBuf, "%s", subCmd);

                    // คำสั่งกลับสู่เมนูหลัก
                    if (strcmp(subCmd, "0") == 0) {
                        printf("\nHeading back to menu...\n");
                        break;
                    }
                    // คำสั่งดูประวัติการเล่นทั้งหมด (จาก Queue)
                    else if (strcmp(subCmd, "history") == 0) {
                        snprintf(msgBuf, sizeof(msgBuf), "--- History Log (Queue) ---\n");
                        Node *temp = historyQueue.head;
                        int step = 1;
                        while (temp != NULL) {
                            char line[100];
                            snprintf(line, sizeof(line), "Step %d: Row %d, Col %d -> Value %d\n",
                                     step, temp->m.r + 1, temp->m.c + 1, temp->m.val);
                            strncat(msgBuf, line, sizeof(msgBuf) - strlen(msgBuf) - 1);
                            temp = temp->next;
                            step++;
                        }
                        if (step == 1) {
                            strncat(msgBuf, "No moves played yet.", sizeof(msgBuf) - strlen(msgBuf) - 1);
                        }
                        msgIsError = 0;
                    }
                    // คำสั่งรีเซ็ตเพื่อเริ่มกระดานใหม่
                    else if (strcmp(subCmd, "reset") == 0) {
                        initQueue(&historyQueue);
                        if (currentN == 4) makeNewPuzzle4x4(board, startBoard);
                        else makeNewPuzzle(cfg.emptyCells);
                        endTime = time(NULL) + cfg.timeLimit;
                        mistakes = 0;
                        for (int i = 0; i < 9; i++)
                            for (int j = 0; j < 9; j++) rewarded[i][j] = 0;
                        hlRow = -1; hlCol = -1;
                        snprintf(msgBuf, sizeof(msgBuf), "[+] Fresh new board is ready! (timer and mistakes reset)");
                        msgIsError = 0;
                    }
                    // คำสั่งเน้นสีแถวและคอลัมน์ (Highlight)
                    else if (strcmp(subCmd, "hl") == 0 || strcmp(subCmd, "highlight") == 0) {
                        int parsed = sscanf(inputBuf, "%*s %d %d", &r, &c);
                        if (parsed == 2 && r >= 1 && r <= currentN && c >= 1 && c <= currentN) {
                            hlRow = r - 1;
                            hlCol = c - 1;
                            snprintf(msgBuf, sizeof(msgBuf), "[+] Spotting row %d, col %d for you.", r, c);
                            msgIsError = 0;
                        } else {
                            snprintf(msgBuf, sizeof(msgBuf), "[!] Dude, use numbers 1 to %d or function", currentN);
                            msgIsError = 1;
                        }
                    }
                    // การลงตัวเลขบนกระดาน (รูปแบบ: แถว คอลัมน์ ตัวเลข)
                    else if (sscanf(inputBuf, "%d %d %d", &r, &c, &val) == 3) {
                        int rowIdx = r - 1;
                        int colIdx = c - 1;

                        // ตรวจสอบว่าพิกัดหรือค่าเกินขอบเขตหรือไม่
                        if (rowIdx < 0 || rowIdx >= currentN || colIdx < 0 || colIdx >= currentN || val < 1 || val > currentN) {
                            snprintf(msgBuf, sizeof(msgBuf), "[!] Dude, use numbers 1 to %d or function", currentN);
                            msgIsError = 1;
                        }
                        // ห้ามแก้ไขตัวเลขตั้งต้นของโจทย์
                        else if (startBoard[rowIdx][colIdx] != 0) {
                            snprintf(msgBuf, sizeof(msgBuf), "[!] Ah Hell Nah! That's a starting number, you can't touch it!");
                            msgIsError = 1;
                        }
                        // มีตัวเลขนั้นอยู่อยู่แล้ว
                        else if (board[rowIdx][colIdx] == val) {
                            snprintf(msgBuf, sizeof(msgBuf), "[!] That number is already there.");
                            msgIsError = 1;
                        }
                        // ตัวเลขผิดกติกาพื้นฐาน (ซ้ำในแถว/คอลัมน์/กล่อง)
                        else if (!isValidMove(board, rowIdx, colIdx, val, currentN)) {
                            mistakes++;
                            endTime -= cfg.penalty; // หักเวลาเพิ่ม
                            snprintf(msgBuf, sizeof(msgBuf), "[!] That doesn't fit there, try another one.\n[-] Wrong answer!  -%ds  (Mistakes: %d/%d)",
                                     cfg.penalty, mistakes, cfg.maxMistakes);
                            msgIsError = 1;
                        }
                        // ตัวเลขผิดคำตอบเฉลย
                        else if (val != solution[rowIdx][colIdx]) {
                            mistakes++;
                            endTime -= cfg.penalty; // หักเวลาเพิ่ม
                            snprintf(msgBuf, sizeof(msgBuf), "[!] It follows the rules, but it's not the right answer for this cell.\n[-] Wrong answer!  -%ds  (Mistakes: %d/%d)",
                                     cfg.penalty, mistakes, cfg.maxMistakes);
                            msgIsError = 1;
                        }
                        // กรอกตัวเลขถูกต้อง
                        else {
                            enqueue(&historyQueue, rowIdx, colIdx, val); // บันทึกลงคิวประวัติ
                            board[rowIdx][colIdx] = val;
                            hlRow = -1;
                            hlCol = -1;

                            // ให้เวลาโบนัสหากเพิ่งเติมถูกในช่องนี้เป็นครั้งแรก
                            if (!rewarded[rowIdx][colIdx]) {
                                rewarded[rowIdx][colIdx] = 1;
                                endTime += cfg.bonus;
                                snprintf(msgBuf, sizeof(msgBuf), "[+] Nice move dude  (+%ds)", cfg.bonus);
                            } else {
                                snprintf(msgBuf, sizeof(msgBuf), "[+] Nice move dude  (no bonus, this cell was already rewarded)");
                            }
                            msgIsError = 0;

                            // ตรวจสอบว่าเติมครบทุกช่องแล้ว ชนะเกมหรือไม่
                            if (isBoardFull(board, currentN)) {
                                int rem = remainingSeconds(endTime);
                                char winMsg[200];
                                snprintf(winMsg, sizeof(winMsg), "*** You solved it! Congrats! *** (Time left %02d:%02d, mistakes %d/%d)",
                                         rem / 60, rem % 60, mistakes, cfg.maxMistakes);
                                renderScreen(&cfg, endTime, mistakes, hlRow, hlCol, "", winMsg, 0);
                                break;
                            }
                        }

                        // ตรวจสอบว่าตอบผิดเกินโควตาที่กำหนดหรือไม่
                        if (mistakes > cfg.maxMistakes) {
                            renderScreen(&cfg, endTime, mistakes, hlRow, hlCol, "", "*** GAME OVER: too many mistakes! You lose. ***", 1);
                            break;
                        }
                    }
                    else {
                        snprintf(msgBuf, sizeof(msgBuf), "[!] Dude, use numbers 1 to %d or function", currentN);
                        msgIsError = 1;
                    }
                }

                inputBuf[0] = '\0';
                inputLen = 0;
                renderScreen(&cfg, endTime, mistakes, hlRow, hlCol, inputBuf, msgBuf, msgIsError);
                lastTime = time(NULL);
            }
            // รองรับการกด Backspace เพื่อลบตัวอักษรที่พิมพ์
            else if (ch == 8 || ch == 127 || ch == '\b') {
                if (inputLen > 0) {
                    inputLen--;
                    inputBuf[inputLen] = '\0';
                    renderScreen(&cfg, endTime, mistakes, hlRow, hlCol, inputBuf, msgBuf, msgIsError);
                }
            }
            // รับตัวอักษรทั่วไปที่พิมพ์ลงในข้อความอินพุต
            else if (ch >= 32 && ch <= 126) {
                if (inputLen < (int)sizeof(inputBuf) - 1) {
                    inputBuf[inputLen++] = (char)ch;
                    inputBuf[inputLen] = '\0';
                    printf("%c", ch);
                    fflush(stdout);
                }
            }
        }

        sleep_ms(20); // หน่วงเวลาเล็กน้อยเพื่อลดการใช้งาน CPU
    }
}