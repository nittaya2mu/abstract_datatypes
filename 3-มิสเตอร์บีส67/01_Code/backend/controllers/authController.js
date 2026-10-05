/**
 * controllers/authController.js
 * Login แบบ Demo: ตรวจสอบ username/password กับตาราง users
 * (ระดับโปรเจกต์นักศึกษา ยังไม่ได้ทำ hashing/JWT เต็มรูปแบบ)
 */
const db = require('../database/init');

exports.login = (req, res) => {
  const { username, password } = req.body || {};
  if (!username || !password) {
    return res.status(400).json({ ok: false, message: 'กรุณาระบุ username และ password' });
  }

  const user = db.prepare('SELECT * FROM users WHERE username = ? AND password = ?').get(username, password);
  if (!user) {
    return res.status(401).json({ ok: false, message: 'Username หรือ Password ไม่ถูกต้อง' });
  }

  // Demo token แบบง่าย (ไม่ใช่ JWT จริง) — เพียงพอสำหรับสาธิตการทำงานระดับโปรเจกต์
  const token = Buffer.from(`${user.username}:${Date.now()}`).toString('base64');
  res.json({ ok: true, token, user: { username: user.username, name: user.name } });
};
