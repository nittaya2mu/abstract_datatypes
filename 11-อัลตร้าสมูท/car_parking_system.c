#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ===================================================
 1. ค่าคงที่ของระบบ (Constants)
 =================================================== */
#define FLOORS 3
#define COLS 4
#define ROWS 5
#define SLOTS_PER_FLOOR (COLS * ROWS)

#define TYPE_DISABLED 1
#define TYPE_ELDERLY 2 
#define TYPE_NORMAL 3  

#define MAX_PARK_HOURS 2
#define MAX_PARK_SECONDS (MAX_PARK_HOURS * 3600)
#define FINE_INTERVAL_SECONDS 1800             
#define FINE_RATE_PER_INTERVAL 50              

#define REPEAT 1000  /* วนค้นหาซ้ำ เพื่อให้เวลามากพอที่จะวัดได้ */

/* ===================================================
 2. โครงสร้างข้อมูล (Data Structures)
 =================================================== */
typedef struct
{
    char plate[20];    
    int type;          
    int occupied;      
    time_t parked_time;
} Car;

Car parking[FLOORS][COLS][ROWS];

/* ===================================================
 3. ฟังก์ชันตรรกะระบบ (Business Logic Functions)
 =================================================== */
int canPark(int floor, int type)
{
    if (floor == 0)
    {
        return (type == TYPE_DISABLED || type == TYPE_ELDERLY);
    }
    if (floor == 1 || floor == 2)
    {
        return (type == TYPE_NORMAL);
    }
    return 0;
}

int calculateFine(time_t parked_time, time_t exit_time, long *outOvertimeSec, int *outTotalBlocks)
{
    double totalSeconds = difftime(exit_time, parked_time);
    if (totalSeconds < 0)
        totalSeconds = 0;
    long overtime = (long)totalSeconds - MAX_PARK_SECONDS;
    
    if (overtime <= 0)
    {
        if (outOvertimeSec)
            *outOvertimeSec = 0;
        if (outTotalBlocks)
            *outTotalBlocks = 0;
        return 0;
    }
    
    int blocks = (int)((overtime + FINE_INTERVAL_SECONDS - 1) / FINE_INTERVAL_SECONDS);
    int fine = blocks * FINE_RATE_PER_INTERVAL;
    if (outOvertimeSec)
        *outOvertimeSec = overtime;
    if (outTotalBlocks)
        *outTotalBlocks = blocks;
    return fine;
}

int isPlateExist(const char plate[])
{
    for (int f = 0; f < FLOORS; f++)
    {
        for (int c = 0; c < COLS; c++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                if (parking[f][c][r].occupied && strcmp(parking[f][c][r].plate, plate) == 0)
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

int findAvailableSlot(int type, int *outFloor, int *outCol, int *outRow)
{
    if (type == TYPE_DISABLED || type == TYPE_ELDERLY)
    {
        for (int c = 0; c < COLS; c++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                if (!parking[0][c][r].occupied)
                {
                    *outFloor = 0;
                    *outCol = c;
                    *outRow = r;
                    return 1;
                }
            }
        }
        return 0;
    }
    if (type == TYPE_NORMAL)
    {
        for (int c = 0; c < COLS; c++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                if (!parking[1][c][r].occupied)
                {
                    *outFloor = 1;
                    *outCol = c;
                    *outRow = r;
                    return 1;
                }
            }
        }
        for (int c = 0; c < COLS; c++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                if (!parking[2][c][r].occupied)
                {
                    *outFloor = 2;
                    *outCol = c;
                    *outRow = r;
                    return 1;
                }
            }
        }
        return 0;
    }
    return 0;
}

