/*
 * ระบบบันทึกรายรับ-รายจ่ายส่วนตัว (Personal Income/Expense Tracker)
 * กลุ่ม Wata sigma
 *
 * โครงสร้างข้อมูล: Dynamic Array (อาร์เรย์ที่ขยายขนาดอัตโนมัติ)
 *  - เพิ่มท้ายอาร์เรย์: O(1) โดยเฉลี่ย (ขยายขนาดเป็น 2 เท่าเมื่อเต็ม)
 *  - เข้าถึงตาม index: O(1)
 *  - ค้นหา/แก้ไข/ลบตาม id: O(n)
 *
 * ฟังก์ชัน: เพิ่ม / แก้ไข / ลบ / ดู / สรุปรายวัน-สัปดาห์-เดือน-หมวดหมู่
 *
 * คอมไพล์: gcc -O2 -Wall -o ledger ledger.c
 *
 * หมายเหตุการออกแบบ
 *  - เก็บจำนวนเงินเป็นจำนวนเต็มหน่วยสตางค์ (long long) ไม่ใช้ double
 *    เพื่อไม่ให้ผลรวมคลาดเคลื่อน
 *  - ข้อความภาษาไทยเป็น UTF-8 (1 ตัวอักษร = 3 ไบต์) การตัดสตริงจึงต้องไม่ตัดกลางตัวอักษร
 *  - เพิ่มรายการใหม่ใช้การ append ท้ายไฟล์ (O(1)) ส่วนแก้ไข/ลบเขียนไฟล์ใหม่ทั้งไฟล์
 *    โดยเขียนลงไฟล์ชั่วคราวก่อนแล้วค่อยแทนที่ไฟล์จริง
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>   /* ใช้ตั้งค่า console ให้แสดงภาษาไทย (UTF-8) บน Windows */
#endif

#define DATA_FILE "ledger.txt"
#define TMP_FILE  "ledger.txt.tmp"

#define MAX_SATANG 100000000000LL   /* จำนวนเงินสูงสุดต่อรายการ 1,000,000,000.00 */
#define YEAR_MIN 1970
#define YEAR_MAX 2100

typedef struct {
    int id;
    char date[11];              /* YYYY-MM-DD */
    int is_income;              /* 1 = รายรับ, 0 = รายจ่าย */
    char category[64];
    long long amount;           /* หน่วยสตางค์ (12000 = 120.00 บาท) */
    char note[128];
} Transaction;

typedef struct {
    Transaction *data;          /* อาร์เรย์เก็บรายการ */
    int size, capacity, next_id;
} Ledger;

/* ---------- สตริง UTF-8 ---------- */

/* คัดลอกสตริงแบบปลอดภัย: ตัดถ้ายาวเกิน buffer แต่ไม่ตัดกลางตัวอักษร UTF-8 */
static void copy_str(char *dst, size_t n, const char *src) {
    size_t len = strlen(src);
    if (len >= n) {
        len = n - 1;
        while (len > 0 && ((unsigned char)src[len] & 0xC0) == 0x80) len--;
    }
    memcpy(dst, src, len);
    dst[len] = '\0';
}

/* ถ้าท้ายสตริงเป็นตัวอักษร UTF-8 ที่มาไม่ครบ (เพราะบัฟเฟอร์เต็ม) ให้ตัดตัวนั้นทิ้ง */
static void trim_partial_utf8(char *s) {
    size_t len = strlen(s), i = len;
    while (i > 0 && ((unsigned char)s[i - 1] & 0xC0) == 0x80) i--;   /* ถอยไปหาไบต์นำ */
    if (i == 0) return;
    unsigned char lead = (unsigned char)s[i - 1];
    size_t need = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : lead >= 0xC0 ? 2 : 1;
    if (len - (i - 1) < need) s[i - 1] = '\0';
}

/* ---------- วันที่ / สัปดาห์ (ISO week) ---------- */
static int is_leap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

static int is_digits(const char *s, int from, int to) {
    for (int i = from; i < to; i++)
        if (s[i] < '0' || s[i] > '9') return 0;
    return 1;
}

