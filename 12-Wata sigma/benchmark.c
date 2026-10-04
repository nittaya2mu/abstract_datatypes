/*
 * benchmark.c — ทดสอบประสิทธิภาพและวิเคราะห์ Big O ของ ledger.c
 * ทดสอบที่ n = 100, 1000, 10000 รายการ
 *
 * แต่ละการทำงานจะรันซ้ำหลายรอบ (reps) แล้วหาเวลาเฉลี่ยต่อ 1 ครั้ง
 * และรันทั้งชุด 3 รอบ เลือกรอบที่เร็วที่สุด เพื่อลดสัญญาณรบกวนจากระบบ
 *
 * คอมไพล์: gcc -O2 -Wall -o benchmark benchmark.c
 * (ไฟล์นี้ include ledger.c โดยตรง จึงใช้โค้ดจริงชุดเดียวกัน)
 */
#define main ledger_main
#include "ledger.c"
#undef main

#define NSIZES 3
#define ROUNDS 3
#define NOPS   7

static volatile long long sink_d;
static volatile int    sink_i;

static const char *CATS[] = {
    "อาหาร", "เดินทาง", "หนังสือ", "ค่าน้ำ", "ค่าไฟ", "ค่าโทรศัพท์", "เสื้อผ้า",
    "บันเทิง", "สุขภาพ", "ของใช้", "ขนม", "เกม", "ท่องเที่ยว", "การศึกษา",
    "เงินเดือน", "รายได้พิเศษ", "ของขวัญ", "ซ่อมแซม", "ออมเงิน", "อื่น ๆ"
};
#define NCATS ((int)(sizeof CATS / sizeof CATS[0]))

static double now_us(void) { return 1e6 * (double)clock() / CLOCKS_PER_SEC; }

/* สร้างข้อมูลทดสอบ n รายการ (สุ่มแบบกำหนด seed ให้ผลซ้ำได้) */
static void build(Ledger *L, int n) {
    srand(42);
    for (int i = 0; i < n; i++) {
        Transaction t = {0};
        unsigned m = 1u + (unsigned)rand() % 12u;
        unsigned d = 1u + (unsigned)rand() % 28u;
        t.id = L->next_id;
        snprintf(t.date, sizeof t.date, "2026-%02u-%02u", m, d);
        t.is_income = rand() % 5 == 0;
        copy_str(t.category, sizeof t.category, CATS[rand() % NCATS]);
        t.amount = (1 + rand() % 5000) * 100LL;   /* สตางค์ */
        copy_str(t.note, sizeof t.note, "test");
        ledger_push(L, t);
    }
}

/* ---- แต่ละฟังก์ชันคืนค่าเวลาเฉลี่ยต่อ 1 การทำงาน (ไมโครวินาที) ---- */

/* 0) เพิ่ม n รายการ (เวลาเฉลี่ยต่อ 1 รายการ, รวมการขยายอาร์เรย์) */
static double bench_add(int n) {
    int trials = 2000000 / n;
    Transaction t = {0};
    copy_str(t.date, sizeof t.date, "2026-09-01");
    copy_str(t.category, sizeof t.category, "อาหาร");
    t.amount = 5000;   /* 50.00 บาท */
    double t0 = now_us();
    for (int r = 0; r < trials; r++) {
        Ledger L;
        ledger_init(&L);
        for (int i = 0; i < n; i++) { t.id = i + 1; ledger_push(&L, t); }
        sink_i += L.size;
        free(L.data);
    }
    return (now_us() - t0) / ((double)trials * n);
}

/* 1) ค้นหาตาม id กรณีแย่สุด (id อยู่ท้ายอาร์เรย์) */
static double bench_find(Ledger *L, int n) {
    int reps = 20000000 / n;
    double t0 = now_us();
    for (int r = 0; r < reps; r++) sink_i += find_index(L, n - (r & 1));
    return (now_us() - t0) / reps;
}

/* 2) ลบรายการแรก (กรณีแย่สุด: ต้องเลื่อนสมาชิกที่เหลือทั้งหมด) แล้วเติมท้ายกลับ
 *    หมายเหตุ: ฟังก์ชันนี้แก้ข้อมูลใน L จึงต้องเรียกกับอาร์เรย์ที่สร้างแยกไว้ใช้เฉพาะการลบ */
static double bench_delete(Ledger *L, int n) {
    int reps = 4000000 / n;
    Transaction tmpl = L->data[0];
    double t0 = now_us();
    for (int r = 0; r < reps; r++) {
        ledger_remove(L, L->data[0].id);
        tmpl.id = L->next_id;
        ledger_push(L, tmpl);
    }
    (void)n;
    return (now_us() - t0) / reps;
}

/* 3) สรุปรายเดือน */
static double bench_month(Ledger *L, int n) {
    int reps = 20000000 / n;
    long long inc, exp;
    double t0 = now_us();
    for (int r = 0; r < reps; r++) {
        compute_totals(L, 'm', "2026-06", &inc, &exp);
        sink_d += inc + exp;
    }
    return (now_us() - t0) / reps;
}

