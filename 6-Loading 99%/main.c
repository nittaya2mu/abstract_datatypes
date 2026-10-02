/* ============================================================
   main.c  --  เมนูหลักแสดงผลทาง terminal
   ส่วนนี้เขียนเสร็จแล้ว รันได้เลย ยังเรียกฟังก์ชันที่ยังไม่ได้เขียน
   ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "booking.h"
#include "data.h"

/* บน Windows: บังคับให้ console รับ/แสดงผลเป็น UTF-8 เสมอ
   ไม่พึ่งคำสั่ง chcp ของผู้ใช้ เพราะบางครั้งค่านั้นไม่ถูกส่งต่อถึงโปรแกรมจริง
   #ifdef _WIN32 ทำให้โค้ดส่วนนี้ถูกข้ามไปเฉยๆ เวลา compile บน Linux/Mac */
#ifdef _WIN32
#include <windows.h>
#endif

/* ธงบอกว่าข้อมูลนำเข้าหมดแล้ว (End Of File)
   จำเป็นเมื่อรันแบบป้อนไฟล์ เช่น ./ferry < test.txt
   ถ้าไม่มีธงนี้ scanf จะคืน EOF ซ้ำไม่รู้จบ ทำให้เมนูวนพิมพ์ไม่หยุด */
static int g_input_ended = 0;

static void flush_line(void) { int c; while ((c = getchar()) != '\n' && c != EOF) {} }

static void read_line(char *buf, int size) {
    if (fgets(buf, size, stdin)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
    } else {
        buf[0] = '\0';
        g_input_ended = 1;      /* อ่านไม่ได้แล้ว = ข้อมูลนำเข้าหมด */
    }
}

static void print_menu(void) {
    printf("\n==================================================\n");
    printf("        ระบบจองตั๋วเรือโดยสาร (Ferry Booking)\n");
    printf("==================================================\n");
    printf("  1. แสดงผังท่าเรือและเส้นทางทั้งหมด\n");
    printf("  2. ค้นหาเส้นทางที่เร็วที่สุด        [Graph + Dijkstra]\n");
    printf("  3. จองตั๋ว                          [Queue / Priority Queue]\n");
    printf("  4. ค้นหาตั๋วจากรหัสการจอง           [Hash Table]\n");
    printf("  5. แสดงรายการจองทั้งหมด (เรียงรหัส) [AVL Tree]\n");
    printf("  6. ยกเลิกการจอง\n");
    printf("  7. ยกเลิกรายการล่าสุด (Undo)        [Stack]\n");
    printf("  8. แสดงคิวสำรอง (Waitlist)\n");
    printf("  9. สถิติ Hash Table\n");
    printf(" 10. ตารางเดินเรือ (ชนิดเรือ/รอบเวลา/วันให้บริการ)\n");
    printf(" 11. คิวขึ้นเรือ / ออกบัตรตามลำดับจอง  [Queue FIFO]\n");
    printf(" 12. ล้างรายการที่ยกเลิกออกจากดัชนี    [AVL + Hash delete]\n");
    printf(" 13. เทียบความเร็วค้นหา Hash vs AVL\n");
    printf("  0. ออกจากโปรแกรม\n");
    printf("--------------------------------------------------\n");
    printf("เลือกเมนู: ");
}

/* ให้ผู้ใช้เลือกชนิดเรือ คืนค่า BoatType หรือ ANY_BOAT (-1) */
static int ask_boat_type(void) {
    int c;
    printf("ชนิดเรือ (0=%s  1=%s  2=ไม่จำกัด): ",
           boat_name(BOAT_FERRY), boat_name(BOAT_SPEEDBOAT));
    {
        int rc = scanf("%d", &c);
        if (rc == EOF) { g_input_ended = 1; return ANY_BOAT; }
        if (rc != 1)   { flush_line();      return ANY_BOAT; }
    }
    flush_line();
    if (c == 0 || c == 1) return c;
    return ANY_BOAT;
}

