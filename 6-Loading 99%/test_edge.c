/* ============================================================
   test_edge.c  --  ชุดทดสอบ "กรณีขอบ" (Edge Cases) ทุก ADT
   ต่างจาก test_adt.c ตรงที่ test_adt.c เน้นทดสอบพฤติกรรมทั่วไปด้วยข้อมูล
   จำนวนมาก ส่วนไฟล์นี้เน้นเฉพาะ "ขอบ" ตามเกณฑ์ให้คะแนน:
     - โครงสร้างข้อมูลว่าง (empty)
     - โครงสร้างข้อมูลมี 1 รายการ (single data)
   ครอบคลุม 2 ระดับ:
     [ส่วน A] ADT ดิบ (เรียกใช้ Queue/Stack/PQueue/Hash/AVL/Graph ตรงๆ)
     [ส่วน B/C] ผ่านชั้น booking.c จริง (จอง/ค้นหา/ยกเลิก/Undo ผ่าน System)
              รวมถึงเช็คว่า Undo คืน "ที่นั่ง" ถูกต้อง ไม่ซ้ำกัน
   ============================================================ */
#include <stdio.h>
#include <string.h>

/* บน Windows: บังคับ console ให้แสดงผลเป็น UTF-8 (เหมือน main.c)
   ถ้าไม่ใส่ ตัวอักษรไทยจะเพี้ยนเป็นขยะ */
#ifdef _WIN32
#include <windows.h>
#endif

#include "queue.h"
#include "stack.h"
#include "pqueue.h"
#include "hashtable.h"
#include "avltree.h"
#include "graph.h"
#include "booking.h"
#include "data.h"

static int p = 0, f = 0;
#define T(c, m) do { if (c) p++; else { f++; printf("  [FAIL] %s\n", m); } } while (0)

/* ------------------------------------------------------------
   ส่วน A: ADT ดิบ -- เหมือนไฟล์เดิมที่มีอยู่ + เติมจุดที่ขาด
   ------------------------------------------------------------ */
static void section_a_raw_adt(void) {
    printf("== [A] กรณีขอบ: โครงสร้างว่าง ==\n");
    Queue *q = queue_create();
    T(queue_dequeue(q) == NULL, "dequeue คิวว่าง");
    T(queue_peek(q) == NULL,    "peek คิวว่าง");
    T(queue_size(q) == 0,       "size คิวว่าง");

    Stack *s = stack_create();
    Action a;
    T(stack_pop(s, &a) == 0,  "pop stack ว่าง");
    T(stack_peek(s, &a) == 0, "peek stack ว่าง");
    T(stack_is_empty(s) == 1, "stack ใหม่ต้องว่าง");   /* เติม: เช็คตอนสร้างใหม่ */

    PriorityQueue *pq = pq_create(4);
    long pr;
    T(pq_is_empty(pq) == 1,      "heap ใหม่ต้องว่าง");   /* เติม: เดิมเช็คแค่ตอนกลับมาว่าง ไม่เช็คตอนสร้างใหม่ */
    T(pq_pop(pq, &pr) == NULL,   "pop heap ว่าง");
    T(pq_peek(pq, &pr) == NULL,  "peek heap ว่าง");

    HashTable *ht = ht_create();
    T(ht_search(ht, "BK000001") == NULL, "ค้น hash ว่าง");
    T(ht_delete(ht, "BK000001") == 0,    "ลบจาก hash ว่าง");

    AVLTree *t = avl_create();
    T(avl_search(t, "BK000001") == NULL, "ค้น AVL ว่าง");
    T(avl_delete(t, "BK000001") == 0,    "ลบจาก AVL ว่าง");

    printf("== [A] กรณีขอบ: ข้อมูลเดียว ==\n");
    Booking d;
    memset(&d, 0, sizeof d);
    queue_enqueue(q, &d);
    T(queue_dequeue(q) == &d,  "คิว 1 ตัว เข้า-ออก");
    T(queue_dequeue(q) == NULL, "คิวกลับมาว่าง");

    pq_push(pq, 5, &d);
    T(pq_pop(pq, &pr) == &d,  "heap 1 ตัว");
    T(pq_is_empty(pq),        "heap กลับมาว่าง");

    ht_insert(ht, "BK000001", &d);
    T(ht_search(ht, "BK000001") == &d, "hash 1 ตัว");
    T(ht_delete(ht, "BK000001") == 1,  "ลบตัวเดียวออก");
    T(ht_search(ht, "BK000001") == NULL, "ลบแล้วค้นไม่เจอ");

    avl_insert(t, "BK000001", &d);
    T(avl_search(t, "BK000001") == &d, "AVL 1 ตัว");
    T(avl_delete(t, "BK000001") == 1,  "ลบราก AVL ตัวเดียว");
    T(avl_search(t, "BK000001") == NULL, "AVL ว่างหลังลบ");

    printf("== [A] กรณีขอบ: กราฟ ==\n");
    /* เติม: กราฟที่ไม่มี node เลย (ว่างสุดๆ) */
    Graph *empty_g = graph_create();
    PathResult er = graph_shortest_path(empty_g, 0, 1);
    T(er.found == 0, "กราฟไม่มี node เลย -> หาทางไม่เจอ ไม่ crash");
    T(graph_find_port(empty_g, "อะไรก็ได้") == -1, "กราฟว่าง ค้นชื่อท่าไม่เจอ");
    graph_destroy(empty_g);

    Graph *g = graph_create();
    int A = graph_add_port(g, "A"), B = graph_add_port(g, "B");
    PathResult r = graph_shortest_path(g, A, A);
    T(r.found && r.total_time == 0, "ต้นทาง=ปลายทาง");
    r = graph_shortest_path(g, A, B);
    T(r.found == 0, "ไปไม่ถึง (ไม่มี edge)");
    r = graph_shortest_path(g, -1, 99);
    T(r.found == 0, "port id ไม่ถูกต้อง");
    T(graph_find_port(g, "ไม่มีท่านี้") == -1, "ค้นชื่อท่าที่ไม่มี");

    queue_destroy(q); stack_destroy(s); pq_destroy(pq);
    ht_destroy(ht); avl_destroy(t); graph_destroy(g);
}

