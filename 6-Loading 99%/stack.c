/* ============================================================
   stack.c  --  Stack (LIFO) สำหรับ Undo / Rollback
   push/pop ที่ top -> O(1) ทั้งคู่
   เก็บ Action ทั้งก้อน (ไม่ใช่ pointer) จึงไม่ต้องกังวลว่าข้อมูลต้นทางจะหาย
   ============================================================ */
#include <stdlib.h>
#include "stack.h"

/* สร้าง stack เปล่า -- O(1) */
Stack *stack_create(void) {
    Stack *s = (Stack *)malloc(sizeof(Stack));
    if (!s) return NULL;
    s->top  = NULL;
    s->size = 0;
    return s;
}

/* คืนหน่วยความจำทุกโหนด -- O(n) */
void stack_destroy(Stack *s) {
    StackNode *cur;
    if (!s) return;
    cur = s->top;
    while (cur) {
        StackNode *next = cur->next;
        free(cur);
        cur = next;
    }
    free(s);
}

/* ใส่ action เข้า stack -- O(1) */
int stack_push(Stack *s, Action a) {
    StackNode *n;
    if (!s) return 0;
    n = (StackNode *)malloc(sizeof(StackNode));
    if (!n) return 0;
    n->action = a;          /* คัดลอกทั้งโครงสร้าง */
    n->next   = s->top;
    s->top    = n;
    s->size++;
    return 1;
}

/* ดึง action ล่าสุดออก -- O(1) คืน 1 ถ้าสำเร็จ */
int stack_pop(Stack *s, Action *out) {
    StackNode *n;
    if (!s || !s->top) return 0;
    n = s->top;
    if (out) *out = n->action;
    s->top = n->next;
    free(n);
    s->size--;
    return 1;
}

/* ดู action ล่าสุดโดยไม่ดึงออก -- O(1) */
int stack_peek(const Stack *s, Action *out) {
    if (!s || !s->top) return 0;
    if (out) *out = s->top->action;
    return 1;
}

int stack_is_empty(const Stack *s) { return (!s || s->size == 0); }  /* O(1) */
