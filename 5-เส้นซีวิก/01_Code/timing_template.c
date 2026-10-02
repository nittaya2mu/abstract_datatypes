/* =====================================================================
   timing_template.c  —  โปรแกรมต้นแบบสำหรับวัดเวลาการค้นหา
   รายวิชา 01204212 แบบชนิดข้อมูลนามธรรมและการแก้ปัญหา
   ภาควิชาวิศวกรรมคอมพิวเตอร์ มหาวิทยาลัยเกษตรศาสตร์
   ผู้สอน: ผศ.ดร.นิตยา เมืองนาค
   
   กลุ่มโครงงาน: Smart Fridge (ระบบช่วยเคลียร์ตู้เย็นและแนะนำเมนูอาหาร)
   อัลกอริทึม MySearch: Binary Search Tree (BST Search)

   วิธีคอมไพล์ :  gcc -O2 timing_template.c -o timing.exe
   วิธีรัน      :  timing.exe data_1000.txt targets_1000.txt
   ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define MAXN       100000
/* 
   ปรับ REPEAT เป็น 10000 รอบ เพื่อแก้ปัญหา Windows Timer Resolution (10-15 ms)
   หากตั้ง 1,000 รอบ การค้นหา Binary/BST จะเร็วเกินไปจน clock() อ่านค่าได้ 0.000000 ms
*/
#define REPEAT     10000
#define NUM_RUNS   5      /* จำนวนครั้งที่รันเพื่อหาค่าเฉลี่ยแบบ Trimmed Mean */

int data[MAXN];          /* ข้อมูลตามลำดับในไฟล์ (ยังไม่เรียง) */
int sorted_data[MAXN];   /* สำเนาที่เรียงแล้ว สำหรับ Binary Search และสร้าง BST */
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
        printf("รูปแบบไฟล์ %s ผิดพลาด\n", filename);
        fclose(fp);
        exit(1);
    }
    for (i = 0; i < count; i++) {
        if (fscanf(fp, "%d", &arr[i]) != 1) {
            printf("อ่านข้อมูลตำแหน่ง %d ในไฟล์ %s ไม่สำเร็จ\n", i, filename);
            fclose(fp);
            exit(1);
        }
    }
    fclose(fp);
    return count;
}

/* ---------- ใช้เรียงข้อมูลก่อนทำ Binary Search (ไม่นับเวลาส่วนนี้) ---------- */
int CompareInt(const void *a, const void *b)
{
    return (*(const int *)a) - (*(const int *)b);
}

int CompareDouble(const void *a, const void *b)
{
    double diff = (*(const double *)a) - (*(const double *)b);
    if (diff < 0) return -1;
    if (diff > 0) return 1;
    return 0;
}

