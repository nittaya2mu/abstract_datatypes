# SmartBudget

ระบบจัดการรายรับ-รายจ่ายและช่วยวางแผนลดค่าใช้จ่ายสำหรับนักศึกษา

## 1. Project นี้คืออะไร

SmartBudget เป็นเว็บแอปพลิเคชันสำหรับนักศึกษาใช้บันทึกรายรับ-รายจ่ายประจำวัน ตั้งงบประมาณรายเดือน
และเมื่อรายจ่ายเริ่มเกินงบ ระบบจะใช้อัลกอริทึมช่วยวิเคราะห์ว่า **ควรลดหรือตัดรายการไหน** เพื่อประหยัดเงิน
ได้มากที่สุด โดยกระทบชีวิตประจำวันน้อยที่สุด พร้อมอธิบายเหตุผลและขั้นตอนการคำนวณให้เห็นทุกขั้นตอน

โปรเจกต์นี้ยังเป็นแบบฝึกหัดการนำ **โครงสร้างข้อมูล (Data Structures)** และ **อัลกอริทึม (Algorithms)**
มาใช้แก้ปัญหาจริง ไม่ใช่แค่ทฤษฎีในตำรา

## 2. Problem Statement

> "หากเดือนนี้ผู้ใช้มีงบประมาณจำกัด แต่มีรายการสิ่งที่อยากได้หรือค่าใช้จ่ายผันแปรหลายรายการ
> ระบบจะใช้อัลกอริทึมอะไรในการเลือกตัดหรือลดค่าใช้จ่าย เพื่อให้ประหยัดเงินได้มากที่สุด
> ภายใต้เงื่อนไขความจำเป็นที่ต่างกัน?"

SmartBudget ตอบโจทย์นี้ด้วยการให้ผู้ใช้กำหนด **ค่าความจำเป็น (necessity 1-5)** ให้ทุกรายจ่าย แล้วให้ระบบ
เลือกอัลกอริทึมมาคำนวณหาชุดรายการที่ควรตัด โดยมีให้เลือก 2 แบบ (ดูหัวข้อ Algorithms)

## 3. Features

- ระบบ Login แบบ Demo (บัญชีทดลองในตัว)
- Dashboard สรุปรายรับ/รายจ่าย/คงเหลือ/งบประมาณ พร้อม progress bar และการแจ้งเตือนเมื่อใกล้/เกินงบ
- CRUD รายการรายรับ-รายจ่ายเต็มรูปแบบ (เพิ่ม/แก้ไข/ลบ/ดูทั้งหมด) พร้อมกำหนดหมวดหมู่ วันที่ เวลา และค่าความจำเป็น
- ค้นหาธุรกรรมย้อนหลังด้วย Keyword / ช่วงวันที่ / หมวดหมู่ / ประเภท และเรียงลำดับผลลัพธ์ได้
- Budget Optimization: วิเคราะห์และแนะนำรายการที่ควรตัด พร้อมเหตุผลและขั้นตอนการคำนวณแบบเข้าใจง่าย
- หน้า Data Structure & Algorithm: ดูสถานะจริงของ Linked List, AVL Tree, Hash Table และรัน Benchmark
  เปรียบเทียบ Quick Sort กับ Merge Sort ได้จากข้อมูลจริงในระบบ
- **ของที่อยากได้ (Wishlist / Savings Goal):** ตั้งเป้าหมายสิ่งของที่อยากซื้อ พร้อมเลือกแผนการเก็บเงิน 3 แบบ
  ให้ระบบคำนวณให้อัตโนมัติ (ดูรายละเอียดในหัวข้อ 5)
- **โหมดสว่าง / โหมดมืด:** สลับธีมได้จากปุ่มมุมขวาบน (หน้า Dashboard) หรือมุมขวาบนของหน้า Login
  ระบบจำโหมดที่เลือกไว้ให้อัตโนมัติในการเปิดใช้งานครั้งถัดไป
