/* ============================================================
   benchmark.c  --  วัดเวลาจริงของทุก ADT เทียบกับ Big-O ที่วิเคราะห์ไว้
   ต่างจาก test_adt.c ตรงที่ไฟล์นี้ "ไม่เช็คว่าถูกไหม" แต่เช็ค "เร็วแค่ไหน"
   ใช้คู่กับหัวข้อ Time Complexity Analysis ในรายงาน (เกณฑ์ 15 คะแนน)

   วิธีคอมไพล์ (แยกจากโปรแกรมหลัก ไม่ต้องผ่าน main.o):
     gcc -std=c99 -O2 -Wall -Wextra benchmark.c graph.o pqueue.o stack.o \
         hashtable.o avltree.o data.o -o benchmark -lm
     ./benchmark
   ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* บน Windows: บังคับให้ console รับ/แสดงผลเป็น UTF-8 เสมอ
   เหตุผลเดียวกับใน main.c -- ไม่พึ่งคำสั่ง chcp ของผู้ใช้ */
#ifdef _WIN32
#include <windows.h>
#endif

#include "types.h"
#include "data.h"
#include "graph.h"
#include "pqueue.h"
#include "stack.h"
#include "hashtable.h"
#include "avltree.h"

/* คืนเวลาที่ผ่านไปเป็นมิลลิวินาที นับจาก t0 ถึงตอนนี้ */
static double ms_since(clock_t t0) {
    return 1000.0 * (double)(clock() - t0) / CLOCKS_PER_SEC;
}

static void print_header(const char *title) {
    printf("\n==================================================\n");
    printf("  %s\n", title);
    printf("==================================================\n");
}

/* ------------------------------------------------------------
   1) Hash Table vs AVL Tree  --  ค้นหา
   ทฤษฎี: Hash O(1) เฉลี่ย , AVL O(log n) เสมอ
   ------------------------------------------------------------ */
static void bench_hash_avl(int n, int rounds) {
    HashTable *ht  = ht_create();
    AVLTree   *avl = avl_create();
    Booking    marker;
    char       key[MAX_ID_LEN];
    clock_t    t0;
    double     t_insert_ht, t_insert_avl, t_search_ht, t_search_avl;

    /* --- insert --- */
    t0 = clock();
    { int i; for (i = 0; i < n; i++) {
        snprintf(key, sizeof key, "BK%06d", i);
        ht_insert(ht, key, &marker);
    } }
    t_insert_ht = ms_since(t0);

    t0 = clock();
    { int i; for (i = 0; i < n; i++) {
        snprintf(key, sizeof key, "BK%06d", i);
        avl_insert(avl, key, &marker);
    } }
    t_insert_avl = ms_since(t0);

    /* --- search (สุ่มคีย์ที่มีอยู่จริง rounds ครั้ง) --- */
    t0 = clock();
    { int i; for (i = 0; i < rounds; i++) {
        snprintf(key, sizeof key, "BK%06d", i % n);
        ht_search(ht, key);
    } }
    t_search_ht = ms_since(t0);

    t0 = clock();
    { int i; for (i = 0; i < rounds; i++) {
        snprintf(key, sizeof key, "BK%06d", i % n);
        avl_search(avl, key);
    } }
    t_search_avl = ms_since(t0);

    printf("  n=%6d | load factor=%5.2f | insert: Hash %8.3f ms, AVL %8.3f ms"
           " | search x%d: Hash %8.3f ms, AVL %8.3f ms\n",
           n, (double)n / HASH_SIZE, t_insert_ht, t_insert_avl,
           rounds, t_search_ht, t_search_avl);

    ht_destroy(ht);
    avl_destroy(avl);
}

/* ------------------------------------------------------------
   2) Priority Queue (Min-Heap)  --  push แล้ว pop ออกให้หมด
   ทฤษฎี: push/pop เป็น O(log n) ต่อครั้ง รวม n ครั้ง = O(n log n)
   ------------------------------------------------------------ */
