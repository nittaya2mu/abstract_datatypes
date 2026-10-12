/*
 * laundry_gui.c — หน้าต่างโปรแกรม SmartLaundry สำหรับ Windows (Win32 API ล้วน ไม่ต้องติดตั้ง library เพิ่ม)
 * วิชา 01204212 แบบชนิดข้อมูลนามธรรมและการแก้ปัญหา — กลุ่ม 10
 *
 * หน้าต่างนี้เป็นเพียง "หน้าจอ" ตรรกะทั้งหมด (Min Heap, Binary Search, Sorting) อยู่ใน laundry_core.c
 *
 * คอมไพล์ด้วย MinGW (gcc บน Windows):
 *   gcc -O2 -Wall -o SmartLaundry.exe laundry_gui.c laundry_core.c -mwindows -lcomctl32 -lgdi32 -lshell32 -lole32
 * คอมไพล์ด้วย Visual Studio (Developer Command Prompt):
 *   cl /utf-8 /O2 laundry_gui.c laundry_core.c user32.lib gdi32.lib comctl32.lib shell32.lib ole32.lib /link /SUBSYSTEM:WINDOWS
 */
#define UNICODE
#define _UNICODE
#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_IE 0x0600
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "laundry_core.h"

/* ---------- ขนาดหน้าต่าง (พื้นที่ใช้งาน) ---------- */
#define CLIENT_W 1096
#define CLIENT_H 720
#define TILE_W   200
#define TILE_H   90
#define TILE_X0  24
#define TILE_Y0  160
#define TILE_GAP 12

/* ---------- รหัสของปุ่ม/ช่องกรอก ---------- */
enum {
    ID_EDIT_ID = 101, ID_EDIT_NAME, ID_COMBO, ID_BTN_ADD, ID_BTN_FINISH, ID_BTN_TOGGLE,
    ID_EDIT_SEARCH, ID_BTN_SEARCH, ID_BTN_DEMO, ID_BTN_ADV5, ID_BTN_ADV15, ID_BTN_RESET,
    ID_BTN_BENCH, ID_LIST, ID_LOG
};

/* ---------- สี ---------- */
#define COL_BG      RGB(243, 247, 250)
#define COL_NAVY    RGB(16, 37, 66)
#define COL_INK     RGB(27, 39, 51)
#define COL_GRAY    RGB(91, 107, 122)
#define COL_FREE    RGB(222, 245, 236)
#define COL_FREE_T  RGB(30, 120, 80)
#define COL_BUSY    RGB(222, 236, 252)
#define COL_BUSY_T  RGB(30, 90, 170)
#define COL_BROKEN  RGB(253, 228, 228)
#define COL_BROKEN_T RGB(185, 45, 45)
#define COL_ACCENT  RGB(90, 220, 235)

static HINSTANCE hInst;
static HWND hMain, hEditId, hEditName, hCombo, hList, hLog, hEditSearch, hBtnFinish, hBtnToggle;
static HFONT fNorm, fBold, fBig, fTitle, fSmall, fMono;
static HBRUSH hBgBrush;
static int selMachine = -1;      /* index ของเครื่องที่เลือก (-1 = ไม่ได้เลือก) */
static int simTime = 0;          /* เวลาจำลอง (นาที) */

/* ---------- ดักข้อความจาก core ---------- */
static char  *capBuf = NULL;
static size_t capLen = 0, capCap = 0;
static int    capturing = 0;

static void u8_to_w(const char *s, wchar_t *out, int n)
{
    if (!MultiByteToWideChar(CP_UTF8, 0, s, -1, out, n)) out[0] = 0;
}

static void log_line(const char *u8)
{
    wchar_t w[512];
    int c;
    u8_to_w(u8, w, 512);
    SendMessageW(hLog, LB_INSERTSTRING, 0, (LPARAM)w);       /* ใหม่สุดอยู่บนสุด */
    c = (int)SendMessageW(hLog, LB_GETCOUNT, 0, 0);
    if (c > 200) SendMessageW(hLog, LB_DELETESTRING, c - 1, 0);
}

static void cap_begin(void)
{
    capLen = 0;
    if (capBuf) capBuf[0] = 0;
    capturing = 1;
}

