/* =====================================================================
   timing_template.c  —  โปรแกรมต้นแบบสำหรับวัดเวลาการค้นหา
   รายวิชา 01204212 แบบชนิดข้อมูลนามธรรมและการแก้ปัญหา
   ใช้ประกอบกิจกรรมนำเสนอโครงงาน วันที่ 5 ตุลาคม 2569

   วิธีคอมไพล์ :  gcc timing_template.c -o timing -O3
   วิธีรัน      :  ./timing data_1000.txt targets_1000.txt
                  (บน Windows ใช้  timing.exe data_1000.txt targets_1000.txt)

   อัลกอริทึมของกลุ่ม: AVL Tree (Self-balancing Binary Search Tree)
   ที่ใช้เป็นแกนหลักในการค้นหารายวิชาในโปรเจกต์ (main_ncurses.c)
   ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAXN   100000
#define REPEAT 1000      /* จำนวนรอบที่วนค้นหาซ้ำ ใช้ค่าเดียวกันทุกการทดลอง */

int data[MAXN];          /* ข้อมูลตามลำดับในไฟล์ (ยังไม่เรียง) */
int sorted_data[MAXN];   /* สำเนาที่เรียงแล้ว สำหรับ Binary Search */
int n;

/* ---------- อ่านไฟล์ข้อมูล: บรรทัดแรกคือ n จากนั้นตามด้วยค่า n ค่า ---------- */
int LoadData(const char *filename, int arr[])
{
    FILE *fp = fopen(filename, "r");
    int  count, i;

    if (fp == NULL) {
        printf("เปิดไฟล์ %s ไม่ได้\n", filename);
        exit(1);
    }
    if (fscanf(fp, "%d", &count) != 1) {
        printf("รูปแบบไฟล์ %s ไม่ถูกต้อง\n", filename);
        fclose(fp);
        exit(1);
    }
    for (i = 0; i < count; i++) {
        if (fscanf(fp, "%d", &arr[i]) != 1)
            break;
    }
    fclose(fp);
    return count;
}

/* ---------- ใช้เรียงข้อมูลก่อนทำ Binary Search (ไม่นับเวลาส่วนนี้) ---------- */
int CompareInt(const void *a, const void *b)
{
    return (*(const int *)a) - (*(const int *)b);
}

/* ---------- อัลกอริทึมที่ 1: Sequential Search ---------- */
int SequentialSearch(int arr[], int size, int target)
{
    int i;
    for (i = 0; i < size; i++)
        if (arr[i] == target)
            return i;            /* คืนตำแหน่งที่พบ */
    return -1;                   /* ไม่พบ */
}

/* ---------- อัลกอริทึมที่ 2: Binary Search (ข้อมูลต้องเรียงแล้ว) ---------- */
int BinarySearch(int arr[], int size, int target)
{
    int first = 0, last = size - 1, mid;

    while (first <= last) {
        mid = (first + last) / 2;
        if (target > arr[mid])      first = mid + 1;
        else if (target < arr[mid]) last  = mid - 1;
        else                        return mid;
    }
    return -1;
}

/* =====================================================================
   โครงสร้างข้อมูลและฟังก์ชัน AVL Tree จาก main_ncurses.c
   ปรับสำหรับค้นหา key ชนิด int ในชุดข้อมูลทดสอบ
   ===================================================================== */
struct AVLNode {
    int key;
    struct AVLNode *left, *right;
    int height;
};

static struct AVLNode *avl_root = NULL;

static int avlH(struct AVLNode *node) {
    return node ? node->height : 0;
}

static void avlUpdH(struct AVLNode *node) {
    if (!node) return;
    int l = avlH(node->left);
    int r = avlH(node->right);
    node->height = 1 + (l > r ? l : r);
}

static struct AVLNode *avlRotR(struct AVLNode *y) {
    struct AVLNode *x = y->left;
    struct AVLNode *T = x->right;
    x->right = y;
    y->left = T;
    avlUpdH(y);
    avlUpdH(x);
    return x;
}

static struct AVLNode *avlRotL(struct AVLNode *x) {
    struct AVLNode *y = x->right;
    struct AVLNode *T = y->left;
    y->left = x;
    x->right = T;
    avlUpdH(x);
    avlUpdH(y);
    return y;
}

static struct AVLNode *avlBal(struct AVLNode *node) {
    avlUpdH(node);
    int bf = avlH(node->left) - avlH(node->right);
    if (bf > 1) {
        if (avlH(node->left->right) > avlH(node->left->left))
            node->left = avlRotL(node->left);
        return avlRotR(node);
    }
    if (bf < -1) {
        if (avlH(node->right->left) > avlH(node->right->right))
            node->right = avlRotR(node->right);
        return avlRotL(node);
    }
    return node;
}

struct AVLNode *avlInsert(struct AVLNode *r, int val) {
    if (!r) {
        struct AVLNode *n = (struct AVLNode *)malloc(sizeof(struct AVLNode));
        n->key = val;
        n->left = n->right = NULL;
        n->height = 1;
        return n;
    }
    if (val < r->key)
        r->left = avlInsert(r->left, val);
    else if (val > r->key)
        r->right = avlInsert(r->right, val);
    return avlBal(r);
}

/* ค้นหาข้อมูลใน AVL Tree แบบ Recursive ตามโค้ดต้นฉบับใน main_ncurses.c */
struct AVLNode *avlSearch(struct AVLNode *r, int target) {
    if (!r)
        return NULL;
    if (target == r->key)
        return r;
    return (target < r->key) ? avlSearch(r->left, target) : avlSearch(r->right, target);
}

