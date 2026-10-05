#ifndef DISPLAY_H
#define DISPLAY_H

#include "puzzle.h"

// ฟังก์ชันแสดงผลตารางเกม Sudoku แบบปกติ
void printUniversalBoard(int b[N][N], int size);

// ฟังก์ชันแสดงผลตารางเกม Sudoku แบบเน้นไฮไลต์แถวและคอลัมน์ที่กำหนด
void printUniversalBoardHighlight(int b[N][N], int size, int hlRow, int hlCol);

#endif