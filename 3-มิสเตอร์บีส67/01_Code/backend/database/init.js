/**
 * database/init.js
 * สร้างไฟล์ฐานข้อมูล SQLite (smartbudget.db) พร้อมตารางและข้อมูลตัวอย่าง
 * ใช้ better-sqlite3 (synchronous, เร็ว, ไม่ต้อง native build ซับซ้อน)
 */
const path = require('path');
const Database = require('better-sqlite3');

const DB_PATH = process.env.SMARTBUDGET_DB_PATH || path.join(__dirname, 'smartbudget.db');
const db = new Database(DB_PATH);

db.pragma('journal_mode = WAL');

db.exec(`
  CREATE TABLE IF NOT EXISTS users (
    username TEXT PRIMARY KEY,
    password TEXT NOT NULL,
    name TEXT NOT NULL
  );

  CREATE TABLE IF NOT EXISTS transactions (
    id TEXT PRIMARY KEY,
    type TEXT NOT NULL CHECK(type IN ('income','expense')),
    amount REAL NOT NULL,
    category TEXT NOT NULL,
    date TEXT NOT NULL,
    time TEXT NOT NULL,
    note TEXT,
    necessity INTEGER
  );

  CREATE TABLE IF NOT EXISTS settings (
    key TEXT PRIMARY KEY,
    value TEXT
  );
`);

const userCount = db.prepare('SELECT COUNT(*) AS c FROM users').get().c;
if (userCount === 0) {
  db.prepare('INSERT INTO users (username, password, name) VALUES (?, ?, ?)')
    .run('student', '1234', 'นิสิต Demo');
}

const txCount = db.prepare('SELECT COUNT(*) AS c FROM transactions').get().c;
if (txCount === 0) {
  const { generateSampleData, SAMPLE_BUDGET } = require('../utils/sampleData');
  const insert = db.prepare(`
    INSERT INTO transactions (id, type, amount, category, date, time, note, necessity)
    VALUES (@id, @type, @amount, @category, @date, @time, @note, @necessity)
  `);
  const insertMany = db.transaction((rows) => rows.forEach(r => insert.run(r)));
  insertMany(generateSampleData());

  db.prepare('INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)').run('budget', String(SAMPLE_BUDGET));
  console.log(`Seeded ${generateSampleData().length} sample transactions.`);
}

console.log('SmartBudget database ready at', DB_PATH);
module.exports = db;