static void bench_pqueue(int n) {
    PriorityQueue *pq = pq_create(64);
    static int marker;
    clock_t t0;
    double  t_push, t_pop;

    srand(42);
    t0 = clock();
    { int i; for (i = 0; i < n; i++) {
        pq_push(pq, rand() % 1000000, &marker);
    } }
    t_push = ms_since(t0);

    t0 = clock();
    long p;
    while (!pq_is_empty(pq)) pq_pop(pq, &p);
    t_pop = ms_since(t0);

    printf("  n=%7d | push (O(n log n) รวม): %8.3f ms | pop จนว่าง: %8.3f ms"
           " | เฉลี่ยต่อครั้ง push=%.5f ms\n",
           n, t_push, t_pop, t_push / n);

    pq_destroy(pq);
}

/* ------------------------------------------------------------
   3) Stack (LIFO)  --  push แล้ว pop ออกให้หมด
   ทฤษฎี: push/pop เป็น O(1) ต่อครั้งเสมอ ไม่ว่า n จะโตแค่ไหน
   ดังนั้นเวลาต่อครั้ง (เฉลี่ย) ควรเกือบคงที่ทุกขนาด n
   ------------------------------------------------------------ */
static void bench_stack(int n) {
    Stack  *s = stack_create();
    Action  a;
    Action  out;
    clock_t t0;
    double  t_push, t_pop;

    memset(&a, 0, sizeof a);
    a.type = ACT_BOOK;

    t0 = clock();
    { int i; for (i = 0; i < n; i++) stack_push(s, a); }
    t_push = ms_since(t0);

    t0 = clock();
    while (!stack_is_empty(s)) stack_pop(s, &out);
    t_pop = ms_since(t0);

    printf("  n=%7d | push รวม: %8.3f ms | pop รวม: %8.3f ms"
           " | เฉลี่ยต่อครั้ง push=%.6f ms (ควรเกือบคงที่ทุก n)\n",
           n, t_push, t_pop, t_push / n);

    stack_destroy(s);
}

/* ------------------------------------------------------------
   4) Graph + Dijkstra  --  หาเส้นทางสั้นสุดระหว่างทุกคู่ท่าเรือ
   ทฤษฎี: 1 ครั้ง = O((V+E) log V)  เพราะใช้ Priority Queue ช่วยเลือก node
   หมายเหตุ: MAX_PORTS = 64 ในโปรเจกต์นี้ จึงทดสอบ V ในช่วง 10/30/60
   (ไม่ใช่ 3 ขนาดแบบทวีคูณเหมือน ADT อื่น เพราะติดเพดานของระบบจริง
    จึงเสริมด้วยการรันซ้ำหลายรอบ (rounds) เพื่อให้เวลาที่วัดได้แม่นยำขึ้น)
   ------------------------------------------------------------ */
static Graph *build_ring_graph(int v, int extra_edges) {
    Graph *g = graph_create();
    char   name[MAX_NAME_LEN];

    { int i; for (i = 0; i < v; i++) {
        snprintf(name, sizeof name, "PORT_%03d", i);
        graph_add_port(g, name);
    } }
    /* ต่อเป็นวงแหวนก่อน ให้ทุก node เข้าถึงกันได้แน่นอน */
    { int i; for (i = 0; i < v; i++) {
        int to = (i + 1) % v;
        graph_add_route(g, i, to, i, BOAT_FERRY, 30 + (i % 20), 100, 40);
        graph_add_route(g, to, i, v + i, BOAT_FERRY, 30 + (i % 20), 100, 40);
    } }
    /* เพิ่มเส้นสุ่มให้ E > V แบบกราฟจริงที่มีทางลัด */
    srand(1);
    { int i; for (i = 0; i < extra_edges; i++) {
        int a = rand() % v, b = rand() % v;
        if (a == b) continue;
        graph_add_route(g, a, b, 10000 + i, BOAT_SPEEDBOAT, 10 + (i % 15), 300, 20);
    } }
    return g;
}