int removeCarByPlate(const char plate[], int *outFloor, int *outCol, int *outRow, time_t *outParkedTime, int *outFine)
{
    time_t exit_time = time(NULL);
    for (int f = 0; f < FLOORS; f++)
    {
        for (int c = 0; c < COLS; c++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                if (parking[f][c][r].occupied && strcmp(parking[f][c][r].plate, plate) == 0)
                {
                    *outFloor = f;
                    *outCol = c;
                    *outRow = r;
                    if (outParkedTime)
                        *outParkedTime = parking[f][c][r].parked_time;
                    if (outFine)
                        *outFine = calculateFine(parking[f][c][r].parked_time, exit_time, NULL, NULL);
                    
                    parking[f][c][r].occupied = 0;
                    parking[f][c][r].plate[0] = '\0';
                    parking[f][c][r].type = 0;
                    parking[f][c][r].parked_time = 0;
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void setInitialCar(int f, int c, int r, const char plate[], int type, int minutes_ago)
{
    strcpy(parking[f][c][r].plate, plate);
    parking[f][c][r].type = type;
    parking[f][c][r].occupied = 1;
    parking[f][c][r].parked_time = time(NULL) - (minutes_ago * 60);
}

void initParking(void)
{
    for (int f = 0; f < FLOORS; f++)
    {
        for (int c = 0; c < COLS; c++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                parking[f][c][r].occupied = 0;
                parking[f][c][r].plate[0] = '\0';
                parking[f][c][r].type = 0;
                parking[f][c][r].parked_time = 0;
            }
        }
    }
    setInitialCar(0, 0, 0, "1กก-1111", TYPE_DISABLED, 45); 
    setInitialCar(0, 0, 1, "2ขข-2222", TYPE_ELDERLY, 140); 
    setInitialCar(0, 0, 2, "3คค-3333", TYPE_DISABLED, 90); 
    setInitialCar(0, 0, 3, "4งง-4444", TYPE_ELDERLY, 195); 
    setInitialCar(0, 0, 4, "5จจ-5555", TYPE_DISABLED, 30); 
    setInitialCar(0, 1, 0, "6ฉฉ-6666", TYPE_ELDERLY, 160); 
    setInitialCar(0, 1, 1, "7ชช-7777", TYPE_DISABLED, 15); 
    
    setInitialCar(1, 0, 0, "1กข-1001", TYPE_NORMAL, 60);
    setInitialCar(1, 0, 1, "2คง-1002", TYPE_NORMAL, 130); 
    setInitialCar(1, 0, 2, "3จฉ-1003", TYPE_NORMAL, 180); 
    setInitialCar(1, 0, 3, "4ชซ-1004", TYPE_NORMAL, 20);
    setInitialCar(1, 0, 4, "5ฌญ-1005", TYPE_NORMAL, 150); 
    setInitialCar(1, 1, 0, "6ฎฏ-1006", TYPE_NORMAL, 75);
    setInitialCar(1, 1, 1, "7ฐฑ-1007", TYPE_NORMAL, 260); 
    setInitialCar(1, 1, 2, "8ฒณ-1008", TYPE_NORMAL, 50);
    
    setInitialCar(2, 0, 0, "1ดต-2001", TYPE_NORMAL, 40);
    setInitialCar(2, 0, 1, "2ถท-2002", TYPE_NORMAL, 125); 
    setInitialCar(2, 0, 2, "3ธน-2003", TYPE_NORMAL, 85);
    setInitialCar(2, 0, 3, "4บป-2004", TYPE_NORMAL, 15);
    setInitialCar(2, 0, 4, "5ผฝ-2005", TYPE_NORMAL, 110);
    setInitialCar(2, 1, 0, "6พฟ-2006", TYPE_NORMAL, 95);
}

/* ===================================================
 4. ฟังก์ชันอัลกอริทึมค้นหา (Sequential Search)
 =================================================== */
int sequentialSearch(Car activeCars[], int n, const char targetPlate[])
{
    for (int i = 0; i < n; i++)
    {
        if (strcmp(activeCars[i].plate, targetPlate) == 0)
        {
            return i; // คืนค่า Index ที่พบ
        }
    }
    return -1; // คืนค่า -1 เมื่อไม่พบ
}

/* ===================================================
 5. ฟังก์ชันจับเวลา (Benchmark ตามรูปแบบของอาจารย์)
 =================================================== */
void runSearchBenchmark(void)
{
    Car activeCars[FLOORS * COLS * ROWS];
    int n = 0; // จำนวนรถที่จอดอยู่จริง (ไม่นับช่องว่าง)
    char target[20];
    
    clock_t start, end;
    double total_sec, avg_msec;
    int i, result;

    /* ---- เตรียมข้อมูลและรับค่าที่ต้องการค้นหา (ไม่จับเวลาส่วนนี้) ---- */
    
    // ดึงเฉพาะรถที่จอดจริงใส่ใน Array
    for (int f = 0; f < FLOORS; f++)
    {
        for (int c = 0; c < COLS; c++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                if (parking[f][c][r].occupied)
                {
                    activeCars[n] = parking[f][c][r];
                    n++;
                }
            }
        }
    }

    printf("\n============ BENCHMARK ============\n");
    printf("จำนวนรถที่จอดอยู่จริง (n) : %d คัน (จากทั้งหมด %d ช่อง)\n", n, FLOORS * COLS * ROWS);
    printf("ป้อนป้ายทะเบียนที่ต้องการค้นหา : ");
    scanf("%19s", target);

    /* ---- เริ่มจับเวลาเฉพาะการค้นหา ---- */
    start = clock();                             /* เริ่มจับเวลา */
    for (i = 0; i < REPEAT; i++) {
        result = sequentialSearch(activeCars, n, target); /* เรียกอัลกอริทึมการค้นหา */
    }
    end = clock();                               /* หยุดจับเวลา */

    total_sec = (double)(end - start) / CLOCKS_PER_SEC;
    avg_msec = total_sec * 1000.0 / REPEAT;

    /* ---- แสดงผลลัพธ์การจับเวลาตามรูปแบบที่กำหนด ---- */
    printf("\n------------------------------------\n");
    printf("n = %d\n", n);
    printf("จำนวนรอบที่วัด : %d รอบ\n", REPEAT);
    printf("เวลารวม : %.6f วินาที\n", total_sec);
    printf("เวลาเฉลี่ยต่อครั้ง : %.6f มิลลิวินาที\n", avg_msec);
    printf("ผลการค้นหา : %d\n", result);
    if (result != -1)
    {
        printf("สถานะ : พบข้อมูลที่ Index [%d] (ทะเบียน %s)\n", result, activeCars[result].plate);
    }
    else
    {
        printf("สถานะ : ไม่พบข้อมูลในระบบ (-1)\n");
    }
    printf("------------------------------------\n");
}

/* ===================================================
 6. ฟังก์ชันส่วนติดต่อผู้ใช้ (UI Functions)
 =================================================== */
void showMainMenu(void)
{
    printf("\n");
    printf("====================================\n");
    printf(" CAR PARKING SYSTEM\n");
    printf("====================================\n");
    printf("1. Search ด้วยป้ายทะเบียน\n");
    printf("2. Add รถ (จัดสรรช่องจอดอัตโนมัติ)\n");
    printf("3. Remove รถ (นำรถออก + คิดค่าปรับ)\n");
    printf("4. Edit รถ\n");
    printf("5. แสดงที่จอดรถ\n");
    printf("6. ทดสอบจับเวลา\n");
    printf("7. Exit\n");
    printf("====================================\n");
}

void showType(int type)
{
    if (type == TYPE_DISABLED)
        printf("คนพิการ");
    else if (type == TYPE_ELDERLY)
        printf("ผู้สูงอายุ");
    else if (type == TYPE_NORMAL)
        printf("คนปกติ");
    else
        printf("ไม่ระบุ");
}

void displayCar(Car car)
{
    if (!car.occupied)
    {
        printf("ว่าง");
        return;
    }
    time_t now = time(NULL);
    int fine = calculateFine(car.parked_time, now, NULL, NULL);
    double totalSec = difftime(now, car.parked_time);
    int hours = (int)(totalSec / 3600);
    int mins = (int)(((long)totalSec % 3600) / 60);
    printf("%-10s - ", car.plate);
    showType(car.type);
    printf(" (จอดแล้ว : %d ชม. %02d นาที", hours, mins);
    if (fine > 0)
    {
        printf(" | ⚠️ ค่าปรับ : %d บาท)", fine);
    }
    else
    {
        printf(")");
    }
}

void displayParking(void)
{
    printf("\n========================================================================\n");
    printf(" PARKING STATUS\n");
    printf("========================================================================\n");
    time_t now = time(NULL);
    for (int f = 0; f < FLOORS; f++)
    {
        int count = 0, fineCount = 0;
        for (int c = 0; c < COLS; c++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                if (parking[f][c][r].occupied)
                {
                    count++;
                    if (calculateFine(parking[f][c][r].parked_time, now, NULL, NULL) > 0)
                    {
                        fineCount++;
                    }
                }
            }
        }
        printf("\n>>> ชั้น %d (Index [%d]) <<<\n", f + 1, f);
        printf("นโยบาย : %s\n", (f == 0) ? "คนพิการ / ผู้สูงอายุเท่านั้น" : "คนปกติเท่านั้น");
        printf("อัตราค่าจอด : ฟรี 2 ชม.แรก (ส่วนเกินปรับทุก 30 นาที ละ 50 บาท)\n");
        printf("สถานะ : จอดแล้ว %d/%d ช่อง (ว่าง %d ช่อง | จอดเกินเวลา %d คัน)\n",
               count, SLOTS_PER_FLOOR, SLOTS_PER_FLOOR - count, fineCount);
        printf("------------------------------------------------------------------------\n");
        for (int c = 0; c < COLS; c++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                int slotNumber = (c * ROWS + r) + 1;
                printf("ช่อง %02d [%d][%d][%d] : ", slotNumber, f, c, r);
                displayCar(parking[f][c][r]);
                printf("\n");
            }
        }
    }
    printf("========================================================================\n");
}

