/*
 * laundry_core.h — แกนของระบบจัดคิวเครื่องซักผ้า (ไม่ขึ้นกับระบบปฏิบัติการ ไม่มี GUI/เมนู)
 * ใช้ร่วมกันทั้งเวอร์ชันเมนู (laundry_console.c) และเวอร์ชันหน้าต่าง Windows (laundry_gui.c)
 */
#ifndef LAUNDRY_CORE_H
#define LAUNDRY_CORE_H

#define NUM_MACHINES 10
#define REPEAT       1000      /* จำนวนรอบวนค้นหาซ้ำ ตามที่อาจารย์กำหนด (ต้องระบุในรายงาน) */
#define RUNS         5         /* รันวัดกี่ครั้งต่อหนึ่งขนาดข้อมูล */
#define DEFAULT_WASH_MINUTES 45

/* สถานะเครื่อง / ผู้ใช้ */
enum { M_FREE = 0, M_BUSY = 1, M_BROKEN = 2 };
enum { U_WAIT = 0, U_WASH = 1, U_DONE = 2 };

typedef struct {            /* node ใน heap (เก็บแค่ข้อมูลที่ใช้เทียบ) */
    int  id;                /* รหัสผู้ใช้ */
    int  priority;          /* 1 = ซักด่วน, 2 = รอปกติ  (เลขน้อย = สำคัญกว่า) */
    long arrival;           /* ลำดับเวลาเข้าคิว ยิ่งน้อยยิ่งมาก่อน */
} QNode;

typedef struct {
    QNode *a;
    int    size;
    int    cap;
} MinHeap;

typedef struct {            /* ทะเบียนผู้ใช้ (เรียงตามรหัสเสมอ) */
    int  id;
    char name[64];          /* UTF-8 */
    int  priority;
    long arrival;
    int  duration;          /* เวลาซัก (นาที) */
    int  status;            /* U_WAIT / U_WASH / U_DONE */
    int  machineId;         /* เครื่องที่ใช้อยู่ (0 = ไม่มี) */
} User;

typedef struct {
    int id;                 /* 1..NUM_MACHINES เรียงจากน้อยไปมาก (จำเป็นต่อ Binary Search) */
    int status;             /* M_FREE / M_BUSY / M_BROKEN */
    int userId;             /* ผู้ใช้ปัจจุบัน (0 = ไม่มี) */
    int remaining;          /* เวลาซักที่เหลือ (นาที) */
} Machine;

extern Machine machines[NUM_MACHINES];
extern User   *users;
extern int     userCount;
extern MinHeap heap;

/* ข้อความที่ระบบแจ้งออกมา (UTF-8) ถ้าเป็น NULL จะพิมพ์ลง stdout */
extern void (*core_out)(const char *utf8);
void core_printf(const char *fmt, ...);

void core_init(void);                 /* เริ่มต้น/ล้างระบบทั้งหมด */

/* คำสั่งของระบบ  คืน 0 = สำเร็จ, ค่าลบ = ผิดพลาด (ข้อความอธิบายส่งผ่าน core_out แล้ว) */
int  core_add_user(int id, const char *name, int priority, int duration);
int  core_finish(int machineId);      /* ซักเสร็จ -> คืนเครื่อง + เรียกคิวถัดไป */
int  core_broken(int machineId);      /* เครื่องเสีย -> ผู้ใช้กลับเข้าคิว priority 1 */
int  core_repair(int machineId);      /* ซ่อมเสร็จ */
void core_advance(int minutes);       /* เดินเวลาจำลอง: เครื่องที่ซักครบเวลาจะว่างเอง */

/* ค้นหา (Binary Search) คืน index หรือ -1 */
int  core_find_machine(int id);
int  core_find_user(int id);
void core_search_machine(int id);
void core_search_user(int id);

/* คิวที่รอ เรียงตาม priority แล้วเวลาเข้าคิว คืนจำนวน (เขียนไม่เกิน max) */
int  core_queue_sorted(QNode *out, int max);
void core_show_queue(void);
void core_show_machines(void);
const char *core_status_text(int machineStatus);

/* ทดสอบจับเวลาด้วยข้อมูลกลาง: dir = โฟลเดอร์ที่มี data_N.txt, data_N_sorted.txt, targets_N.txt */
void core_benchmark(const char *dir);

#endif