/* ------------------------------------------------------------
   ส่วน B: ผ่านชั้น booking.c จริง (System) -- ที่ section A ยังไม่ครอบคลุม
   ------------------------------------------------------------ */
static void section_b_empty_system(void) {
    printf("\n== [B] กรณีขอบ: System ว่าง (ยังไม่มีการจองเลย) ==\n");
    System *sys = system_create();
    system_load_sample_data(sys);   /* มีกราฟ/ตารางเรือ แต่ยังไม่มี booking ใดๆ */

    T(booking_find(sys, "BK000001") == NULL, "ค้น booking ตอนยังไม่มีใครจอง");
    T(booking_cancel(sys, "BK000001") == 0,  "ยกเลิก booking ที่ไม่มีอยู่จริง");
    T(booking_undo(sys) == 0,                "undo ตอน history ว่าง");
    T(booking_call_next(sys) == NULL,        "เรียกคิวขึ้นเรือตอนไม่มีใครรอ");
    T(booking_purge_cancelled(sys) == 0,     "purge ตอนไม่มีอะไรให้ล้าง");

    system_destroy(sys);
}

static void section_b_single_booking(void) {
    printf("\n== [B] กรณีขอบ: จองสำเร็จ 1 รายการ (ผ่านชั้น booking.c จริง) ==\n");
    System *sys = system_create();
    system_load_sample_data(sys);

    int from = graph_find_port(sys->graph, "ท่าเรือน้ำลึกภูเก็ต");
    int to   = graph_find_port(sys->graph, "เกาะพีพี");
    T(from >= 0 && to >= 0, "หาท่าเรือต้นทาง/ปลายทางเจอจาก sample data");

    Booking *bk = booking_create(sys, "Edge Case Tester", from, to,
                                  PRIO_NORMAL, BOAT_FERRY, /*day=*/0, /*slot=*/0);
    T(bk != NULL, "จองตั๋ว 1 ใบสำเร็จ");
    T(booking_find(sys, bk->booking_id) == bk, "ค้นหาตั๋วที่เพิ่งจองเจอ (Hash Table)");

    /* จองซ้ำด้วยพารามิเตอร์ผิด ต้อง fail อย่างสุภาพ ไม่ crash */
    T(booking_create(sys, "Bad", from, to, PRIO_NORMAL, BOAT_FERRY, 99, 0) == NULL,
      "จองวันเดินทางไม่ถูกต้อง (day=99) ต้องถูกปฏิเสธ");
    T(booking_create(sys, "Bad", from, to, PRIO_NORMAL, BOAT_FERRY, 0, 999) == NULL,
      "จองรอบเวลาไม่ถูกต้อง (slot=999) ต้องถูกปฏิเสธ");

    int seat_before = bk->seat_no;

    T(booking_cancel(sys, bk->booking_id) == 1, "ยกเลิกตั๋วใบเดียวสำเร็จ");
    T(booking_find(sys, bk->booking_id) == NULL || bk->status != STATUS_CONFIRMED,
      "หลังยกเลิก สถานะ/ดัชนีต้องไม่ใช่ยืนยันแล้วอีกต่อไป");

    /* หมายเหตุสำคัญ: booking_create() เองก็ push ACT_BOOK เข้า history ด้วย
       (ดู booking.c บรรทัด ~175) ไม่ใช่แค่ booking_cancel() ที่ push ACT_CANCEL
       ดังนั้นหลังจอง 1 ครั้ง + ยกเลิก 1 ครั้ง stack จะมี 2 actions ไม่ใช่ 1
       Undo ครั้งที่ 1 = ย้อนการ "ยกเลิก" (คืนตั๋วกลับมา)
       Undo ครั้งที่ 2 = ย้อนการ "จอง" ครั้งแรกสุด (ถอนตั๋วทิ้งไปเลย) */
    T(booking_undo(sys) == 1, "undo #1: คืนรายการที่เพิ่งยกเลิก");
    T(bk->seat_no == seat_before, "undo #1: ที่นั่งต้องกลับมาเป็นเลขเดิม (ไม่ใช่ที่นั่งใหม่)");
    T(booking_find(sys, bk->booking_id) == bk, "undo #1: ค้นหาผ่าน Hash Table เจอเหมือนเดิม");

    T(booking_undo(sys) == 1, "undo #2: ย้อนการจองครั้งแรกสุด (ตาม LIFO ต้องทำได้ ไม่ใช่ 0)");
    T(booking_find(sys, bk->booking_id) == NULL, "undo #2: หลังถอนการจองแล้ว ค้นหาต้องไม่เจอ");

    T(booking_undo(sys) == 0, "undo #3: history ว่างจริงแล้ว ต้องคืน 0 และไม่ crash");

    system_destroy(sys);
}