/* รูปแบบเข้มงวด: ต้องเป็น YYYY-MM-DD เท่านั้น ปี ค.ศ. 1970-2100 และเป็นวันที่มีจริง */
static int parse_date(const char *s, struct tm *out) {
    if (strlen(s) != 10 || s[4] != '-' || s[7] != '-' ||
        !is_digits(s, 0, 4) || !is_digits(s, 5, 7) || !is_digits(s, 8, 10)) return 0;
    int y = atoi(s), m = atoi(s + 5), d = atoi(s + 8);
    if (y < YEAR_MIN || y > YEAR_MAX) return 0;
    struct tm tm = {0};
    tm.tm_year = y - 1900; tm.tm_mon = m - 1; tm.tm_mday = d; tm.tm_hour = 12;
    struct tm chk = tm;
    if (mktime(&chk) == (time_t)-1) return 0;
    if (chk.tm_year != tm.tm_year || chk.tm_mon != tm.tm_mon ||
        chk.tm_mday != tm.tm_mday) return 0;        /* เช่น 2026-02-31 */
    if (out) *out = chk;
    return 1;
}

static int valid_month_key(const char *s) {          /* YYYY-MM */
    if (strlen(s) != 7 || s[4] != '-' || !is_digits(s, 0, 4) || !is_digits(s, 5, 7)) return 0;
    int y = atoi(s), m = atoi(s + 5);
    return y >= YEAR_MIN && y <= YEAR_MAX && m >= 1 && m <= 12;
}

static int valid_week_key(const char *s) {           /* YYYY-Www */
    if (strlen(s) != 8 || s[4] != '-' || s[5] != 'W' ||
        !is_digits(s, 0, 4) || !is_digits(s, 6, 8)) return 0;
    int y = atoi(s), w = atoi(s + 6);
    return y >= YEAR_MIN && y <= YEAR_MAX && w >= 1 && w <= 53;
}

static int week_key(const char *date, char *out, size_t n) {
    struct tm tm;
    if (!parse_date(date, &tm)) return 0;
    int wday = (tm.tm_wday + 6) % 7;                /* จันทร์ = 0 */
    int y = tm.tm_year + 1900;
    int thu = tm.tm_yday - wday + 3;                /* วันพฤหัสของสัปดาห์นั้น */
    if (thu < 0) { y--; thu += is_leap(y) ? 366 : 365; }
    else if (thu >= (is_leap(y) ? 366 : 365)) { thu -= is_leap(y) ? 366 : 365; y++; }
    snprintf(out, n, "%d-W%02d", y, thu / 7 + 1);
    return 1;
}

static void today_str(char *out, size_t n) {
    time_t t = time(NULL);
    strftime(out, n, "%Y-%m-%d", localtime(&t));
}

/* ---------- จำนวนเงิน (หน่วยสตางค์) ---------- */

/* รับเฉพาะตัวเลข เช่น 120 หรือ 45.5 หรือ 45.50 (ทศนิยมไม่เกิน 2 ตำแหน่ง, > 0) */
static int parse_money(const char *s, long long *out) {
    long long whole = 0, frac = 0;
    int nd = 0, fd = 0;
    const char *p = s;
    while (*p >= '0' && *p <= '9') {
        whole = whole * 10 + (*p - '0');
        if (whole > MAX_SATANG / 100) return 0;
        p++; nd++;
    }
    if (nd == 0) return 0;
    if (*p == '.') {
        p++;
        while (*p >= '0' && *p <= '9') {
            if (fd >= 2) return 0;
            frac = frac * 10 + (*p - '0');
            p++; fd++;
        }
        if (fd == 0) return 0;
        if (fd == 1) frac *= 10;
    }
    if (*p != '\0') return 0;
    long long v = whole * 100 + frac;
    if (v <= 0 || v > MAX_SATANG) return 0;
    *out = v;
    return 1;
}

/* แปลงสตางค์เป็นข้อความ เช่น -1234.56 (รองรับค่าติดลบของยอดคงเหลือ) */
static void money_str(char *out, size_t n, long long v) {
    const char *sign = v < 0 ? "-" : "";
    if (v < 0) v = -v;
    snprintf(out, n, "%s%lld.%02lld", sign, v / 100, v % 100);
}

/* id ต้องเป็นจำนวนเต็มบวกล้วน ๆ */
static int parse_id(const char *s, int *out) {
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || v <= 0 || v > 2000000000L) return 0;
    *out = (int)v;
    return 1;
}

/* ---------- Ledger (Dynamic Array) ---------- */
static void ledger_init(Ledger *L) {
    L->capacity = 16;
    L->size = 0;
    L->next_id = 1;
    L->data = malloc(L->capacity * sizeof *L->data);
    if (!L->data) { perror("malloc"); exit(1); }
}

