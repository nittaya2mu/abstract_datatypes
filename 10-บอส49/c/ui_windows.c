/* Native Windows interface written in C11. The same C model powers GUI and CLI.
   No Python runtime, browser, DLL download or third-party UI library is needed. */
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <wchar.h>
#define main cli_main
#include "main.c"
#undef main

enum { B_DEMO=101,B_5,B_20,B_RESET,B_BENCH,B_JOIN,B_START,B_PICKUP,B_CANCEL,B_UNDO,B_ACTIVE,B_LOG,B_RESULTS };
static System model;
static HWND window,user_edit,code_edit,program,table_view,buttons[13];
static HFONT body_font,title_font,small_font,card_font;
static HBRUSH background;
static wchar_t notice[300]=L"พร้อมใช้งาน • ข้อมูลอยู่ใน RAM • กดโหลด Demo เพื่อเริ่มสาธิต";
static int view_mode,busy;
static double bench[3][8];
static int bench_count;
static const COLORREF navy=RGB(16,31,59),paper=RGB(241,245,250);

static void utf8wide(const char *s,wchar_t *out,int capacity){
    if(!MultiByteToWideChar(CP_UTF8,0,s,-1,out,capacity))out[0]=0;
}
static int edit_utf8(HWND edit,char *out){
    wchar_t text[80];GetWindowTextW(edit,text,80);
    return WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text,-1,out,KEY_BYTES,NULL,NULL)>0;
}
static void clock_tick(void){
    double wall=difftime(time(NULL),model.started)+model.offset;
    tick_to(&model,fmax(model.now,wall));invariant(&model);
}
static const wchar_t *status_label(int status){
    if(status==WAITING)return L"รอคิว";
    if(status==READY)return LAUNDRY?L"รอเริ่มซัก":L"จองแล้ว";
    if(status==RUNNING)return LAUNDRY?L"กำลังซัก":L"เข้าจอดแล้ว";
    if(status==DONE)return L"รอรับผ้า";
    return L"ว่าง";
}
static void draw_text(HDC dc,const wchar_t *text,int x,int y,int w,int h,HFONT font,COLORREF color){
    RECT rect={x,y,x+w,y+h};HFONT old=SelectObject(dc,font);
    SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);
    DrawTextW(dc,text,-1,&rect,DT_LEFT|DT_TOP|DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);
}
static void fill(HDC dc,int x,int y,int w,int h,COLORREF color){
    RECT rect={x,y,x+w,y+h};HBRUSH brush=CreateSolidBrush(color);FillRect(dc,&rect,brush);DeleteObject(brush);
}
static void paint(HDC target){
    RECT client;GetClientRect(window,&client);int width=client.right,height=client.bottom;
    HDC dc=CreateCompatibleDC(target);HBITMAP bitmap=CreateCompatibleBitmap(target,width,height),old=SelectObject(dc,bitmap);
    SetMapMode(dc,MM_ANISOTROPIC);SetWindowExtEx(dc,1280,900,NULL);SetViewportExtEx(dc,width,height,NULL);
    fill(dc,0,0,1280,900,paper);fill(dc,0,0,1280,98,navy);
    draw_text(dc,LAUNDRY?L"SmartLaundry":L"SmartParking",28,18,690,45,title_font,RGB(255,255,255));
    draw_text(dc,LAUNDRY?L"ระบบจัดคิวเครื่องซักผ้าหอพัก • โปรแกรมภาษา C":L"ระบบจองที่จอดรถใกล้ทางเข้า • โปรแกรมภาษา C",30,65,900,26,body_font,RGB(200,220,240));
    draw_text(dc,LAUNDRY?L"ADT / GROUP 10":L"ADT / GROUP 14",1040,36,220,30,body_font,RGB(99,222,237));
    wchar_t text[300];int free_count=0,ready=0,running=0,done=0;
    for(int m=0;m<10;m++){int id=model.owner[m];if(id<0&&model.online[m])free_count++;else if(id>=0){int st=model.tickets[id].status;ready+=st==READY;running+=st==RUNNING;done+=st==DONE;}}
    swprintf(text,300,L"ว่าง %d    จอง/รอเริ่ม %d    กำลังใช้งาน %d    รอรับผ้า %d    รอคิว %d    |    เวลา +%.0f วินาที",free_count,ready,running,done,model.queue.size,model.now);
    draw_text(dc,text,28,157,1225,31,body_font,navy);
    for(int m=0;m<10;m++){
        int x=28+(m%5)*249,y=205+(m/5)*124,id=model.owner[m];int st=id<0?UNUSED:model.tickets[id].status;
        COLORREF color=st==READY?RGB(255,243,205):st==RUNNING?RGB(219,234,254):st==DONE?RGB(237,233,254):RGB(221,245,236);
        if(!model.online[m])color=RGB(225,230,238);fill(dc,x,y,237,112,color);
        swprintf(text,300,L"%c%d • %ls",LAUNDRY?L'W':L'P',m+1,model.online[m]?status_label(st):L"ซ่อมบำรุง");
        draw_text(dc,text,x+12,y+10,215,30,card_font,navy);
        if(id>=0){Ticket *t=&model.tickets[id];wchar_t user[80],code[80];utf8wide(t->user,user,80);utf8wide(t->code,code,80);
            swprintf(text,300,L"%ls  |  %ls",code,user);draw_text(dc,text,x+12,y+44,212,28,body_font,navy);
            if(st==READY||(LAUNDRY&&st==RUNNING))swprintf(text,300,L"เหลือ %.0f วินาที",fmax(0,t->deadline-model.now));
            else swprintf(text,300,L"%ls",LAUNDRY?L"รับผ้าก่อนคืนเครื่อง":L"ยืนยันเข้าแล้ว");
        }else swprintf(text,300,L"%ls",model.online[m]?L"พร้อมรับผู้ใช้":L"ดับเบิลคลิกเพื่อเปิดเครื่อง");
        if(!LAUNDRY)swprintf(text,300,L"ระยะทาง %d เมตร",model.distance[6+m]);
        draw_text(dc,text,x+12,y+78,214,25,small_font,RGB(70,90,110));
    }
    fill(dc,28,466,1224,29,RGB(226,236,248));
    if(model.heap.size){Event e=model.heap.items[0];wchar_t code[80];utf8wide(model.tickets[e.id].code,code,80);swprintf(text,300,L"Min-Heap: %ls เป็นเหตุการณ์ถัดไป • เหลือ %.0f วินาที • ประวัติ %d/30",code,fmax(0,e.at-model.now),model.log.size);}
    else swprintf(text,300,L"Min-Heap: ไม่มีเหตุการณ์ค้าง • ประวัติ %d/30",model.log.size);
    draw_text(dc,text,39,469,1190,24,small_font,navy);
    draw_text(dc,L"จัดการคิว / สิทธิ์",28,516,330,32,card_font,navy);
    draw_text(dc,LAUNDRY?L"รหัสห้อง (หนึ่งห้องต่อหนึ่งคิว)":L"ทะเบียนรถ (หนึ่งทะเบียนต่อหนึ่งสิทธิ์)",28,556,340,25,small_font,navy);
    draw_text(dc,L"รหัสรายการ หรือ ห้อง/ทะเบียน",28,715,337,25,small_font,navy);
    fill(dc,0,882,1280,18,navy);
    draw_text(dc,notice,390,845,860,34,small_font,busy?RGB(160,90,0):navy);
    SetMapMode(dc,MM_TEXT);BitBlt(target,0,0,width,height,dc,0,0,SRCCOPY);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);
}
static void set_columns(const wchar_t **labels,const int *widths,int count){
    ListView_DeleteAllItems(table_view);while(ListView_DeleteColumn(table_view,0)){}
    for(int i=0;i<count;i++){LVCOLUMNW column={0};column.mask=LVCF_TEXT|LVCF_WIDTH;column.pszText=(wchar_t*)labels[i];column.cx=widths[i];SendMessageW(table_view,LVM_INSERTCOLUMNW,i,(LPARAM)&column);}
}
static void row_text(int row,int col,const wchar_t *text){
    LVITEMW item={0};item.iItem=row;item.iSubItem=col;item.pszText=(wchar_t*)text;
    if(!col){item.mask=LVIF_TEXT;SendMessageW(table_view,LVM_INSERTITEMW,0,(LPARAM)&item);}
    else SendMessageW(table_view,LVM_SETITEMTEXTW,row,(LPARAM)&item);
}
static void ticket_row(int row,int id){
    Ticket *t=&model.tickets[id];wchar_t text[180];utf8wide(t->code,text,180);row_text(row,0,text);
    utf8wide(t->user,text,180);row_text(row,1,text);row_text(row,2,status_label(t->status));
    if(t->machine<0)swprintf(text,180,L"—");else swprintf(text,180,L"%c%d",LAUNDRY?L'W':L'P',t->machine+1);row_text(row,3,text);
    if(t->status==READY||(LAUNDRY&&t->status==RUNNING))swprintf(text,180,L"%.0f s",fmax(0,t->deadline-model.now));else swprintf(text,180,L"—");row_text(row,4,text);
}
static void refresh_table(void){
    wchar_t text[300];SendMessageW(table_view,WM_SETREDRAW,FALSE,0);
    if(view_mode==0){const wchar_t *labels[]={L"รหัส",L"ห้อง / ทะเบียน",L"สถานะ",L"เครื่อง/ช่อง",L"เวลาคงเหลือ"};const int widths[]={118,205,180,140,150};set_columns(labels,widths,5);int row=0;
        for(int q=0;q<model.queue.size;q++)ticket_row(row++,model.queue.items[(model.queue.head+q)%MAX_TICKETS]);
        for(int i=0;i<MAX_TICKETS;i++)if(model.tickets[i].status!=UNUSED&&model.tickets[i].status!=WAITING)ticket_row(row++,i);
    }else if(view_mode==1){const wchar_t *labels[]={L"ประวัติ: เวลา / เหตุการณ์ / รหัส / ผู้ใช้ / เครื่องหรือช่อง"};const int widths[]={820};set_columns(labels,widths,1);
        for(int i=0;i<model.log.size;i++){utf8wide(model.log.lines[(model.log.head+i)%30],text,300);row_text(i,0,text);}
    }else{const wchar_t *labels[]={L"n",L"ครั้ง 1 (ms)",L"ครั้ง 2",L"ครั้ง 3",L"ครั้ง 4",L"ครั้ง 5",L"เฉลี่ย 3",L"ratio",L"k"};const int widths[]={75,105,100,100,100,100,105,85,85};set_columns(labels,widths,9);
        for(int i=0;i<bench_count;i++){swprintf(text,300,L"%d",i==0?1000:i==1?10000:100000);row_text(i,0,text);
            for(int j=0;j<6;j++){swprintf(text,300,L"%.9f",bench[i][j]);row_text(i,j+1,text);}
            for(int j=6;j<8;j++){if(!i)swprintf(text,300,L"—");else swprintf(text,300,L"%.6f",bench[i][j]);row_text(i,j+1,text);}
        }
    }
    SendMessageW(table_view,WM_SETREDRAW,TRUE,0);InvalidateRect(table_view,NULL,TRUE);InvalidateRect(window,NULL,FALSE);
}
static HWND control(const wchar_t *class_name,const wchar_t *text,DWORD style,int id){
    HWND h=CreateWindowExW(!wcscmp(class_name,L"EDIT")?WS_EX_CLIENTEDGE:0,class_name,text,WS_CHILD|WS_VISIBLE|style,0,0,10,10,window,(HMENU)(INT_PTR)id,GetModuleHandleW(NULL),NULL);
    if(h)SendMessageW(h,WM_SETFONT,(WPARAM)body_font,TRUE);return h;
}
static void position(HWND h,int x,int y,int width,int height){
    RECT r;GetClientRect(window,&r);MoveWindow(h,x*r.right/1280,y*r.bottom/900,width*r.right/1280,height*r.bottom/900,TRUE);
}
static void layout(void){
    int xs[]={28,189,323,457,920};int ws[]={150,123,123,130,332};
    for(int i=0;i<5;i++)position(buttons[i],xs[i],111,ws[i],35);
    position(user_edit,28,584,330,31);position(program,28,627,330,150);position(buttons[5],28,670,330,38);
    position(code_edit,28,744,330,31);position(buttons[6],28,790,160,38);position(buttons[7],198,790,160,38);
    position(buttons[8],28,837,160,35);position(buttons[9],198,837,160,35);
    for(int i=10;i<13;i++)position(buttons[i],390+(i-10)*285,509,273,33);
    position(table_view,390,552,862,284);
}
static DWORD WINAPI benchmark_worker(LPVOID unused){
    (void)unused;SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};
    HANDLE file=CreateFileW(L"results/latest_run.txt",GENERIC_WRITE,FILE_SHARE_READ,&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    DWORD exit_code=2;
    if(file!=INVALID_HANDLE_VALUE){
        STARTUPINFOW si={0};PROCESS_INFORMATION pi={0};si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;si.hStdOutput=file;si.hStdError=file;
        HANDLE input=CreateFileW(L"NUL",GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,&sa,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);si.hStdInput=input;
        wchar_t command[300];wcscpy(command,LAUNDRY?L"SmartLaundryC.exe --benchmark data/instructor results/latest_run.csv":L"SmartParkingC.exe --benchmark data/instructor results/latest_run.csv");
        if(CreateProcessW(NULL,command,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi)){
            WaitForSingleObject(pi.hProcess,INFINITE);GetExitCodeProcess(pi.hProcess,&exit_code);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);
        }
        if(input!=INVALID_HANDLE_VALUE)CloseHandle(input);CloseHandle(file);
    }
    PostMessageW(window,WM_APP+1,exit_code,0);return 0;
}
static void load_benchmark(void){
    bench_count=0;FILE *f=fopen("results/latest_run.csv","r");if(!f)return;char line[600];if(!fgets(line,sizeof(line),f)){fclose(f);return;}
    while(bench_count<3&&fgets(line,sizeof(line),f)){
        int n,repeat,targets,found,missing;double *b=bench[bench_count];memset(b,0,8*sizeof(double));
        int got=sscanf(line,"%d,%d,%d,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",&n,&repeat,&targets,&found,&missing,&b[0],&b[1],&b[2],&b[3],&b[4],&b[5],&b[6],&b[7]);
        if(got<11||n!=(bench_count==0?1000:bench_count==1?10000:100000)||repeat!=1000000||targets!=10||found!=7||missing!=3||b[5]<=0)break;
        bench_count++;
    }fclose(f);
}
static void command(int id){
    if(busy)return;clock_tick();char key[KEY_BYTES]={0};int ok=0;
    if(id==B_ACTIVE||id==B_LOG||id==B_RESULTS){view_mode=id-B_ACTIVE;refresh_table();return;}
    if(id==B_BENCH){
        CreateDirectoryW(L"results",NULL);busy=1;KillTimer(window,1);wcscpy(notice,L"กำลังวัด 15 ครั้ง • ไม่รวม I/O และการวาดหน้าจอ • กรุณาปิดโปรแกรมอื่นก่อนรันบนเครื่องกลาง");
        for(int i=0;i<13;i++)EnableWindow(buttons[i],FALSE);
        HANDLE thread=CreateThread(NULL,0,benchmark_worker,NULL,0,NULL);
        if(thread)CloseHandle(thread);else PostMessageW(window,WM_APP+1,2,0);InvalidateRect(window,NULL,FALSE);return;
    }
    if(id==B_RESET){if(MessageBoxW(window,L"ล้างข้อมูลจำลองทั้งหมดและเริ่มระบบใหม่?",L"เริ่มใหม่",MB_YESNO|MB_ICONQUESTION)==IDYES){destroy(&model);ok=init(&model);}}
    else if(id==B_DEMO){if(!model.count){demo(&model);ok=1;}else{wcscpy(notice,L"Demo ต้องเริ่มจากระบบว่าง กดเริ่มใหม่ก่อนโหลด Demo");refresh_table();return;}}
    else if(id==B_5||id==B_20){double seconds=id==B_5?300:1200;model.offset+=seconds;tick_to(&model,model.now+seconds);ok=1;}
    else if(id==B_JOIN){if(edit_utf8(user_edit,key)){int choice=(int)SendMessageW(program,CB_GETCURSEL,0,0);int duration=choice==1?1800:choice==2?2700:1200;int item=join(&model,key,duration);ok=item>=0;
        if(ok){wchar_t code[80];utf8wide(model.tickets[item].code,code,80);SetWindowTextW(code_edit,code);SetWindowTextW(user_edit,L"");swprintf(notice,300,L"สร้าง %ls สำเร็จ • %ls",code,status_label(model.tickets[item].status));invariant(&model);refresh_table();return;}}}
    else if(id==B_UNDO)ok=undo(&model);
    else if(edit_utf8(code_edit,key)){if(id==B_START)ok=start(&model,key);else if(id==B_PICKUP)ok=pickup(&model,key);else if(id==B_CANCEL)ok=cancel(&model,key);}
    wcscpy(notice,ok?L"ดำเนินการสำเร็จ • อัปเดตสถานะแล้ว":L"ไม่สำเร็จ: ตรวจรหัส สถานะ ข้อมูลซ้ำ ความจุ หรือสิทธิ์หมดเวลา");invariant(&model);refresh_table();
}
static LRESULT CALLBACK procedure(HWND hwnd,UINT message,WPARAM wp,LPARAM lp){
    switch(message){
    case WM_CREATE:{window=hwnd;
        body_font=CreateFontW(-18,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Tahoma");
        small_font=CreateFontW(-16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Tahoma");
        card_font=CreateFontW(-22,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Tahoma");
        title_font=CreateFontW(-38,0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Tahoma");background=CreateSolidBrush(paper);
        const wchar_t *labels[]={L"โหลด Demo",L"+5 นาที",L"+20 นาที",L"เริ่มใหม่",L"วัดเวลาค้นหา (C)",LAUNDRY?L"รับคิวใหม่ →":L"จองช่องใกล้ที่สุด →",LAUNDRY?L"เริ่มซัก":L"ยืนยันเข้า",LAUNDRY?L"รับผ้า":L"บันทึกออก",L"ยกเลิกสิทธิ์",L"Undo ล่าสุด",L"คิว / สิทธิ์ปัจจุบัน",L"ประวัติ 30 เหตุการณ์",L"ผลเวลาค้นหา (ms)"};
        for(int i=0;i<13;i++)buttons[i]=control(L"BUTTON",labels[i],BS_PUSHBUTTON|WS_TABSTOP,B_DEMO+i);
        user_edit=control(L"EDIT",L"",ES_AUTOHSCROLL|WS_TABSTOP,201);code_edit=control(L"EDIT",L"",ES_AUTOHSCROLL|WS_TABSTOP,202);
        SendMessageW(user_edit,EM_SETLIMITTEXT,79,0);SendMessageW(code_edit,EM_SETLIMITTEXT,79,0);
        program=control(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP,203);
        SendMessageW(program,CB_ADDSTRING,0,(LPARAM)L"ซักด่วน • 20 นาที");SendMessageW(program,CB_ADDSTRING,0,(LPARAM)L"ซักปกติ • 30 นาที");SendMessageW(program,CB_ADDSTRING,0,(LPARAM)L"ผ้าหนา • 45 นาที");SendMessageW(program,CB_SETCURSEL,0,0);
        if(!LAUNDRY){ShowWindow(program,SW_HIDE);}
        table_view=control(WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|LVS_SHOWSELALWAYS|WS_TABSTOP,204);
        ListView_SetExtendedListViewStyle(table_view,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER|LVS_EX_GRIDLINES);SendMessageW(table_view,WM_SETFONT,(WPARAM)small_font,TRUE);
        SetTimer(hwnd,1,250,NULL);layout();refresh_table();return 0;
    }
    case WM_SIZE:layout();return 0;
    case WM_GETMINMAXINFO:((MINMAXINFO*)lp)->ptMinTrackSize.x=1280;((MINMAXINFO*)lp)->ptMinTrackSize.y=930;return 0;
    case WM_TIMER:if(!busy){
        int head=model.log.head,size=model.log.size,count=model.count,heap_size=model.heap.size;
        clock_tick();
        if(head!=model.log.head||size!=model.log.size||count!=model.count||heap_size!=model.heap.size)refresh_table();
        else {
            if(view_mode==0){int row=0;wchar_t remaining[40];
                for(int q=0;q<model.queue.size;q++)row++;
                for(int i=0;i<MAX_TICKETS;i++)if(model.tickets[i].status!=UNUSED&&model.tickets[i].status!=WAITING){Ticket *t=&model.tickets[i];
                    if(t->status==READY||(LAUNDRY&&t->status==RUNNING))swprintf(remaining,40,L"%.0f s",fmax(0,t->deadline-model.now));else wcscpy(remaining,L"—");row_text(row++,4,remaining);}
            }InvalidateRect(hwnd,NULL,FALSE);
        }
    }return 0;
    case WM_COMMAND:if(HIWORD(wp)==BN_CLICKED&&LOWORD(wp)>=B_DEMO&&LOWORD(wp)<=B_RESULTS)command(LOWORD(wp));return 0;
    case WM_NOTIFY:{NMHDR *header=(NMHDR*)lp;if(header->idFrom==204&&header->code==LVN_ITEMCHANGED&&view_mode==0){
        NMLISTVIEW *event=(NMLISTVIEW*)lp;if(event->iItem>=0&&(event->uNewState&LVIS_SELECTED)){wchar_t code[80];LVITEMW item={0};item.iSubItem=0;item.cchTextMax=80;item.pszText=code;SendMessageW(table_view,LVM_GETITEMTEXTW,event->iItem,(LPARAM)&item);SetWindowTextW(code_edit,code);}}
        return 0;}
    case WM_LBUTTONDBLCLK:if(LAUNDRY&&!busy){RECT r;GetClientRect(hwnd,&r);int x=(short)LOWORD(lp)*1280/r.right,y=(short)HIWORD(lp)*900/r.bottom;
        for(int m=0;m<10;m++){int a=28+m%5*249,b=205+m/5*124;if(x>=a&&x<a+237&&y>=b&&y<b+112){clock_tick();int ok=maintenance(&model,m);wcscpy(notice,ok?L"เปลี่ยนสถานะซ่อมบำรุงแล้ว":L"เปลี่ยนสถานะได้เฉพาะเครื่องที่ว่าง");invariant(&model);refresh_table();break;}}}return 0;
    case WM_APP+1:busy=0;for(int i=0;i<13;i++)EnableWindow(buttons[i],TRUE);SetTimer(hwnd,1,250,NULL);
        if(!wp){load_benchmark();view_mode=2;wcscpy(notice,bench_count==3?L"ตรวจผ่าน 30 targets • ผลของเครื่องนี้อยู่ใน results/latest_run.csv • วัดใหม่บนเครื่องกลางก่อนส่ง":L"อ่านผล CSV ไม่ครบ ดู results/latest_run.txt");}
        else wcscpy(notice,L"วัดเวลาไม่สำเร็จ ตรวจข้อมูล/โปรแกรม C และดูสาเหตุใน results/latest_run.txt");clock_tick();refresh_table();return 0;
    case WM_CTLCOLORSTATIC:SetBkColor((HDC)wp,paper);SetTextColor((HDC)wp,navy);return (LRESULT)background;
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);paint(dc);EndPaint(hwnd,&ps);return 0;}
    case WM_CLOSE:if(busy){MessageBoxW(hwnd,L"การวัดเวลายังทำงานอยู่ กรุณารอให้เสร็จก่อนปิด",L"กำลังวัดเวลา",MB_OK);return 0;}DestroyWindow(hwnd);return 0;
    case WM_DESTROY:KillTimer(hwnd,1);destroy(&model);DeleteObject(body_font);DeleteObject(small_font);DeleteObject(card_font);DeleteObject(title_font);DeleteObject(background);PostQuitMessage(0);return 0;
    }return DefWindowProcW(hwnd,message,wp,lp);
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command_line,int show){
    (void)previous;(void)command_line;SetProcessDPIAware();
    wchar_t directory[MAX_PATH];DWORD length=GetModuleFileNameW(NULL,directory,MAX_PATH);
    if(!length||length>=MAX_PATH)return 1;wchar_t *slash=wcsrchr(directory,L'\\');if(slash){*slash=0;SetCurrentDirectoryW(directory);}
    if(!init(&model)){MessageBoxW(NULL,L"หน่วยความจำไม่เพียงพอ",L"เปิดโปรแกรมไม่ได้",MB_OK|MB_ICONERROR);return 1;}
    INITCOMMONCONTROLSEX controls={sizeof(controls),ICC_LISTVIEW_CLASSES};InitCommonControlsEx(&controls);
    WNDCLASSW cls={0};cls.style=CS_DBLCLKS;cls.lpfnWndProc=procedure;cls.hInstance=instance;cls.hCursor=LoadCursorW(NULL,IDC_ARROW);cls.lpszClassName=LAUNDRY?L"SmartLaundryNativeC":L"SmartParkingNativeC";
    if(!RegisterClassW(&cls)){destroy(&model);return 1;}
    HWND h=CreateWindowExW(0,cls.lpszClassName,LAUNDRY?L"SmartLaundry C | ADT Group 10":L"SmartParking C | ADT Group 14",WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,CW_USEDEFAULT,CW_USEDEFAULT,1320,980,NULL,NULL,instance,NULL);
    if(!h){destroy(&model);return 1;}ShowWindow(h,show);UpdateWindow(h);
    MSG message;BOOL result;while((result=GetMessageW(&message,NULL,0,0))>0){if(!IsDialogMessageW(h,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
    return result<0?1:(int)message.wParam;
}
