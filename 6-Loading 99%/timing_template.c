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

/* [กลุ่มแก้] เรียกใช้ ADT ของโปรเจกต์ระบบจองตั๋วเรือ (ไฟล์เดิม ไม่ได้เขียนใหม่) */
#include "hashtable.h"
#include "avltree.h"

/* [กลุ่มแก้] บน Windows ให้ console แสดงภาษาไทย (UTF-8) ได้ถูกต้อง */
#ifdef _WIN32
#include <windows.h>
#endif

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

/* =====================================================================
   จุดที่แต่ละกลุ่มต้องแก้ไข
   แทนที่เนื้อในของฟังก์ชันนี้ด้วยอัลกอริทึมการค้นหาของกลุ่มตนเอง
   เช่น การค้นหาใน BST, AVL Tree, Hash Table หรือวิธีอื่นที่ออกแบบไว้
   ===================================================================== */
/* [กลุ่มแก้] ------------------------------------------------------------
   ADT ของกลุ่มใช้ key เป็น "ข้อความ" (เช่น "BK000001") และเก็บค่าเป็น Booking*
   จึงต้องมีตัวเชื่อม 2 อย่าง
     1) แปลงตัวเลข target เป็นข้อความก่อนค้น  -> IntToKey()
     2) ใช้ Booking เป็น "ป้าย" เก็บตำแหน่งของค่าในไฟล์ไว้ในช่อง seat_no
        เพื่อให้ MySearch คืนตำแหน่งได้เหมือน SequentialSearch
   โครงสร้างทั้งหมดถูกสร้างใน main() ช่วงเตรียมข้อมูล ซึ่งไม่ถูกจับเวลา
   ----------------------------------------------------------------------- */
HashTable *g_hash = NULL;       /* Hash Table ของกลุ่ม (hashtable.c) */
AVLTree   *g_avl  = NULL;       /* AVL Tree ของกลุ่ม  (avltree.c)   */
Booking   *g_tags = NULL;       /* ป้ายเก็บตำแหน่ง 1 ป้ายต่อ 1 ค่า   */

/* แปลงจำนวนเต็มบวกเป็นข้อความฐานสิบ เขียนเองแทน sprintf เพื่อให้เร็ว
   ส่วนนี้ถูกนับรวมในเวลาค้นหาด้วย เพราะเป็นต้นทุนจริงของการใช้ key แบบข้อความ */
void IntToKey(int x, char *out)
{
    char tmp[16];
    int  len = 0, i;
    do { tmp[len++] = (char)('0' + x % 10); x /= 10; } while (x > 0);
    for (i = 0; i < len; i++) out[i] = tmp[len - 1 - i];
    out[len] = '\0';
}

/* อัลกอริทึมที่ 3 ของกลุ่ม: ค้นใน Hash Table  -- O(1) เฉลี่ย */
int MySearchHash(int arr[], int size, int target)
{
    char     key[16];
    Booking *b;
    (void)arr; (void)size;                     /* ไม่ใช้ array ค้นจากโครงสร้างของกลุ่มแทน */
    IntToKey(target, key);
    b = ht_search(g_hash, key);
    return (b != NULL) ? b->seat_no : -1;      /* คืนตำแหน่งที่พบ หรือ -1 */
}

/* อัลกอริทึมที่ 4 ของกลุ่ม: ค้นใน AVL Tree  -- O(log n) */
int MySearchAVL(int arr[], int size, int target)
{
    char     key[16];
    Booking *b;
    (void)arr; (void)size;
    IntToKey(target, key);
    b = avl_search(g_avl, key);
    return (b != NULL) ? b->seat_no : -1;
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
    int    targets[100], tcount, i;
    double ms_seq, ms_bin, ms_hash, ms_avl;   /* [กลุ่มแก้] แยกเวลา Hash กับ AVL */
    int    found_hash = 0, found_avl = 0, pos_ok = 1;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);               /* [กลุ่มแก้] แสดงภาษาไทยบน Windows */
