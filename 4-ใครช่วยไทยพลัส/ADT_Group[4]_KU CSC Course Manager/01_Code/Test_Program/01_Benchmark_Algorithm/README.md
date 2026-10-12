# ⏱️ คู่มือการทดสอบประสิทธิภาพอัลกอริทึมการค้นหา (Algorithm Benchmark)
### โครงงานรายวิชา 01204212 แบบชนิดข้อมูลนามธรรมและการแก้ปัญหา

โฟลเดอร์นี้บรรจุโปรแกรมสำหรับวัดและเปรียบเทียบประสิทธิภาพเชิงเวลา (Execution Time & Speedup) ของอัลกอริทึมการค้นหา 3 ชนิด:
1. **Sequential Search (Linear Search):** ความซับซ้อนเชิงเวลา $O(N)$
2. **Binary Search:** ความซับซ้อนเชิงเวลา $O(\log N)$ บนชุดข้อมูลที่เรียงลำดับแล้ว
3. **AVL Tree Search (Self-balancing Binary Search Tree):** ความซับซ้อนเชิงเวลา $O(\log N)$ ซึ่งเป็นโครงสร้างข้อมูลแกนหลักที่ใช้ในโปรเจกต์ Course Manager

---

## 🛠️ ความต้องการของระบบและการติดตั้งเครื่องมือ

โปรแกรมทดสอบนี้พัฒนาด้วยภาษา C มาตรฐาน (Standard C Library: `<stdio.h>`, `<stdlib.h>`, `<time.h>`) **ไม่จำเป็นต้องติดตั้งไลบรารีภายนอกเพิ่มเติม** เพียงแค่มี C Compiler (GCC/Clang):

### 1. สำหรับ Windows
- **ผ่าน MSYS2 (แนะนำ):**
  เปิดเทอร์มินัล MSYS2 (UCRT64 หรือ MINGW64) แล้วติดตั้ง GCC:
  ```bash
  pacman -S mingw-w64-ucrt-x86_64-gcc
  ```
- **หรือผ่าน MinGW-w64 / WSL:**
  ตรวจสอบว่ามี `gcc` ใน Command Prompt หรือ PowerShell โดยพิมพ์ `gcc --version`

### 2. สำหรับ macOS
- ติดตั้ง Xcode Command Line Tools (หากยังไม่มี):
  ```bash
  xcode-select --install
  ```

### 3. สำหรับ Linux (Ubuntu / Debian)
- ติดตั้ง `build-essential`:
  ```bash
  sudo apt-get update
  sudo apt-get install build-essential
  ```

---

## ⚙️ ขั้นตอนการคอมไพล์ (Compilation)

ใช้คำสั่งคอมไพล์ด้วย GCC พร้อมตัวเลือก `-O3` เพื่อเปิดใช้งาน Optimization สูงสุด:

- **สำหรับ Windows (Command Prompt / PowerShell / MSYS2):**
  ```bash
  gcc timing_template.c -o timing.exe -O3
  ```
- **สำหรับ macOS / Linux:**
  ```bash
  gcc timing_template.c -o timing -O3
  ```

*(ในโฟลเดอร์นี้มีไฟล์ `timing.exe` ที่คอมไพล์สำเร็จแล้ว สามารถเปิดรันได้ทันที)*

---

## 🚀 ขั้นตอนการรันและการทดสอบ (Execution & Usage)

### วิธีที่ 1: รันอัตโนมัติด้วย Batch File (สำหรับ Windows)
ดับเบิลคลิกไฟล์ **`run_benchmark.bat`** โปรแกรมจะทำการรันการทดสอบครบทุกขนาดข้อมูลต่อเนื่องกันโดยอัตโนมัติ

### วิธีที่ 2: รันผ่าน Terminal / Command Line
สามารถระบุไฟล์ชุดข้อมูลและเป้าหมายการค้นหาตามขนาดที่ต้องการ:

#### 1. การทดสอบกับชุดข้อมูลที่เรียงลำดับแล้ว (`data_*_sorted.txt` - แนะนำสำหรับ Binary Search & AVL Tree):
- **ขนาด 1,000 ค่า:**
  ```bash
  ./timing.exe data_1000_sorted.txt targets_1000.txt       # Windows
  ./timing data_1000_sorted.txt targets_1000.txt           # macOS / Linux
  ```