/* 4) สรุปรายสัปดาห์ (ค่าคงที่สูงกว่า เพราะต้องคำนวณเลขสัปดาห์ด้วย mktime ทุกรายการ) */
static double bench_week(Ledger *L, int n) {
    int reps = 500000 / n;
    long long inc, exp;
    double t0 = now_us();
    for (int r = 0; r < reps; r++) {
        compute_totals(L, 'w', "2026-W20", &inc, &exp);
        sink_d += inc + exp;
    }
    return (now_us() - t0) / reps;
}

/* 5) เรียงตามวันที่ (คัดลอกอาร์เรย์แล้ว qsort) */
static double bench_sort(Ledger *L, int n) {
    int reps = 2000000 / n;
    Transaction *tmp = malloc((size_t)n * sizeof *tmp);
    if (!tmp) { perror("malloc"); exit(1); }
    double t0 = now_us();
    for (int r = 0; r < reps; r++) {
        memcpy(tmp, L->data, (size_t)n * sizeof *tmp);
        qsort(tmp, (size_t)n, sizeof *tmp, cmp_date);
        sink_i += tmp[0].id;
    }
    double per = (now_us() - t0) / reps;
    free(tmp);
    return per;
}

/* 6) สรุปตามหมวดหมู่ (คัดลอก + qsort ตามหมวด + วนจัดกลุ่ม) */
static double bench_category(Ledger *L, int n) {
    int reps = 2000000 / n;
    Transaction *tmp = malloc((size_t)n * sizeof *tmp);
    if (!tmp) { perror("malloc"); exit(1); }
    double t0 = now_us();
    for (int r = 0; r < reps; r++) {
        memcpy(tmp, L->data, (size_t)n * sizeof *tmp);
        qsort(tmp, (size_t)n, sizeof *tmp, cmp_category);
        int groups = 0;
        for (int i = 0; i < n; i++)
            if (i == 0 || strcmp(tmp[i].category, tmp[i - 1].category) != 0) groups++;
        sink_i += groups;
    }
    double per = (now_us() - t0) / reps;
    free(tmp);
    return per;
}

int main(void) {
    const int sizes[NSIZES] = {100, 1000, 10000};
    const char *names[NOPS] = {
        "เพิ่มรายการ (ต่อ 1 รายการ)", "ค้นหาตาม id (แย่สุด)", "ลบรายการแรก (แย่สุด)",
        "สรุปรายเดือน", "สรุปรายสัปดาห์", "เรียงตามวันที่", "สรุปตามหมวดหมู่"
    };
    const char *expected[NOPS] = {
        "O(1) เฉลี่ย", "O(n)", "O(n)", "O(n)", "O(n)", "O(n log n)", "O(n log n)"
    };
    double us[NOPS][NSIZES];

    for (int s = 0; s < NSIZES; s++) {
        int n = sizes[s];
        for (int op = 0; op < NOPS; op++) us[op][s] = 1e300;

        for (int round = 0; round < ROUNDS; round++) {
            Ledger L;
            ledger_init(&L);
            build(&L, n);
            double v[NOPS];
            v[0] = bench_add(n);
            v[1] = bench_find(&L, n);
            /* การลบแก้ข้อมูลในอาร์เรย์ (เติมสำเนากลับท้าย) ถ้าทำบน L เดียวกัน
             * ข้อมูลที่ benchmark ถัดไปใช้จะเพี้ยน (ที่ n เล็กทุกรายการกลายเป็นสำเนาเดียวกัน)
             * จึงลบบนอาร์เรย์แยกต่างหาก */
            Ledger D;
            ledger_init(&D);
            build(&D, n);
            v[2] = bench_delete(&D, n);
            free(D.data);
            v[3] = bench_month(&L, n);
            v[4] = bench_week(&L, n);
            v[5] = bench_sort(&L, n);
            v[6] = bench_category(&L, n);
            for (int op = 0; op < NOPS; op++)
                if (v[op] < us[op][s]) us[op][s] = v[op];   /* เก็บรอบที่เร็วสุด */
            free(L.data);
        }
        fprintf(stderr, "เสร็จ n = %d\n", n);
    }

    printf("\nเวลาเฉลี่ยต่อ 1 การทำงาน (ไมโครวินาที, ยิ่งน้อยยิ่งเร็ว)\n");
    printf("%-30s %12s %12s %12s %10s %10s  %s\n", "การทำงาน", "n=100", "n=1000",
           "n=10000", "x(1k/100)", "x(10k/1k)", "คาดว่า");
    for (int op = 0; op < NOPS; op++)
        printf("%-30s %12.3f %12.3f %12.3f %10.1f %10.1f  %s\n", names[op],
               us[op][0], us[op][1], us[op][2],
               us[op][1] / us[op][0], us[op][2] / us[op][1], expected[op]);

    puts("\nอัตราส่วนที่คาดหวังเมื่อ n เพิ่มขึ้น 10 เท่า:");
    puts("  O(1)       ~1x");
    puts("  O(n)       ~10x");
    puts("  O(n log n) ~15x (100->1000) และ ~13x (1000->10000)");
    return 0;
}