static void cap_end(void) { capturing = 0; }

static void cap_append(const char *s)
{
    size_t n = strlen(s);
    if (capLen + n + 1 > capCap) {
        capCap = (capLen + n + 1) * 2;
        capBuf = (char *)realloc(capBuf, capCap);
    }
    memcpy(capBuf + capLen, s, n);
    capLen += n;
    capBuf[capLen] = 0;
}

/* core_out: ข้อความจากระบบ -> เข้า buffer (ตอนดัก) หรือเข้ารายการเหตุการณ์ */
static void gui_out(const char *u8)
{
    char line[512];
    const char *p = u8, *q;
    size_t n;
    if (capturing) { cap_append(u8); return; }
    while (*p) {
        q = strchr(p, '\n');
        n = q ? (size_t)(q - p) : strlen(p);
        if (n >= sizeof line) n = sizeof line - 1;
        memcpy(line, p, n);
        line[n] = 0;
        {
            char *s = line;
            while (*s == ' ') s++;
            if (*s) log_line(s);
        }
        if (!q) break;
        p = q + 1;
    }
}

/* ---------- รีเฟรชหน้าจอ ---------- */
static void refresh_list(void)
{
    int i, n, ui, cnt = heap.size;
    QNode *q;
    wchar_t w[96];
    LVITEMW it;
    ListView_DeleteAllItems(hList);
    if (cnt <= 0) return;
    q = (QNode *)malloc(sizeof(QNode) * cnt);
    n = core_queue_sorted(q, cnt);
    for (i = 0; i < n; i++) {
        ui = core_find_user(q[i].id);
        memset(&it, 0, sizeof it);
        it.mask = LVIF_TEXT;
        it.iItem = i;
        it.iSubItem = 0;
        _snwprintf(w, 96, L"%d", i + 1);
        it.pszText = w;
        ListView_InsertItem(hList, &it);
        _snwprintf(w, 96, L"%d", q[i].id);
        ListView_SetItemText(hList, i, 1, w);
        u8_to_w(users[ui].name, w, 96);
        ListView_SetItemText(hList, i, 2, w);
        ListView_SetItemText(hList, i, 3, q[i].priority == 1 ? (LPWSTR)L"ซักด่วน (1)" : (LPWSTR)L"รอปกติ (2)");
        _snwprintf(w, 96, L"%d", users[ui].duration);
        ListView_SetItemText(hList, i, 4, w);
        _snwprintf(w, 96, L"%ld", q[i].arrival);
        ListView_SetItemText(hList, i, 5, w);
    }
    free(q);
}

static void refresh(void)
{
    int busy = (selMachine >= 0 && machines[selMachine].status == M_BUSY);
    int broken = (selMachine >= 0 && machines[selMachine].status == M_BROKEN);
    EnableWindow(hBtnFinish, busy);
    EnableWindow(hBtnToggle, selMachine >= 0);
    SetWindowTextW(hBtnToggle, broken ? L"ซ่อมเสร็จ" : L"แจ้งเครื่องเสีย");
    refresh_list();
    InvalidateRect(hMain, NULL, FALSE);
}

/* ---------- วาดหน้าจอ ---------- */
static RECT tile_rect(int i)
{
    RECT r;
    r.left = TILE_X0 + (i % 5) * (TILE_W + TILE_GAP);
    r.top = TILE_Y0 + (i / 5) * (TILE_H + 10);
    r.right = r.left + TILE_W;
    r.bottom = r.top + TILE_H;
    return r;
}

static void put_text(HDC dc, HFONT f, COLORREF c, int x, int y, int w, int h, const wchar_t *s, UINT fmt)
{
    RECT r;
    r.left = x; r.top = y; r.right = x + w; r.bottom = y + h;
    SelectObject(dc, f);
    SetTextColor(dc, c);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, s, -1, &r, fmt | DT_NOPREFIX);
}

