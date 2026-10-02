/* ============================================================
   booking.h  --  ตัวระบบหลัก มัด ADT ทั้ง 5 เข้าด้วยกัน
   ไฟล์นี้คือหัวใจของโปรเจกต์ ควรเขียนเป็นลำดับที่ 2 ต่อจาก graph
   ============================================================ */
#ifndef BOOKING_H
#define BOOKING_H

#include "types.h"
#include "graph.h"
#include "hashtable.h"
#include "avltree.h"
#include "queue.h"
#include "pqueue.h"
#include "stack.h"

#define MAX_BOOKINGS 500   /* จำนวนรายการจองสูงสุดที่ระบบเก็บได้ */

typedef struct {
    Graph         *graph;     /* ผังท่าเรือ + เส้นทาง        */
    HashTable     *index;     /* booking_id -> Booking*  O(1) */
    AVLTree       *sorted;    /* booking_id เรียงลำดับ  O(log n) */
    Queue         *pending;   /* คิวจองตามลำดับก่อน-หลัง (FIFO) */
    PriorityQueue *waitlist;  /* คิวสำรอง เรียงตามสิทธิ์      */
    Stack         *history;   /* ประวัติสำหรับ Undo           */
    long           seq;       /* running number สร้าง booking id */
    int            boarding_seq;  /* ตัวนับลำดับบัตรขึ้นเรือ (Queue) */

    /* เจ้าของหน่วยความจำตัวจริงของทุกรายการจอง
       ADT อื่นเก็บแค่ pointer ชี้มาที่นี่ จึงไม่มีปัญหา dangling pointer
       และ free ที่เดียวตอน system_destroy */
    Booking       *all[MAX_BOOKINGS];
    int            all_count;
} System;

System *system_create(void);            /* จองหน่วยความจำให้ ADT ทุกตัว */
void    system_destroy(System *sys);    /* คืนหน่วยความจำทั้งหมด        */
int     system_load_sample_data(System *sys);  /* โหลดท่าเรือจาก data.c */

/* --- การจอง --- */
char    *booking_make_id(System *sys, char *out);   /* สร้างรหัส "BK000001" */
/* จองตั๋ว 1 ใบ
   boat  = ชนิดเรือ, day = 0..6 (0=อาทิตย์), slot = index รอบเวลาใน BOAT_INFO[boat]
   คืน NULL ถ้าไม่มีเรือวิ่งเส้นนี้ / วันนั้นเรือหยุด / รอบเวลาไม่ถูกต้อง */
Booking *booking_create(System *sys, const char *name,
                        int from_port, int to_port, PassengerType type,
                        BoatType boat, int day, int slot);
Booking *booking_find(System *sys, const char *booking_id);   /* O(1) ผ่าน hash */
int      booking_cancel(System *sys, const char *booking_id);
int      booking_undo(System *sys);                 /* ย้อน 1 ขั้นจาก Stack */

/* --- คิวสำรอง --- */
long     waitlist_priority(PassengerType type, long seq); /* คำนวณลำดับสิทธิ์ */
int      waitlist_promote(System *sys, int route_id);    /* ดึงคนขึ้นแทนที่ว่าง */

/* --- แสดงผล --- */
void     booking_show(const System *sys, const Booking *b);

/* --- เรียกคิวขึ้นเรือ: ดึงจาก Queue (FIFO) ตามลำดับที่จองเข้ามาจริง --- */
Booking *booking_call_next(System *sys);              /* O(1) ต่อการเรียก 1 คน */
void     boarding_queue_show(const System *sys);      /* O(n) ดูว่าใครยังรอออกบัตร */

/* --- ล้างรายการที่ยกเลิกออกจาก Hash Table และ AVL Tree --- */
int      booking_purge_cancelled(System *sys);        /* คืนจำนวนที่ล้าง */

/* --- เทียบความเร็ว Hash Table O(1) กับ AVL Tree O(log n) --- */
void     search_benchmark(System *sys);
void     booking_list_all(System *sys);
void     waitlist_show(const System *sys);

#endif /* BOOKING_H */