- ข้อมูลตัวอย่าง (seed data) พร้อมใช้งานทันทีตั้งแต่เปิดเว็บครั้งแรก
- บันทึกข้อมูลถาวร: โหมด Frontend-only ใช้ `localStorage` (ไม่ต้องตั้งเซิร์ฟเวอร์ก็ใช้งานได้จริง), โหมด Backend
  ใช้ฐานข้อมูล SQLite จริง

## 4. Data Structures

| โครงสร้างข้อมูล | ใช้เก็บ/ทำอะไร | ไฟล์ (Frontend) |
|---|---|---|
| **Doubly Linked List** | เก็บลำดับธุรกรรมทั้งหมด แต่ละ Node มี `prev`/`next`, เพิ่ม-ลบ O(1) | `frontend/js/linkedList.js` |
| **AVL Tree (Self-Balancing BST)** | index ตาม timestamp วันเวลา ใช้ทำ Range Search ตามช่วงเวลา O(log n + k) | `frontend/js/avlTree.js` |
| **Hash Table (Separate Chaining)** | สรุปยอดเงินตามหมวดหมู่แบบเร็ว O(1) โดยเฉลี่ย ผ่าน DJB2 hash function | `frontend/js/hashTable.js` |

ทั้ง 3 โครงสร้างสามารถดูสถานะจริง (จำนวน node, ความสูงต้นไม้, การกระจายตัวใน bucket ฯลฯ) ได้ที่หน้า
**Data Structure & Algorithm** ในแอป

## 5. Algorithms

| อัลกอริทึม | ใช้ทำอะไร | Complexity |
|---|---|---|
| **Quick Sort** | เรียงลำดับธุรกรรม (Lomuto partition) | เฉลี่ย O(n log n), แย่สุด O(n²) |
| **Merge Sort** | เรียงลำดับธุรกรรมแบบเสถียร (stable) ใช้เป็นค่าเริ่มต้นของหน้า Search | O(n log n) ทุกกรณี |
| **Greedy Algorithm** | Budget Optimization โดยตัดรายการ necessity สูงสุด (ตัดง่ายสุด) ก่อน จนพอกับยอดที่เกินงบ | O(n log n) |
| **0/1 Knapsack (Dynamic Programming)** | Budget Optimization โดยหาชุดรายการที่ตัดแล้วได้ผลกระทบรวม (impact) ต่ำที่สุด รับประกันผลลัพธ์ดีที่สุด | O(n × ยอดเกินงบ) |

สามารถเลือกสลับระหว่าง Greedy และ Knapsack ได้ที่หน้า Budget Optimization เพื่อเปรียบเทียบผลลัพธ์
และหน้า Data Structure & Algorithm มีปุ่ม "รันการทดสอบ" เพื่อเทียบเวลา/จำนวนการเปรียบเทียบของ Quick Sort กับ Merge Sort จริง

## 6. ของที่อยากได้ (Wishlist / Savings Goal)

หน้านี้ให้ตั้งเป้าหมายสิ่งของที่อยากซื้อ (ชื่อ + ราคา) แล้วเลือกแผนการเก็บเงิน 3 รูปแบบ โดยระบบจะคำนวณตัวเลขที่
เกี่ยวข้องให้ทันที (แสดง preview แบบ live ในฟอร์ม):

| รูปแบบ | ผู้ใช้กำหนด | ระบบคำนวณให้ |
|---|---|---|
| **1. เก็บเท่ากันทุกวัน** | จำนวนวันที่ต้องการเก็บให้ครบ | จำนวนเงินที่ต้องเก็บต่อวัน (ราคา ÷ จำนวนวัน) |
| **2. เก็บวันละทวีคูณ** | เงินวันแรก + ตัวคูณต่อวัน (เช่น ×2) | จำนวนวันที่ต้องใช้ (คำนวณจากผลรวมอนุกรมเรขาคณิต) พร้อมตารางเก็บเงินรายวัน |
| **3. กำหนดเอง** | จำนวนเงินที่จะเก็บต่อวัน (คงที่) | จำนวนวันที่ต้องใช้ และวันที่คาดว่าจะเก็บครบ |