static void paint_tile(HDC dc, int i)
{
    RECT r = tile_rect(i), in;
    Machine *m = &machines[i];
    COLORREF bg, fg;
    HBRUSH b;
    wchar_t w[160], nm[96];
    int bw = (i == selMachine) ? 3 : 1, ui;

    if (m->status == M_FREE)        { bg = COL_FREE;   fg = COL_FREE_T; }
    else if (m->status == M_BUSY)   { bg = COL_BUSY;   fg = COL_BUSY_T; }
    else                            { bg = COL_BROKEN; fg = COL_BROKEN_T; }

    b = CreateSolidBrush(i == selMachine ? COL_NAVY : RGB(200, 212, 224));
    FillRect(dc, &r, b);
    DeleteObject(b);
    in.left = r.left + bw; in.top = r.top + bw; in.right = r.right - bw; in.bottom = r.bottom - bw;
    b = CreateSolidBrush(bg);
    FillRect(dc, &in, b);
    DeleteObject(b);

    _snwprintf(w, 160, L"เครื่อง %d", m->id);
    put_text(dc, fBig, COL_INK, r.left + 12, r.top + 6, 110, 28, w, DT_SINGLELINE);
    _snwprintf(w, 160, L"\x25CF %ls", m->status == M_FREE ? L"ว่าง" : (m->status == M_BUSY ? L"ใช้งาน" : L"เสีย"));
    put_text(dc, fBold, fg, r.left + 110, r.top + 8, TILE_W - 120, 24, w, DT_SINGLELINE | DT_RIGHT);

    if (m->status == M_BUSY) {
        ui = core_find_user(m->userId);
        u8_to_w(users[ui].name, nm, 96);
        _snwprintf(w, 160, L"%ls  (รหัส %d)", nm, users[ui].id);
        put_text(dc, fNorm, COL_INK, r.left + 12, r.top + 38, TILE_W - 24, 22, w, DT_SINGLELINE | DT_END_ELLIPSIS);
        _snwprintf(w, 160, L"เหลือ %d นาที", m->remaining);
        put_text(dc, fSmall, COL_GRAY, r.left + 12, r.top + 60, TILE_W - 24, 18, w, DT_SINGLELINE);
        {   /* แถบความคืบหน้า */
            RECT tr, fr;
            int total = users[ui].duration > 0 ? users[ui].duration : 1;
            int done = total - m->remaining;
            if (done < 0) done = 0;
            if (done > total) done = total;
            tr.left = r.left + 12; tr.right = r.right - 12; tr.top = r.bottom - 12; tr.bottom = r.bottom - 6;
            b = CreateSolidBrush(RGB(200, 212, 224));
            FillRect(dc, &tr, b);
            DeleteObject(b);
            fr = tr;
            fr.right = tr.left + (tr.right - tr.left) * done / total;
            b = CreateSolidBrush(COL_BUSY_T);
            FillRect(dc, &fr, b);
            DeleteObject(b);
        }
    } else {
        put_text(dc, fNorm, COL_GRAY, r.left + 12, r.top + 42, TILE_W - 24, 22,
                 m->status == M_FREE ? L"พร้อมรับผู้ใช้" : L"รอซ่อม", DT_SINGLELINE);
    }
}