void searchCar(void)
{
    char plate[20];
    printf("\n========== SEARCH ==========\n");
    printf("ป้อนป้ายทะเบียน : ");
    scanf("%19s", plate);
    time_t now = time(NULL);
    for (int f = 0; f < FLOORS; f++)
    {
        for (int c = 0; c < COLS; c++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                if (parking[f][c][r].occupied && strcmp(parking[f][c][r].plate, plate) == 0)
                {
                    int slotNumber = (c * ROWS + r) + 1;
                    time_t parked = parking[f][c][r].parked_time;
                    char timeStr[64];
                    struct tm tm_info = *localtime(&parked);
                    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm_info);
                    double totalSec = difftime(now, parked);
                    int hours = (int)(totalSec / 3600);
                    int mins = (int)(((long)totalSec % 3600) / 60);
                    int secs = (int)((long)totalSec % 60);
                    long overtimeSec = 0;
                    int blocks = 0;
                    int fine = calculateFine(parked, now, &overtimeSec, &blocks);
                    printf("\nพบข้อมูลรถ\n");
                    printf("------------------------------------\n");
                    printf("ป้ายทะเบียน : %s\n", parking[f][c][r].plate);
                    printf("ชั้น : %d\n", f + 1);
                    printf("พิกัด 3D : [%d][%d][%d] (Col: %d, Row: %d)\n", f, c, r, c, r);
                    printf("ช่องที่ : %d (จาก 20 ช่อง)\n", slotNumber);
                    printf("ประเภท : ");
                    showType(parking[f][c][r].type);
                    printf("\n");
                    printf("เวลาเข้าจอด : %s\n", timeStr);
                    printf("ระยะเวลาจอด : %d ชั่วโมง %d นาที %d วินาที\n", hours, mins, secs);
                    if (fine > 0)
                    {
                        printf("สถานะเวลา : ⚠️ จอดเกินกำหนด (เกินมา %ld นาที)\n", overtimeSec / 60);
                        printf("ช่วงการปรับ : %d ช่วง (ช่วงละ 30 นาที ละ 50 บาท)\n", blocks);
                        printf("ค่าปรับสะสม : %d บาท\n", fine);
                    }
                    else
                    {
                        printf("สถานะเวลา : ปกติ (ไม่เกิน 2 ชั่วโมง)\n");
                        printf("ค่าปรับสะสม : 0 บาท\n");
                    }
                    printf("------------------------------------\n");
                    return;
                }
            }
        }
    }
    printf("\nไม่พบป้ายทะเบียน %s ในระบบ\n", plate);
}

