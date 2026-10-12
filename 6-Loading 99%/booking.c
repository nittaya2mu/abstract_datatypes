/* ============================================================
   booking.c  --  ตัวระบบหลัก มัด ADT ทั้ง 5 เข้าด้วยกัน
   Graph = เส้นทาง | Queue = ลำดับจอง | Priority Queue = คิวสำรอง
   Hash Table = ค้นด้วยรหัส | Stack = Undo
   ============================================================ */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "booking.h"
#include "data.h"

/* ---------- สร้าง / ทำลายระบบ ---------- */

/* จองหน่วยความจำให้ ADT ทุกตัว -- O(1) */
System *system_create(void) {
    System *sys = (System *)malloc(sizeof(System));
    if (!sys) return NULL;

    sys->graph     = graph_create();
    sys->index     = ht_create();
    sys->sorted    = avl_create();          /* ยังเป็นโครงเปล่า ใช้ NULL ได้ */
    sys->pending   = queue_create();
    sys->waitlist  = pq_create(64);
    sys->history   = stack_create();
    sys->seq          = 0;
    sys->boarding_seq = 0;
    sys->all_count = 0;

    /* ถ้าตัวที่จำเป็นตัวใดสร้างไม่สำเร็จ ให้ยกเลิกทั้งหมด */
    if (!sys->graph || !sys->index || !sys->pending ||
        !sys->waitlist || !sys->history) {
        system_destroy(sys);
        return NULL;
    }
    return sys;
}

/* คืนหน่วยความจำทั้งหมด -- O(V+E+n) */
void system_destroy(System *sys) {
    if (!sys) return;
    { int i; for (i = 0; i < sys->all_count; i++) free(sys->all[i]); }  /* เจ้าของตัวจริง */
    graph_destroy(sys->graph);
    ht_destroy(sys->index);
    avl_destroy(sys->sorted);
    queue_destroy(sys->pending);
    pq_destroy(sys->waitlist);
    stack_destroy(sys->history);
    free(sys);
}

/* โหลดท่าเรือและเส้นทางจาก data.c -- O(V+E)
   เพิ่ม edge สองทิศทาง เพราะเรือวิ่งกลับได้ */
int system_load_sample_data(System *sys) {
    if (!sys || !sys->graph) return 0;

    { int i; for (i = 0; i < SAMPLE_PORT_COUNT; i++)
        graph_add_port(sys->graph, SAMPLE_PORTS[i]); }

    { int i; for (i = 0; i < SAMPLE_ROUTE_COUNT; i++) {
        int f = graph_find_port(sys->graph, SAMPLE_ROUTES[i].from);
        int t = graph_find_port(sys->graph, SAMPLE_ROUTES[i].to);
        if (f < 0 || t < 0) continue;
        /* เพิ่มทั้งขาไปและขากลับ ใช้ route_id เดียวกันและเรือชนิดเดียวกัน */
        graph_add_route(sys->graph, f, t, i, SAMPLE_ROUTES[i].boat,
                        SAMPLE_ROUTES[i].travel_time,
                        SAMPLE_ROUTES[i].fare, SAMPLE_ROUTES[i].capacity);
        graph_add_route(sys->graph, t, f, i, SAMPLE_ROUTES[i].boat,
                        SAMPLE_ROUTES[i].travel_time,
                        SAMPLE_ROUTES[i].fare, SAMPLE_ROUTES[i].capacity);
    } }
    return 1;
}

/* ---------- การจอง ---------- */

/* สร้างรหัสจองแบบ BK000001 -- O(1) */
char *booking_make_id(System *sys, char *out) {
    if (!sys || !out) return out;
    sys->seq++;
    snprintf(out, MAX_ID_LEN, "BK%06ld", sys->seq);
    return out;
}

/* คำนวณลำดับสิทธิ์ในคิวสำรอง -- O(1)
   ประเภทสำคัญกว่าเวลา: ด่วน(0) < ผู้สูงอายุ(1) < ทั่วไป(2)
   ถ้าประเภทเดียวกัน ใครมาก่อนได้ก่อน (seq น้อยกว่า) */
long waitlist_priority(PassengerType type, long seq) {
    return (long)type * 1000000L + seq;
}