static void paint_all(HDC dc, const RECT *rc)
{
    RECT banner;
    wchar_t w[200];
    int i, nFree = 0, nBusy = 0, nBroken = 0;

    FillRect(dc, rc, hBgBrush);

    /* หัวเรื่อง */
    banner.left = 0; banner.top = 0; banner.right = rc->right; banner.bottom = 80;
    {
        HBRUSH b = CreateSolidBrush(COL_NAVY);
        FillRect(dc, &banner, b);
        DeleteObject(b);
    }
    put_text(dc, fTitle, RGB(255, 255, 255), 24, 6, 500, 44, L"SmartLaundry", DT_SINGLELINE);
    put_text(dc, fNorm, RGB(200, 214, 228), 26, 48, 800, 24,
             L"ระบบจัดคิวเครื่องซักผ้าหอพัก \x2022 ภาษา C \x2022 Min Heap + Binary Search", DT_SINGLELINE);
    put_text(dc, fBold, COL_ACCENT, rc->right - 324, 30, 300, 24, L"ADT / GROUP 10", DT_SINGLELINE | DT_RIGHT);

    /* สรุปสถานะ */
    for (i = 0; i < NUM_MACHINES; i++) {
        if (machines[i].status == M_FREE) nFree++;
        else if (machines[i].status == M_BUSY) nBusy++;
        else nBroken++;
    }
    _snwprintf(w, 200, L"ว่าง %d     กำลังใช้งาน %d     เสีย %d     รอคิว %d     |     เวลาจำลอง %d นาที",
               nFree, nBusy, nBroken, heap.size, simTime);
    put_text(dc, fNorm, COL_INK, 24, 130, 900, 24, w, DT_SINGLELINE);

    for (i = 0; i < NUM_MACHINES; i++) paint_tile(dc, i);

    /* ป้ายหัวข้อ */
    put_text(dc, fBold, COL_NAVY, 24, 368, 330, 24, L"เพิ่มผู้ใช้เข้าคิว", DT_SINGLELINE);
    put_text(dc, fSmall, COL_GRAY, 24, 392, 330, 18, L"รหัสผู้ใช้ (ตัวเลข)", DT_SINGLELINE);
    put_text(dc, fSmall, COL_GRAY, 24, 440, 330, 18, L"ชื่อ (ไม่เกิน 20 ตัวอักษร)", DT_SINGLELINE);
    put_text(dc, fSmall, COL_GRAY, 24, 488, 330, 18, L"ประเภทการซัก", DT_SINGLELINE);
    if (selMachine >= 0)
        _snwprintf(w, 200, L"เครื่องที่เลือก: เครื่อง %d (%ls)", machines[selMachine].id,
                   machines[selMachine].status == M_FREE ? L"ว่าง" : (machines[selMachine].status == M_BUSY ? L"ใช้งาน" : L"เสีย"));
    else
        _snwprintf(w, 200, L"เครื่องที่เลือก: (คลิกที่เครื่องด้านบน)");
    put_text(dc, fBold, COL_NAVY, 24, 582, 330, 22, w, DT_SINGLELINE);
    put_text(dc, fBold, COL_NAVY, 24, 642, 330, 20, L"ค้นหาผู้ใช้ด้วยรหัส (Binary Search)", DT_SINGLELINE);

    put_text(dc, fBold, COL_NAVY, 380, 368, 716, 24, L"คิวที่รออยู่ (เรียงตามความเร่งด่วน แล้วตามเวลาเข้าคิว)", DT_SINGLELINE);
    put_text(dc, fBold, COL_NAVY, 380, 570, 716, 22, L"บันทึกเหตุการณ์ (ล่าสุดอยู่บนสุด)", DT_SINGLELINE);
}

/* ---------- ตัวช่วยสร้างตัวควบคุม ---------- */
static HWND mk_btn(HWND p, const wchar_t *t, int id, int x, int y, int w, int h)
{
    return CreateWindowW(L"BUTTON", t, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                         x, y, w, h, p, (HMENU)(INT_PTR)id, hInst, NULL);
}

static HWND mk_edit(HWND p, int id, int x, int y, int w, int h, DWORD extra)
{
    return CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | extra,
                           x, y, w, h, p, (HMENU)(INT_PTR)id, hInst, NULL);
}

static BOOL CALLBACK setfont_cb(HWND h, LPARAM lp)
{
    SendMessageW(h, WM_SETFONT, (WPARAM)lp, TRUE);
    return TRUE;
}

static HFONT make_font(int height, int weight, const wchar_t *face)
{
    return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                       DEFAULT_PITCH | FF_DONTCARE, face);
}

static void add_column(int idx, int width, const wchar_t *title)
{
    LVCOLUMNW c;
    memset(&c, 0, sizeof c);
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.cx = width;
    c.pszText = (LPWSTR)title;
    ListView_InsertColumn(hList, idx, &c);
}

/* ---------- การทำงานของปุ่ม ---------- */
static void do_reset(void)
{
    core_init();
    simTime = 0;
    selMachine = -1;
    SendMessageW(hLog, LB_RESETCONTENT, 0, 0);
    refresh();
}

static void do_demo(void)
{
    int m;
    do_reset();
    for (m = 3; m <= NUM_MACHINES; m++) core_broken(m);   /* เหลือเครื่อง 1, 2 ให้เห็นการต่อคิวชัด */
    core_add_user(101, "Ann", 2, 45);
    core_add_user(102, "Bob", 2, 45);
    core_add_user(103, "Chai", 2, 45);
    core_add_user(104, "Dao", 1, 20);                      /* ซักด่วน: ต้องขึ้นก่อน Chai */
    core_add_user(105, "Eak", 2, 60);
    refresh();
}

