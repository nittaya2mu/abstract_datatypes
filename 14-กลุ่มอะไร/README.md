# SmartParking — ADT กลุ่ม 14 — ภาษา C

โปรแกรมหลัก หน้าจอ Windows และชุดจับเวลาเขียนด้วย C11 ทั้งหมด ไม่มี Python ในชุดส่งงาน

## สมาชิก
- ศรัณย์วิทย์ มีชำนาญ — 6840203241
- วรชัย ตาลเยี่ยน — 6840202946
- วิสุทธิภูมิ กุลสานต์ — 6840205923

## เปิดใช้งาน
1. แตก ZIP **ทั้งชุด** แล้วเข้า 01_Code อย่าย้าย EXE ออกจากโฟลเดอร์ข้อมูล
2. เปิด SmartParking.exe หรือ run.bat เพื่อใช้หน้าจอ Windows (C / Win32 API)
3. เปิด run_c.bat เพื่อใช้คำสั่ง C แบบ console; ปิดด้วย quit
4. ปุ่มวัดเวลาค้นหา (C) รัน SmartParkingC.exe และแสดงผลดิบ 5 ครั้ง/ขนาด บันทึก results/latest_run.csv และ .txt
5. benchmark.bat ใช้รันบนเครื่องกลางโดยตรง บันทึก results/classroom_latest.csv

EXE ใช้ Windows x64 ไม่ต้องติดตั้ง Python หรือ library เพิ่ม ใช้แป้น Tab ย้ายช่อง ปุ่ม Demo สำหรับข้อมูลตัวอย่าง กดเริ่มใหม่เพื่อล้างข้อมูลจำลอง

## เครื่องมือสำหรับ Compile
- C11 compiler: GCC / MinGW-w64 (พร้อม Windows headers) หรือ Zig cc
- ไลบรารีระบบ Windows: user32, gdi32, comctl32 และ C standard library
- ทดสอบ build ด้วย Zig cc 0.15.2, target x86_64-windows-gnu, -std=c11 -O2 -Wall -Wextra -Werror
- build_c.bat compile console และ GUI; ui_windows.c รวมโมเดลจาก main.c จึง **ไม่ใส่ main.c ซ้ำ** ในคำสั่ง build GUI
- compiler console บนระบบอื่น: gcc -std=c11 -O2 -Wall -Wextra -DLAUNDRY=0 c/main.c c/adt.c c/benchmark.c -o SmartParkingC -lm
- หากใช้ Zig cc build GUI ให้เพิ่ม -Wl,--subsystem,windows และ link user32/gdi32/comctl32 (GCC ใช้ -mwindows ตาม build_c.bat)
- GUI ต้องใช้ Windows; โมเดลและ console ใช้ C standard library

## ขอบเขตและ ADT
ช่อง P1–P10 ทางเข้าเดียว ใช้ Graph + Dijkstra เลือกช่องว่างใกล้สุดจากกราฟ 16 จุดยอด 15 เส้นเชื่อม จองหมดสิทธิ์ใน 5 นาที ยืนยันเข้า/ออกจำลอง หนึ่งทะเบียนมีหนึ่งสิทธิ์
ทั้งสองใช้ Hash Table (FNV-1a + separate chaining), indexed Min-Heap, Stack สำหรับ Undo รายการ pending ล่าสุด และ Circular Buffer ประวัติ 30 เหตุการณ์ ข้อมูลอยู่ใน RAM ไม่มีฮาร์ดแวร์จริง
GUI และ console ใช้โมเดล C ชุดเดียวกัน แต่แต่ละโปรเซสมีสถานะของตนเอง GUI ตรวจเวลาทุก 250 ms และก่อนคำสั่ง Console ตรวจก่อนคำสั่ง Heap อ่าน root O(1), เพิ่ม/นำออก O(log H); Hash เฉลี่ย O(1), worst O(n); Stack remove แบบ array O(N)

## Demo คำสั่ง
demo → advance 300 → join NEW-001 → start R0004 → undo → pickup R0004
คำสั่ง console: join USER [20|30|45], start CODE/USER, pickup CODE/USER, cancel CODE/USER, undo, advance SECONDS, maintenance MACHINE, status, history, demo, quit
Parking: join=จอง, start=ยืนยันเข้า, pickup=ออก; maintenance ใช้เฉพาะ Laundry