void addCar(void)
{
    char plate[20];
    int type, f, c, r;
    printf("\n========== ADD CAR ==========\n");
    printf("ป้อนป้ายทะเบียน : ");
    scanf("%19s", plate);
    if (isPlateExist(plate))
    {
        printf("\nข้อผิดพลาด : ป้ายทะเบียนนี้มีอยู่ในระบบแล้ว\n");
        return;
    }
    printf("\nประเภทผู้ใช้\n");
    printf("1. คนพิการ\n");
    printf("2. ผู้สูงอายุ\n");
    printf("3. คนปกติ\n");
    printf("เลือกประเภท: ");
    if (scanf("%d", &type) != 1 || type < TYPE_DISABLED || type > TYPE_NORMAL)
    {
        printf("\nประเภทไม่ถูกต้อง\n");
        while (getchar() != '\n');
        return;
    }
    if (!findAvailableSlot(type, &f, &c, &r))
    {
        if (type == TYPE_DISABLED || type == TYPE_ELDERLY)
        {
            printf("\nที่จอดรถชั้น 1 (สำหรับคนพิการและผู้สูงอายุ) เต็มแล้ว (20/20)\n");
        }
        else
        {
            printf("\nที่จอดรถสำหรับคนปกติ (ชั้น 2 และ 3) เต็มทั้งหมดแล้ว (40/40)\n");
        }
        return;
    }
    time_t now = time(NULL);
    strcpy(parking[f][c][r].plate, plate);
    parking[f][c][r].type = type;
    parking[f][c][r].occupied = 1;
    parking[f][c][r].parked_time = now;
    char timeStr[64];
    struct tm tm_info = *localtime(&now);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm_info);
    int slotNumber = (c * ROWS + r) + 1;
    printf("\nเพิ่มรถเรียบร้อยแล้ว (จัดช่องจอดอัตโนมัติ)\n");
    printf("------------------------------------\n");
    printf("ป้ายทะเบียน : %s\n", plate);
    printf("ชั้น : %d\n", f + 1);
    printf("พิกัด 3D : [%d][%d][%d] (Col: %d, Row: %d)\n", f, c, r, c, r);
    printf("ช่องที่ : %d\n", slotNumber);
    printf("ประเภท : ");
    showType(type);
    printf("\n");
    printf("เวลาเข้าจอด : %s\n", timeStr);
    printf("ข้อกำหนด : ฟรี 2 ชั่วโมงแรก (ส่วนเกินปรับทุก 30 นาที ละ 50 บาท)\n");
    printf("------------------------------------\n");
}

