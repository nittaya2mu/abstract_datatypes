#ifndef ADT_H
#define ADT_H

#include <stdlib.h>   // ใช้สำหรับ malloc / free / NULL

// โครงสร้างข้อมูลเก็บการเดินในแต่ละรอบ (แถว, คอลัมน์, ค่าตัวเลขที่กรอก)
typedef struct {
    int r, c, val;
} Move;

// โหนดของ Linked List สำหรับใช้งานใน Queue
typedef struct Node {
    Move m;            // ข้อมูลการเดิน (Move)
    struct Node *next; // ตัวชี้ไปยังโหนดถัดไป
} Node;

// โครงสร้างข้อมูล Queue (FIFO: First In, First Out)
typedef struct {
    Node *head;        // ตัวชี้ไปยังโหนดหัวคิว (ออกก่อน)
    Node *tail;        // ตัวชี้ไปยังโหนดท้ายคิว (เข้าทีหลัง)
} Queue;

// ฟังก์ชันจัดการ Queue
void initQueue(Queue *q);
void enqueue(Queue *q, int r, int c, int val);

#endif