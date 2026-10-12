/*
 * laundry_console.c — เวอร์ชันเมนูบนเทอร์มินัล (ใช้ได้ทุกระบบ ใช้ส่งวัดเวลาบนเครื่องกลาง)
 *
 * คอมไพล์:   gcc -Wall -Wextra -o laundry laundry_console.c laundry_core.c
 * รัน:       ./laundry              (เมนู 8 จะถามโฟลเดอร์ข้อมูลกลาง Enter = โฟลเดอร์ปัจจุบัน)
 *            ./laundry <โฟลเดอร์>   (ระบุโฟลเดอร์ข้อมูลล่วงหน้า)
 */
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>   /* ใช้ตั้ง console เป็น UTF-8 บน Windows เพื่อให้ภาษาไทยไม่เพี้ยน */
#endif
#include "laundry_core.h"

#define INPUT_EOF (-999999)

static int read_int(const char *prompt)
{
    char buf[64];
    int v;
    for (;;) {
        printf("%s", prompt);
        if (fgets(buf, sizeof buf, stdin) == NULL) return INPUT_EOF;
        if (sscanf(buf, "%d", &v) == 1) return v;
        printf("กรุณากรอกเป็นตัวเลข\n");
    }
}

static int read_word(const char *prompt, char *out)
{
    char buf[128];
    printf("%s", prompt);
    if (fgets(buf, sizeof buf, stdin) == NULL) return 0;
    if (sscanf(buf, "%39s", out) != 1) strcpy(out, "no-name");
    return 1;
}

static void menu_add_user(void)
{
    char name[40];
    int id, pr;
    id = read_int("รหัสผู้ใช้ (ตัวเลข): ");
    if (id == INPUT_EOF) return;
    if (core_find_user(id) != -1) { printf("มีรหัสนี้อยู่แล้ว\n"); return; }
    if (!read_word("ชื่อ (ไม่เว้นวรรค): ", name)) return;
    do {
        pr = read_int("ประเภท (1 = ซักด่วน, 2 = รอปกติ): ");
        if (pr == INPUT_EOF) return;
    } while (pr != 1 && pr != 2);
    core_add_user(id, name, pr, pr == 1 ? 20 : DEFAULT_WASH_MINUTES);
}

static void menu_with_machine(const char *prompt, int (*fn)(int))
{
    int id = read_int(prompt);
    if (id == INPUT_EOF) return;
    fn(id);
}

static void menu_search_machine(void)
{
    int id = read_int("หมายเลขเครื่องที่ต้องการค้นหา: ");
    if (id != INPUT_EOF) core_search_machine(id);
}

static void menu_search_user(void)
{
    int id = read_int("รหัสผู้ใช้ที่ต้องการค้นหา: ");
    if (id != INPUT_EOF) core_search_user(id);
}

static void menu_benchmark(const char *defaultDir)
{
    char buf[512];
    const char *dir = defaultDir;
    if (!dir) {
        printf("โฟลเดอร์ที่เก็บไฟล์ข้อมูลกลาง (Enter = โฟลเดอร์ปัจจุบัน): ");
        if (fgets(buf, sizeof buf, stdin) == NULL) return;
        buf[strcspn(buf, "\r\n")] = '\0';
        dir = buf;
    }
    core_benchmark(dir);
}

static void print_menu(void)
{
    printf("\n==============================================\n");
    printf("  ระบบจัดคิวเครื่องซักผ้าหอพัก (SMART LAUNDRY QUEUE)\n");
    printf("==============================================\n");
    printf("1. เพิ่มผู้ใช้เข้าคิว (จัดเครื่องให้อัตโนมัติ)\n");
    printf("2. ซักเสร็จ (คืนเครื่อง + เรียกคิวถัดไป)\n");
    printf("3. แจ้งเครื่องเสีย (จัดผู้ใช้ใหม่)\n");
    printf("4. แจ้งซ่อมเสร็จ\n");
    printf("5. ค้นหาเครื่องด้วยหมายเลข (Binary Search)\n");
    printf("6. ค้นหาผู้ใช้ด้วยรหัส (Binary Search)\n");
    printf("7. แสดงคิวและสถานะเครื่อง\n");
    printf("8. ทดสอบจับเวลาด้วยข้อมูลกลางของอาจารย์\n");
    printf("9. Exit\n");
    printf("==============================================\n");
}

int main(int argc, char *argv[])
{
    const char *dataDir = (argc > 1) ? argv[1] : NULL;
    int choice;

#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
    core_init();

    for (;;) {
        print_menu();
        choice = read_int("เลือกเมนู: ");
        if (choice == INPUT_EOF || choice == 9) break;
        switch (choice) {
            case 1: menu_add_user();                                              break;
            case 2: menu_with_machine("หมายเลขเครื่องที่ซักเสร็จ: ", core_finish);  break;
            case 3: menu_with_machine("หมายเลขเครื่องที่เสีย: ", core_broken);      break;
            case 4: menu_with_machine("หมายเลขเครื่องที่ซ่อมเสร็จ: ", core_repair); break;
            case 5: menu_search_machine();                                        break;
            case 6: menu_search_user();                                           break;
            case 7: core_show_queue(); core_show_machines();                      break;
            case 8: menu_benchmark(dataDir);                                      break;
            default: printf("เมนูไม่ถูกต้อง\n");
        }
    }
    printf("จบการทำงาน\n");
    return 0;
}