static void ledger_push(Ledger *L, Transaction t) {
    if (L->size == L->capacity) {                   /* เต็ม -> ขยาย 2 เท่า */
        L->capacity *= 2;
        Transaction *p = realloc(L->data, L->capacity * sizeof *L->data);
        if (!p) { perror("realloc"); exit(1); }
        L->data = p;
    }
    L->data[L->size++] = t;
    if (t.id >= L->next_id) L->next_id = t.id + 1;
}

static int find_index(const Ledger *L, int id) {
    for (int i = 0; i < L->size; i++)
        if (L->data[i].id == id) return i;
    return -1;
}

static int ledger_remove(Ledger *L, int id) {
    int i = find_index(L, id);
    if (i < 0) return 0;
    memmove(&L->data[i], &L->data[i + 1], (L->size - i - 1) * sizeof *L->data);
    L->size--;
    return 1;
}

/* ---------- บันทึก / โหลดไฟล์ ---------- */
/* รูปแบบ 1 บรรทัด: id|date|is_income|category|amount|note */
static void write_line(FILE *f, const Transaction *t) {
    char amt[32];
    money_str(amt, sizeof amt, t->amount);
    fprintf(f, "%d|%s|%d|%s|%s|%s\n", t->id, t->date, t->is_income,
            t->category, amt, t->note);
}

/* เขียนทั้งไฟล์: เขียนลงไฟล์ชั่วคราวก่อน แล้วค่อยแทนที่ไฟล์จริง
 * ถ้าโปรแกรมหยุดกลางคัน ledger.txt เดิมยังอยู่ครบ */
static int ledger_save(const Ledger *L) {
    FILE *f = fopen(TMP_FILE, "w");
    if (!f) { perror("เปิดไฟล์ไม่ได้"); return 0; }
    for (int i = 0; i < L->size; i++) write_line(f, &L->data[i]);
    int bad = ferror(f);
    if (fclose(f) != 0) bad = 1;
    if (bad) { puts("เขียนไฟล์ไม่สำเร็จ (ข้อมูลเดิมยังอยู่)"); remove(TMP_FILE); return 0; }
#ifdef _WIN32
    if (!MoveFileExA(TMP_FILE, DATA_FILE, MOVEFILE_REPLACE_EXISTING)) {
        puts("แทนที่ไฟล์ข้อมูลไม่สำเร็จ"); return 0;
    }
#else
    if (rename(TMP_FILE, DATA_FILE) != 0) { perror("แทนที่ไฟล์ข้อมูลไม่สำเร็จ"); return 0; }
#endif
    return 1;
}

/* เพิ่มรายการเดียวต่อท้ายไฟล์ (O(1)) */
static int ledger_append_file(const Transaction *t) {
    FILE *f = fopen(DATA_FILE, "a");
    if (!f) { perror("เปิดไฟล์ไม่ได้"); return 0; }
    write_line(f, t);
    int bad = ferror(f);
    if (fclose(f) != 0) bad = 1;
    return !bad;
}

static void ledger_load(Ledger *L) {
    FILE *f = fopen(DATA_FILE, "r");
    if (!f) return;
    char line[512];
    int first = 1, skipped = 0;
    while (fgets(line, sizeof line, f)) {
        if (first) {                                /* ข้าม BOM (EF BB BF) ที่ Notepad อาจใส่มา */
            first = 0;
            if ((unsigned char)line[0] == 0xEF && (unsigned char)line[1] == 0xBB &&
                (unsigned char)line[2] == 0xBF)
                memmove(line, line + 3, strlen(line + 3) + 1);
        }
        if (line[strspn(line, " \t\r\n")] == '\0') continue;   /* บรรทัดว่าง */

        char *fld[6], *p = line;
        int n = 0;
        while (n < 6) {
            fld[n++] = p;
            char *bar = strchr(p, '|');
            if (!bar) break;
            *bar = '\0';
            p = bar + 1;
        }
        int id;
        long long amount;
        if (n < 6) { skipped++; continue; }
        fld[5][strcspn(fld[5], "\r\n")] = '\0';
        if (!parse_id(fld[0], &id) || !parse_date(fld[1], NULL) ||
            !parse_money(fld[4], &amount)) { skipped++; continue; }

        Transaction t = {0};
        t.id = id;
        snprintf(t.date, sizeof t.date, "%s", fld[1]);
        t.is_income = atoi(fld[2]) != 0;
        copy_str(t.category, sizeof t.category, fld[3]);
        t.amount = amount;
        copy_str(t.note, sizeof t.note, fld[5]);
        ledger_push(L, t);
    }
    fclose(f);
    if (skipped) printf("คำเตือน: ข้าม %d บรรทัดในไฟล์ข้อมูลที่อ่านไม่ได้\n", skipped);
}

