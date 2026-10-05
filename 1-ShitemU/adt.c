#include "adt.h"

/* ---------------- Queue ---------------- */

// เริ่มต้นกำหนดให้ Queue อยู่ในสถานะว่าง (head และ tail เป็น NULL)
void initQueue(Queue *q) {
    q->head = NULL;
    q->tail = NULL;
}

// เพิ่มข้อมูลการเดินเข้าไปต่อท้ายคิว (Enqueue) เพื่อบันทึกประวัติตามลำดับเวลา
void enqueue(Queue *q, int r, int c, int val) {
    Node *newNode = (Node*)malloc(sizeof(Node));
    newNode->m.r = r;
    newNode->m.c = c;
    newNode->m.val = val;
    newNode->next = NULL;

    if (q->tail == NULL) {
        // หากคิวยังว่างอยู่ โหนดใหม่จะเป็นทั้งหัวคิวและท้ายคิว
        q->head = q->tail = newNode;
    } else {
        // ต่อโหนดใหม่เข้าที่ท้ายคิวเดิม และอัปเดต tail
        q->tail->next = newNode;
        q->tail = newNode;
    }
}