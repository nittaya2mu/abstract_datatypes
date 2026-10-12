/* ============================================================
   queue.h  --  Queue (FIFO) แบบ linked list
   ใช้เก็บลำดับการจองตามเวลาที่เข้ามาก่อน-หลัง
   ============================================================ */
#ifndef QUEUE_H
#define QUEUE_H

typedef struct QueueNode {
    void             *data;
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *front;
    QueueNode *rear;
    int        size;
} Queue;

Queue *queue_create(void);                    /* O(1) */
void   queue_destroy(Queue *q);               /* O(n) */
int    queue_enqueue(Queue *q, void *data);   /* O(1) คืน 1 ถ้าสำเร็จ */
void  *queue_dequeue(Queue *q);               /* O(1) คืน NULL ถ้าว่าง */
void  *queue_peek(const Queue *q);            /* O(1) */
int    queue_is_empty(const Queue *q);        /* O(1) */
int    queue_size(const Queue *q);            /* O(1) */

#endif /* QUEUE_H */
