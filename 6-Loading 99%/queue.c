/* ============================================================
   queue.c  --  Queue (FIFO) แบบ linked list
   เก็บลำดับการจองตามเวลาที่เข้ามาก่อน-หลัง
   enqueue ต่อท้ายที่ rear, dequeue ดึงจาก front -> O(1) ทั้งคู่
   ============================================================ */
#include <stdlib.h>
#include "queue.h"

/* สร้างคิวเปล่า -- O(1) */
Queue *queue_create(void) {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    if (!q) return NULL;
    q->front = NULL;
    q->rear  = NULL;
    q->size  = 0;
    return q;
}

/* คืนหน่วยความจำทุกโหนด -- O(n)
   ไม่ free ตัว data เพราะเจ้าของข้อมูลคือผู้เรียกใช้ */
void queue_destroy(Queue *q) {
    QueueNode *cur;
    if (!q) return;
    cur = q->front;
    while (cur) {
        QueueNode *next = cur->next;
        free(cur);
        cur = next;
    }
    free(q);
}

/* ต่อท้ายคิว -- O(1) คืน 1 ถ้าสำเร็จ */
int queue_enqueue(Queue *q, void *data) {
    QueueNode *n;
    if (!q) return 0;
    n = (QueueNode *)malloc(sizeof(QueueNode));
    if (!n) return 0;

    n->data = data;
    n->next = NULL;

    if (q->rear) q->rear->next = n;   /* ต่อท้ายตัวเดิม */
    else         q->front      = n;   /* คิวว่างอยู่ ตัวนี้เป็นตัวแรก */
    q->rear = n;
    q->size++;
    return 1;
}

/* ดึงตัวหน้าสุดออก -- O(1) คืน NULL ถ้าคิวว่าง */
void *queue_dequeue(Queue *q) {
    QueueNode *n;
    void *data;
    if (!q || !q->front) return NULL;

    n    = q->front;
    data = n->data;
    q->front = n->next;
    if (!q->front) q->rear = NULL;    /* ดึงตัวสุดท้ายออกไปแล้ว */
    free(n);
    q->size--;
    return data;
}

/* ดูตัวหน้าสุดโดยไม่ดึงออก -- O(1) */
void *queue_peek(const Queue *q) {
    if (!q || !q->front) return NULL;
    return q->front->data;
}

int queue_is_empty(const Queue *q) { return (!q || q->size == 0); }  /* O(1) */
int queue_size(const Queue *q)     { return q ? q->size : 0; }       /* O(1) */