static void do_add(void)
{
    wchar_t wid[32], wname[64], *end;
    char name[64];
    long id;
    int sel, k;

    GetWindowTextW(hEditId, wid, 32);
    GetWindowTextW(hEditName, wname, 64);
    id = wcstol(wid, &end, 10);
    if (wid[0] == 0 || *end != 0 || id <= 0) {
        MessageBoxW(hMain, L"กรุณากรอกรหัสผู้ใช้เป็นตัวเลขจำนวนเต็มบวก", L"ข้อมูลไม่ถูกต้อง", MB_OK | MB_ICONWARNING);
        SetFocus(hEditId);
        return;
    }
    if (wname[0] == 0 || !WideCharToMultiByte(CP_UTF8, 0, wname, -1, name, (int)sizeof name, NULL, NULL)) {
        MessageBoxW(hMain, L"กรุณากรอกชื่อผู้ใช้ (ไม่เกิน 20 ตัวอักษร)", L"ข้อมูลไม่ถูกต้อง", MB_OK | MB_ICONWARNING);
        SetFocus(hEditName);
        return;
    }
    for (k = 0; name[k]; k++) if (name[k] == ' ') name[k] = '_';

    sel = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
    if (sel == 0)      core_add_user((int)id, name, 1, 20);
    else if (sel == 1) core_add_user((int)id, name, 2, 45);
    else               core_add_user((int)id, name, 2, 60);

    SetWindowTextW(hEditId, L"");
    SetWindowTextW(hEditName, L"");
    refresh();
    SetFocus(hEditId);
}

static void do_search(void)
{
    wchar_t wid[32], res[512], *end;
    long id;
    GetWindowTextW(hEditSearch, wid, 32);
    id = wcstol(wid, &end, 10);
    if (wid[0] == 0 || *end != 0 || id <= 0) {
        MessageBoxW(hMain, L"กรุณากรอกรหัสผู้ใช้เป็นตัวเลขจำนวนเต็มบวก", L"ค้นหาผู้ใช้", MB_OK | MB_ICONWARNING);
        return;
    }
    cap_begin();
    core_search_user((int)id);
    cap_end();
    if (capBuf) {
        char *nl = strchr(capBuf, '\n');
        if (nl) *nl = 0;
        log_line(capBuf);
        u8_to_w(capBuf, res, 512);
        MessageBoxW(hMain, res, L"ผลการค้นหา (Binary Search)", MB_OK | MB_ICONINFORMATION);
    }
}

static int has_data(const char *dir)
{
    char p[700];
    FILE *f;
    snprintf(p, sizeof p, "%s/data_1000.txt", dir);
    f = fopen(p, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

static void show_text_window(const char *u8, const wchar_t *title)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, u8, -1, NULL, 0);
    wchar_t *w, *w2;
    size_t extra = 0, i, j = 0;
    HWND h;
    if (n <= 0) return;
    w = (wchar_t *)malloc(sizeof(wchar_t) * n);
    MultiByteToWideChar(CP_UTF8, 0, u8, -1, w, n);
    for (i = 0; w[i]; i++) if (w[i] == L'\n') extra++;
    w2 = (wchar_t *)malloc(sizeof(wchar_t) * (n + extra + 1));
    for (i = 0; w[i]; i++) {
        if (w[i] == L'\n') w2[j++] = L'\r';
        w2[j++] = w[i];
    }
    w2[j] = 0;
    h = CreateWindowExW(0, L"EDIT", title,
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
                        CW_USEDEFAULT, CW_USEDEFAULT, 880, 620, hMain, NULL, hInst, NULL);
    SendMessageW(h, WM_SETFONT, (WPARAM)fMono, TRUE);
    SetWindowTextW(h, w2);
    free(w);
    free(w2);
}