แต่ละเป้าหมายมีปุ่ม **"ฝากเงิน"** ให้บันทึกยอดที่ออมจริงในแต่ละวัน โดยระบบจะเสนอจำนวนเงินที่แนะนำให้อัตโนมัติ
ตามแผนที่เลือกไว้และจำนวนครั้งที่ฝากไปแล้ว (แก้ไขจำนวนเองได้ก่อนยืนยัน) เมื่อยอดออมครบราคาสินค้า ระบบจะขึ้น
สถานะ "ครบเป้าหมาย 🎉" ให้อัตโนมัติ

ไฟล์ที่เกี่ยวข้อง: `frontend/js/wishlistPlanner.js` (สูตรคำนวณทั้ง 3 แบบ), `frontend/js/wishlistStore.js`
(บันทึกข้อมูลลง localStorage แยกจากธุรกรรมหลัก)

> หมายเหตุ: ฟีเจอร์นี้เป็นข้อมูลอิสระจากรายรับ-รายจ่ายหลัก (ยังไม่หักเงินที่ฝากออกจากยอดคงเหลือใน Dashboard
> โดยอัตโนมัติ) และปัจจุบันมีเฉพาะฝั่ง Frontend (localStorage) เท่านั้น ยังไม่มี API ฝั่ง Backend รองรับ

## 7. โหมดสว่าง / โหมดมืด (Light / Dark Mode)

กดปุ่มสลับโหมดที่มุมขวาบนของหน้า Dashboard หรือมุมขวาบนของหน้า Login ได้ทันที ระบบจะจดจำโหมดที่เลือกไว้ผ่าน
`localStorage` (ไฟล์ `frontend/js/theme.js`) และตั้งค่าตั้งแต่ก่อนหน้าเว็บ render เพื่อไม่ให้จอกะพริบสลับสี

## 8. Database

- **Frontend-only mode (ค่าเริ่มต้น):** ข้อมูลเก็บใน `localStorage` ของเบราว์เซอร์ ใช้งานได้ทันทีโดยไม่ต้องรัน backend
  รีเฟรชหน้าเว็บแล้วข้อมูลไม่หาย เหมาะสำหรับสาธิต/ทดสอบอย่างรวดเร็ว
- **Full-stack mode:** Backend ใช้ Node.js + Express.js + **SQLite** (ผ่านไลบรารี `better-sqlite3`)
  ไฟล์ฐานข้อมูลจะถูกสร้างอัตโนมัติที่ `backend/database/smartbudget.db` พร้อม seed ข้อมูลตัวอย่างในการรันครั้งแรก

> หมายเหตุ: หน้าเว็บ (frontend) และ backend API เป็นคนละชุดข้อมูลกันในตอนนี้ (frontend ใช้ localStorage,
> backend ใช้ SQLite ผ่าน REST API แยกต่างหาก) หากต้องการเชื่อมสองส่วนเข้าด้วยกันจริง ให้แก้ `frontend/js/store.js`
> ให้เรียก REST API ของ backend แทนการอ่าน/เขียน localStorage โดยตรง (โครงสร้างฟังก์ชันของ Store ถูกออกแบบ
> ให้สลับไส้ในได้ง่ายโดยไม่กระทบหน้า UI)

## 9. การทดสอบ (Testing)

โปรเจกต์นี้มีชุดทดสอบอัตโนมัติ (`tests/test-suite.js`) ครอบคลุมทั้งกรณีปกติและ **กรณีขอบ (edge cases)**
ของทุกโครงสร้างข้อมูลและอัลกอริทึมหลัก เช่น list/tree ว่าง, ลบ head/tail, key ซ้ำใน AVL Tree, งบประมาณ
เป็น 0, ไม่มีรายการให้ตัด, เปรียบเทียบผลลัพธ์ Greedy กับ Knapsack ฯลฯ