static void do_find_path(System *sys) {
    char a[MAX_NAME_LEN], b[MAX_NAME_LEN];
    int from, to, filter;
    printf("ท่าต้นทาง: ");   read_line(a, sizeof(a));
    printf("ท่าปลายทาง: ");  read_line(b, sizeof(b));
    from = graph_find_port(sys->graph, a);
    to   = graph_find_port(sys->graph, b);
    if (from < 0 || to < 0) { printf(">> ไม่พบชื่อท่าเรือ\n"); return; }
    filter = ask_boat_type();
    {
        PathResult r = graph_shortest_path_filtered(sys->graph, from, to, filter);
        if (!r.found) {
            printf(">> ไม่พบเส้นทางที่เชื่อมถึงกัน");
            if (filter != ANY_BOAT) printf(" ด้วย%s (ลองเลือก 2=ไม่จำกัด)", boat_name((BoatType)filter));
            printf("\n");
        } else {
            path_print(sys->graph, &r);
        }
    }
}

/* ============================================================
   ตัวช่วยสำหรับ flow การจองตั๋วแบบนำทาง (Guided Booking)

   หลักการออกแบบ: "ไม่ให้ผู้ใช้เลือกสิ่งที่จองไม่ได้ตั้งแต่แรก"
   แทนที่จะถามครบทุกข้อแล้วค่อยบอกว่าล้มเหลว ระบบจะกรองตัวเลือก
   ให้เหลือเฉพาะอันที่จองได้จริงในทุกขั้น

   ลำดับคำถาม (เรียงตามอำนาจการกรอง มากไปน้อย):
     1. ท่าต้นทาง
     2. ท่าปลายทาง   <- แสดงเฉพาะท่าที่มีเรือไปถึงจริง
     3. วันเดินทาง    <- แสดงเฉพาะวันที่เส้นทางนี้มีเรือวิ่ง
     4. เที่ยวเรือ    <- รวม รอบเวลา+ชนิดเรือ+ราคา+ที่นั่งว่าง ไว้บรรทัดเดียว
     5. ประเภทผู้โดยสาร
     6. ชื่อผู้โดยสาร  <- ถามท้ายสุดเพราะไม่ได้ใช้กรองอะไร
     7. สรุปแล้วยืนยัน

   หมายเหตุ: booking_create() ยังตรวจซ้ำอีกชั้นเสมอ (defense in depth)
   เผื่อมีคนเรียกใช้จากที่อื่นโดยไม่ผ่าน flow นี้
   ============================================================ */

/* อ่านตัวเลข 1 ตัวจากผู้ใช้ คืน -1 ถ้ากรอกไม่ถูก */
static int ask_int(const char *prompt) {
    int v, rc;
    if (g_input_ended) return -1;        /* ข้อมูลหมดแล้ว ไม่ต้องถามอีก */
    printf("%s", prompt);
    rc = scanf("%d", &v);
    if (rc == EOF) { g_input_ended = 1; return -1; }   /* ข้อมูลนำเข้าหมด */
    if (rc != 1)   { flush_line(); return -1; }        /* กรอกไม่ใช่ตัวเลข */
    flush_line();
    return v;
}

/* 1 เที่ยวเรือ = เรือ 1 ชนิด + รอบเวลา 1 รอบ ของเส้นทางและวันที่เลือกไว้ */
typedef struct {
    BoatType  boat;
    int       slot;        /* index รอบเวลาใน BOAT_INFO[boat].depart_times[] */
    int       minutes;     /* เวลาออก (นาทีจากเที่ยงคืน) ใช้เรียงลำดับ */
    Edge     *edge;        /* Edge ที่ตรงกับชนิดเรือนี้ */
} Trip;

#define MAX_TRIPS (BOAT_TYPE_COUNT * MAX_DEPART_TIMES)

/* รวบรวมเที่ยวเรือทั้งหมดของ (from -> to) ในวัน day เรียงตามเวลาออก
   คืนจำนวนเที่ยวที่หาได้ -- O(deg + k^2), k = จำนวนเที่ยว (<= 16) */
