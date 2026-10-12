/*
 * laundry_core.c — แกนของระบบจัดคิวเครื่องซักผ้าหอพัก (ไม่ขึ้นกับระบบปฏิบัติการ)
 * วิชา 01204212 แบบชนิดข้อมูลนามธรรมและการแก้ปัญหา
 *
 * โครงสร้างข้อมูล (เขียนเองทั้งหมด ไม่ใช้ qsort/bsearch ของ library)
 *   1. Priority Queue (Min Heap)  -> จัดคิวผู้ใช้ เรียงตาม (priority, เวลาเข้าคิว)
 *   2. Array                      -> เก็บข้อมูลเครื่องซักผ้า / ทะเบียนผู้ใช้
 *   3. Binary Search              -> ค้นหาเครื่อง / ค้นหาผู้ใช้ (ข้อมูลต้องเรียงแล้ว)
 *   4. Sorting                    -> Merge Sort (ชุดข้อมูลทดสอบ) / Insertion Sort (แสดงคิว)
 */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include "laundry_core.h"

Machine machines[NUM_MACHINES];
User   *users = NULL;             /* เก็บเรียงตาม id เสมอ */
int     userCount = 0;
static int userCap = 0;
MinHeap heap;
static long ticker = 0;           /* ตัวนับเวลาเข้าคิว */

void (*core_out)(const char *utf8) = NULL;