/* ---------- Input helpers ---------- */
/* คืน 0 เมื่อ stdin ปิด (EOF) เพื่อให้ main ออกจากลูปได้ */
static int read_line(const char *prompt, char *buf, size_t n) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, (int)n, stdin)) { buf[0] = '\0'; return 0; }
    size_t len = strcspn(buf, "\r\n");
    if (buf[len] == '\0' && len == n - 1) {         /* บรรทัดยาวเกิน: ทิ้งส่วนที่เหลือ ไม่ให้ค้างไปคำถามถัดไป */
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
        trim_partial_utf8(buf);
    }
    buf[len] = '\0';
    for (char *c = buf; *c; c++) if (*c == '|') *c = ' ';   /* กันชนตัวคั่นไฟล์ */
    return 1;
}

/* ---------- เมนูการทำงาน ---------- */
static void add_flow(Ledger *L, int is_income) {
    Transaction t = {0};
    char buf[128], today[16];
    today_str(today, sizeof today);

    read_line("วันที่ (YYYY-MM-DD ปี ค.ศ., เว้นว่าง = วันนี้): ", buf, sizeof buf);
    if (!buf[0]) snprintf(buf, sizeof buf, "%s", today);
    if (!parse_date(buf, NULL)) {
        printf("รูปแบบวันที่ไม่ถูกต้อง (ต้องเป็น YYYY-MM-DD ปี ค.ศ. %d-%d)\n", YEAR_MIN, YEAR_MAX);
        return;
    }
    copy_str(t.date, sizeof t.date, buf);

    read_line("หมวดหมู่ (เช่น อาหาร, เดินทาง, เงินเดือน): ", buf, sizeof buf);
    copy_str(t.category, sizeof t.category, buf[0] ? buf : "อื่น ๆ");

    read_line("จำนวนเงิน (เช่น 120 หรือ 45.50): ", buf, sizeof buf);
    if (!parse_money(buf, &t.amount)) {
        puts("จำนวนเงินไม่ถูกต้อง (ตัวเลขมากกว่า 0 ทศนิยมไม่เกิน 2 ตำแหน่ง)");
        return;
    }

    read_line("โน้ต (ไม่ใส่ก็ได้): ", t.note, sizeof t.note);

    t.id = L->next_id;
    t.is_income = is_income;
    ledger_push(L, t);
    if (ledger_append_file(&t)) printf("บันทึกแล้ว (id=%d)\n", t.id);
    else puts("คำเตือน: เพิ่มรายการในหน่วยความจำแล้ว แต่เขียนลงไฟล์ไม่สำเร็จ");
}

static void edit_flow(Ledger *L) {
    char buf[128];
    int id;
    read_line("id ที่ต้องการแก้ไข: ", buf, sizeof buf);
    int i = parse_id(buf, &id) ? find_index(L, id) : -1;
    if (i < 0) { puts("ไม่พบรายการ"); return; }

    Transaction t = L->data[i];                     /* แก้ในสำเนาก่อน ถ้าผิดพลาดจะไม่กระทบของเดิม */
    printf("แก้ไขรายการ #%d (เว้นว่าง = ไม่เปลี่ยน)\n", t.id);

    printf("วันที่เดิม: %s\n", t.date);
    read_line("วันที่ใหม่: ", buf, sizeof buf);
    if (buf[0]) {
        if (!parse_date(buf, NULL)) { puts("รูปแบบวันที่ไม่ถูกต้อง"); return; }
        copy_str(t.date, sizeof t.date, buf);
    }

    printf("ประเภทเดิม: %s\n", t.is_income ? "รายรับ" : "รายจ่าย");
    read_line("ประเภทใหม่ (1 = รายรับ, 2 = รายจ่าย): ", buf, sizeof buf);
    if (buf[0] == '1' && !buf[1]) t.is_income = 1;
    else if (buf[0] == '2' && !buf[1]) t.is_income = 0;
    else if (buf[0]) { puts("ตัวเลือกไม่ถูกต้อง"); return; }

    printf("หมวดหมู่เดิม: %s\n", t.category);
    read_line("หมวดหมู่ใหม่: ", buf, sizeof buf);
    if (buf[0]) copy_str(t.category, sizeof t.category, buf);

    char amt[32];
    money_str(amt, sizeof amt, t.amount);
    printf("จำนวนเงินเดิม: %s\n", amt);
    read_line("จำนวนเงินใหม่: ", buf, sizeof buf);
    if (buf[0] && !parse_money(buf, &t.amount)) { puts("จำนวนเงินไม่ถูกต้อง"); return; }

    printf("โน้ตเดิม: %s\n", t.note);
    read_line("โน้ตใหม่ (พิมพ์ - เพื่อล้างโน้ต): ", buf, sizeof buf);
    if (strcmp(buf, "-") == 0) t.note[0] = '\0';
    else if (buf[0]) copy_str(t.note, sizeof t.note, buf);

    L->data[i] = t;
    if (ledger_save(L)) puts("แก้ไขแล้ว");
}