static int collect_trips(System *sys, int from, int to, int day, Trip *out) {
    int n = 0;

    { int b; for (b = 0; b < BOAT_TYPE_COUNT; b++) {
        Edge *e;
        const BoatInfo *info;

        if (!boat_runs_on_day((BoatType)b, day)) continue;   /* วันนั้นเรือชนิดนี้หยุด */

        e = graph_find_edge_boat(sys->graph, from, to, (BoatType)b);
        if (!e) continue;                                    /* เรือชนิดนี้ไม่วิ่งเส้นนี้ */

        info = boat_info((BoatType)b);
        { int s; for (s = 0; s < info->depart_count && n < MAX_TRIPS; s++) {
            out[n].boat    = (BoatType)b;
            out[n].slot    = s;
            out[n].minutes = info->depart_times[s];
            out[n].edge    = e;
            n++;
        } }
    } }

    /* insertion sort ตามเวลาออก -- k เล็กมากจึงไม่จำเป็นต้องใช้อัลกอริทึมที่ซับซ้อน */
    { int i; for (i = 1; i < n; i++) {
        Trip key = out[i];
        int j = i - 1;
        while (j >= 0 && out[j].minutes > key.minutes) { out[j + 1] = out[j]; j--; }
        out[j + 1] = key;
    } }
    return n;
}

/* วันนั้นมีเที่ยวเรือของเส้นทางนี้ไหม -- O(deg) */
static int day_has_trips(System *sys, int from, int to, int day) {
    Trip t[MAX_TRIPS];
    return collect_trips(sys, from, to, day, t) > 0;
}

/* ---------- ขั้นที่ 1: เลือกท่าต้นทาง ---------- */
static int choose_origin(System *sys) {
    int n = sys->graph->port_count;
    int c;

    printf("\n--- ขั้นที่ 1/6: เลือกท่าต้นทาง ---\n");
    { int i; for (i = 0; i < n; i++)
        printf("   %d) %s\n", i + 1, graph_port_name(sys->graph, i)); }
    printf("   0) ยกเลิกการจอง\n");

    c = ask_int("เลือก: ");
    if (c <= 0 || c > n) return -1;
    return c - 1;
}

/* ---------- ขั้นที่ 2: เลือกปลายทาง ----------
   แสดงเฉพาะท่าที่มีเรือวิ่งตรงจากต้นทาง
   ถ้าผู้ใช้อยากไปที่ที่ต้องต่อเรือ ระบบเรียก Dijkstra หาเส้นทางให้
   แล้วเสนอให้จองช่วงแรกต่อได้เลย ไม่ปล่อยให้ค้างกลางทาง */
static int choose_destination(System *sys, int from) {
    int direct[MAX_PORTS], nd = 0;
    Edge *e;
    int c, final_to;
    PathResult r;

    for (e = sys->graph->ports[from].head; e != NULL; e = e->next) {
        int dup = 0;
        { int i; for (i = 0; i < nd; i++) if (direct[i] == e->to_port) { dup = 1; break; } }
        if (!dup) direct[nd++] = e->to_port;   /* 1 ท่าอาจมี 2 Edge (เรือ 2 ชนิด) */
    }

    printf("\n--- ขั้นที่ 2/6: เลือกปลายทาง (ออกจาก %s) ---\n",
           graph_port_name(sys->graph, from));
    { int i; for (i = 0; i < nd; i++)
        printf("   %d) %s\n", i + 1, graph_port_name(sys->graph, direct[i])); }
    printf("   %d) ปลายทางอื่น (ต้องต่อเรือ ระบบจะหาเส้นทางให้)\n", nd + 1);
    printf("   0) ยกเลิกการจอง\n");

    c = ask_int("เลือก: ");
    if (c <= 0 || c > nd + 1) return -1;
    if (c <= nd) return direct[c - 1];

    /* --- ปลายทางที่ไม่มีเรือตรง: ใช้ Dijkstra ช่วยวางแผน --- */
    printf("\n   ปลายทางทั้งหมดในระบบ:\n");
    { int i; for (i = 0; i < sys->graph->port_count; i++)
        if (i != from) printf("   %d) %s\n", i + 1, graph_port_name(sys->graph, i)); }
    printf("   0) ย้อนกลับ\n");

    c = ask_int("เลือกปลายทางสุดท้ายที่ต้องการ: ");
    if (c <= 0 || c > sys->graph->port_count) return -1;
    final_to = c - 1;
    if (final_to == from) return -1;

    r = graph_shortest_path(sys->graph, from, final_to);
    if (!r.found) {
        printf(">> ขออภัย ไม่มีเส้นทางเรือไปถึง %s ได้เลย\n",
               graph_port_name(sys->graph, final_to));
        return -1;
    }

    printf("\n>> ไป %s ต้องต่อเรือ %d ช่วง (รวม %d นาที %d บาท):\n",
           graph_port_name(sys->graph, final_to), r.length - 1,
           r.total_time, r.total_fare);
    { int i; for (i = 0; i + 1 < r.length; i++)
        printf("      ช่วงที่ %d) %s -> %s  [%s]\n", i + 1,
               graph_port_name(sys->graph, r.path[i]),
               graph_port_name(sys->graph, r.path[i + 1]),
               boat_name(r.boats[i])); }

    printf("\n   ระบบจองได้ทีละช่วง เริ่มจองช่วงแรก (%s -> %s) เลยไหม\n",
           graph_port_name(sys->graph, r.path[0]),
           graph_port_name(sys->graph, r.path[1]));
    if (ask_int("   1 = จองช่วงแรก, 0 = ยกเลิก: ") != 1) return -1;
    return r.path[1];
}