static void bench_graph(int v, int rounds) {
    int    extra_edges = v * 2;
    Graph *g = build_ring_graph(v, extra_edges);
    clock_t t0 = clock();
    int found = 0;

    srand(7);
    { int i; for (i = 0; i < rounds; i++) {
        int src = rand() % v, dst = rand() % v;
        PathResult r = graph_shortest_path(g, src, dst);
        if (r.found) found++;
    } }
    double t_total = ms_since(t0);

    printf("  V=%3d, E~%4d | Dijkstra x%d ครั้ง: %8.3f ms รวม"
           " | เฉลี่ยต่อครั้ง %7.4f ms | หาทางเจอ %d/%d\n",
           v, v + extra_edges, rounds, t_total, t_total / rounds, found, rounds);

    graph_destroy(g);
}

int main(void) {
#ifdef _WIN32
    /* แก้ปัญหาตัวอักษรไทยแสดงผลเพี้ยนบน Windows โดยไม่ต้องพึ่ง chcp ข้างนอก */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    printf("################################################\n");
    printf("   Benchmark: วัดเวลาจริงเทียบ Big-O ทุก ADT\n");
    printf("   (ค่าเวลาจริงจะต่างกันไปตามเครื่องที่รัน แต่ 'แนวโน้ม'\n");
    printf("    การเติบโตเทียบระหว่างขนาด ควรสอดคล้องกับ Big-O)\n");
    printf("################################################\n");

    /* ---------- 1) Hash vs AVL ---------- */
    print_header("[1] Hash Table (คาดหวัง O(1) เฉลี่ย) vs AVL Tree (O(log n))");
    bench_hash_avl(1000,   200000);
    bench_hash_avl(10000,  200000);
    bench_hash_avl(100000, 200000);
    printf("  --> อ่านผล: n เพิ่ม 10 เท่า ถ้า Hash เป็น O(1) จริง เวลา search ควร\n");
    printf("      แทบไม่เพิ่ม แต่ AVL (O(log n)) จะเพิ่มขึ้นทีละน้อยแบบลอการิทึม\n");

    /* ---------- 2) Priority Queue ---------- */
    print_header("[2] Priority Queue / Min-Heap (คาดหวัง O(log n) ต่อครั้ง)");
    bench_pqueue(1000);
    bench_pqueue(10000);
    bench_pqueue(100000);
    printf("  --> อ่านผล: n เพิ่ม 10 เท่า เวลาต่อครั้ง (push เฉลี่ย) ควรเพิ่มขึ้น\n");
    printf("      แค่เล็กน้อย (สัดส่วน log n) ไม่ใช่เพิ่มแบบเชิงเส้นตาม n\n");

    /* ---------- 3) Stack ---------- */
    print_header("[3] Stack / LIFO (คาดหวัง O(1) ต่อครั้ง เสมอ)");
    bench_stack(1000);
    bench_stack(10000);
    bench_stack(100000);
    printf("  --> อ่านผล: เวลาเฉลี่ยต่อครั้งควร 'เกือบคงที่' ไม่ขึ้นกับ n เลย\n");
    printf("      ถ้าเห็นเวลาต่อครั้งเพิ่มขึ้นชัดเจนตาม n แปลว่าผิดปกติ\n");

    /* ---------- 4) Graph + Dijkstra ---------- */
    print_header("[4] Graph + Dijkstra (คาดหวัง O((V+E) log V) ต่อครั้ง)");
    bench_graph(10, 20000);
    bench_graph(30, 20000);
    bench_graph(60, 20000);   /* MAX_PORTS = 64 ในโปรเจกต์นี้ */
    printf("  --> อ่านผล: V และ E โตขึ้น เวลาเฉลี่ยต่อครั้งควรโตตาม (V+E) log V\n");
    printf("      ไม่ใช่โตแบบก้าวกระโดด (ถ้าก้าวกระโดด อาจไม่ได้ใช้ Priority Queue จริง)\n");

    printf("\n================ จบการวัดผล ================\n");
    printf("นำตัวเลขชุดนี้ไปใส่ตาราง/กราฟในรายงาน หัวข้อ Time Complexity Analysis\n");
    return 0;
}