static void delete_flow(Ledger *L) {
    char buf[32];
    int id;
    read_line("id ที่ต้องการลบ: ", buf, sizeof buf);
    if (parse_id(buf, &id) && ledger_remove(L, id)) {
        if (ledger_save(L)) puts("ลบแล้ว");
    } else puts("ไม่พบรายการ");
}

static int cmp_date(const void *a, const void *b) {
    const Transaction *x = a, *y = b;
    int c = strcmp(x->date, y->date);
    return c ? c : x->id - y->id;
}

static int cmp_category(const void *a, const void *b) {
    return strcmp(((const Transaction *)a)->category, ((const Transaction *)b)->category);
}

static void list_flow(const Ledger *L) {
    if (L->size == 0) { puts("ยังไม่มีรายการ"); return; }
    Transaction *tmp = malloc(L->size * sizeof *tmp);   /* เรียงบนสำเนา ไม่แก้ลำดับข้อมูลจริง */
    if (!tmp) { perror("malloc"); exit(1); }
    memcpy(tmp, L->data, L->size * sizeof *tmp);
    qsort(tmp, L->size, sizeof *tmp, cmp_date);

    long long inc = 0, spd = 0;
    char amt[32];
    for (int i = 0; i < L->size; i++) {
        const Transaction *t = &tmp[i];
        money_str(amt, sizeof amt, t->amount);
        printf("#%-3d %s  %c%12s  %s  %s\n", t->id, t->date,
               t->is_income ? '+' : '-', amt, t->category, t->note);
        if (t->is_income) inc += t->amount; else spd += t->amount;
    }
    char a[32], b[32], c[32];
    money_str(a, sizeof a, inc); money_str(b, sizeof b, spd); money_str(c, sizeof c, inc - spd);
    printf("\nรวมทั้งหมด: รายรับ %s | รายจ่าย %s | คงเหลือ %s\n", a, b, c);
    free(tmp);
}

/* สรุปตามช่วงเวลา: mode 'd' = วัน, 'w' = สัปดาห์, 'm' = เดือน */
static void compute_totals(const Ledger *L, char mode, const char *key,
                           long long *out_inc, long long *out_exp) {
    long long inc = 0, spd = 0;
    char wk[32];
    for (int i = 0; i < L->size; i++) {
        const Transaction *t = &L->data[i];
        int match = 0;
        if (mode == 'd') match = strcmp(t->date, key) == 0;
        else if (mode == 'm') match = strlen(key) == 7 && strncmp(t->date, key, 7) == 0;
        else if (week_key(t->date, wk, sizeof wk)) match = strcmp(wk, key) == 0;
        if (!match) continue;
        if (t->is_income) inc += t->amount; else spd += t->amount;
    }
    *out_inc = inc;
    *out_exp = spd;
}

static void summarize(const Ledger *L, char mode, const char *title, const char *key) {
    long long inc, spd;
    char a[32], b[32], c[32];
    compute_totals(L, mode, key, &inc, &spd);
    money_str(a, sizeof a, inc); money_str(b, sizeof b, spd); money_str(c, sizeof c, inc - spd);
    printf("\n[%s: %s]\n", title, key);
    printf("  รายรับ   %12s\n  รายจ่าย  %12s\n  คงเหลือ  %12s\n", a, b, c);
}

