/**
 * models/transactionModel.js
 * เลเยอร์เข้าถึงข้อมูลตาราง transactions และ settings (SQLite ผ่าน better-sqlite3)
 */
const db = require('../database/init');

const TransactionModel = {
  all() {
    return db.prepare('SELECT * FROM transactions ORDER BY date, time').all();
  },

  allDesc() {
    return db.prepare('SELECT * FROM transactions ORDER BY date DESC, time DESC').all();
  },

  findById(id) {
    return db.prepare('SELECT * FROM transactions WHERE id = ?').get(id);
  },

  create(data) {
    const id = 'tx' + Date.now().toString(36) + Math.floor(Math.random() * 1000);
    db.prepare(`
      INSERT INTO transactions (id, type, amount, category, date, time, note, necessity)
      VALUES (@id, @type, @amount, @category, @date, @time, @note, @necessity)
    `).run({ id, ...data, note: data.note || '', necessity: data.type === 'expense' ? data.necessity : null });
    return this.findById(id);
  },

  update(id, data) {
    const existing = this.findById(id);
    if (!existing) return null;
    db.prepare(`
      UPDATE transactions SET type=@type, amount=@amount, category=@category, date=@date,
      time=@time, note=@note, necessity=@necessity WHERE id=@id
    `).run({ id, ...data, note: data.note || '', necessity: data.type === 'expense' ? data.necessity : null });
    return this.findById(id);
  },

  remove(id) {
    const info = db.prepare('DELETE FROM transactions WHERE id = ?').run(id);
    return info.changes > 0;
  },

  search({ keyword, start, end, category, type }) {
    let sql = 'SELECT * FROM transactions WHERE 1=1';
    const params = {};
    if (start) { sql += ' AND date >= @start'; params.start = start; }
    if (end) { sql += ' AND date <= @end'; params.end = end; }
    if (category) { sql += ' AND category = @category'; params.category = category; }
    if (type) { sql += ' AND type = @type'; params.type = type; }
    if (keyword) {
      sql += ' AND (category LIKE @kw OR note LIKE @kw)';
      params.kw = `%${keyword}%`;
    }
    return db.prepare(sql).all(params);
  },

  getBudget() {
    const row = db.prepare("SELECT value FROM settings WHERE key = 'budget'").get();
    return row ? Number(row.value) : 0;
  },

  setBudget(amount) {
    db.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES ('budget', ?)").run(String(amount));
  }
};

module.exports = TransactionModel;