static void do_bench(void)
{
    static const char *cand[] = { "data", ".", "..\\data", "01_Code\\data" };
    char dir[700];
    int i, found = 0;
    HCURSOR oldc;

    for (i = 0; i < 4; i++) {
        if (has_data(cand[i])) { strcpy(dir, cand[i]); found = 1; break; }
    }
    if (!found) {
        BROWSEINFOW bi;
        LPITEMIDLIST pidl;
        wchar_t wpath[MAX_PATH];
        if (MessageBoxW(hMain, L"ไม่พบโฟลเดอร์ข้อมูลกลาง (data_1000.txt ฯลฯ) ข้างโปรแกรม\nต้องการเลือกโฟลเดอร์เองหรือไม่?",
                        L"วัดเวลาค้นหา", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
        memset(&bi, 0, sizeof bi);
        bi.hwndOwner = hMain;
        bi.lpszTitle = L"เลือกโฟลเดอร์ที่เก็บไฟล์ข้อมูลกลางของอาจารย์";
        pidl = SHBrowseForFolderW(&bi);
        if (!pidl) return;
        if (!SHGetPathFromIDListW(pidl, wpath)) { CoTaskMemFree(pidl); return; }
        CoTaskMemFree(pidl);
        if (!WideCharToMultiByte(CP_ACP, 0, wpath, -1, dir, (int)sizeof dir, NULL, NULL)) return;
    }

    oldc = SetCursor(LoadCursor(NULL, IDC_WAIT));
    cap_begin();
    core_benchmark(dir);
    cap_end();
    SetCursor(oldc);
    if (capBuf) show_text_window(capBuf, L"ผลการทดสอบจับเวลา (Sequential vs Binary Search)");
}

static void on_toggle(void)
{
    if (selMachine < 0) return;
    if (machines[selMachine].status == M_BROKEN) core_repair(machines[selMachine].id);
    else core_broken(machines[selMachine].id);
    refresh();
}

/* ---------- หน้าต่างหลัก ---------- */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        hBgBrush = CreateSolidBrush(COL_BG);
        fNorm  = make_font(-16, FW_NORMAL, L"Tahoma");
        fBold  = make_font(-16, FW_BOLD,   L"Tahoma");
        fBig   = make_font(-21, FW_BOLD,   L"Tahoma");
        fTitle = make_font(-34, FW_BOLD,   L"Tahoma");
        fSmall = make_font(-14, FW_NORMAL, L"Tahoma");
        fMono  = make_font(-15, FW_NORMAL, L"Consolas");

        /* แถบปุ่มด้านบน */
        mk_btn(hwnd, L"โหลดสถานการณ์ตัวอย่าง", ID_BTN_DEMO, 24, 92, 190, 32);
        mk_btn(hwnd, L"เดินเวลา +5 นาที",      ID_BTN_ADV5, 224, 92, 140, 32);
        mk_btn(hwnd, L"เดินเวลา +15 นาที",     ID_BTN_ADV15, 374, 92, 140, 32);
        mk_btn(hwnd, L"ล้างระบบ",               ID_BTN_RESET, 524, 92, 100, 32);
        mk_btn(hwnd, L"วัดเวลาค้นหา (C)",       ID_BTN_BENCH, 894, 92, 178, 32);

        /* แบบฟอร์มเพิ่มผู้ใช้ */
        hEditId   = mk_edit(hwnd, ID_EDIT_ID,   24, 412, 330, 24, ES_NUMBER);
        hEditName = mk_edit(hwnd, ID_EDIT_NAME, 24, 460, 330, 24, 0);
        SendMessageW(hEditName, EM_SETLIMITTEXT, 20, 0);
        SendMessageW(hEditId, EM_SETLIMITTEXT, 9, 0);
        hCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
                               24, 508, 330, 200, hwnd, (HMENU)(INT_PTR)ID_COMBO, hInst, NULL);
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"ซักด่วน \x2022 20 นาที (priority 1)");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"ซักปกติ \x2022 45 นาที (priority 2)");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"ผ้าหนา/ผ้าห่ม \x2022 60 นาที (priority 2)");
        SendMessageW(hCombo, CB_SETCURSEL, 1, 0);
        mk_btn(hwnd, L"เข้าคิว \x2192", ID_BTN_ADD, 24, 540, 330, 34);

        /* ปุ่มของเครื่องที่เลือก */
        hBtnFinish = mk_btn(hwnd, L"ซักเสร็จ",       ID_BTN_FINISH, 24, 606, 160, 30);
        hBtnToggle = mk_btn(hwnd, L"แจ้งเครื่องเสีย", ID_BTN_TOGGLE, 194, 606, 160, 30);

        /* ค้นหา */
        hEditSearch = mk_edit(hwnd, ID_EDIT_SEARCH, 24, 664, 214, 26, ES_NUMBER);
        mk_btn(hwnd, L"ค้นหา", ID_BTN_SEARCH, 246, 663, 108, 28);

        /* ตารางคิว */
        hList = CreateWindowExW(WS_EX_CLIENTEDGE, L"SysListView32", L"",
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                380, 394, 716 - 24, 168, hwnd, (HMENU)(INT_PTR)ID_LIST, hInst, NULL);
        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
        add_column(0, 60,  L"ลำดับ");
        add_column(1, 90,  L"รหัส");
        add_column(2, 180, L"ชื่อ");
        add_column(3, 130, L"ประเภท");
        add_column(4, 100, L"เวลาซัก (นาที)");
        add_column(5, 120, L"เข้าคิวลำดับที่");

        /* บันทึกเหตุการณ์ */
        hLog = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                               WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOSEL,
                               380, 594, 716 - 24, 106, hwnd, (HMENU)(INT_PTR)ID_LOG, hInst, NULL);

        EnumChildWindows(hwnd, setfont_cb, (LPARAM)fNorm);
        refresh();
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        RECT rc;
        HDC hdc = BeginPaint(hwnd, &ps), mem;
        HBITMAP bmp, old;
        GetClientRect(hwnd, &rc);
        mem = CreateCompatibleDC(hdc);
        bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
        old = (HBITMAP)SelectObject(mem, bmp);
        paint_all(mem, &rc);
        BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bmp);
        DeleteDC(mem);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int x = (short)LOWORD(lParam), y = (short)HIWORD(lParam), i;
        POINT pt;
        pt.x = x; pt.y = y;
        for (i = 0; i < NUM_MACHINES; i++) {
            RECT r = tile_rect(i);
            if (PtInRect(&r, pt)) { selMachine = i; refresh(); break; }
        }
        return 0;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BTN_ADD:    do_add(); break;
        case ID_BTN_FINISH:
            if (selMachine >= 0) { core_finish(machines[selMachine].id); refresh(); }
            break;
        case ID_BTN_TOGGLE: on_toggle(); break;
        case ID_BTN_SEARCH: do_search(); break;
        case ID_BTN_DEMO:   do_demo(); break;
        case ID_BTN_ADV5:   core_advance(5);  simTime += 5;  refresh(); break;
        case ID_BTN_ADV15:  core_advance(15); simTime += 15; refresh(); break;
        case ID_BTN_RESET:  do_reset(); break;
        case ID_BTN_BENCH:  do_bench(); break;
        case IDOK:          /* กด Enter */
            if (GetFocus() == hEditSearch) do_search(); else do_add();
            break;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hPrev, LPSTR cmd, int show)
{
    INITCOMMONCONTROLSEX ic;
    WNDCLASSEXW wc;
    RECT r;
    MSG m;
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;

    (void)hPrev; (void)cmd;
    hInst = hi;
    ic.dwSize = sizeof ic;
    ic.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&ic);

    core_init();
    core_out = gui_out;

    memset(&wc, 0, sizeof wc);
    wc.cbSize = sizeof wc;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.lpszClassName = L"SmartLaundryWnd";
    RegisterClassExW(&wc);

    r.left = 0; r.top = 0; r.right = CLIENT_W; r.bottom = CLIENT_H;
    AdjustWindowRect(&r, style, FALSE);
    hMain = CreateWindowExW(0, L"SmartLaundryWnd", L"SmartLaundry C | ADT Group 10", style,
                            CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
                            NULL, NULL, hi, NULL);
    ShowWindow(hMain, show);
    UpdateWindow(hMain);

    while (GetMessageW(&m, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(hMain, &m)) {
            TranslateMessage(&m);
            DispatchMessageW(&m);
        }
    }
    return (int)m.wParam;
}