void avlFree(struct AVLNode *r) {
    if (!r) return;
    avlFree(r->left);
    avlFree(r->right);
    free(r);
}

/* =====================================================================
   จุดที่แต่ละกลุ่มต้องแก้ไข
   แทนที่เนื้อในของฟังก์ชันนี้ด้วยอัลกอริทึมการค้นหาของกลุ่มตนเอง:
   AVL Tree Search (O(log n)) ที่ใช้ใน main_ncurses.c
   ===================================================================== */
int MySearch(int arr[], int size, int target)
{
    (void)arr;
    (void)size;
    return (avlSearch(avl_root, target) != NULL) ? 1 : -1;
}

#ifdef _WIN32
#include <windows.h>
#endif

/* ---------- วัดเวลาเฉลี่ยต่อการค้นหาหนึ่งครั้ง หน่วยเป็นมิลลิวินาที ---------- */
double MeasureMillisec(int (*SearchFunc)(int[], int, int),
                       int arr[], int size, int targets[], int tcount)
{
#ifdef _WIN32
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    int r, i, result = 0;
    for (r = 0; r < REPEAT; r++)
        for (i = 0; i < tcount; i++)
            result += SearchFunc(arr, size, targets[i]);
    QueryPerformanceCounter(&end);
    if (result == -99999999) printf(" ");
    double total_ms = (double)(end.QuadPart - start.QuadPart) * 1000.0 / (double)freq.QuadPart;
    return total_ms / (REPEAT * tcount);
#else
    clock_t start, end;
    double  total_sec;
    int     r, i, result = 0;

    start = clock();                                  /* เริ่มจับเวลา */
    for (r = 0; r < REPEAT; r++)
        for (i = 0; i < tcount; i++)
            result += SearchFunc(arr, size, targets[i]);
    end = clock();                                    /* หยุดจับเวลา */

    if (result == -99999999) printf(" ");  /* กันคอมไพเลอร์ตัดโค้ดทิ้ง */

    total_sec = (double)(end - start) / CLOCKS_PER_SEC;
    return total_sec * 1000.0 / (REPEAT * tcount);    /* เฉลี่ยต่อหนึ่งครั้ง */
#endif
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    int    targets[100], tcount, i;
    double ms_seq, ms_bin, ms_my;

    if (argc < 3) {
        printf("วิธีใช้: %s <ไฟล์ข้อมูล> <ไฟล์ค่าที่ค้นหา>\n", argv[0]);
        printf("ตัวอย่าง: %s data_1000.txt targets_1000.txt\n", argv[0]);
        return 1;
    }

    /* ---- เตรียมข้อมูล (ไม่จับเวลาส่วนนี้) ---- */
    n      = LoadData(argv[1], data);
    tcount = LoadData(argv[2], targets);

    for (i = 0; i < n; i++) sorted_data[i] = data[i];
    qsort(sorted_data, n, sizeof(int), CompareInt);

    /* สร้าง AVL Tree จากชุดข้อมูล (ขั้นตอนเตรียมข้อมูล ไม่นับเวลาค้นหา) */
    avl_root = NULL;
    for (i = 0; i < n; i++) {
        avl_root = avlInsert(avl_root, data[i]);
    }

    printf("=====================================================\n");
    printf(" ไฟล์ข้อมูล      : %s\n", argv[1]);
    printf(" จำนวนข้อมูล n   : %d\n", n);
    printf(" จำนวนค่าที่ค้นหา : %d\n", tcount);
    printf(" จำนวนรอบที่วัด  : %d รอบต่อค่า\n", REPEAT);
    printf(" อัลกอริทึมกลุ่ม  : AVL Tree (จาก main_ncurses.c)\n");
    printf("=====================================================\n");

    /* ---- ตรวจความถูกต้องก่อนวัดเวลา ---- */
    printf("\n[ ตรวจความถูกต้องของผลการค้นหา ]\n");
    printf(" %-12s %-12s %-12s %-16s\n", "ค่าที่ค้นหา", "Sequential", "Binary", "AVL (MySearch)");
    for (i = 0; i < tcount; i++) {
        int a = SequentialSearch(data, n, targets[i]);
        int b = BinarySearch(sorted_data, n, targets[i]);
        int c = MySearch(data, n, targets[i]);
        printf(" %-12d %-12s %-12s %-16s\n", targets[i],
               (a >= 0) ? "พบ  " : "ไม่พบ",
               (b >= 0) ? "พบ  " : "ไม่พบ",
               (c >= 0) ? "พบ  " : "ไม่พบ");
    }

    /* ---- วัดเวลา ---- */
    ms_seq = MeasureMillisec(SequentialSearch, data,        n, targets, tcount);
    ms_bin = MeasureMillisec(BinarySearch,     sorted_data, n, targets, tcount);
    ms_my  = MeasureMillisec(MySearch,         data,        n, targets, tcount);

    printf("\n[ ผลการวัดเวลา เฉลี่ยต่อการค้นหาหนึ่งครั้ง ]\n");
    printf(" Sequential Search : %.6f ms\n", ms_seq);
    printf(" Binary Search     : %.6f ms\n", ms_bin);
    printf(" MySearch (ของกลุ่ม): %.6f ms\n", ms_my);
    printf("\nนำค่าที่ได้ไปกรอกในตารางบันทึกผล แล้วรันซ้ำจนครบ 5 ครั้ง\n");

    avlFree(avl_root);
    return 0;
}