/* หาหมายเลขที่นั่งว่างที่น้อยที่สุดบนเส้นทางนั้น -- O(n)
   สแกนรายการจองที่ยืนยันแล้วทั้งหมด เพื่อไม่ให้เลขที่นั่งซ้ำกัน
   เมื่อมีคนยกเลิกแล้วมีคนจากคิวสำรองมาแทน */
/* หมายเหตุ: ฟังก์ชัน next_free_seat() เดิมถูกถอดออกแล้ว
   เดิมวนดู booking ทุกใบคูณจำนวนที่นั่ง = O(n x capacity)
   ตอนนี้ย้ายไปใช้ graph_seat_take() ที่สแกน bitmap ใน Edge แทน = O(1) */


/* จองตั๋วบนเส้นทางตรงระหว่างสองท่า -- O(1) เฉลี่ย (+ O(log n) ถ้าเข้าคิวสำรอง)
   ที่นั่งเต็ม -> เข้าคิวสำรองอัตโนมัติ */
Booking *booking_create(System *sys, const char *name,
                        int from_port, int to_port, PassengerType type,
                        BoatType boat, int day, int slot) {
    Edge           *e;
    Booking        *b;
    Action          act;
    const BoatInfo *info;

    if (!sys || !name || sys->all_count >= MAX_BOOKINGS) return NULL;
    if (type < PRIO_EXPRESS || type > PRIO_NORMAL) type = PRIO_NORMAL;

    /* --- ตรวจความถูกต้องของชนิดเรือ วัน และรอบเวลา --- */
    info = boat_info(boat);
    if (!info) { printf(">> ชนิดเรือไม่ถูกต้อง\n"); return NULL; }

    if (day < 0 || day >= DAY_COUNT) { printf(">> วันเดินทางไม่ถูกต้อง\n"); return NULL; }

    if (!boat_runs_on_day(boat, day)) {
        printf(">> %s ไม่ให้บริการวัน%s (ดูตารางเดินเรือที่เมนู 10)\n",
               info->name, DAY_NAME_TH[day]);
        return NULL;
    }

    if (slot < 0 || slot >= info->depart_count) {
        printf(">> รอบเวลาไม่ถูกต้อง (%s มี %d รอบ)\n", info->name, info->depart_count);
        return NULL;
    }

    /* ต้องใช้ graph_find_edge_boat ไม่ใช่ graph_find_edge
       เพราะท่าคู่เดียวกันอาจมีทั้งเรือข้ามฟากและสปีดโบ๊ท */
    e = graph_find_edge_boat(sys->graph, from_port, to_port, boat);
    if (!e) {
        printf(">> ไม่มี%sวิ่งตรงระหว่างสองท่านี้ ลองเมนู 2 เพื่อดูเส้นทางต่อเรือ\n",
               info->name);
        return NULL;
    }

    b = (Booking *)malloc(sizeof(Booking));
    if (!b) return NULL;

    booking_make_id(sys, b->booking_id);
    strncpy(b->passenger_name, name, MAX_NAME_LEN - 1);
    b->passenger_name[MAX_NAME_LEN - 1] = '\0';
    b->route_id    = e->route_id;
    b->from_port   = from_port;
    b->to_port     = to_port;
    b->boat        = boat;
    b->travel_day  = day;
    b->depart_slot = slot;
    b->type        = type;
    b->boarding_no = 0;                          /* ยังไม่ได้ออกบัตรขึ้นเรือ */
    b->created_at  = sys->seq;

    {
        int seat = graph_seat_take(e, day, slot);  /* O(1) -- คืน -1 ถ้าเต็ม */
        if (seat > 0) {                            /* รอบนี้ยังมีที่นั่งว่าง */
            b->seat_no = seat;
            b->status  = STATUS_CONFIRMED;
        } else {                                   /* เต็ม -> เข้าคิวสำรอง */
            b->seat_no = -1;
            b->status  = STATUS_WAITLISTED;
            pq_push(sys->waitlist, waitlist_priority(type, b->created_at), b);
        }
    }

    sys->all[sys->all_count++] = b;              /* เจ้าของหน่วยความจำ */
    ht_insert(sys->index, b->booking_id, b);     /* ค้นด้วยรหัส O(1)   */
    avl_insert(sys->sorted, b->booking_id, b);   /* เรียงตามรหัส       */
    queue_enqueue(sys->pending, b);              /* ลำดับจองก่อน-หลัง  */

    act.type     = ACT_BOOK;
    act.snapshot = *b;
    act.route_id = b->route_id;
    stack_push(sys->history, act);               /* เผื่อกด Undo */
    return b;
}

