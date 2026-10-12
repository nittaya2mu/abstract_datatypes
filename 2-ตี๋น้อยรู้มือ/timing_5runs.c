/* =====================================================================
   timing_5runs.c  —  โปรแกรมวัดเวลาการค้นหา 5 ครั้ง พร้อมคำนวณ Trimmed Mean
   รายวิชา 01204212 แบบชนิดข้อมูลนามธรรมและการแก้ปัญหา
   ใช้ประกอบกิจกรรมนำเสนอโครงงาน วันที่ 5 ตุลาคม 2569

   กลุ่ม Teenoi Suki: ค้นหาด้วย Binary Search Tree (BST)
   ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define MAXN     100000
#define REPEAT   10000    /* 10,000 รอบต่อค่า เพื่อความแม่นยำของนาฬิการะบบ */
#define NUM_RUNS 5        /* วัดผลซ้ำ 5 ครั้งตามเกณฑ์ */

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
    fscanf(fp, "%d", &count);
    for (i = 0; i < count; i++)
        fscanf(fp, "%d", &arr[i]);
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

/* ---------- อัลกอริทึมที่ 3: Binary Search Tree (BST) ของกลุ่ม Teenoi Suki ---------- */
typedef struct BSTNode {
    int key;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

static BSTNode bst_pool[MAXN]; /* static memory pool: zero malloc overhead */
static int bst_pool_idx = 0;
static BSTNode *bst_root = NULL;

/* สร้าง Balanced BST จาก sorted array ในเวลา O(n) (ทำในขั้นตอนเตรียมข้อมูล ไม่นับเวลาค้นหา) */
BSTNode* BuildBalancedBST(int arr[], int start, int end)
{
    if (start > end) return NULL;
    int mid = (start + end) / 2;
    BSTNode *node = &bst_pool[bst_pool_idx++];
    node->key = arr[mid];
    node->left = BuildBalancedBST(arr, start, mid - 1);
    node->right = BuildBalancedBST(arr, mid + 1, end);
    return node;
}

void InitBST(int arr[], int size)
{
    bst_pool_idx = 0;
    bst_root = BuildBalancedBST(arr, 0, size - 1);
}

/* ค้นหาใน BST แบบ Iterative: O(log n) */
int BSTSearch(BSTNode *root, int target)
{
    BSTNode *curr = root;
    while (curr != NULL) {
        if (target == curr->key) return 1; /* คืนค่า >= 0 แปลว่าพบ */
        if (target < curr->key)  curr = curr->left;
        else                     curr = curr->right;
    }
    return -1; /* ไม่พบ */
}

int MySearch(int arr[], int size, int target)
{
    if (bst_root == NULL) {
        InitBST(sorted_data, size);
    }
    return BSTSearch(bst_root, target);
}

/* ---------- วัดเวลาเฉลี่ยต่อการค้นหาหนึ่งครั้ง หน่วยเป็นมิลลิวินาที ---------- */
double MeasureMillisec(int (*SearchFunc)(int[], int, int),
                       int arr[], int size, int targets[], int tcount)
{
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
}

/* ---------- คำนวณ Trimmed Mean: ตัดค่าสูงสุด 1 ครั้ง และต่ำสุด 1 ครั้ง ---------- */
double CalcTrimmedMean(double arr[], int count)
{
    double sorted[NUM_RUNS];
    int i, j;
    for (i = 0; i < count; i++) sorted[i] = arr[i];
    for (i = 0; i < count - 1; i++) {
        for (j = i + 1; j < count; j++) {
            if (sorted[i] > sorted[j]) {
                double tmp = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = tmp;
            }
        }
    }
    double sum = 0.0;
    for (i = 1; i < count - 1; i++) {
        sum += sorted[i];
    }
    return sum / (count - 2);
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif
    int    targets[100], tcount, i, run;
    double seq_times[NUM_RUNS], bin_times[NUM_RUNS], my_times[NUM_RUNS];

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
    InitBST(sorted_data, n); /* สร้าง Balanced BST ล่วงหน้าในขั้นตอนเตรียมข้อมูล */

    printf("=====================================================\n");
    printf(" ไฟล์ข้อมูล      : %s\n", argv[1]);
    printf(" จำนวนข้อมูล n   : %d\n", n);
    printf(" จำนวนค่าที่ค้นหา : %d\n", tcount);
    printf(" จำนวนรอบที่วัด  : %d รอบต่อค่า\n", REPEAT);
    printf("=====================================================\n");

    /* ---- ตรวจความถูกต้องก่อนวัดเวลา ---- */
    printf("\n[ ตรวจความถูกต้องของผลการค้นหา ]\n");
    printf(" ค่าที่ค้นหา Sequential Binary     MySearch\n");
    for (i = 0; i < tcount; i++) {
        int a = SequentialSearch(data, n, targets[i]);
        int b = BinarySearch(sorted_data, n, targets[i]);
        int c = MySearch(data, n, targets[i]);
        printf(" %-12d %s %s %s\n", targets[i],
               (a >= 0) ? "พบ    " : "ไม่พบ ",
               (b >= 0) ? "พบ    " : "ไม่พบ ",
               (c >= 0) ? "พบ" : "ไม่พบ");
    }

    /* ---- วัดเวลา 5 ครั้ง ---- */
    for (run = 0; run < NUM_RUNS; run++) {
        seq_times[run] = MeasureMillisec(SequentialSearch, data,        n, targets, tcount);
        bin_times[run] = MeasureMillisec(BinarySearch,     sorted_data, n, targets, tcount);
        my_times[run]  = MeasureMillisec(MySearch,         data,        n, targets, tcount);
    }

    printf("\n[ ผลการวัดเวลา 5 ครั้ง ]\n");
    printf(" ครั้งที่   Sequential (ms)    Binary Search (ms)       MySearch (ms)\n");
    printf(" --------------------------------------------------------------------\n");
    for (run = 0; run < NUM_RUNS; run++) {
        printf("  [%d]           %.6f             %.6f             %.6f\n",
               run + 1, seq_times[run], bin_times[run], my_times[run]);
    }
    printf(" --------------------------------------------------------------------\n");
    printf(" ค่าเฉลี่ย 3 ครั้งที่เหลือ (Trimmed Mean):\n");
    printf("  Sequential Search : %.6f ms\n", CalcTrimmedMean(seq_times, NUM_RUNS));
    printf("  Binary Search     : %.6f ms\n", CalcTrimmedMean(bin_times, NUM_RUNS));
    printf("  MySearch (BST)    : %.6f ms\n", CalcTrimmedMean(my_times, NUM_RUNS));
    printf("=====================================================\n");

    return 0;
}