#endif

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

    /* [กลุ่มแก้] สร้าง Hash Table และ AVL Tree ของกลุ่มจากข้อมูลทั้งหมด
       อยู่ในช่วงเตรียมข้อมูล จึงไม่ถูกนับรวมในเวลาค้นหา (ตามกติกาข้อ 1) */
    g_hash = ht_create();
    g_avl  = avl_create();
    g_tags = (Booking *)malloc(sizeof(Booking) * (size_t)n);
    if (!g_hash || !g_avl || !g_tags) { printf("หน่วยความจำไม่พอ\n"); return 1; }
    for (i = 0; i < n; i++) {
        char key[16];
        IntToKey(data[i], key);
        g_tags[i].seat_no = i;                 /* ตำแหน่งของค่านี้ในไฟล์ที่ยังไม่เรียง */
        ht_insert(g_hash, key, &g_tags[i]);
        avl_insert(g_avl,  key, &g_tags[i]);
    }

    printf("=====================================================\n");
    printf(" ไฟล์ข้อมูล      : %s\n", argv[1]);
    printf(" จำนวนข้อมูล n   : %d\n", n);
    printf(" จำนวนค่าที่ค้นหา : %d\n", tcount);
    printf(" จำนวนรอบที่วัด  : %d รอบต่อค่า\n", REPEAT);
    printf("=====================================================\n");

    /* ---- ตรวจความถูกต้องก่อนวัดเวลา ---- */
    printf("\n[ ตรวจความถูกต้องของผลการค้นหา ]\n");
    printf(" %-12s %-10s %-10s %-10s %-10s\n", "ค่าที่ค้นหา", "Sequential", "Binary",
           "Hash", "AVL");                                          /* [กลุ่มแก้] */
    for (i = 0; i < tcount; i++) {
        int a = SequentialSearch(data, n, targets[i]);
        int b = BinarySearch(sorted_data, n, targets[i]);
        int h = MySearchHash(data, n, targets[i]);                  /* [กลุ่มแก้] */
        int v = MySearchAVL(data, n, targets[i]);                   /* [กลุ่มแก้] */
        printf(" %-12d %-10s %-10s %-10s %-10s\n", targets[i],
               (a >= 0) ? "พบ" : "ไม่พบ",
               (b >= 0) ? "พบ" : "ไม่พบ",
               (h >= 0) ? "พบ" : "ไม่พบ",
               (v >= 0) ? "พบ" : "ไม่พบ");
        /* [กลุ่มแก้] ตำแหน่งที่ Hash/AVL คืนมาต้องตรงกับ Sequential ทุกค่า */
        if (h >= 0) found_hash++;
        if (v >= 0) found_avl++;
        if (h != a || v != a) pos_ok = 0;
    }
    printf("\n สรุป: Hash พบ %d ไม่พบ %d | AVL พบ %d ไม่พบ %d | ตำแหน่งตรงกับ Sequential: %s\n",
           found_hash, tcount - found_hash, found_avl, tcount - found_avl,
           pos_ok ? "ถูกต้องทุกค่า" : "ไม่ตรง (โปรแกรมผิด)");   /* [กลุ่มแก้] */

    /* ---- วัดเวลา ---- */
    ms_seq = MeasureMillisec(SequentialSearch, data,        n, targets, tcount);
    ms_bin = MeasureMillisec(BinarySearch,     sorted_data, n, targets, tcount);
    ms_hash = MeasureMillisec(MySearchHash,    data,        n, targets, tcount); /* [กลุ่มแก้] */
    ms_avl  = MeasureMillisec(MySearchAVL,     data,        n, targets, tcount); /* [กลุ่มแก้] */

    printf("\n[ ผลการวัดเวลา เฉลี่ยต่อการค้นหาหนึ่งครั้ง ]\n");
    printf(" Sequential Search : %.6f ms\n", ms_seq);
    printf(" Binary Search     : %.6f ms\n", ms_bin);
    printf(" Hash Table (กลุ่ม) : %.6f ms\n", ms_hash);   /* [กลุ่มแก้] */
    printf(" AVL Tree (กลุ่ม)   : %.6f ms\n", ms_avl);    /* [กลุ่มแก้] */
    printf("\nนำค่าที่ได้ไปกรอกในตารางบันทึกผล แล้วรันซ้ำจนครบ 5 ครั้ง\n");

    ht_destroy(g_hash);                        /* [กลุ่มแก้] คืนหน่วยความจำ */
    avl_destroy(g_avl);
    free(g_tags);
    return 0;
}