/* ค้นรายการจองจากรหัส -- O(1) เฉลี่ย ผ่าน Hash Table */
Booking *booking_find(System *sys, const char *booking_id) {
    if (!sys) return NULL;
    return ht_search(sys->index, booking_id);
}

/* ดึงคนจากคิวสำรองขึ้นมาแทนที่ว่างของเส้นทางนั้น -- O(k log k)
   ต้องคัดเฉพาะคนที่รอเส้นทางเดียวกัน คนอื่นดันกลับเข้า heap */
int waitlist_promote(System *sys, int route_id) {
    Booking *hold[MAX_BOOKINGS];
    long     hold_p[MAX_BOOKINGS];
    int      n = 0, promoted = 0;
    Edge    *e;

    if (!sys || pq_is_empty(sys->waitlist)) return 0;

    while (!pq_is_empty(sys->waitlist)) {
        long     p;
        Booking *b = (Booking *)pq_pop(sys->waitlist, &p);
        if (!b) break;

        if (b->status == STATUS_WAITLISTED && b->route_id == route_id) {
            /* ที่นั่งว่างของ "รอบที่คนนี้รออยู่" เท่านั้น ไม่ใช่รอบอื่น */
            e = graph_find_edge_boat(sys->graph, b->from_port, b->to_port, b->boat);
            if (e && graph_seats_left(e, b->travel_day, b->depart_slot) > 0) {
                Action act;
                act.type     = ACT_WAITLIST_PROMOTE;
                act.snapshot = *b;               /* สถานะก่อนเลื่อนขึ้น */
                act.route_id = route_id;

                b->seat_no = graph_seat_take(e, b->travel_day, b->depart_slot);
                b->status  = STATUS_CONFIRMED;
                stack_push(sys->history, act);
                promoted = 1;
                printf(">> เลื่อน %s (%s) จากคิวสำรองขึ้นเป็นที่นั่ง %d\n",
                       b->booking_id, b->passenger_name, b->seat_no);
                break;
            }
        }
        if (n < MAX_BOOKINGS) { hold[n] = b; hold_p[n] = p; n++; }
    }
    { int i; for (i = 0; i < n; i++) pq_push(sys->waitlist, hold_p[i], hold[i]); }
    return promoted;
}

/* ยกเลิกการจอง -- O(1) เฉลี่ย + งาน promote
   คืนที่นั่งแล้วดึงคนจากคิวสำรองขึ้นมาแทนทันที */
int booking_cancel(System *sys, const char *booking_id) {
    Booking *b;
    Action   act;
    Edge    *e;

    if (!sys) return 0;
    b = ht_search(sys->index, booking_id);
    if (!b || b->status == STATUS_CANCELLED) return 0;

    act.type     = ACT_CANCEL;
    act.snapshot = *b;                   /* เก็บสถานะก่อนยกเลิกไว้ย้อนกลับ */
    act.route_id = b->route_id;
    stack_push(sys->history, act);

    if (b->status == STATUS_CONFIRMED) {
        e = graph_find_edge_boat(sys->graph, b->from_port, b->to_port, b->boat);
        graph_seat_release(e, b->travel_day, b->depart_slot, b->seat_no);
    }
    b->status  = STATUS_CANCELLED;
    b->seat_no = -1;

    waitlist_promote(sys, act.route_id);
    return 1;
}

/* ย้อนรายการล่าสุด 1 ขั้น -- O(1) เฉลี่ย
   หมายเหตุ: ถ้าการยกเลิกทำให้มีคนถูกเลื่อนขึ้นจากคิวสำรอง
   จะต้องกด Undo สองครั้ง (ครั้งแรกถอนการเลื่อน ครั้งที่สองคืนการจอง)
   เพราะ Stack ทำงานแบบ LIFO ย้อนทีละขั้นตามลำดับที่เกิดจริง */
