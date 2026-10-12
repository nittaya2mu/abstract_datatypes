/* ============================================================
   pqueue.h  --  Priority Queue (Min-Heap)
   ใช้ 2 ที่:  1) คิวสำรอง Waitlist   2) Dijkstra ใน graph.c
   priority น้อย = ออกก่อน
   ============================================================ */
#ifndef PQUEUE_H
#define PQUEUE_H

typedef struct {
    long  priority;
    void *data;      /* ชี้ไปที่ Booking* หรือข้อมูลอื่น ตามผู้เรียกใช้ */
} HeapNode;

typedef struct {
    HeapNode *nodes;
    int       size;
    int       capacity;
} PriorityQueue;

PriorityQueue *pq_create(int capacity);          /* O(1)      */
void           pq_destroy(PriorityQueue *pq);    /* O(1)      */
int            pq_push(PriorityQueue *pq, long priority, void *data); /* O(log n) คืน 1 ถ้าสำเร็จ */
void          *pq_pop(PriorityQueue *pq, long *out_priority);         /* O(log n) คืน NULL ถ้าว่าง */
void          *pq_peek(const PriorityQueue *pq, long *out_priority);  /* O(1)      */
int            pq_is_empty(const PriorityQueue *pq);                  /* O(1)      */
int            pq_size(const PriorityQueue *pq);                      /* O(1)      */

#endif /* PQUEUE_H */