## ชุดจับเวลาและวิธีทดลองตามเอกสาร
1. ใช้ **คอมพิวเตอร์เครื่องกลางในห้องเรียนเครื่องเดียวกันทุกกลุ่ม**
2. compile/run บนเครื่องอื่นอย่างน้อยหนึ่งเครื่องและเครื่องห้องปฏิบัติการล่วงหน้า ตรวจคำตอบก่อนนำเสนอ
3. ใช้ clock() จาก time.h คร่อมเฉพาะ MySearch; ไม่รวมอ่านไฟล์ รับข้อมูล แปลง key สร้าง Hash ตรวจคำตอบ print หรือบันทึก CSV
4. ใช้ data/instructor/data_1000.txt, data_10000.txt, data_100000.txt และ targets ครบ 10 ค่าต่อขนาด ไฟล์ sorted เก็บต้นฉบับไว้แต่ไม่ใช้สำหรับ Hash
5. รัน **5 ครั้งต่อขนาด** ตัดสูงสุดหนึ่งค่าและต่ำสุดหนึ่งค่า เฉลี่ยสามค่าที่เหลือ
6. **ปิดโปรแกรมอื่นทั้งหมดขณะวัดเวลา**; GUI หยุด timer และปรับหน้าจอระหว่างรอ benchmark

REPEAT=1,000,000 **ค่าเดียวกันทุกขนาด ทุก run และทั้งสองกลุ่ม** 10 targets/รอบ = 10,000,000 searches/run
REPEAT=1,000 ในภาพเป็นตัวอย่าง สามารถใช้จำนวนซ้ำมากขึ้นเพื่อให้ Hash ที่เร็วมากพ้นความละเอียด clock ได้ โดยต้องคงค่าเดียวกันและรายงานค่า เราไม่ลด/เปลี่ยน REPEAT ระหว่างทดลอง
ms/lookup = (end-start)/CLOCKS_PER_SEC × 1000 / (REPEAT × 10)
ratio = mean_new / mean_old; k = log(ratio)/log(10)
วน Hash lookup ซ้ำใช้ volatile sink ป้องกัน compiler ตัดโค้ด; overhead ของ loop/store ยังรวมอยู่ ไม่หักลบเวลาเพื่อให้ได้ค่าที่ต้องการ

ผล local_benchmark เป็นผลจริงของเครื่องผู้พัฒนา **ไม่ใช่ผลเครื่องกลาง** จึงยังไม่ใช้เปรียบเทียบกลุ่มหรือจัดอันดับ เวลาผิดจาก Big-O อธิบายจากขนาดข้อมูล cache collision constant factor และความละเอียด clock ได้ k ใกล้ 0 ไม่พิสูจน์ว่า O(1) และ k ติดลบเล็กน้อยไม่ใช่ Big-O เลขชี้กำลังติดลบ
ถ้าคำตอบผิด ข้อมูลผิด หรือ clock เฉลี่ยเป็นศูนย์ โปรแกรมหยุด ไม่ส่งค่าหรือคำนวณ k ต่อ

## ข้อมูลกลางและคำตอบ
คัดลอกต้นฉบับอาจารย์ครบ 11 ไฟล์ ไม่แก้ตัวเลข/ลำดับ แต่ละขนาดต้องพบ 7 เป้าหมาย ไม่พบ 3 คืน index เดิมแบบ 0-based หรือ -1
RAR ที่ได้รับไม่มี answer_key.csv แม้ README ต้นฉบับกล่าวถึงไฟล์นี้ จึงตรวจ Hash เทียบ Sequential Search อิสระก่อน clock() ทั้ง 30 targets ผลตรวจอยู่ results/local_benchmark.txt

## ตรวจและเตรียมส่ง
- SmartParkingC.exe --self-test: ทดสอบ ADT, ขอบเขต/ความจุ, Demo, 1,000 รอบหมุนเวียน และ 1,500 คำสั่งสุ่ม พร้อม invariant
- สไลด์ 12 หน้า: นำเสนอ 10 นาที + ถามตอบ 5 นาที; รายงาน PDF ใน 02_Report
- สำรอง **ทั้งโฟลเดอร์** รวมโค้ด EXE ข้อมูล รายงาน และสไลด์บน USB Flash Drive
- ยังต้องทดสอบเครื่องอื่น/เครื่องห้องเรียน ปิดโปรแกรมอื่นและวัดเครื่องกลางตามจริง ไม่อ้างว่าส่วนนี้เสร็จแล้วจากการวัด local
- โค้ดฉบับนี้จัดไว้ในโฟลเดอร์กลุ่มของ repository อาจารย์แล้ว ส่วน Classroom และการประเมินรายบุคคลยังต้องดำเนินการตามกำหนด
