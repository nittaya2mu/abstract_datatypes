/* ============================================================
   pqueue.c  --  Priority Queue (Min-Heap) เต็มรูปแบบ
   ใช้ array แทน binary tree: parent ของ index i อยู่ที่ (i-1)/2
   ลูกซ้าย 2*i+1, ลูกขวา 2*i+2
   คุณสมบัติ Min-Heap: priority ของ parent <= priority ของลูกเสมอ
   ============================================================ */
#include <stdlib.h>
#include "pqueue.h"

/* O(1) */
PriorityQueue *pq_create(int capacity) {
    PriorityQueue *pq = (PriorityQueue *)malloc(sizeof(PriorityQueue));
    if (!pq) return NULL;
    pq->nodes = (HeapNode *)malloc(sizeof(HeapNode) * capacity);
    if (!pq->nodes) { free(pq); return NULL; }
    pq->size = 0;
    pq->capacity = capacity;
    return pq;
}

/* O(1) */
void pq_destroy(PriorityQueue *pq) {
    if (!pq) return;
    free(pq->nodes);
    free(pq);
}

static void swap_node(HeapNode *a, HeapNode *b) {
    HeapNode tmp = *a;
    *a = *b;
    *b = tmp;
}

/* O(log n) -- ดันขึ้นไปหา root จนกว่าจะไม่ผิดคุณสมบัติ heap */
static void sift_up(PriorityQueue *pq, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (pq->nodes[i].priority < pq->nodes[parent].priority) {
            swap_node(&pq->nodes[i], &pq->nodes[parent]);
            i = parent;
        } else {
            break;
        }
    }
}

/* O(log n) -- ดันลงจาก root จนกว่าจะไม่ผิดคุณสมบัติ heap */
static void sift_down(PriorityQueue *pq, int i) {
    while (1) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;

        if (left < pq->size && pq->nodes[left].priority < pq->nodes[smallest].priority)
            smallest = left;
        if (right < pq->size && pq->nodes[right].priority < pq->nodes[smallest].priority)
            smallest = right;

        if (smallest == i) break;
        swap_node(&pq->nodes[i], &pq->nodes[smallest]);
        i = smallest;
    }
}

/* O(log n) -- ถ้า array เต็ม ขยายเป็น 2 เท่า (amortized O(1) ต่อครั้ง) */
int pq_push(PriorityQueue *pq, long priority, void *data) {
    if (!pq) return 0;
    if (pq->size >= pq->capacity) {
        int new_cap = pq->capacity * 2;
        HeapNode *new_nodes = (HeapNode *)realloc(pq->nodes, sizeof(HeapNode) * new_cap);
        if (!new_nodes) return 0;   /* ขยายไม่ได้ ล้มเหลว */
        pq->nodes = new_nodes;
        pq->capacity = new_cap;
    }
    pq->nodes[pq->size].priority = priority;
    pq->nodes[pq->size].data = data;
    sift_up(pq, pq->size);
    pq->size++;
    return 1;
}

/* O(log n) */
void *pq_pop(PriorityQueue *pq, long *out_priority) {
    if (!pq || pq->size == 0) return NULL;

    void *data = pq->nodes[0].data;
    if (out_priority) *out_priority = pq->nodes[0].priority;

    pq->size--;
    pq->nodes[0] = pq->nodes[pq->size];   /* เอาตัวสุดท้ายมาไว้ที่ root */
    sift_down(pq, 0);

    return data;
}

/* O(1) */
void *pq_peek(const PriorityQueue *pq, long *out_priority) {
    if (!pq || pq->size == 0) return NULL;
    if (out_priority) *out_priority = pq->nodes[0].priority;
    return pq->nodes[0].data;
}

/* O(1) */
int pq_is_empty(const PriorityQueue *pq) {
    return (!pq || pq->size == 0);
}

/* O(1) */
int pq_size(const PriorityQueue *pq) {
    return pq ? pq->size : 0;
}