int booking_undo(System *sys) {
    Action   act;
    Booking *b;
    Edge    *e;

    if (!sys || !stack_pop(sys->history, &act)) return 0;

    b = ht_search(sys->index, act.snapshot.booking_id);
    if (!b) return 0;

    switch (act.type) {
        case ACT_BOOK:                   /* ถอนการจองที่เพิ่งทำ */
            if (b->status == STATUS_CONFIRMED) {
                e = graph_find_edge_boat(sys->graph, b->from_port, b->to_port, b->boat);
                graph_seat_release(e, b->travel_day, b->depart_slot, b->seat_no);
            }
            b->status  = STATUS_CANCELLED;
            b->seat_no = -1;
            ht_delete(sys->index, b->booking_id);   /* ค้นด้วยรหัสไม่เจออีก */
            printf(">> ถอนการจอง %s แล้ว\n", act.snapshot.booking_id);
            break;

        case ACT_CANCEL:                 /* คืนการจองที่เพิ่งยกเลิก */
            {
                int got = -1;
                if (act.snapshot.status == STATUS_CONFIRMED) {
                    e = graph_find_edge_boat(sys->graph, b->from_port, b->to_port,
                                             act.snapshot.boat);
                    /* ต้องได้ "ที่นั่งเดิม" กลับมา ไม่ใช่เลขว่างต่ำสุด
                       (เดิมใช้ graph_seat_take ทำให้ที่นั่งซ้ำกับคนที่จองทีหลัง) */
                    got = graph_seat_take_exact(e, act.snapshot.travel_day,
                                                act.snapshot.depart_slot,
                                                act.snapshot.seat_no);
                    if (got < 0)   /* ที่นั่งเดิมไม่ว่าง (ไม่ควรเกิดเพราะ Undo เป็น LIFO) */
                        got = graph_seat_take(e, act.snapshot.travel_day,
                                              act.snapshot.depart_slot);
                }
                *b = act.snapshot;                   /* คืนสถานะเดิมทั้งก้อน */
                if (act.snapshot.status == STATUS_CONFIRMED) {
                    if (got > 0) b->seat_no = got;   /* ให้ Booking ตรงกับ bitmap เสมอ */
                    else { b->seat_no = -1; b->status = STATUS_WAITLISTED;
                           pq_push(sys->waitlist,
                                   waitlist_priority(b->type, b->created_at), b); }
                }
            }
            printf(">> คืนการจอง %s กลับมาแล้ว\n", b->booking_id);
            break;

        case ACT_WAITLIST_PROMOTE:       /* ถอนคนที่ถูกเลื่อนขึ้นกลับเข้าคิวสำรอง */
            e = graph_find_edge_boat(sys->graph, b->from_port, b->to_port, b->boat);
            graph_seat_release(e, b->travel_day, b->depart_slot, b->seat_no);
            *b = act.snapshot;
            pq_push(sys->waitlist,
                    waitlist_priority(b->type, b->created_at), b);
            printf(">> ถอน %s กลับเข้าคิวสำรอง\n", b->booking_id);
            break;
    }
    return 1;
}

/* ---------- แสดงผล ---------- */

static const char *status_text(BookingStatus s) {
    switch (s) {
        case STATUS_CONFIRMED:  return "ยืนยันแล้ว";
        case STATUS_WAITLISTED: return "คิวสำรอง";
        default:                return "ยกเลิกแล้ว";
    }
}
static const char *type_text(PassengerType t) {
    switch (t) {
        case PRIO_EXPRESS: return "ด่วน";
        case PRIO_SENIOR:  return "ผู้สูงอายุ";
        default:           return "ทั่วไป";
    }
}

/* แสดงรายละเอียดการจอง 1 รายการ -- O(1) */
void booking_show(const System *sys, const Booking *b) {
    if (!sys || !b) return;
    printf("\n--- รายละเอียดการจอง ---\n");
    printf("  รหัส        : %s\n", b->booking_id);
    printf("  ผู้โดยสาร    : %s (%s)\n", b->passenger_name, type_text(b->type));
    printf("  เส้นทาง      : %s -> %s\n",
           graph_port_name(sys->graph, b->from_port),
           graph_port_name(sys->graph, b->to_port));
    {
        const BoatInfo *info = boat_info(b->boat);
        char hhmm[8] = "--:--";
        if (info && b->depart_slot >= 0 && b->depart_slot < info->depart_count)
            minutes_to_hhmm(info->depart_times[b->depart_slot], hhmm);
        printf("  เรือ         : %s\n", boat_name(b->boat));
        printf("  วันเดินทาง   : %s\n",
               (b->travel_day >= 0 && b->travel_day < DAY_COUNT)
                   ? DAY_NAME_TH[b->travel_day] : "-");
        printf("  รอบออกเรือ   : %s น.\n", hhmm);
    }
    printf("  สถานะ       : %s\n", status_text(b->status));
    if (b->seat_no > 0) printf("  ที่นั่ง       : %d\n", b->seat_no);
    else                printf("  ที่นั่ง       : ยังไม่ได้รับ\n");
}

