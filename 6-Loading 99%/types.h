/* ============================================================
   types.h  --  ชนิดข้อมูลกลางที่ทุกโมดูลใช้ร่วมกัน
   แก้ค่าคงที่ตรงนี้ที่เดียว มีผลทั้งโปรเจกต์
   ============================================================ */
#ifndef TYPES_H
#define TYPES_H

#include "data.h"   /* BoatType, DAY_COUNT, MAX_DEPART_TIMES */

#define MAX_SEATS_PER_TRIP 256                      /* เพดานที่นั่งต่อ 1 รอบเรือ */
#define SEAT_BITMAP_BYTES  (MAX_SEATS_PER_TRIP / 8) /* 256 บิต = 32 ไบต์ */

#define MAX_NAME_LEN   96   /* ชื่อไทย UTF-8 กินตัวละ 3 ไบต์ ต้องเผื่อไว้ */
#define MAX_ID_LEN     16
#define MAX_PORTS      64      /* จำนวนท่าเรือสูงสุด เพิ่มได้ตามต้องการ */
#define INF_TIME       1000000000

/* ประเภทผู้โดยสาร -- ตัวเลขน้อย = ได้สิทธิ์ก่อน (ใช้ใน Priority Queue) */
typedef enum {
    PRIO_EXPRESS = 0,   /* จองแบบด่วน */
    PRIO_SENIOR  = 1,   /* ผู้สูงอายุ */
    PRIO_NORMAL  = 2    /* ทั่วไป */
} PassengerType;

typedef enum {
    STATUS_CONFIRMED,   /* ได้ที่นั่งแล้ว */
    STATUS_WAITLISTED,  /* อยู่ในคิวสำรอง */
    STATUS_CANCELLED    /* ยกเลิกแล้ว */
} BookingStatus;

/* หนึ่งรายการจอง -- ใช้ร่วมกันทุกโมดูล */
typedef struct {
    char           booking_id[MAX_ID_LEN];    /* เช่น "BK000001" */
    char           passenger_name[MAX_NAME_LEN];
    int            route_id;                  /* เส้นทางที่จอง */
    int            from_port;
    int            to_port;
    BoatType       boat;                      /* ชนิดเรือที่จอง */
    int            travel_day;                /* วันเดินทาง 0=อาทิตย์ .. 6=เสาร์ */
    int            depart_slot;               /* index ของรอบเวลาใน BOAT_INFO[boat].depart_times[] */
    int            seat_no;
    int            boarding_no;               /* ลำดับบัตรขึ้นเรือ 0 = ยังไม่ได้ออกบัตร */                   /* -1 = ยังไม่ได้ที่นั่ง */
    PassengerType  type;
    BookingStatus  status;
    long           created_at;                /* ลำดับที่จอง ใช้ตัดสินก่อน-หลัง */
} Booking;

#endif /* TYPES_H */