```bash
npm test
# หรือ
node tests/test-suite.js
```

ผลการรันล่าสุด (52 เคสทดสอบ ผ่านทั้งหมด) ถูกบันทึกไว้ที่ [`tests/test-results.md`](tests/test-results.md)

## 10. การวัดประสิทธิภาพ (Benchmark)

### 10.1 Benchmark ภายในโปรเจกต์

ไฟล์ `tests/benchmark.js` ใช้สำหรับสาธิตและตรวจแนวโน้มประสิทธิภาพของโครงสร้างข้อมูล/อัลกอริทึมภายใน SmartBudget (Quick Sort, Merge Sort, AVL Tree, Hash Table และ Budget Optimization)

```bash
npm run benchmark
# หรือ
node tests/benchmark.js
```

> Benchmark นี้เป็นการทดสอบภายในโปรเจกต์ ไม่ใช่ชุดวัดเวลาสำหรับกิจกรรมหน้าห้องตามเอกสารรายวิชา

### 10.2 C Timing สำหรับกิจกรรมวัดประสิทธิภาพของรายวิชา 01204212

เพื่อให้ตรงกับเกณฑ์กิจกรรมในวันนำเสนอ ให้ใช้ `timing/timing_avl.c` ซึ่งเป็นไฟล์เต็มของกลุ่มและมี **AVL Search ของกลุ่ม** อยู่ใน `MySearch(AVLNode *root, int target)`

คุณสมบัติของโปรแกรม:

- ภาษา C และคอมไพล์ด้วย `gcc` ได้
- ใช้ `REPEAT = 1000` ค่าคงที่เดียวกันทุกการทดลอง
- รองรับข้อมูล n = 1,000 / 10,000 / 100,000
- ตรวจผลค้นหาก่อนจับเวลา
- ใช้ target 10 ค่าตามชุดข้อมูลกลาง
- จับเวลาเฉพาะ `MySearch()` ไม่รวมอ่านไฟล์และสร้าง AVL
- วัด 5 ครั้งต่อขนาดข้อมูล
- ตัดค่าสูงสุดและต่ำสุด แล้วเฉลี่ย 3 ค่าที่เหลือ

คอมไพล์ (เข้าโฟลเดอร์ `timing/` ก่อน เพื่อให้ตรงกับคำสั่งของชุดข้อมูลกลาง):

```bash
cd timing
gcc timing_avl.c -o timing_avl
```

รัน (เมื่ออยู่ในโฟลเดอร์ `timing/`):

```bash
./timing_avl ../../04_TestData/ชุดข้อมูลทดสอบ_01204212/data_1000.txt ../../04_TestData/ชุดข้อมูลทดสอบ_01204212/targets_1000.txt
./timing_avl ../../04_TestData/ชุดข้อมูลทดสอบ_01204212/data_10000.txt ../../04_TestData/ชุดข้อมูลทดสอบ_01204212/targets_10000.txt
./timing_avl ../../04_TestData/ชุดข้อมูลทดสอบ_01204212/data_100000.txt ../../04_TestData/ชุดข้อมูลทดสอบ_01204212/targets_100000.txt
```

บน Windows ใช้ `gcc timing_avl.c -o timing_avl.exe` ในโฟลเดอร์ `timing/` และรัน `timing_avl.exe` ตามตัวอย่างใน `timing/README.md` หรือใช้ `run_all.bat`.

**สำคัญ:** ให้ใช้ชุดข้อมูลกลางจาก `ชุดข้อมูลทดสอบ_01204212.rar` ของผู้สอนในวันนำเสนอ และบันทึกผลจาก **เครื่องกลางในห้องเรียน** ลงตารางรายงาน ไม่ควรนำเวลาจากเครื่องอื่นไปอ้างว่าเป็นเวลาของเครื่องกลาง

