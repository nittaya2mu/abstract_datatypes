# 🖥️ คู่มือการทดสอบประสิทธิภาพภายในแอปพลิเคชัน (In-App TUI Performance Test)
### โครงงาน KU CSC Course Manager — การวัดเวลาประมวลผลจริง

โฟลเดอร์นี้บรรจุโปรแกรม **KU CSC Course Manager เวอร์ชันทดสอบประสิทธิภาพ (Performance Instrumentation)** ซึ่งมีการฝังตัวจับเวลาระดับไมโครวินาที (High-Resolution Timer) ลงในแต่ละขั้นตอนของอัลกอริทึม เพื่อแสดงผลระยะเวลาที่ใช้ในการประมวลผลบนหน้าจอ TUI โดยตรง:

1. **AVL Tree Construction & Filtering:** วัดเวลาสร้างและค้นหาข้อมูลวิชาที่นิสิตลงเรียนได้
2. **Merge Sort:** วัดเวลาจัดเรียงรายวิชาในตารางเรียนประจำสัปดาห์
3. **Backtracking Algorithm:** วัดเวลาค้นหาและจัดชุดกลุ่มวิชาที่เหมาะสมที่สุดตามโควตาหน่วยกิต
4. **Topological Sort (Kahn's / DFS DAG):** วัดเวลาประมวลผลโครงสร้างลำดับวิชาบังคับก่อน

---

## 🛠️ การติดตั้งไลบรารีที่จำเป็น (Dependencies)

โปรแกรมนี้จำเป็นต้องใช้ไลบรารี **`ncurses`** ในการวาดและควบคุมหน้าจอ TUI:

### 1. สำหรับ Windows (ผ่าน MSYS2 - แนะนำ)
1. ติดตั้ง **MSYS2** จาก [https://www.msys2.org/](https://www.msys2.org/)
2. เปิดเทอร์มินัล **MSYS2 UCRT64** แล้วรันคำสั่ง:
   ```bash
   pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ncurses
   ```
   *(หากใช้ MINGW64 ให้ใช้ `mingw-w64-x86_64-...` แทน)*

### 2. สำหรับ macOS
1. ติดตั้ง Xcode Command Line Tools:
   ```bash
   xcode-select --install
   ```
2. ติดตั้ง `ncurses` ผ่าน Homebrew:
   ```bash
   brew install ncurses
   ```

### 3. สำหรับ Linux (Ubuntu / Debian)
```bash
sudo apt-get update
sudo apt-get install build-essential libncurses5-dev libncursesw5-dev
```

---

## ⚙️ ขั้นตอนการคอมไพล์ (Compilation)

### บน Windows (ผ่าน MSYS2 Terminal):
```bash
gcc main_perf_test.c -o main_perf_test.exe -lncursesw
```
*(หากพบปัญหา `-lncursesw` ให้ลองสลับเป็น `-lncurses` แทน)*

### บน macOS:
```bash
gcc main_perf_test.c -o main_perf_test -I$(brew --prefix ncurses)/include -L$(brew --prefix ncurses)/lib -lncurses
```

### บน Linux:
```bash
gcc main_perf_test.c -o main_perf_test -lncursesw
```

*(ในโฟลเดอร์นี้มีไฟล์ `main_perf_test.exe` สำหรับ Windows ที่คอมไพล์ไว้ให้พร้อมใช้งานแล้ว)*

---

## 🚀 ขั้นตอนการรันโปรแกรม (Execution)

### 1. วิธีที่สะดวกที่สุดบน Windows:
ดับเบิลคลิกไฟล์ **`run_perf_test.bat`** 
> สคริปต์นี้จะตั้งค่า `PATH` และ `TERMINFO` ไปยัง MSYS2 ให้อัตโนมัติ พร้อมเปิดหน้าต่างคอนโซลขนาดเหมาะสมและรัน `main_perf_test.exe` ทันที

### 2. รันผ่าน Terminal:
- **Windows:**
  ```bash
  ./main_perf_test.exe
  ```
- **macOS / Linux:**
  ```bash
  ./main_perf_test
  ```

> ⚠️ **คำแนะนำการแสดงผล:** หน้าจอคอนโซล/Terminal ควรมีขนาดอย่างน้อย **95 คอลัมน์ x 20 บรรทัด** เพื่อให้หน้าต่าง TUI แสดงผลได้อย่างสมบูรณ์

---

## 📋 ขั้นตอนการทดสอบและการสังเกตผลจับเวลา (Walkthrough)

เมื่อเปิดโปรแกรมขึ้นมา ให้เข้าสู่ระบบโดยใช้บัญชีทดสอบ:
* **Username:** `6840202250`
* **Password:** *(รหัสผ่านที่ตั้งไว้ เช่น `1234` หรือกดสมัครสมาชิกใหม่ในหน้า Register)*

จากนั้นเข้าทดสอบแต่ละฟังก์ชันใน Main Menu เพื่อดูเวลาประมวลผล:

| เมนูที่ทดสอบ | อัลกอริทึมที่ทำงานเบื้องหลัง | จุดสังเกตเวลาบนหน้าจอ |
|---|---|---|
| **1. View Available Courses** | **AVL Tree Search** | ดูที่ Title bar ด้านบน: `AVAILABLE COURSES [AVL Time: X.XXX ms]` |
| **2. View My Timetable** | **Merge Sort** | ดูที่ Title bar ด้านบน: `TIMETABLE [Sort Time: X.XXX ms]` |
| **5. Recommend Courses** | **Backtracking** | หลังจากเลือกสาขา (Major) จะแสดงเวลาค้นหาชุดวิชาบนหัวตาราง: `[Backtrack Time: X.XXX ms]` |
| **6. Prerequisite Map** | **Topological Sort** | ดูที่ Title bar ด้านบน: `PREREQUISITE MAP [TopoSort Time: X.XXX ms]` |

---

## 📁 ไฟล์ประกอบในโฟลเดอร์นี้

| ชื่อไฟล์ | คำอธิบาย |
|---|---|
| `main_perf_test.c` | ซอร์สโค้ดโปรแกรมเวอร์ชันแทรกตัวจับเวลาประสิทธิภาพ |
| `main_perf_test.exe` | ไฟล์ Executable สำหรับเปิดทดสอบ |
| `run_perf_test.bat` | สคริปต์รันโปรแกรมทดสอบสำหรับ Windows แบบคลิกเดียว |
| `patch_perf.py` | สคริปต์ Python ที่ใช้แปลงโค้ดต้นฉบับและแทรกตัวจับเวลา |
| `courses.txt` | ฐานข้อมูลรายวิชาทั้งหมดของมหาวิทยาลัย |
| `users.txt` | ฐานข้อมูลบัญชีผู้ใช้งาน |
| `student*.txt` | ข้อมูลประวัติการลงทะเบียนและการเรียนของนิสิต |
