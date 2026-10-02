/* ============================================================
   space_bench.c  --  วิเคราะห์ Space Complexity ของทุก ADT (หน่วยความจำ)
   วิธีทำ: ใช้ sizeof() ของ struct จริงในโปรเจกต์ คูณจำนวนข้อมูล n
           (เป็นตัวเลขที่คำนวณได้แน่นอน ไม่ขึ้นกับเครื่อง/ระบบปฏิบัติการมากนัก
            ยกเว้นขนาด pointer 4 หรือ 8 ไบต์ ซึ่งขึ้นกับว่า compile แบบ 32/64 บิต)
   หมายเหตุ: ไม่นับ overhead ภายในของ malloc (ประมาณ 8-16 ไบต์ต่อก้อน)
   คอมไพล์: gcc -std=c99 -Wall -Wextra space_bench.c booking.c graph.c pqueue.c \
            queue.c stack.c hashtable.c avltree.c data.c -o space_bench -lm
   ============================================================ */
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "types.h"
#include "graph.h"
#include "pqueue.h"
#include "stack.h"
#include "queue.h"
#include "hashtable.h"
#include "avltree.h"
#include "booking.h"

static void row(const char *name, const char *big_o, double per_item, double fixed, int n) {
    double total = fixed + per_item * n;
    printf("  %-22s %-9s n=%-7d : %12.0f ไบต์ = %9.2f KB\n",
           name, big_o, n, total, total / 1024.0);
}

/* นับจำนวนเส้นทาง (Edge) ทั้งหมดในกราฟ */
static int count_edges(const Graph *g) {
    int e = 0;
    { int i; for (i = 0; i < g->port_count; i++)
        { const Edge *x; for (x = g->ports[i].head; x; x = x->next) e++; } }
    return e;
}

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    printf("################################################\n");
    printf("  Space Complexity: หน่วยความจำของแต่ละ ADT\n");
    printf("################################################\n");

    printf("\n[1] ขนาดของ 1 หน่วยข้อมูล (sizeof จริงจากโค้ด)\n");
    printf("  Booking (1 รายการจอง)   : %4zu ไบต์\n", sizeof(Booking));
    printf("  HashEntry (1 โหนดใน hash): %4zu ไบต์\n", sizeof(HashEntry));
    printf("  AVLNode  (1 โหนดต้นไม้)  : %4zu ไบต์\n", sizeof(AVLNode));
    printf("  HeapNode (1 ช่องใน heap) : %4zu ไบต์\n", sizeof(HeapNode));
    printf("  QueueNode                : %4zu ไบต์\n", sizeof(QueueNode));
    printf("  StackNode (มี Booking snapshot): %4zu ไบต์\n", sizeof(StackNode));
    printf("  Edge (1 เส้นทางเดินเรือ) : %4zu ไบต์  (มี bitmap ที่นั่งทุกวัน x ทุกรอบ)\n", sizeof(Edge));
    printf("  Graph (ท่าเรือ %d ท่า คงที่) : %4zu ไบต์\n", MAX_PORTS, sizeof(Graph));
    printf("  HashTable (ตาราง %d ช่อง คงที่): %3zu ไบต์\n", HASH_SIZE, sizeof(HashTable));

    int sizes[3] = {1000, 10000, 100000};

    printf("\n[2] รวมหน่วยความจำเมื่อเก็บข้อมูล n รายการ\n");
    { int i; for (i = 0; i < 3; i++) {
        int n = sizes[i];
        printf("\n  --- n = %d ---\n", n);
        /* Hash: ตารางคงที่ HASH_SIZE ช่อง (~800 KB) + 1 โหนด/รายการ  => O(n + HASH_SIZE) */
        row("Hash Table", "O(n)", (double)sizeof(HashEntry), (double)sizeof(HashTable), n);
        /* AVL: 1 โหนด/รายการ => O(n) พอดีกับข้อมูล ไม่มีช่องว่างเหลือ */
        row("AVL Tree", "O(n)", (double)sizeof(AVLNode), (double)sizeof(AVLTree), n);
        /* Heap: array ขยายเท่าตัว => ใช้สูงสุด ~2 เท่าของที่ต้องใช้จริง แต่ยังเป็น O(n) */
        row("Priority Queue (min)", "O(n)", (double)sizeof(HeapNode), (double)sizeof(PriorityQueue), n);
        row("Priority Queue (max)", "O(n)", 2.0 * sizeof(HeapNode), (double)sizeof(PriorityQueue), n);
        row("Queue (FIFO)", "O(n)", (double)sizeof(QueueNode), (double)sizeof(Queue), n);
        /* Stack: เก็บ snapshot ของ Booking ทั้งก้อนต่อ 1 action => แพงที่สุดต่อ 1 รายการ */
        row("Stack (Undo history)", "O(n)", (double)sizeof(StackNode), (double)sizeof(Stack), n);
    } }

    printf("\n[3] Graph: O(V + E) ขึ้นกับจำนวนท่าเรือและเส้นทาง ไม่ขึ้นกับจำนวนการจอง\n");
    {
        System *sys = system_create();
        system_load_sample_data(sys);
        int v = sys->graph->port_count;
        int e = count_edges(sys->graph);
        printf("  ข้อมูลตัวอย่างในโปรแกรม: V = %d ท่าเรือ, E = %d เส้นทาง\n", v, e);
        /* ใช้ %lu + แปลงเป็น unsigned long แทน %zu เพราะ printf ของ Dev-C++ รุ่นเก่าไม่รู้จัก %zu */
        printf("  ส่วนที่ใช้จริงของ Edge  : %d x %lu = %lu ไบต์ (%.2f KB)\n",
               e, (unsigned long)sizeof(Edge), (unsigned long)((size_t)e * sizeof(Edge)),
               (double)e * sizeof(Edge) / 1024.0);
        printf("  ตาราง Port คงที่ (จองไว้ %d ช่อง): %lu ไบต์ (%.2f KB)\n",
               MAX_PORTS, (unsigned long)sizeof(Graph), sizeof(Graph) / 1024.0);
        system_destroy(sys);
    }

    printf("\n[4] ข้อสังเกตสำหรับรายงาน\n");
    printf("  - ทุก ADT เป็น O(n) ตามจำนวนข้อมูล ยกเว้น Graph ที่เป็น O(V+E)\n");
    printf("  - Hash Table และ Graph มีส่วน \"คงที่\" ที่จองไว้ล่วงหน้า แม้ข้อมูลน้อย\n");
    printf("  - Stack แพงสุดต่อ 1 รายการ เพราะเก็บ Booking ทั้งก้อนเป็น snapshot\n");
    printf("  - Priority Queue ใช้ array ที่ขยายเท่าตัว จึงอาจเหลือช่องว่างได้ถึงครึ่งหนึ่ง\n");
    printf("  - Booking ตัวจริงเก็บที่ System เพียงที่เดียว (sys->all) ADT อื่นเก็บแค่ pointer\n");
    printf("    จึงไม่มีการสำเนา Booking ซ้ำซ้อนใน Hash/AVL/Queue/Heap\n");
    return 0;
}