/* ---------- ขั้นที่ 3: เลือกวัน (เฉพาะวันที่มีเรือวิ่งเส้นนี้) ---------- */
static int choose_day(System *sys, int from, int to) {
    int avail[DAY_COUNT], na = 0;
    int printed = 0, c;

    { int d; for (d = 0; d < DAY_COUNT; d++)
        if (day_has_trips(sys, from, to, d)) avail[na++] = d; }

    if (na == 0) {
        printf("\n>> ขออภัย เส้นทางนี้ไม่มีเรือวิ่งเลยในทุกวัน\n");
        return -1;
    }

    printf("\n--- ขั้นที่ 3/6: เลือกวันเดินทาง ---\n");
    printf("    (%s -> %s)\n", graph_port_name(sys->graph, from),
           graph_port_name(sys->graph, to));
    { int i; for (i = 0; i < na; i++) {
        Trip t[MAX_TRIPS];
        int k = collect_trips(sys, from, to, avail[i], t);
        printf("   %d) วัน%s  (มี %d เที่ยว)\n", i + 1, DAY_NAME_TH[avail[i]], k);
    } }

    /* บอกวันที่ไม่มีเรือด้วย ผู้ใช้จะได้ไม่ต้องเดาว่าทำไมหาย */
    { int d; for (d = 0; d < DAY_COUNT; d++) {
        if (day_has_trips(sys, from, to, d)) continue;
        if (!printed) { printf("   (วันที่ไม่มีเรือเส้นนี้:"); printed = 1; }
        printf(" %s", DAY_NAME_TH[d]);
    } }
    if (printed) printf(")\n");
    printf("   0) ยกเลิกการจอง\n");

    c = ask_int("เลือก: ");
    if (c <= 0 || c > na) return -1;
    return avail[c - 1];
}

/* ---------- ขั้นที่ 4: เลือกเที่ยวเรือ (รอบเวลา + ชนิดเรือ รวมเป็นคำถามเดียว) ----------
   เที่ยวที่เต็มแล้วยังเลือกได้ แต่จะแจ้งชัดว่าจะได้เป็นคิวสำรอง */
static int choose_trip(int day, Trip *trips, int ntrip) {
    int fastest = 0, cheapest = 0, c;
    char hhmm[8];

    { int i; for (i = 1; i < ntrip; i++) {
        if (trips[i].edge->travel_time < trips[fastest].edge->travel_time)  fastest = i;
        if (trips[i].edge->fare        < trips[cheapest].edge->fare)        cheapest = i;
    } }

    printf("\n--- ขั้นที่ 4/6: เลือกเที่ยวเรือ (วัน%s) ---\n", DAY_NAME_TH[day]);
    { int i; for (i = 0; i < ntrip; i++) {
        int left = graph_seats_left(trips[i].edge, day, trips[i].slot);
        minutes_to_hhmm(trips[i].minutes, hhmm);
        printf("   %d) %s น. | %s | %d นาที | %d บาท | ",
               i + 1, hhmm, boat_name(trips[i].boat),
               trips[i].edge->travel_time, trips[i].edge->fare);
        if (left > 0) printf("ว่าง %d/%d", left, trips[i].edge->capacity);
        else          printf("เต็ม (จองเป็นคิวสำรองได้)");
        if (ntrip > 1 && i == fastest)                          printf("  [เร็วที่สุด]");
        if (ntrip > 1 && i == cheapest && cheapest != fastest)  printf("  [ถูกที่สุด]");
        printf("\n");
    } }
    printf("   0) ยกเลิกการจอง\n");

    c = ask_int("เลือก: ");
    if (c <= 0 || c > ntrip) return -1;
    return c - 1;
}