- **ขนาด 10,000 ค่า:**
  ```bash
  ./timing.exe data_10000_sorted.txt targets_10000.txt     # Windows
  ./timing data_10000_sorted.txt targets_10000.txt         # macOS / Linux
  ```
- **ขนาด 100,000 ค่า:**
  ```bash
  ./timing.exe data_100000_sorted.txt targets_100000.txt   # Windows
  ./timing data_100000_sorted.txt targets_100000.txt       # macOS / Linux
  ```

#### 2. การทดสอบกับชุดข้อมูลที่ยังไม่เรียงลำดับ (`data_*.txt`):
- **ขนาด 1,000 ค่า:**
  ```bash
  ./timing.exe data_1000.txt targets_1000.txt
  ```
- **ขนาด 10,000 ค่า:**
  ```bash
  ./timing.exe data_10000.txt targets_10000.txt
  ```
- **ขนาด 100,000 ค่า:**
  ```bash
  ./timing.exe data_100000.txt targets_100000.txt
  ```

---

## 📝 กติกาการบันทึกผลการทดลอง (5-Run Evaluation Rule)
ตามข้อกำหนดของรายวิชา 01204212:
1. ให้รันวัดเวลาแต่ละขนาดข้อมูล **ซ้ำ 5 ครั้ง**
2. **ตัดค่าที่มากที่สุด (Max) และน้อยที่สุด (Min) ออก 2 ค่า**
3. คำนวณหา **ค่าเฉลี่ย (Average) จาก 3 ค่าที่เหลือ** แล้วนำไปกรอกลงในตารางรายงานโครงงาน
4. ตรวจสอบตารางความถูกต้องเสมอ: ต้องพบ 7 ค่า และไม่พบ 3 ค่า จึงจะถือว่าผลการทดสอบถูกต้องสมบูรณ์

---

## 📊 การอ่านและแปลผลการทดสอบ

เมื่อรันโปรแกรมเสร็จสิ้น จะแสดงผลลัพธ์ดังรูปแบบตัวอย่าง:
```text
Reading data from data_100000.txt ... loaded 100000 items.
Reading targets from targets_100000.txt ... loaded 10 targets.

[1] Sequential Search:
    Time: 0.123456 seconds (Found: 10/10)
    Speedup: 1.00x (Baseline)

[2] Binary Search:
    Time: 0.000789 seconds (Found: 10/10)
    Speedup: 156.47x faster than Sequential

[3] AVL Tree Search:
    Time: 0.000654 seconds (Found: 10/10)
    Speedup: 188.77x faster than Sequential
```

* **จำนวนรอบการวนซ้ำ (REPEAT = 1,000):** ระบบจะทำการค้นหาเป้าหมายซ้ำ 1,000 รอบ เพื่อให้ได้ค่าเวลาในระดับมิลลิวินาที/วินาทีที่เสถียรและแม่นยำ
* **Found:** จำนวนเป้าหมายที่ค้นพบ ต้องตรงกันทุกอัลกอริทึมเพื่อยืนยันความถูกต้อง (Correctness)
* **Speedup:** อัตราส่วนความเร็วเทียบกับ Sequential Search ยิ่งขนาดข้อมูล $N$ มีขนาดใหญ่ขึ้น ($10^3 \to 10^5$) AVL Tree Search และ Binary Search จะยิ่งแสดงความเร็วเหนือกว่าอย่างชัดเจนตามทฤษฎี Big-O

---

## 📁 รายละเอียดไฟล์ในโฟลเดอร์นี้

| ชื่อไฟล์ | คำอธิบาย |
|---|---|
| `timing_template.c` | ซอร์สโค้ดภาษา C การวัดเวลาและการค้นหาทั้ง 3 อัลกอริทึม |
| `timing.exe` / `timing_avl.exe` | ไฟล์ Executable ที่คอมไพล์สำเร็จแล้ว |
| `run_benchmark.bat` | สคริปต์รันการทดสอบครบทุกขนาดข้อมูลด้วยการคลิกเดียว |
| `data_1000.txt`, `targets_1000.txt` | ชุดข้อมูลและเป้าหมายการค้นหาขนาด 1,000 ค่า |
| `data_10000.txt`, `targets_10000.txt` | ชุดข้อมูลและเป้าหมายการค้นหาขนาด 10,000 ค่า |
| `data_100000.txt`, `targets_100000.txt` | ชุดข้อมูลและเป้าหมายการค้นหาขนาด 100,000 ค่า |