/* แสดงรายการจองทั้งหมด -- O(n) */
void booking_list_all(System *sys) {
    if (!sys) return;
    printf("\n--- รายการจองทั้งหมด (%d รายการ) ---\n", sys->all_count);
    if (sys->all_count == 0) { printf("  (ยังไม่มีการจอง)\n"); return; }
    printf("  %-10s %-20s %-12s %-8s\n", "รหัส", "ผู้โดยสาร", "สถานะ", "ที่นั่ง");
    { int i; for (i = 0; i < sys->all_count; i++) {
        Booking *b = sys->all[i];
        printf("  %-10s %-20s %-12s ", b->booking_id,
               b->passenger_name, status_text(b->status));
        if (b->seat_no > 0) printf("%d\n", b->seat_no);
        else                printf("-\n");
    } }
}

/* แสดงคิวสำรอง -- O(n) อ่านตรงจาก heap โดยไม่ดึงข้อมูลออก */
void waitlist_show(const System *sys) {
    int shown = 0;
    if (!sys || !sys->waitlist) return;
    printf("\n--- คิวสำรอง (Waitlist) ---\n");
    { int i; for (i = 0; i < sys->waitlist->size; i++) {
        Booking *b = (Booking *)sys->waitlist->nodes[i].data;
        if (!b || b->status != STATUS_WAITLISTED) continue;
        {
            const BoatInfo *info = boat_info(b->boat);
            char hhmm[8] = "--:--";
            if (info && b->depart_slot >= 0 && b->depart_slot < info->depart_count)
                minutes_to_hhmm(info->depart_times[b->depart_slot], hhmm);
            printf("  ");
            print_padded(b->booking_id, 11);
            print_padded(b->passenger_name, 18);
            print_padded(type_text(b->type), 12);
            printf("%s -> %s  [%s วัน%s %s น.]\n",
                   graph_port_name(sys->graph, b->from_port),
                   graph_port_name(sys->graph, b->to_port),
                   boat_name(b->boat),
                   (b->travel_day >= 0 && b->travel_day < DAY_COUNT)
                       ? DAY_NAME_TH[b->travel_day] : "-", hhmm);
        }
        shown++;
    } }
    if (!shown) printf("  (ไม่มีใครรอคิวสำรอง)\n");
}


/* ============================================================
   เรียกคิวขึ้นเรือ -- นี่คือจุดที่ Queue (FIFO) ถูกใช้งานจริง

   ตอนจองตั๋ว ทุกใบถูก enqueue เข้า sys->pending ตามลำดับเวลาที่จอง
   ฟังก์ชันนี้ dequeue ออกมาทีละใบเพื่อออกบัตรขึ้นเรือ
   จึงรับประกันว่า "ใครจองก่อนได้ขึ้นก่อน" ตามคุณสมบัติ FIFO

   ต่างจาก Priority Queue ตรงที่คิวสำรองเรียงตาม "สิทธิ์" (ด่วน/ผู้สูงอายุ)
   แต่คิวขึ้นเรือเรียงตาม "เวลาที่จอง" ล้วนๆ ไม่มีใครลัดคิวได้
   ============================================================ */
Booking *booking_call_next(System *sys) {
    if (!sys) return NULL;

    /* วนข้ามคนที่ยกเลิกไปแล้วหรือยังติดคิวสำรอง (ยังไม่มีที่นั่งจริง) */
    while (!queue_is_empty(sys->pending)) {
        Booking *b = (Booking *)queue_dequeue(sys->pending);   /* O(1) */
        if (!b) continue;
        if (b->status != STATUS_CONFIRMED) continue;           /* ข้ามไป ไม่ต้องคืนเข้าคิว */
        b->boarding_no = ++sys->boarding_seq;
        return b;
    }
    return NULL;   /* ไม่มีใครรอออกบัตรแล้ว */
}