/* ---------- ขั้นที่ 5: ประเภทผู้โดยสาร ---------- */
static int choose_passenger_type(void) {
    int c;
    printf("\n--- ขั้นที่ 5/6: ประเภทผู้โดยสาร ---\n");
    printf("   1) ทั่วไป\n");
    printf("   2) ผู้สูงอายุ  (ได้สิทธิ์ก่อนเมื่อมีที่นั่งว่างจากคิวสำรอง)\n");
    printf("   3) ตั๋วด่วน    (ได้สิทธิ์ก่อนสูงสุด)\n");
    printf("   0) ยกเลิกการจอง\n");
    c = ask_int("เลือก: ");
    if (c == 1) return PRIO_NORMAL;
    if (c == 2) return PRIO_SENIOR;
    if (c == 3) return PRIO_EXPRESS;
    return -1;
}

/* ---------- เมนู 3: จองตั๋วแบบนำทางทีละขั้น ---------- */
static void do_book(System *sys) {
    char  name[MAX_NAME_LEN], hhmm[8];
    Trip  trips[MAX_TRIPS];
    int   from, to, day, ntrip, pick, ptype, left;

    from = choose_origin(sys);
    if (from < 0) { printf(">> ยกเลิกการจอง\n"); return; }

    to = choose_destination(sys, from);
    if (to < 0) { printf(">> ยกเลิกการจอง\n"); return; }

    day = choose_day(sys, from, to);
    if (day < 0) { printf(">> ยกเลิกการจอง\n"); return; }

    ntrip = collect_trips(sys, from, to, day, trips);
    if (ntrip == 0) { printf(">> ไม่มีเที่ยวเรือในวันนี้\n"); return; }

    pick = choose_trip(day, trips, ntrip);
    if (pick < 0) { printf(">> ยกเลิกการจอง\n"); return; }

    /* เที่ยวเต็ม: ถามให้ชัดก่อน ไม่ปล่อยให้เซอร์ไพรส์ตอนจบ */
    left = graph_seats_left(trips[pick].edge, day, trips[pick].slot);
    if (left <= 0) {
        printf("\n>> เที่ยวนี้ที่นั่งเต็มแล้ว จองต่อจะได้เป็น \"คิวสำรอง\"\n");
        printf("   หากมีคนยกเลิก ระบบจะเลื่อนคุณขึ้นอัตโนมัติตามลำดับสิทธิ์\n");
        if (ask_int("   1 = รับคิวสำรอง, 0 = กลับไปเลือกใหม่: ") != 1) {
            printf(">> ยกเลิกการจอง เลือกเที่ยวอื่นได้ที่เมนู 3 อีกครั้ง\n");
            return;
        }
    }

    ptype = choose_passenger_type();
    if (ptype < 0) { printf(">> ยกเลิกการจอง\n"); return; }

    printf("\n--- ขั้นที่ 6/6: ชื่อผู้โดยสาร ---\n");
    printf("ชื่อ-นามสกุล: ");
    read_line(name, sizeof(name));
    if (name[0] == '\0') { printf(">> ไม่ได้กรอกชื่อ ยกเลิกการจอง\n"); return; }

    /* ---- สรุปให้ตรวจก่อนยืนยัน ---- */
    minutes_to_hhmm(trips[pick].minutes, hhmm);
    printf("\n===== กรุณาตรวจสอบก่อนยืนยัน =====\n");
    printf("  ผู้โดยสาร : %s\n", name);
    printf("  เส้นทาง   : %s -> %s\n", graph_port_name(sys->graph, from),
                                        graph_port_name(sys->graph, to));
    printf("  วันเดินทาง: %s\n", DAY_NAME_TH[day]);
    printf("  เที่ยวเรือ : %s น. โดย%s (%d นาที)\n", hhmm,
           boat_name(trips[pick].boat), trips[pick].edge->travel_time);
    printf("  ค่าโดยสาร : %d บาท\n", trips[pick].edge->fare);
    printf("==================================\n");
    if (ask_int("  1 = ยืนยันการจอง, 0 = ยกเลิก: ") != 1) {
        printf(">> ยกเลิกการจอง ยังไม่มีการบันทึกใดๆ\n");
        return;
    }

    {
        Booking *bk = booking_create(sys, name, from, to, (PassengerType)ptype,
                                     trips[pick].boat, day, trips[pick].slot);
        if (!bk) { printf(">> จองไม่สำเร็จ\n"); return; }
        printf("\n>> จองสำเร็จ!\n");
        booking_show(sys, bk);
    }
}