void removeCar(void)
{
    char plate[20];
    int f, c, r, fine = 0;
    time_t parkedTime = 0;
    printf("\n========== REMOVE CAR ==========\n");
    printf("ป้อนป้ายทะเบียนที่ต้องการนำออก : ");
    scanf("%19s", plate);
    if (removeCarByPlate(plate, &f, &c, &r, &parkedTime, &fine))
    {
        time_t exitTime = time(NULL);
        int slotNumber = (c * ROWS + r) + 1;
        char enterStr[64], exitStr[64];
        struct tm t1 = *localtime(&parkedTime);
        strftime(enterStr, sizeof(enterStr), "%Y-%m-%d %H:%M:%S", &t1);
        struct tm t2 = *localtime(&exitTime);
        strftime(exitStr, sizeof(exitStr), "%Y-%m-%d %H:%M:%S", &t2);
        double totalSec = difftime(exitTime, parkedTime);
        int hours = (int)(totalSec / 3600);
        int mins = (int)(((long)totalSec % 3600) / 60);
        printf("\n================================================\n");
        printf(" ใบเสร็จและสรุปการนำรถออก \n");
        printf("================================================\n");
        printf("ป้ายทะเบียน : %s\n", plate);
        printf("ออกจากตำแหน่ง : ชั้น %d พิกัด [%d][%d][%d] (ช่อง %d)\n", f + 1, f, c, r, slotNumber);
        printf("เวลาเข้าจอด : %s\n", enterStr);
        printf("เวลาออกจากที่จอด : %s\n", exitStr);
        printf("รวมระยะเวลาจอด : %d ชั่วโมง %d นาที\n", hours, mins);
        printf("เงื่อนไข : ฟรี 2 ชั่วโมงแรก (ส่วนเกินปรับทุก 30 นาที ละ 50 บาท)\n");
        if (fine > 0)
        {
            long overSec = (long)totalSec - MAX_PARK_SECONDS;
            int blocks = (int)((overSec + FINE_INTERVAL_SECONDS - 1) / FINE_INTERVAL_SECONDS);
            printf("สถานะ : ⚠️ จอดเกินกำหนด (เกินมา %ld นาที)\n", overSec / 60);
            printf("คิดค่าปรับ : %d ช่วง (ช่วงละ 30 นาที ละ 50 บาท)\n", blocks);
            printf("ยอดค่าปรับชำระ : %d บาท\n", fine);
        }
        else
        {
            printf("สถานะ : ปกติ (จอดไม่เกิน 2 ชั่วโมง)\n");
            printf("ยอดค่าปรับชำระ : 0 บาท (ฟรี)\n");
        }
        printf("------------------------------------------------\n");
        printf("สถานะช่องจอด : คืนพื้นที่เรียบร้อยแล้ว ช่องนี้ว่างพร้อมรับรถคันใหม่\n");
        printf("================================================\n");
    }
    else
    {
        printf("\nไม่พบป้ายทะเบียน %s ในระบบ\n", plate);
    }
}