/* O(n) -- ดูว่าใครยังรออยู่ในคิวขึ้นเรือ โดยไม่ดึงออก (ใช้ queue_peek + เดิน list) */
void boarding_queue_show(const System *sys) {
    QueueNode *n;
    int i = 1;
    if (!sys) return;

    printf("\n--- คิวขึ้นเรือ (Queue: จองก่อนได้ก่อน) ---\n");
    if (queue_is_empty(sys->pending)) { printf("  (ไม่มีใครรอออกบัตรขึ้นเรือ)\n"); return; }

    printf("  คิวถัดไปคือ: %s\n\n",
           ((Booking *)queue_peek(sys->pending))->passenger_name);   /* queue_peek O(1) */
    for (n = sys->pending->front; n != NULL; n = n->next) {
        Booking *b = (Booking *)n->data;
        printf("  %d) ", i++);
        print_padded(b->booking_id, 11);
        print_padded(b->passenger_name, 20);
        printf("%s\n", status_text(b->status));
    }
    printf("  รวม %d คนในคิว\n", queue_size(sys->pending));
}

/* ============================================================
   ล้างรายการที่ยกเลิกแล้วออกจากดัชนีค้นหา
   นี่คือจุดที่ avl_delete() และ ht_delete() ถูกใช้งานจริง

   หมายเหตุสำคัญ: ลบออกจาก Hash Table และ AVL Tree เท่านั้น
   ไม่ free ตัว Booking และไม่ถอดออกจาก sys->all[]
   เพราะ all[] เป็นเจ้าของหน่วยความจำตัวจริง และ Stack (Undo)
   อาจยังอ้างถึงรายการนั้นอยู่ ถ้า free ตรงนี้จะเกิด dangling pointer
   ============================================================ */
int booking_purge_cancelled(System *sys) {
    int removed = 0;
    if (!sys) return 0;

    { int i; for (i = 0; i < sys->all_count; i++) {
        Booking *b = sys->all[i];
        if (b->status != STATUS_CANCELLED) continue;
        if (!avl_search(sys->sorted, b->booking_id)) continue;   /* ล้างไปแล้ว ข้าม */

        avl_delete(sys->sorted, b->booking_id);   /* O(log n) */
        ht_delete(sys->index,  b->booking_id);    /* O(1) เฉลี่ย */
        removed++;
    } }
    printf("\n>> ล้างรายการที่ยกเลิกแล้วออกจากดัชนี %d รายการ\n", removed);
    if (removed > 0)
        printf("   (ข้อมูลยังอยู่ในหน่วยความจำเพื่อให้ Undo ทำงานได้ แต่จะค้นหาไม่เจอแล้ว)\n");
    return removed;
}

/* ============================================================
   เทียบความเร็วการค้นหา: Hash Table O(1) กับ AVL Tree O(log n)
   นี่คือจุดที่ avl_search() ถูกใช้งานจริง และใช้เป็นหลักฐาน
   ประกอบหัวข้อ Time Complexity Analysis ในรายงานได้
   ============================================================ */