void core_printf(const char *fmt, ...)
{
    char buf[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (core_out) core_out(buf);
    else fputs(buf, stdout);
}

/* ============================ Min Heap ============================ */

/* x ต้องออกก่อน y หรือไม่: เทียบ priority ก่อน ถ้าเท่ากันเทียบเวลาเข้าคิว (FIFO) */
static int node_less(QNode x, QNode y)
{
    if (x.priority != y.priority) return x.priority < y.priority;
    return x.arrival < y.arrival;
}

static void heap_init(MinHeap *h, int cap)
{
    h->a = (QNode *)malloc(sizeof(QNode) * cap);
    h->size = 0;
    h->cap = cap;
}

/* O(log n) */
static void heap_push(MinHeap *h, QNode n)
{
    int i, p;
    if (h->size == h->cap) {
        h->cap *= 2;
        h->a = (QNode *)realloc(h->a, sizeof(QNode) * h->cap);
    }
    i = h->size++;
    while (i > 0) {                       /* sift-up */
        p = (i - 1) / 2;
        if (!node_less(n, h->a[p])) break;
        h->a[i] = h->a[p];
        i = p;
    }
    h->a[i] = n;
}

/* O(log n)  คืน 1 ถ้าสำเร็จ, 0 ถ้า heap ว่าง */
static int heap_pop(MinHeap *h, QNode *out)
{
    QNode last;
    int i = 0, l, r, m;
    if (h->size == 0) return 0;
    *out = h->a[0];
    last = h->a[--h->size];
    for (;;) {                            /* sift-down */
        l = 2 * i + 1;
        r = l + 1;
        if (l >= h->size) break;
        m = l;
        if (r < h->size && node_less(h->a[r], h->a[l])) m = r;
        if (!node_less(h->a[m], last)) break;
        h->a[i] = h->a[m];
        i = m;
    }
    if (h->size > 0) h->a[i] = last;
    return 1;
}

/* ============================ Binary Search ============================ */

/* ค้นหาเครื่องจากหมายเลข  O(log n)  คืน index หรือ -1 */
static int find_machine(int id)
{
    int lo = 0, hi = NUM_MACHINES - 1, mid;
    while (lo <= hi) {
        mid = lo + (hi - lo) / 2;
        if (machines[mid].id == id) return mid;
        if (machines[mid].id < id) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

/* ค้นหาผู้ใช้จากรหัส  O(log n)  คืน index หรือ -1 */
static int find_user(int id)
{
    int lo = 0, hi = userCount - 1, mid;
    while (lo <= hi) {
        mid = lo + (hi - lo) / 2;
        if (users[mid].id == id) return mid;
        if (users[mid].id < id) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

/* แทรกผู้ใช้ใหม่โดยคงลำดับเรียงตาม id (insertion)  O(n) */
static void register_user(User u)
{
    int i;
    if (userCount == userCap) {
        userCap = userCap ? userCap * 2 : 64;
        users = (User *)realloc(users, sizeof(User) * userCap);
    }
    i = userCount - 1;
    while (i >= 0 && users[i].id > u.id) {
        users[i + 1] = users[i];
        i--;
    }
    users[i + 1] = u;
    userCount++;
}

/* ============================ Sorting ============================ */

/* Merge Sort  O(n log n)  ใช้เรียงชุดข้อมูลทดสอบก่อนทำ Binary Search */
static void merge_sort(int *a, int *tmp, int l, int r)
{
    int m, i, j, k;
    if (r - l < 1) return;
    m = l + (r - l) / 2;
    merge_sort(a, tmp, l, m);
    merge_sort(a, tmp, m + 1, r);
    i = l; j = m + 1; k = l;
    while (i <= m && j <= r) tmp[k++] = (a[i] <= a[j]) ? a[i++] : a[j++];
    while (i <= m) tmp[k++] = a[i++];
    while (j <= r) tmp[k++] = a[j++];
    for (k = l; k <= r; k++) a[k] = tmp[k];
}

/* Insertion Sort  ใช้แสดงคิวที่มีไม่กี่คน (เรียงตาม priority แล้วตามเวลา) */
static void sort_nodes(QNode *a, int n)
{
    int i, j;
    QNode key;
    for (i = 1; i < n; i++) {
        key = a[i];
        j = i - 1;
        while (j >= 0 && node_less(key, a[j])) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = key;
    }
}


/* ============================ ฟังก์ชันของระบบคิว ============================ */

int core_find_machine(int id) { return find_machine(id); }
int core_find_user(int id)    { return find_user(id); }

const char *core_status_text(int s)
{
    return s == M_FREE ? "ว่าง" : (s == M_BUSY ? "กำลังใช้งาน" : "เสีย");
}

void core_init(void)
{
    int i;
    for (i = 0; i < NUM_MACHINES; i++) {
        machines[i].id = i + 1;
        machines[i].status = M_FREE;
        machines[i].userId = 0;
        machines[i].remaining = 0;
    }
    if (heap.a == NULL) heap_init(&heap, 64);
    heap.size = 0;
    free(users);
    users = NULL;
    userCount = 0;
    userCap = 0;
    ticker = 0;
}

/* ถ้ามีเครื่องว่าง ให้ดึงผู้ใช้ที่ priority สูงสุดออกจาก heap ไปใช้เครื่องนั้น */
static void dispatch(void)
{
    int m, ui;
    QNode n;
    for (m = 0; m < NUM_MACHINES; m++) {
        if (machines[m].status != M_FREE) continue;
        if (!heap_pop(&heap, &n)) break;
        ui = find_user(n.id);
        machines[m].status = M_BUSY;
        machines[m].userId = n.id;
        machines[m].remaining = users[ui].duration;
        users[ui].status = U_WASH;
        users[ui].machineId = machines[m].id;
        core_printf("  >> เรียกคุณ %s (รหัส %d, priority %d) ไปใช้เครื่อง %d\n",
                    users[ui].name, n.id, n.priority, machines[m].id);
    }
}

int core_add_user(int id, const char *name, int priority, int duration)
{
    User u;
    QNode n;
    if (priority != 1 && priority != 2) { core_printf("priority ต้องเป็น 1 หรือ 2\n"); return -2; }
    if (find_user(id) != -1) { core_printf("มีรหัสนี้อยู่แล้ว\n"); return -1; }
    memset(&u, 0, sizeof u);
    u.id = id;
    strncpy(u.name, name, sizeof u.name - 1);
    u.priority = priority;
    u.arrival = ++ticker;
    u.duration = duration > 0 ? duration : DEFAULT_WASH_MINUTES;
    u.status = U_WAIT;
    u.machineId = 0;
    register_user(u);

    n.id = id; n.priority = priority; n.arrival = u.arrival;
    heap_push(&heap, n);
    core_printf("เข้าคิวแล้ว\n");
    dispatch();
    return 0;
}

/* ซักเสร็จ: คืนเครื่องแล้วเรียกคิวถัดไป */
int core_finish(int machineId)
{
    int mi, ui;
    mi = find_machine(machineId);                  /* Binary Search */
    if (mi < 0) { core_printf("ไม่พบเครื่องหมายเลข %d\n", machineId); return -1; }
    if (machines[mi].status != M_BUSY) { core_printf("เครื่องนี้ไม่ได้กำลังใช้งาน\n"); return -2; }
    ui = find_user(machines[mi].userId);
    users[ui].status = U_DONE;
    users[ui].machineId = 0;
    machines[mi].status = M_FREE;
    machines[mi].userId = 0;
    machines[mi].remaining = 0;
    core_printf("เครื่อง %d ว่างแล้ว\n", machineId);
    dispatch();
    return 0;
}

/* เครื่องเสีย: ถ้ามีคนใช้อยู่ ให้กลับเข้าคิวด้วย priority 1 (ชดเชย) คงเวลาเข้าคิวเดิม แล้วจัดสรรใหม่ */
int core_broken(int machineId)
{
    int mi, ui;
    QNode n;
    mi = find_machine(machineId);
    if (mi < 0) { core_printf("ไม่พบเครื่องหมายเลข %d\n", machineId); return -1; }
    if (machines[mi].status == M_BROKEN) { core_printf("เครื่องนี้เสียอยู่แล้ว\n"); return -2; }
    if (machines[mi].status == M_BUSY) {
        ui = find_user(machines[mi].userId);
        users[ui].status = U_WAIT;
        users[ui].machineId = 0;
        users[ui].priority = 1;                    /* ชดเชยผู้ใช้ที่เครื่องเสียกลางคัน */
        n.id = users[ui].id; n.priority = 1; n.arrival = users[ui].arrival;
        heap_push(&heap, n);
        core_printf("ผู้ใช้ %s ถูกส่งกลับเข้าคิว (priority 1)\n", users[ui].name);
    }
    machines[mi].status = M_BROKEN;
    machines[mi].userId = 0;
    machines[mi].remaining = 0;
    core_printf("บันทึกเครื่อง %d เสียแล้ว\n", machineId);
    dispatch();
    return 0;
}

int core_repair(int machineId)
{
    int mi = find_machine(machineId);
    if (mi < 0) { core_printf("ไม่พบเครื่องหมายเลข %d\n", machineId); return -1; }
    if (machines[mi].status != M_BROKEN) { core_printf("เครื่องนี้ไม่ได้เสีย\n"); return -2; }
    machines[mi].status = M_FREE;
    core_printf("เครื่อง %d กลับมาใช้งานได้\n", machineId);
    dispatch();
    return 0;
}

/* เดินเวลาจำลอง: ลดเวลาที่เหลือของทุกเครื่องที่ใช้งาน เครื่องที่ครบเวลาจะว่าง แล้วเรียกคิวถัดไป */
void core_advance(int minutes)
{
    int m, ui;
    if (minutes <= 0) return;
    for (m = 0; m < NUM_MACHINES; m++) {
        if (machines[m].status != M_BUSY) continue;
        machines[m].remaining -= minutes;
        if (machines[m].remaining > 0) continue;
        ui = find_user(machines[m].userId);
        core_printf("เครื่อง %d ซักเสร็จ: %s\n", machines[m].id, users[ui].name);
        users[ui].status = U_DONE;
        users[ui].machineId = 0;
        machines[m].status = M_FREE;
        machines[m].userId = 0;
        machines[m].remaining = 0;
    }
    dispatch();
}

int core_queue_sorted(QNode *out, int max)
{
    int n = heap.size < max ? heap.size : max;
    memcpy(out, heap.a, sizeof(QNode) * n);
    sort_nodes(out, n);      /* heap ไม่ได้เรียงทั้งอาร์เรย์ จึงต้อง sort สำเนาเพื่อแสดง */
    return n;
}

void core_show_machines(void)
{
    int i, ui;
    core_printf("\n--- สถานะเครื่องซักผ้า ---\n");
    for (i = 0; i < NUM_MACHINES; i++) {
        core_printf("เครื่อง %2d : %-12s", machines[i].id, core_status_text(machines[i].status));
        if (machines[i].status == M_BUSY) {
            ui = find_user(machines[i].userId);
            core_printf(" ผู้ใช้: %s (รหัส %d)", users[ui].name, users[ui].id);
        }
        core_printf("\n");
    }
}

void core_show_queue(void)
{
    QNode *copy;
    int i, ui, n;
    core_printf("\n--- คิวที่รออยู่ (%d คน) ---\n", heap.size);
    if (heap.size == 0) { core_printf("(ไม่มีคิว)\n"); return; }
    copy = (QNode *)malloc(sizeof(QNode) * heap.size);
    n = core_queue_sorted(copy, heap.size);
    for (i = 0; i < n; i++) {
        ui = find_user(copy[i].id);
        core_printf("%2d. %-15s รหัส %-8d priority %d (%s)\n", i + 1, users[ui].name,
                    copy[i].id, copy[i].priority, copy[i].priority == 1 ? "ซักด่วน" : "รอปกติ");
    }
    free(copy);
}

void core_search_machine(int id)
{
    int mi = find_machine(id);
    if (mi < 0) core_printf("ไม่พบเครื่องหมายเลข %d\n", id);
    else core_printf("เครื่อง %d : %s\n", machines[mi].id, core_status_text(machines[mi].status));
}

void core_search_user(int id)
{
    int ui = find_user(id);
    if (ui < 0) { core_printf("ไม่พบผู้ใช้รหัส %d\n", id); return; }
    core_printf("%s (priority %d) : ", users[ui].name, users[ui].priority);
    if (users[ui].status == U_WAIT) core_printf("รอคิว\n");
    else if (users[ui].status == U_WASH) core_printf("กำลังซักที่เครื่อง %d\n", users[ui].machineId);
    else core_printf("ซักเสร็จแล้ว\n");
}

/* ============================ ทดสอบจับเวลา ============================ */

/* Sequential Search  O(n) */
static int linear_search(const int *a, int n, int target)
{
    int i;
    for (i = 0; i < n; i++)
        if (a[i] == target) return i;
    return -1;
}

/* Binary Search  O(log n)  (a ต้องเรียงจากน้อยไปมาก) */
static int binary_search(const int *a, int n, int target)
{
    int lo = 0, hi = n - 1, mid;
    while (lo <= hi) {
        mid = lo + (hi - lo) / 2;
        if (a[mid] == target) return mid;
        if (a[mid] < target) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

typedef int (*SearchFn)(const int *, int, int);

/* ln(x) เขียนเอง เพื่อไม่ต้องพึ่ง math.h/-lm (กันคอมไพล์ไม่ผ่านบนเครื่องกลาง)  x > 0 */
static double my_ln(double x)
{
    double y, y2, term, sum = 0.0;
    int e = 0, i;
    while (x >= 2.0) { x /= 2.0; e++; }
    while (x < 1.0)  { x *= 2.0; e--; }
    y = (x - 1.0) / (x + 1.0);
    y2 = y * y;
    term = y;
    for (i = 1; i < 40; i += 2) { sum += term / i; term *= y2; }
    return 2.0 * sum + e * 0.69314718055994531;
}

/* อ่านไฟล์ข้อมูลกลางของอาจารย์: ตัวแรก = จำนวน n แล้วตามด้วยค่า n ค่า (คั่นด้วยช่องว่าง/ขึ้นบรรทัดใหม่)
   คืนจำนวนข้อมูล หรือ -1 ถ้าเปิด/อ่านไม่ได้ */
static int load_central(const char *path, int **out)
{
    FILE *f = fopen(path, "r");
    int count, i;
    int *arr;
    if (!f) return -1;
    if (fscanf(f, "%d", &count) != 1 || count <= 0) { fclose(f); return -1; }
    arr = (int *)malloc(sizeof(int) * count);
    for (i = 0; i < count; i++) {
        if (fscanf(f, "%d", &arr[i]) != 1) { fclose(f); free(arr); return -1; }
    }
    fclose(f);
    *out = arr;
    return count;
}

/* จับเวลาตามรูปแบบเดียวกับ timing_template.c ของอาจารย์:
   วนค้นหาทุก target ซ้ำ REPEAT รอบ แล้วเฉลี่ยเป็นเวลาต่อการค้นหา 1 ครั้ง (ms)
   จับเฉพาะส่วนค้นหา ไม่รวมการอ่านไฟล์/เรียงข้อมูล/printf */
static double time_once(SearchFn fn, const int *arr, int n, const int *targets, int tcount)
{
    clock_t start, end;
    double total_sec;
    int r, i;
    volatile int sink = 0;                 /* กัน compiler ตัดโค้ดทิ้ง */
    const int *volatile vp = arr;          /* กัน compiler ยกการค้นหาออกนอก loop */

    start = clock();
    for (r = 0; r < REPEAT; r++)
        for (i = 0; i < tcount; i++)
            sink += fn(vp, n, targets[i]);
    end = clock();
    (void)sink;

    total_sec = (double)(end - start) / CLOCKS_PER_SEC;
    return total_sec * 1000.0 / ((double)REPEAT * tcount);
}

/* รัน RUNS ครั้ง ตัดค่าสูงสุด/ต่ำสุด เฉลี่ยจาก 3 ค่าที่เหลือ */
static double measure(SearchFn fn, const int *arr, int n, const int *targets, int tcount,
                      double runs[RUNS])
{
    int i, imin = 0, imax = 0;
    double sum = 0.0;
    for (i = 0; i < RUNS; i++) runs[i] = time_once(fn, arr, n, targets, tcount);
    for (i = 1; i < RUNS; i++) {
        if (runs[i] < runs[imin]) imin = i;
        if (runs[i] > runs[imax]) imax = i;
    }
    if (imin == imax) imax = (imin + 1) % RUNS;       /* กรณีทุกค่าเท่ากันหมด */
    for (i = 0; i < RUNS; i++)
        if (i != imin && i != imax) sum += runs[i];
    return sum / (RUNS - 2);
}

/* ตรวจความถูกต้องก่อนดูเวลา: Sequential(ไฟล์ไม่เรียง) กับ Binary(ไฟล์เรียงแล้ว) ต้องตอบตรงกัน
   และตำแหน่งที่คืนต้องชี้ไปที่ค่านั้นจริง  คืน 1 ถ้าผ่าน */
static int verify(const int *raw, const int *sorted, int n, const int *targets, int tcount)
{
    int i, ok = 1, found = 0, a, b;
    core_printf("  ค่าที่ค้น | Sequential | Binary\n");
    for (i = 0; i < tcount; i++) {
        a = linear_search(raw, n, targets[i]);
        b = binary_search(sorted, n, targets[i]);
        core_printf("  %-9d | %-10s | %s\n", targets[i], a >= 0 ? "found" : "NOT found", b >= 0 ? "found" : "NOT found");
        if ((a >= 0) != (b >= 0)) ok = 0;
        if (a >= 0 && raw[a] != targets[i]) ok = 0;
        if (b >= 0 && sorted[b] != targets[i]) ok = 0;
        if (a >= 0) found++;
    }
    core_printf("  พบ %d ค่า / ไม่พบ %d ค่า (ที่ถูกต้องควรเป็น 7 / 3)  -> %s\n", found, tcount - found,
           (ok && found == 7) ? "ผ่าน" : "ผิดพลาด! ตรวจโปรแกรมก่อนดูเวลา");
    return ok && found == 7;
}

/* dir = โฟลเดอร์ที่เก็บ data_N.txt, data_N_sorted.txt, targets_N.txt */
static void benchmark(const char *dir)
{
    static const int sizes[3] = {1000, 10000, 100000};
    static const char *names[2] = {"Sequential Search  O(n)    (ข้อมูลไม่เรียง)",
                                   "Binary Search      O(log n) (ข้อมูลเรียงแล้ว)"};
    double runs[2][3][RUNS], avg[2][3];
    char path[1100];
    int *raw, *sorted, *targets, *mine, *tmp;
    int s, a, i, n, tn, sn;

    if (!dir || !dir[0]) dir = ".";
    for (a = 0; a < 2; a++) for (s = 0; s < 3; s++) avg[a][s] = 0.0;

    core_printf("\nREPEAT = %d, วัดขนาดละ %d ครั้ง ตัดสูง/ต่ำสุด เฉลี่ย 3 ค่า\n", REPEAT, RUNS);

    for (s = 0; s < 3; s++) {
        raw = sorted = targets = NULL;
        core_printf("\n########## n = %d ##########\n", sizes[s]);
        snprintf(path, sizeof path, "%s/data_%d.txt", dir, sizes[s]);
        n = load_central(path, &raw);
        snprintf(path, sizeof path, "%s/data_%d_sorted.txt", dir, sizes[s]);
        sn = load_central(path, &sorted);
        snprintf(path, sizeof path, "%s/targets_%d.txt", dir, sizes[s]);
        tn = load_central(path, &targets);
        if (n < 0 || sn != n || tn < 0) {
            core_printf("อ่านไฟล์ชุด n=%d ไม่ได้ (ตรวจชื่อไฟล์/โฟลเดอร์ \"%s\")\n", sizes[s], dir);
            free(raw); free(sorted); free(targets);
            continue;
        }

        /* พิสูจน์ว่า Merge Sort ของเราเรียงได้ตรงกับไฟล์เรียงของอาจารย์ (ไม่นับเวลา) */
        mine = (int *)malloc(sizeof(int) * n);
        tmp  = (int *)malloc(sizeof(int) * n);
        memcpy(mine, raw, sizeof(int) * n);
        merge_sort(mine, tmp, 0, n - 1);
        core_printf("  Merge Sort ของเรา %s ไฟล์เรียงของอาจารย์\n",
               memcmp(mine, sorted, sizeof(int) * n) == 0 ? "ตรงกับ" : "ไม่ตรงกับ");
        free(mine); free(tmp);

        if (!verify(raw, sorted, n, targets, tn)) {
            free(raw); free(sorted); free(targets);
            continue;
        }

        avg[0][s] = measure(linear_search, raw,    n, targets, tn, runs[0][s]);
        avg[1][s] = measure(binary_search, sorted, n, targets, tn, runs[1][s]);
        free(raw); free(sorted); free(targets);
    }

    for (a = 0; a < 2; a++) {
        core_printf("\n=== %s ===\n", names[a]);
        core_printf("%-9s %-11s %-11s %-11s %-11s %-11s %-12s\n",
               "n", "ครั้งที่1", "ครั้งที่2", "ครั้งที่3", "ครั้งที่4", "ครั้งที่5", "เฉลี่ย(ms)");
        for (s = 0; s < 3; s++) {
            core_printf("%-9d", sizes[s]);
            if (avg[a][s] <= 0.0) { core_printf(" (ไม่มีผล)\n"); continue; }
            for (i = 0; i < RUNS; i++) core_printf(" %-11.6f", runs[a][s][i]);
            core_printf(" %-12.6f\n", avg[a][s]);
        }
        for (s = 1; s < 3; s++) {
            if (avg[a][s - 1] > 0 && avg[a][s] > 0) {
                double ratio = avg[a][s] / avg[a][s - 1];
                double k = my_ln(ratio) / my_ln(10.0);
                core_printf("n %d -> %d : เวลาเพิ่ม %.2f เท่า, k = %.2f\n",
                       sizes[s - 1], sizes[s], ratio, k);
            } else {
                core_printf("n %d -> %d : คำนวณ k ไม่ได้ (ไม่มีผล หรือเวลาเป็น 0)\n",
                       sizes[s - 1], sizes[s]);
            }
        }
    }
}


void core_benchmark(const char *dir)
{
    benchmark(dir);
}