void editCar(void)
{
    char plate[20], newPlate[20];
    int newType;
    printf("\n========== EDIT CAR ==========\n");
    printf("ป้อนป้ายทะเบียนที่ต้องการแก้ไข : ");
    scanf("%19s", plate);
    for (int f = 0; f < FLOORS; f++)
    {
        for (int c = 0; c < COLS; c++)
        {
            for (int r = 0; r < ROWS; r++)
            {
                if (parking[f][c][r].occupied && strcmp(parking[f][c][r].plate, plate) == 0)
                {
                    int slotNumber = (c * ROWS + r) + 1;
                    printf("\nพบข้อมูลรถ (ชั้น %d ช่อง %d พิกัด [%d][%d][%d])\n", f + 1, slotNumber, f, c, r);
                    printf("ป้ายทะเบียนเดิม : %s\n", parking[f][c][r].plate);
                    printf("ประเภทเดิม : ");
                    showType(parking[f][c][r].type);
                    printf("\n");
                    printf("\nป้อนป้ายทะเบียนใหม่ : ");
                    scanf("%19s", newPlate);
                    if (strcmp(plate, newPlate) != 0 && isPlateExist(newPlate))
                    {
                        printf("\nป้ายทะเบียนใหม่มีอยู่ในระบบแล้ว\n");
                        return;
                    }
                    printf("ประเภทใหม่ (1.คนพิการ, 2.ผู้สูงอายุ, 3.คนปกติ) : ");
                    if (scanf("%d", &newType) != 1 || newType < TYPE_DISABLED || newType > TYPE_NORMAL)
                    {
                        printf("\nประเภทไม่ถูกต้อง\n");
                        while (getchar() != '\n');
                        return;
                    }
                    if (!canPark(f, newType))
                    {
                        printf("\nไม่สามารถเปลี่ยนประเภทได้: ชั้น %d ไม่รองรับประเภทนี้\n", f + 1);
                        return;
                    }
                    strcpy(parking[f][c][r].plate, newPlate);
                    parking[f][c][r].type = newType;
                    printf("\nแก้ไขข้อมูลเรียบร้อยแล้ว\n");
                    return;
                }
            }
        }
    }
    printf("\nไม่พบป้ายทะเบียน %s ในระบบ\n", plate);
}

/* ===================================================
 7. ฟังก์ชันหลัก (Main Function)
 =================================================== */
int main(void)
{
    int menu;
    initParking();
    while (1)
    {
        showMainMenu();
        printf("เลือกเมนู: ");
        if (scanf("%d", &menu) != 1)
        {
            break;
        }
        switch (menu)
        {
        case 1:
            searchCar();
            break;
        case 2:
            addCar();
            break;
        case 3:
            removeCar();
            break;
        case 4:
            editCar();
            break;
        case 5:
            displayParking();
            break;
        case 6:
            runSearchBenchmark();
            break;
        case 7:
            printf("\nออกจากโปรแกรม ขอบคุณที่ใช้บริการ\n");
            return 0;
        default:
            printf("\nเมนูไม่ถูกต้อง กรุณาเลือก 1 - 7\n");
        }
    }
}