static void category_summary(const Ledger *L) {
    if (L->size == 0) { puts("ยังไม่มีรายการ"); return; }
    Transaction *tmp = malloc(L->size * sizeof *tmp);
    if (!tmp) { perror("malloc"); exit(1); }
    memcpy(tmp, L->data, L->size * sizeof *tmp);
    qsort(tmp, L->size, sizeof *tmp, cmp_category);        /* จัดกลุ่มตามหมวดหมู่ */

    puts("\nสรุปตามหมวดหมู่");
    char amt[32];
    for (int i = 0; i < L->size; ) {
        long long inc = 0, spd = 0;
        int j = i;
        while (j < L->size && strcmp(tmp[j].category, tmp[i].category) == 0) {
            if (tmp[j].is_income) inc += tmp[j].amount; else spd += tmp[j].amount;
            j++;
        }
        printf("  %s:", tmp[i].category);
        if (inc != 0) { money_str(amt, sizeof amt, inc); printf("  รับ %s", amt); }
        if (spd != 0) { money_str(amt, sizeof amt, spd); printf("  จ่าย %s", amt); }
        putchar('\n');
        i = j;
    }
    free(tmp);
}

static void summary_flow(const Ledger *L) {
    char c[16], key[64], today[16], wk[32];
    today_str(today, sizeof today);
    puts("1) รายวัน  2) รายสัปดาห์  3) รายเดือน  4) แยกตามหมวดหมู่");
    read_line("เลือก: ", c, sizeof c);

    if (c[0] == '1') {
        read_line("วันที่ YYYY-MM-DD (เว้นว่าง = วันนี้): ", key, sizeof key);
        if (!key[0]) snprintf(key, sizeof key, "%s", today);
        if (!parse_date(key, NULL)) { puts("รูปแบบวันที่ไม่ถูกต้อง"); return; }
        summarize(L, 'd', "รายวัน", key);
    } else if (c[0] == '2') {
        read_line("สัปดาห์ เช่น 2026-W40 (เว้นว่าง = สัปดาห์นี้): ", key, sizeof key);
        if (!key[0]) { week_key(today, wk, sizeof wk); copy_str(key, sizeof key, wk); }
        if (!valid_week_key(key)) { puts("รูปแบบสัปดาห์ไม่ถูกต้อง (YYYY-Www เช่น 2026-W40)"); return; }
        summarize(L, 'w', "รายสัปดาห์", key);
    } else if (c[0] == '3') {
        read_line("เดือน เช่น 2026-09 (เว้นว่าง = เดือนนี้): ", key, sizeof key);
        if (!key[0]) { memcpy(key, today, 7); key[7] = '\0'; }
        if (!valid_month_key(key)) { puts("รูปแบบเดือนไม่ถูกต้อง (YYYY-MM เช่น 2026-09)"); return; }
        summarize(L, 'm', "รายเดือน", key);
    } else if (c[0] == '4') {
        category_summary(L);
    } else {
        puts("ตัวเลือกไม่ถูกต้อง");
    }
}

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);   /* แสดงผลเป็น UTF-8 */
    SetConsoleCP(CP_UTF8);         /* รับอินพุตเป็น UTF-8 */
#endif
    Ledger L;
    ledger_init(&L);
    ledger_load(&L);

    char c[16];
    for (;;) {
        puts("\n===== บันทึกรายรับ-รายจ่ายส่วนตัว =====");
        puts("1) เพิ่มรายรับ      2) เพิ่มรายจ่าย");
        puts("3) แก้ไขรายการ      4) ลบรายการ");
        puts("5) ดูทุกรายการ      6) ดูสรุป");
        puts("0) ออก");
        if (!read_line("เลือก: ", c, sizeof c)) break;      /* stdin ปิด -> ออก */

        if (c[0] == '1') add_flow(&L, 1);
        else if (c[0] == '2') add_flow(&L, 0);
        else if (c[0] == '3') edit_flow(&L);
        else if (c[0] == '4') delete_flow(&L);
        else if (c[0] == '5') list_flow(&L);
        else if (c[0] == '6') summary_flow(&L);
        else if (c[0] == '0') break;
        else puts("ตัวเลือกไม่ถูกต้อง");
    }
    free(L.data);
    return 0;
}
