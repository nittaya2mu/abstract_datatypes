/* ============================================================
   data.h / data.c  --  ข้อมูลระบบจองเรือเที่ยวภูเก็ต
   *** แก้ท่าเรือ ชนิดเรือ เวลาเดินเรือ วันเดินเรือ ได้ที่ data.c ที่เดียว ***

   [สำหรับผู้รับงานต่อ]
   ไฟล์นี้เป็น "ชั้นข้อมูลดิบ" ล้วนๆ ไม่มี logic ไม่ขึ้นกับ ADT ใดๆ
   โมดูลอื่น (booking.c) เป็นฝ่ายอ่านค่าจากที่นี่ไปสร้าง Graph
   ============================================================ */
#ifndef DATA_H
#define DATA_H

/* ---------- ชนิดของเรือ ----------
   เพิ่มชนิดเรือใหม่: เติม enum ตรงนี้ แล้วเติมข้อมูลใน BOAT_INFO[] (data.c)
   ค่าตัวเลขต้องเรียงต่อกันจาก 0 และ BOAT_TYPE_COUNT ต้องอยู่ท้ายสุดเสมอ */
typedef enum {
    BOAT_FERRY     = 0,   /* เรือข้ามฟาก  - ช้า ถูก จุคนเยอะ */
    BOAT_SPEEDBOAT = 1,   /* เรือสปีดโบ๊ท - เร็ว แพง จุคนน้อย */
    BOAT_TYPE_COUNT       /* <-- ตัวนับ ห้ามลบ ห้ามย้าย */
} BoatType;

/* ---------- วันในสัปดาห์ ----------
   ใช้เป็น index ของ array service_days[7] */
typedef enum {
    DAY_SUN = 0, DAY_MON = 1, DAY_TUE = 2, DAY_WED = 3,
    DAY_THU = 4, DAY_FRI = 5, DAY_SAT = 6,
    DAY_COUNT = 7
} WeekDay;

#define MAX_DEPART_TIMES 8   /* จำนวนรอบเวลาต่อเรือ 1 ชนิด (เพิ่มได้) */

/* ---------- ข้อมูลประจำเรือแต่ละชนิด ---------- */
typedef struct {
    BoatType    type;
    const char *name;                          /* ชื่อไทย เช่น "เรือข้ามฟาก" */
    const char *code;                          /* ชื่อย่อ ASCII เช่น "FERRY" */
    int         depart_times[MAX_DEPART_TIMES];/* เวลาออก = นาทีนับจาก 00:00 */
    int         depart_count;                  /* ใช้จริงกี่รอบ */
    int         service_days[DAY_COUNT];       /* 1 = วันนั้นมีเรือ, 0 = ไม่มี */
} BoatInfo;

/* ---------- เส้นทางเดินเรือ 1 เส้น ---------- */
typedef struct {
    const char *from;
    const char *to;
    BoatType    boat;         /* เส้นนี้วิ่งด้วยเรือชนิดไหน */
    int         travel_time;  /* นาที  -- ใช้เป็น weight ของ Dijkstra */
    int         fare;         /* บาท */
    int         capacity;     /* ที่นั่งต่อรอบ */
} SampleRoute;

/* ---------- ตัวแปรข้อมูล (นิยามจริงอยู่ใน data.c) ---------- */
extern const char        *SAMPLE_PORTS[];
extern const int          SAMPLE_PORT_COUNT;
extern const SampleRoute  SAMPLE_ROUTES[];
extern const int          SAMPLE_ROUTE_COUNT;
extern const BoatInfo     BOAT_INFO[];
extern const char        *DAY_NAME_TH[DAY_COUNT];

/* ---------- ฟังก์ชันช่วยอ่านข้อมูล ---------- */
const BoatInfo *boat_info(BoatType t);                  /* O(1) คืน NULL ถ้าชนิดผิด */
const char     *boat_name(BoatType t);                  /* O(1) ชื่อไทยของเรือ */
int             boat_runs_on_day(BoatType t, int day);  /* O(1) 1 = วันนั้นวิ่ง */
void            minutes_to_hhmm(int minutes, char *out);/* 300 -> "05:00" (out >= 6 ไบต์) */
void            data_print_boat_schedule(void);         /* พิมพ์ตารางเวลา+วันเดินเรือ */

/* --- ตัวช่วยจัดคอลัมน์ให้ตรงเมื่อมีภาษาไทยปนอยู่ ---
   printf("%-10s") นับเป็น "ไบต์" ไม่ใช่ "ตัวอักษร"
   ภาษาไทย 1 ตัวกิน 3 ไบต์ใน UTF-8 คอลัมน์จึงเพี้ยนถ้าใช้ %-Ns ตรงๆ */
int             utf8_display_width(const char *s);      /* นับจำนวนตัวอักษรจริง */
void            print_padded(const char *s, int width); /* พิมพ์แล้วเติมช่องว่างให้ครบ width */

#endif /* DATA_H */