รายละเอียดขั้นตอนและการวิเคราะห์ Big-O อยู่ที่ `timing/README.md`

## 11. เหตุผลการเลือกโครงสร้างข้อมูล (Design Rationale)

คำอธิบายละเอียดว่าทำไมเลือกใช้ Doubly Linked List / AVL Tree / Hash Table / Greedy+Knapsack แทนทางเลือก
อื่นที่ดูง่ายกว่า (เช่น Array ธรรมดา, Array+sort ใหม่ทุกครั้ง, Object ของ JavaScript) พร้อมอ้างอิงผลทดสอบจริง
อยู่ที่ [`docs/design-rationale.md`](docs/design-rationale.md)

## เครื่องมือ / ภาษา / Library ที่ต้องใช้

| ส่วน | ที่ใช้ |
|---|---|
| Frontend | HTML, CSS, JavaScript (Vanilla) — ไม่ต้องติดตั้ง Library เพิ่ม |
| Backend (ไม่บังคับ) | Node.js 18+, Express.js 4, better-sqlite3 |
| Database | SQLite (สร้างไฟล์อัตโนมัติ) หรือ localStorage ในโหมด Frontend-only |
| เบราว์เซอร์ | Chrome / Edge / Firefox เวอร์ชันล่าสุด |

## ไฟล์ข้อมูลตัวอย่าง (Data Files)

- `data/sample_data.json` — ข้อมูลธุรกรรมตัวอย่าง 12 รายการ + งบประมาณ 10,000 บาท ใช้ทดสอบ Budget Optimization
- ระบบโหลดข้อมูลชุดเดียวกันนี้อัตโนมัติเมื่อเปิดใช้งานครั้งแรก (จาก `sampleData.js` ในโค้ด)
- `backend/database/smartbudget.db` ถูกสร้างอัตโนมัติเมื่อรัน `npm start` ครั้งแรก

## 12. วิธีติดตั้ง

### แบบที่ 1: เปิดใช้งานทันที (Frontend-only, ไม่ต้องติดตั้งอะไรเลย)

```bash
# เปิดไฟล์นี้ด้วยเบราว์เซอร์โดยตรง หรือใช้ VSCode "Live Server"
frontend/index.html
```

### แบบที่ 2: รันแบบ Full-stack (Frontend + Backend + Database)