/* เมนู 11 -- คิวขึ้นเรือ ใช้ Queue (FIFO) ดึงคนตามลำดับที่จองเข้ามาจริง */
static void do_boarding(System *sys) {
    boarding_queue_show(sys);
    printf("\n   1 = เรียกคิวถัดไปออกบัตรขึ้นเรือ, 0 = กลับเมนูหลัก\n");
    if (ask_int("เลือก: ") != 1) return;
    {
        Booking *b = booking_call_next(sys);
        if (!b) { printf(">> ไม่มีใครรอออกบัตรขึ้นเรือแล้ว\n"); return; }
        printf("\n>> ออกบัตรขึ้นเรือลำดับที่ %d ให้ %s เรียบร้อย\n",
               b->boarding_no, b->passenger_name);
        booking_show(sys, b);
    }
}

static void do_search(System *sys) {
    char id[MAX_ID_LEN];
    printf("รหัสการจอง: "); read_line(id, sizeof(id));
    {
        Booking *bk = booking_find(sys, id);
        if (!bk) printf(">> ไม่พบรหัสนี้\n");
        else booking_show(sys, bk);
    }
}

static void do_cancel(System *sys) {
    char id[MAX_ID_LEN];
    printf("รหัสการจองที่จะยกเลิก: "); read_line(id, sizeof(id));
    printf(booking_cancel(sys, id) ? ">> ยกเลิกเรียบร้อย\n" : ">> ยกเลิกไม่สำเร็จ\n");
}

int main(void) {
#ifdef _WIN32
    /* บังคับ console ให้อ่าน/เขียนเป็น UTF-8 ตั้งแต่โปรแกรมเริ่มทำงาน
       แก้ปัญหาตัวอักษรไทยแสดงผลเพี้ยนบน Windows โดยไม่ต้องพึ่ง chcp ข้างนอก */
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    System *sys = system_create();
    int choice;

    if (!sys) {
        printf("!! system_create() ยังไม่ได้เขียน (booking.c)\n");
        printf("   โครงโปรเจกต์คอมไพล์ผ่านแล้ว ขั้นต่อไปคือเติมเนื้อในทีละไฟล์\n");
        return 0;
    }
    system_load_sample_data(sys);

    for (;;) {
        int rc;
        if (g_input_ended) {           /* เผื่อธงถูกตั้งจากฟังก์ชันย่อย */
            printf("\n>> ข้อมูลนำเข้าหมด ปิดโปรแกรม\n");
            system_destroy(sys);
            return 0;
        }
        print_menu();
        rc = scanf("%d", &choice);
        if (rc == EOF) {               /* กด Ctrl+D หรือไฟล์ป้อนข้อมูลจบแล้ว */
            g_input_ended = 1;
            printf("\n>> ข้อมูลนำเข้าหมด ปิดโปรแกรม\n");
            system_destroy(sys);
            return 0;
        }
        if (rc != 1) { flush_line(); continue; }   /* กรอกไม่ใช่ตัวเลข ถามใหม่ */
        flush_line();
        switch (choice) {
            case 1: graph_print(sys->graph);   break;
            case 2: do_find_path(sys);         break;
            case 3: do_book(sys);              break;
            case 4: do_search(sys);            break;
            case 5: avl_inorder_print(sys->sorted); break;
            case 6: do_cancel(sys);            break;
            case 7: printf(booking_undo(sys) ? ">> ย้อนรายการล่าสุดแล้ว\n"
                                             : ">> ไม่มีรายการให้ย้อน\n"); break;
            case 8: waitlist_show(sys);        break;
            case 9: ht_print_stats(sys->index); break;
            case 10: data_print_boat_schedule(); break;
            case 11: do_boarding(sys); break;
            case 12: booking_purge_cancelled(sys); break;
            case 13: search_benchmark(sys); break;
            case 0: system_destroy(sys); printf("ปิดโปรแกรม\n"); return 0;
            default: printf(">> ไม่มีเมนูนี้\n");
        }
    }
}
