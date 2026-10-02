/* ============================================================
   stack.h  --  Stack (LIFO) สำหรับ Undo / Rollback
   เก็บ "ภาพก่อนหน้า" ของรายการที่ถูกแก้ ย้อนกลับได้ทีละขั้น
   ============================================================ */
#ifndef STACK_H
#define STACK_H

#include "types.h"

typedef enum {
    ACT_BOOK,             /* เพิ่งจอง -> undo = ยกเลิก        */
    ACT_CANCEL,           /* เพิ่งยกเลิก -> undo = คืนการจอง  */
    ACT_WAITLIST_PROMOTE  /* เลื่อนคนจากคิวสำรองขึ้นมา        */
} ActionType;

typedef struct {
    ActionType type;
    Booking    snapshot;   /* สถานะของรายการ "ก่อน" ทำ action นี้ */
    int        route_id;
} Action;

typedef struct StackNode {
    Action            action;
    struct StackNode *next;
} StackNode;

typedef struct {
    StackNode *top;
    int        size;
} Stack;

Stack *stack_create(void);                       /* O(1) */
void   stack_destroy(Stack *s);                  /* O(n) */
int    stack_push(Stack *s, Action a);           /* O(1) */
int    stack_pop(Stack *s, Action *out);         /* O(1) คืน 1 ถ้าสำเร็จ */
int    stack_peek(const Stack *s, Action *out);  /* O(1) */
int    stack_is_empty(const Stack *s);           /* O(1) */

#endif /* STACK_H */
