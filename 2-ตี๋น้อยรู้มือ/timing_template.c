/* =====================================================================
   timing_template.c  —  โปรแกรมต้นแบบสำหรับวัดเวลาการค้นหา
   รายวิชา 01204212 แบบชนิดข้อมูลนามธรรมและการแก้ปัญหา
   ใช้ประกอบกิจกรรมนำเสนอโครงงาน วันที่ 5 ตุลาคม 2569

   วิธีคอมไพล์ :  gcc timing_template.c -o timing
   วิธีรัน      :  ./timing data_1000.txt targets_1000.txt
                  (บน Windows ใช้  timing.exe data_1000.txt targets_1000.txt)

   ให้แต่ละกลุ่มแทนที่ฟังก์ชัน MySearch ด้วยอัลกอริทึมการค้นหาของกลุ่มตนเอง
   ===================================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define MAXN   100000
#define REPEAT 10000      /* จำนวนรอบที่วนค้นหาซ้ำ (10,000 รอบ เพื่อความแม่นยำของนาฬิการะบบ) */

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

/* =====================================================================
   จุดที่แต่ละกลุ่มต้องแก้ไข
   แทนที่เนื้อในของฟังก์ชันนี้ด้วยอัลกอริทึมการค้นหาของกลุ่มตนเอง
   กลุ่ม Teenoi Suki: ค้นหาด้วย Binary Search Tree (BST)
   ===================================================================== */
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
    InitBST(sorted_data, n); /* สร้าง Balanced BST ล่วงหน้าในขั้นตอนเตรียมข้อมูล */

    printf("=====================================================\n");
    printf(" ไฟล์ข้อมูล      : %s\n", argv[1]);
    printf(" จำนวนข้อมูล n   : %d\n", n);
    printf(" จำนวนค่าที่ค้นหา : %d\n", tcount);
    printf(" จำนวนรอบที่วัด  : %d รอบต่อค่า\n", REPEAT);
    printf("=====================================================\n");

    /* ---- ตรวจความถูกต้องก่อนวัดเวลา ---- */
    printf("\n[ ตรวจความถูกต้องของผลการค้นหา ]\n");
    printf(" %-12s %-12s %-12s %-16s\n", "ค่าที่ค้นหา", "Sequential", "Binary", "MySearch (BST)");
    for (i = 0; i < tcount; i++) {
        int a = SequentialSearch(data, n, targets[i]);
        int b = BinarySearch(sorted_data, n, targets[i]);
        int c = MySearch(data, n, targets[i]);
        printf(" %-12d %-12s %-12s %-16s\n", targets[i],
               (a >= 0) ? "พบ" : "ไม่พบ",
               (b >= 0) ? "พบ" : "ไม่พบ",
               (c >= 0) ? "พบ" : "ไม่พบ");
    }

    /* ---- วัดเวลา ---- */
    ms_seq = MeasureMillisec(SequentialSearch, data,        n, targets, tcount);
    ms_bin = MeasureMillisec(BinarySearch,     sorted_data, n, targets, tcount);
    ms_my  = MeasureMillisec(MySearch,         data,        n, targets, tcount);

    printf("\n[ ผลการวัดเวลา เฉลี่ยต่อการค้นหาหนึ่งครั้ง ]\n");
    printf(" Sequential Search   : %.6f ms\n", ms_seq);
    printf(" Binary Search       : %.6f ms\n", ms_bin);
    printf(" MySearch (BST กลุ่ม) : %.6f ms\n", ms_my);
    printf("\nนำค่าที่ได้ไปกรอกในตารางบันทึกผล แล้วรันซ้ำจนครบ 5 ครั้ง\n");

    return 0;
}