/* ------------------------------------------------------------
   ส่วน C: Undo คืนที่นั่งไม่ซ้ำกัน -- บั๊กที่พบบ่อยที่สุดของฟีเจอร์นี้
   จำลอง A, B, C จอง 3 ที่ -> ยกเลิก A แล้ว C -> undo (ต้องคืนแค่ C ตาม LIFO)
   -> จอง D ใหม่ -> D ต้องไม่ได้ที่นั่งซ้ำกับ B (ที่ยังไม่ถูกยกเลิก)
   ------------------------------------------------------------ */
static void section_c_undo_seat_no_duplicate(void) {
    printf("\n== [C] กรณีขอบ: Undo คืนที่นั่งต้องไม่ซ้ำกับที่นั่งที่ยังจองอยู่ ==\n");
    System *sys = system_create();
    system_load_sample_data(sys);
    int from = graph_find_port(sys->graph, "ท่าเรือน้ำลึกภูเก็ต");
    int to   = graph_find_port(sys->graph, "เกาะพีพี");

    Booking *A = booking_create(sys, "A", from, to, PRIO_NORMAL, BOAT_FERRY, 0, 0);
    Booking *B = booking_create(sys, "B", from, to, PRIO_NORMAL, BOAT_FERRY, 0, 0);
    Booking *C = booking_create(sys, "C", from, to, PRIO_NORMAL, BOAT_FERRY, 0, 0);
    T(A && B && C, "จองต่อกัน 3 ใบสำเร็จ (ที่นั่งต้องไม่ซ้ำกันตั้งแต่แรก)");
    T(A->seat_no != B->seat_no && B->seat_no != C->seat_no && A->seat_no != C->seat_no,
      "ที่นั่งของ A/B/C ต้องไม่ซ้ำกันเลย");

    int c_seat = C->seat_no;
    int b_seat = B->seat_no;

    booking_cancel(sys, A->booking_id);   /* ที่ 1 ว่าง */
    booking_cancel(sys, C->booking_id);   /* ที่ 3 ว่าง */
    booking_undo(sys);                    /* LIFO: ต้องคืนแค่ C เท่านั้น (ตัวล่าสุด) */

    T(C->seat_no == c_seat, "หลัง undo ที่นั่งของ C ต้องกลับมาเป็นเลขเดิม");
    T(B->seat_no == b_seat, "การ undo ต้องไม่ไปกระทบที่นั่งของ B ที่ไม่เกี่ยวข้อง");

    Booking *D = booking_create(sys, "D", from, to, PRIO_NORMAL, BOAT_FERRY, 0, 0);
    T(D != NULL, "จองคนใหม่ D ต่อได้หลัง undo");
    T(D->seat_no != B->seat_no, "ที่นั่งของ D ต้องไม่ซ้ำกับ B ที่ยังจองอยู่จริง");

    system_destroy(sys);
}

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    section_a_raw_adt();
    section_b_empty_system();
    section_b_single_booking();
    section_c_undo_seat_no_duplicate();

    printf("\n==================================\n");
    printf("ผ่าน %d / ล้มเหลว %d\n", p, f);
    printf("==================================\n");
    return f ? 1 : 0;
}