/* วัดเวลาค้นหา 1 รอบที่ขนาดข้อมูล n -- ใช้ภายใน search_benchmark */
static void bench_one(int n, int rounds) {
    HashTable *ht  = ht_create();
    AVLTree   *avl = avl_create();
    Booking    marker;
    char       key[MAX_ID_LEN];
    clock_t    t0, t1;
    double     ht_ms, avl_ms;
    int        hit_ht = 0, hit_avl = 0;

    if (!ht || !avl) {
        if (ht)  ht_destroy(ht);
        if (avl) avl_destroy(avl);
        printf("  (สร้างโครงสร้างทดสอบไม่สำเร็จ)\n");
        return;
    }

    { int i; for (i = 0; i < n; i++) {
        snprintf(key, sizeof key, "BK%06d", i);
        ht_insert(ht, key, &marker);
        avl_insert(avl, key, &marker);
    } }

    t0 = clock();
    { int i; for (i = 0; i < rounds; i++) {
        snprintf(key, sizeof key, "BK%06d", i % n);
        if (ht_search(ht, key)) hit_ht++;
    } }
    t1 = clock();
    ht_ms = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;

    t0 = clock();
    { int i; for (i = 0; i < rounds; i++) {
        snprintf(key, sizeof key, "BK%06d", i % n);
        if (avl_search(avl, key)) hit_avl++;
    } }
    t1 = clock();
    avl_ms = 1000.0 * (double)(t1 - t0) / CLOCKS_PER_SEC;

    {
        /* ถ้าต่างกันไม่ถึง 10% ถือว่าอยู่ในช่วงความคลาดเคลื่อนของการวัด
           ไม่ควรสรุปว่าตัวไหนเร็วกว่า เพราะรันใหม่ผลอาจสลับกันได้ */
        const char *verdict;
        double diff = (ht_ms > avl_ms) ? (ht_ms - avl_ms) : (avl_ms - ht_ms);
        double base = (ht_ms < avl_ms) ? ht_ms : avl_ms;
        if (base > 0.0 && diff / base < 0.10) verdict = "ใกล้เคียงกัน";
        else if (ht_ms < avl_ms)             verdict = "Hash เร็วกว่า";
        else                                 verdict = "AVL เร็วกว่า";

        printf("  ข้อมูล %5d รายการ | load factor %5.2f | Hash %7.2f ms | AVL %7.2f ms | %s\n",
               n, (double)n / HASH_SIZE, ht_ms, avl_ms, verdict);
    }

    if (hit_ht != hit_avl) printf("  ** ผลลัพธ์ไม่ตรงกัน ผิดปกติ **\n");

    ht_destroy(ht);
    avl_destroy(avl);
}

/* ============================================================
   เทียบความเร็ว Hash Table กับ AVL Tree ที่ขนาดข้อมูลต่างกัน
   นี่คือจุดที่ avl_search() ถูกใช้งานจริง และใช้เป็นหลักฐาน
   ประกอบหัวข้อ Time Complexity Analysis ในรายงานได้

   สิ่งที่ต้องการให้เห็น: Hash เป็น O(1) "เฉลี่ย" ไม่ใช่ O(1) เสมอ
   ความเร็วขึ้นกับ load factor = จำนวนข้อมูล / จำนวนบักเก็ต
   ถ้า load factor สูงมาก โซ่ในแต่ละบักเก็ตจะยาว ประสิทธิภาพตกลงเป็น O(n/k)
   จนอาจแพ้ AVL ที่รับประกัน O(log n) เสมอได้
   ============================================================ */
void search_benchmark(System *sys) {
    const int ROUNDS = 200000;
    (void)sys;

    printf("\n--- เทียบความเร็วค้นหา: Hash Table vs AVL Tree ---\n");
    printf("  ค้นซ้ำขนาดละ %d ครั้ง  (ตาราง Hash มี %d บักเก็ต)\n\n", ROUNDS, HASH_SIZE);

    bench_one(100,  ROUNDS);
    bench_one(500,  ROUNDS);   /* = MAX_BOOKINGS ขีดจำกัดจริงของระบบนี้ */
    bench_one(2000, ROUNDS);
    bench_one(5000, ROUNDS);

    printf("\n  อ่านผลอย่างไร:\n");
    printf("    load factor = จำนวนข้อมูล / จำนวนบักเก็ต = ความยาวโซ่เฉลี่ย\n");
    printf("    ตราบใดที่ load factor ต่ำ Hash จะชนะเพราะเทียบ key แค่ 1-2 ครั้ง\n");
    printf("    พอ load factor สูงขึ้น โซ่ยาวขึ้น Hash ตกเป็น O(n/k) จนแพ้ AVL ได้\n");
    printf("    ระบบนี้จำกัดที่ %d รายการ จึงยังอยู่ในช่วงที่ Hash ได้เปรียบ\n", MAX_BOOKINGS);
    printf("\n  เหตุผลที่เก็บทั้งสองโครงสร้าง:\n");
    printf("    Hash ใช้ค้นตั๋วจากรหัส (เมนู 4) -- เร็วแต่ไม่มีลำดับ\n");
    printf("    AVL ใช้แสดงรายการเรียงตามรหัส (เมนู 5) -- inorder ได้ผลเรียงทันที\n");
}
