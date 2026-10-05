#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "game.h"

int main() {
    // กำหนด seed สำหรับสุ่มตัวเลขตามเวลาปัจจุบัน
    srand(time(NULL));

    char input[100];
    int choice = -1;

    // วนลูปแสดงเมนูหลักของโปรแกรม
    while (1) {
        printf("\033[31m=======================================\033[0m\n");
        printf("\033[36m      ShitemU Sudoku (4x4 & 9x9)      \033[0m\n");
        printf("\033[31m=======================================\033[0m\n");
        printf(" Menu:\n");
        printf(" - \033[32m1\033[0m : Play game\n");
        printf(" - \033[31m0\033[0m : Exit\n");
        printf("=======================================\n");
        printf("Select option (1 or 0) > ");

        // รับค่าตัวเลือกเมนูจากผู้ใช้
        if (fgets(input, sizeof(input), stdin) == NULL) break;
        if (sscanf(input, "%d", &choice) != 1) {
            printf("\n[!] Please enter a valid number (1 or 0).\n\n");
            continue;
        }

        // ตัวเลือก 0: ออกจากโปรแกรม
        if (choice == 0) {
            printf("\nSee ya!\n");
            break;
        }
        // ตัวเลือก 1: เริ่มเล่นเกม
        else if (choice == 1) {
            playGame();
        }
        // กรณีพิมพ์ตัวเลขอุปกรณ์อื่นที่ไม่ใช่ 1 หรือ 0
        else {
            printf("\n[!] Invalid choice! Choose 1 to play or 0 to exit.\n\n");
        }
    }

    return 0;
}