/**
 * server.js
 * จุดเริ่มต้นของ Backend API (Express.js)
 * รัน: npm start (จาก root ของโปรเจกต์) หรือ node backend/server.js
 */
const express = require('express');
const path = require('path');

require('./database/init'); // สร้าง/เชื่อมต่อฐานข้อมูลและ seed ข้อมูลตัวอย่างถ้ายังไม่มี

const authRoutes = require('./routes/auth');
const transactionRoutes = require('./routes/transactions');

const app = express();
const PORT = process.env.PORT || 3000;

app.use(express.json());

// อนุญาต CORS แบบง่ายสำหรับใช้งานร่วมกับ frontend ที่รันคนละพอร์ต (เช่น Live Server)
app.use((req, res, next) => {
  res.header('Access-Control-Allow-Origin', '*');
  res.header('Access-Control-Allow-Methods', 'GET,POST,PUT,DELETE,OPTIONS');
  res.header('Access-Control-Allow-Headers', 'Content-Type, Authorization');
  if (req.method === 'OPTIONS') return res.sendStatus(200);
  next();
});

app.use('/api', authRoutes);
app.use('/api', transactionRoutes);

// เสิร์ฟไฟล์ frontend แบบ static ด้วย (ทำให้เปิดได้จาก http://localhost:3000 โดยตรง)
app.use(express.static(path.join(__dirname, '..', 'frontend')));

app.get('/api/health', (req, res) => res.json({ ok: true, service: 'SmartBudget API', time: new Date().toISOString() }));

app.use((req, res) => res.status(404).json({ ok: false, message: 'ไม่พบ endpoint ที่ร้องขอ' }));

app.use((err, req, res, next) => {
  console.error(err);
  res.status(500).json({ ok: false, message: 'เกิดข้อผิดพลาดภายในเซิร์ฟเวอร์', error: err.message });
});

app.listen(PORT, () => {
  console.log(`SmartBudget API is running at http://localhost:${PORT}`);
  console.log(`Frontend (static): http://localhost:${PORT}/index.html`);
});