ต้องติดตั้ง [Node.js](https://nodejs.org) เวอร์ชัน 18 ขึ้นไปก่อน

```bash
cd SmartBudget
npm install && npm start
```

เมื่อรันสำเร็จ backend จะสร้างไฟล์ฐานข้อมูลและ seed ข้อมูลตัวอย่างให้อัตโนมัติ

> ตรวจสอบก่อนนำเสนอ: บนเครื่องห้องปฏิบัติการให้รัน `npm install && npm start` จริงก่อนสาธิต Full-stack และตรวจ `http://localhost:3000/api/health` ให้ตอบกลับสำเร็จ

## 13. วิธี Run

- **Frontend-only:** เปิด `frontend/index.html` ด้วยเบราว์เซอร์ (หรือรันผ่าน Live Server ที่พอร์ตใดก็ได้)
- **Full-stack:** รัน `npm start` แล้วเปิด browser ไปที่ `http://localhost:3000` (server จะเสิร์ฟทั้งหน้าเว็บและ API)
  - Frontend: `http://localhost:3000/index.html`
  - API Health check: `http://localhost:3000/api/health`

## 14. Username / Password (Demo)

```
Username: student
Password: 1234
```

## 15. ตัวอย่าง API (Backend)

| Method | Endpoint | คำอธิบาย |
|---|---|---|
| POST | `/api/login` | เข้าสู่ระบบด้วยบัญชี Demo |
| GET | `/api/transactions` | ดูรายการธุรกรรมทั้งหมด (เรียงใหม่สุดก่อน) |
| POST | `/api/transactions` | เพิ่มรายการใหม่ |
| PUT | `/api/transactions/:id` | แก้ไขรายการ |
| DELETE | `/api/transactions/:id` | ลบรายการ |
| GET | `/api/transactions/search?keyword=&start=&end=&category=&type=&sortBy=&order=` | ค้นหา/กรอง/เรียงลำดับ |
| GET | `/api/dashboard` | สรุปข้อมูลสำหรับหน้า Dashboard |
| GET / POST | `/api/budget` | ดู/ตั้งค่างบประมาณรายเดือน |
| POST | `/api/optimization` | รัน Budget Optimization (`body: { "method": "greedy" \| "knapsack" }`) |

ตัวอย่างการเรียก:

```bash
curl -X POST http://localhost:3000/api/login \
  -H "Content-Type: application/json" \
  -d '{"username":"student","password":"1234"}'

curl -X POST http://localhost:3000/api/optimization \
  -H "Content-Type: application/json" \
  -d '{"method":"knapsack"}'
```

## 16. วิธีทดสอบ Budget Optimization

1. เปิดแอป (frontend-only หรือ full-stack ก็ได้) แล้ว Login ด้วยบัญชี Demo
2. ไปที่เมนู **Budget Optimization**
3. ข้อมูลตัวอย่างตั้งงบประมาณไว้ที่ **10,000 บาท** ในขณะที่รายจ่ายรวมมากกว่านั้น (ดูได้จาก Dashboard)
   ระบบจะแสดงว่าเกินงบไปเท่าไหร่ทันที
4. ลองสลับปุ่มระหว่าง **Greedy Algorithm** และ **0/1 Knapsack (DP)** เพื่อเปรียบเทียบว่าแต่ละอัลกอริทึม
   แนะนำให้ตัดรายการไหนบ้าง ประหยัดได้เท่าไหร่ และดูเหตุผล/ขั้นตอนการคำนวณด้านล่าง
5. ลองเพิ่มรายจ่ายใหม่ (เช่น หมวดช้อปปิ้ง necessity 5) หรือแก้ไขงบประมาณ แล้วกลับมาดูหน้านี้อีกครั้ง
   เพื่อดูว่าคำแนะนำเปลี่ยนไปตามข้อมูลจริงหรือไม่

## 17. Flowchart

มี Flowchart ให้ 2 แบบ ไฟล์อยู่ที่ `docs/flowcharts/` (ทั้งไฟล์ `.svg` แบบเวกเตอร์คมชัดทุกขนาด และ `.png`
พร้อมใช้) สามารถเปิดดูตรงๆ หรือแทรกลงรายงาน/สไลด์ได้ทันที:

1. **`system-overview-flowchart`** — ภาพรวมการทำงานทั้งระบบ ตั้งแต่ Login → ตรวจสอบสิทธิ์ → Dashboard →
   เลือกเมนู (ธุรกรรม / ค้นหา / Optimization / Wishlist / Data Structure) → Logout
2. **`budget-optimization-flowchart`** — ขั้นตอนการทำงานของอัลกอริทึม Budget Optimization ตั้งแต่คำนวณยอด
   ที่เกินงบ, แยกสาขา Greedy กับ Knapsack, จนถึงตรวจสอบว่าตัดรายการแล้วพอหรือไม่

## 18. โครงสร้างไฟล์

```
SmartBudget/
├── frontend/
│   ├── index.html          # หน้า Login
│   ├── dashboard.html      # SPA หลัก (Dashboard, รายรับ-รายจ่าย, ค้นหา, Optimization, Data Structure)
│   ├── css/style.css
│   └── js/
│       ├── app.js          # Routing + render ทุกหน้า + ผูก event
│       ├── auth.js         # Login แบบ Demo (localStorage session)
│       ├── store.js        # เชื่อมโครงสร้างข้อมูลทั้งหมด + persist ลง localStorage
│       ├── linkedList.js    # Doubly Linked List
│       ├── avlTree.js       # AVL Tree
│       ├── hashTable.js     # Hash Table
│       ├── quickSort.js
│       ├── mergeSort.js
│       ├── optimizer.js     # Greedy + 0/1 Knapsack
│       ├── sampleData.js
│       ├── theme.js         # สลับโหมดสว่าง/มืด
│       ├── wishlistPlanner.js # สูตรคำนวณแผนเก็บเงิน 3 แบบ
│       └── wishlistStore.js   # บันทึกข้อมูล Wishlist ลง localStorage
├── backend/
│   ├── server.js
│   ├── routes/ (auth.js, transactions.js)
│   ├── controllers/ (authController.js, transactionController.js)
│   ├── models/ (transactionModel.js)
│   ├── utils/ (optimizer.js, sampleData.js)
│   └── database/ (init.js, smartbudget.db สร้างอัตโนมัติตอนรันครั้งแรก)
├── data/
│   └── sample_data.json   # ข้อมูลตัวอย่าง
├── tests/
│   ├── testFramework.js     # เฟรมเวิร์คทดสอบขนาดเล็ก (ไม่พึ่ง library ภายนอก)
│   ├── test-suite.js        # ชุดทดสอบ 52 เคส (ปกติ + กรณีขอบ + regression)
│   ├── test-results.md      # ผลการรันล่าสุด
│   ├── benchmark.js         # วัดประสิทธิภาพจริงที่ 3 ขนาดข้อมูล
│   └── benchmark-results.md # ผลการวัดล่าสุด
├── docs/
│   ├── design-rationale.md  # เหตุผลเลือกโครงสร้างข้อมูล/อัลกอริทึม เทียบกับทางเลือกอื่น
│   └── flowcharts/          # system-overview + budget-optimization (.svg และ .png)
├── timing/
│   ├── timing_template.c    # AVL Search timing ตามรูปแบบคำสั่งของชุดข้อมูลกลาง
│   ├── timing_avl.c         # source เดิมของ AVL timing (สำรอง/อ้างอิง)
│   ├── README.md
│   ├── run_all.bat
│   └── run_all.sh
├── package.json
├── README.md
└── .gitignore
```

## หมายเหตุสิ่งที่ยังไม่สมบูรณ์ 100%

- Backend ใช้ token แบบง่าย (base64) ไม่ใช่ JWT พร้อม expiry จริง — เพียงพอสำหรับสาธิตระดับโปรเจกต์ แต่ไม่ควรใช้ใน production
- Frontend (localStorage) และ Backend (SQLite) ยังเป็นคนละชุดข้อมูลกัน ไม่ได้เชื่อมสดเข้าหากันอัตโนมัติ
  (ดูหมายเหตุในหัวข้อ Database ด้านบนสำหรับวิธีเชื่อมต่อ)
- AVL Tree visualization ในหน้า Data Structure แสดงแบบย่อ (3 ระดับบนสุด) เพื่อไม่ให้หน้าจอแน่นเกินไป ไม่ใช่การวาดต้นไม้เต็มรูปแบบ
- ฟีเจอร์ "ของที่อยากได้" ยังทำงานเฉพาะฝั่ง Frontend (localStorage) ยังไม่มี API/ตารางฝั่ง Backend รองรับ
  และยังไม่เชื่อมกับยอดเงินคงเหลือในหน้า Dashboard โดยอัตโนมัติ
- ชุดทดสอบ (`tests/`) ทดสอบเฉพาะ **ตรรกะ (logic)** ของโครงสร้างข้อมูล/อัลกอริทึมผ่าน Node.js โดยตรง
  (เพราะไฟล์เหล่านี้เขียนแบบไม่พึ่ง DOM) ยังไม่ใช่การทดสอบ UI/E2E ในเบราว์เซอร์จริง และยังไม่ครอบคลุม
  Backend API หรือ WishlistStore (ซึ่งต้องใช้ localStorage ของเบราว์เซอร์)