/* ---------- อัลกอริทึมที่ 1: Sequential Search ---------- */
int SequentialSearch(int arr[], int size, int target)
{
    int i;
    for (i = 0; i < size; i++) {
        if (arr[i] == target)
            return i;            /* คืนตำแหน่งที่พบ */
    }
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
   [อัลกอริทึมที่ 3: MySearch] ของกลุ่ม Smart Fridge
   โครงสร้างข้อมูล: Binary Search Tree (BST Search)
   สอดคล้องกับ FoodBST ในโปรแกรมหลักของกลุ่ม
   ===================================================================== */
typedef struct BSTNode {
    int key;
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

/* Static Memory Pool เพื่อลด Memory Overhead และเวลา malloc ในการทดลอง */
static BSTNode bst_pool[MAXN];
static int     bst_pool_idx = 0;
static BSTNode *bst_root = NULL;

/* ฟังก์ชันสร้าง Balanced BST จาก Sorted Array ในเวลา O(n) (เตรียมข้อมูลล่วงหน้า) */
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
        if (target == curr->key) return 1; /* พบข้อมูล คืนค่า >= 0 */
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
    for (r = 0; r < REPEAT; r++) {
        for (i = 0; i < tcount; i++) {
            result += SearchFunc(arr, size, targets[i]);
        }
    }
    end = clock();                                    /* หยุดจับเวลา */

    if (result == -99999999) printf(" ");  /* กันคอมไพเลอร์ตัดโค้ดทิ้ง */

    total_sec = (double)(end - start) / CLOCKS_PER_SEC;
    return (total_sec * 1000.0) / (REPEAT * tcount);  /* เฉลี่ยต่อหนึ่งครั้ง */
}

/* คำนวณ Trimmed Mean (ตัดค่าต่ำสุด 1 ครั้ง และสูงสุด 1 ครั้ง แล้วเฉลี่ย 3 ครั้งที่เหลือ) */
double CalculateTrimmedMean(double times[], int count)
{
    double sorted[NUM_RUNS];
    for (int i = 0; i < count; i++) sorted[i] = times[i];
    qsort(sorted, count, sizeof(double), CompareDouble);

    double sum = 0.0;
    for (int i = 1; i < count - 1; i++) {
        sum += sorted[i];
    }
    return sum / (count - 2);
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    /* ตั้งค่า Console ให้แสดงผลภาษาไทย UTF-8 อย่างถูกต้องบน Windows */
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    int    targets[100], tcount, i;
    double ms_seq[NUM_RUNS], ms_bin[NUM_RUNS], ms_my[NUM_RUNS];

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

    /* สร้าง Balanced BST ล่วงหน้าสำหรับ MySearch */
    InitBST(sorted_data, n);

    printf("=====================================================\n");
    printf(" ไฟล์ข้อมูล      : %s\n", argv[1]);
    printf(" จำนวนข้อมูล n   : %d\n", n);
    printf(" จำนวนค่าที่ค้นหา : %d\n", tcount);
    printf(" จำนวนรอบที่วัด  : %d รอบต่อค่า\n", REPEAT);
    printf("=====================================================\n");

    /* ---- ตรวจความถูกต้องก่อนวัดเวลา ---- */
    printf("\n[ ตรวจความถูกต้องของผลการค้นหา ]\n");
    printf(" %-12s %-10s %-10s %-10s\n", "ค่าที่ค้นหา", "Sequential", "Binary", "MySearch");
    for (i = 0; i < tcount; i++) {
        int a = SequentialSearch(data, n, targets[i]);
        int b = BinarySearch(sorted_data, n, targets[i]);
        int c = MySearch(data, n, targets[i]);
        printf(" %-12d %-10s %-10s %-10s\n", targets[i],
               (a >= 0) ? "พบ" : "ไม่พบ",
               (b >= 0) ? "พบ" : "ไม่พบ",
               (c >= 0) ? "พบ" : "ไม่พบ");
    }

    /* ---- วัดเวลารวม 5 ครั้ง ---- */
    printf("\n[ ผลการวัดเวลา 5 ครั้ง ]\n");
    printf(" ครั้งที่   Sequential (ms)    Binary Search (ms)       MySearch (ms)\n");
    printf(" --------------------------------------------------------------------\n");

    for (int run = 0; run < NUM_RUNS; run++) {
        ms_seq[run] = MeasureMillisec(SequentialSearch, data,        n, targets, tcount);
        ms_bin[run] = MeasureMillisec(BinarySearch,     sorted_data, n, targets, tcount);
        ms_my[run]  = MeasureMillisec(MySearch,         data,        n, targets, tcount);

        printf("  [%d]     %14.6f       %14.6f       %14.6f\n",
               run + 1, ms_seq[run], ms_bin[run], ms_my[run]);
    }

    /* ---- คำนวณ Trimmed Mean ---- */
    double trim_seq = CalculateTrimmedMean(ms_seq, NUM_RUNS);
    double trim_bin = CalculateTrimmedMean(ms_bin, NUM_RUNS);
    double trim_my  = CalculateTrimmedMean(ms_my,  NUM_RUNS);

    printf(" --------------------------------------------------------------------\n");
    printf(" ค่าเฉลี่ย 3 ครั้งที่เหลือ (Trimmed Mean):\n");
    printf("  Sequential Search : %.6f ms\n", trim_seq);
    printf("  Binary Search     : %.6f ms\n", trim_bin);
    printf("  MySearch (BST)    : %.6f ms\n", trim_my);
    printf("=====================================================\n");

    return 0;
}
