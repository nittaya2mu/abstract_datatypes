/**
 * controllers/transactionController.js
 * CRUD ธุรกรรม + ค้นหา + สรุป Dashboard + Budget Optimization
 */
const TransactionModel = require('../models/transactionModel');
const { runOptimization } = require('../utils/optimizer');

const VALID_TYPES = new Set(['income', 'expense']);
const DATE_RE = /^\d{4}-\d{2}-\d{2}$/;
const TIME_RE = /^([01]\d|2[0-3]):[0-5]\d(?::[0-5]\d)?$/;

function validateTransaction({ type, amount, category, date, time, necessity }) {
  if (!VALID_TYPES.has(type)) return 'type ต้องเป็น income หรือ expense';
  const numericAmount = Number(amount);
  if (!Number.isFinite(numericAmount) || numericAmount <= 0) return 'amount ต้องเป็นตัวเลขที่มากกว่า 0';
  if (typeof category !== 'string' || !category.trim()) return 'category ต้องไม่ว่าง';
  if (typeof date !== 'string' || !DATE_RE.test(date)) return 'date ต้องอยู่ในรูปแบบ YYYY-MM-DD';
  if (typeof time !== 'string' || !TIME_RE.test(time)) return 'time ต้องอยู่ในรูปแบบ HH:MM หรือ HH:MM:SS';

  if (type === 'expense') {
    const n = Number(necessity);
    if (!Number.isInteger(n) || n < 1 || n > 5) return 'necessity ของ expense ต้องเป็นจำนวนเต็ม 1-5';
  }
  return null;
}

exports.list = (req, res) => {
  res.json({ ok: true, data: TransactionModel.allDesc() });
};

exports.create = (req, res) => {
  const { type, amount, category, date, time, note, necessity } = req.body || {};
  const error = validateTransaction({ type, amount, category, date, time, necessity });
  if (error) return res.status(400).json({ ok: false, message: error });
  const record = TransactionModel.create({ type, amount: Number(amount), category: category.trim(), date, time, note, necessity: type === 'expense' ? Number(necessity) : null });
  res.status(201).json({ ok: true, data: record });
};

exports.update = (req, res) => {
  const { id } = req.params;
  const { type, amount, category, date, time, note, necessity } = req.body || {};
  const error = validateTransaction({ type, amount, category, date, time, necessity });
  if (error) return res.status(400).json({ ok: false, message: error });
  const updated = TransactionModel.update(id, { type, amount: Number(amount), category: category.trim(), date, time, note, necessity: type === 'expense' ? Number(necessity) : null });
  if (!updated) return res.status(404).json({ ok: false, message: 'ไม่พบรายการที่ต้องการแก้ไข' });
  res.json({ ok: true, data: updated });
};

exports.remove = (req, res) => {
  const ok = TransactionModel.remove(req.params.id);
  if (!ok) return res.status(404).json({ ok: false, message: 'ไม่พบรายการที่ต้องการลบ' });
  res.json({ ok: true });
};

exports.search = (req, res) => {
  const { keyword, start, end, category, type, sortBy = 'date', order = 'desc' } = req.query;
  let results = TransactionModel.search({ keyword, start, end, category, type });

  results.sort((a, b) => {
    let va, vb;
    if (sortBy === 'amount') { va = a.amount; vb = b.amount; }
    else if (sortBy === 'category') { va = a.category; vb = b.category; }
    else { va = `${a.date}T${a.time}`; vb = `${b.date}T${b.time}`; }
    if (va < vb) return order === 'asc' ? -1 : 1;
    if (va > vb) return order === 'asc' ? 1 : -1;
    return 0;
  });

  res.json({ ok: true, data: results, count: results.length });
};

exports.dashboard = (req, res) => {
  const all = TransactionModel.all();
  const income = all.filter(t => t.type === 'income').reduce((s, t) => s + t.amount, 0);
  const expense = all.filter(t => t.type === 'expense').reduce((s, t) => s + t.amount, 0);
  const budget = TransactionModel.getBudget();

  const categoryTotals = {};
  all.filter(t => t.type === 'expense').forEach(t => {
    categoryTotals[t.category] = (categoryTotals[t.category] || 0) + t.amount;
  });

  res.json({
    ok: true,
    data: {
      totalIncome: income,
      totalExpense: expense,
      balance: income - expense,
      budget,
      count: all.length,
      categoryTotals: Object.entries(categoryTotals).map(([category, total]) => ({ category, total })).sort((a, b) => b.total - a.total),
      recent: TransactionModel.allDesc().slice(0, 6)
    }
  });
};

exports.getBudget = (req, res) => {
  res.json({ ok: true, budget: TransactionModel.getBudget() });
};

exports.setBudget = (req, res) => {
  const { budget } = req.body || {};
  const numericBudget = Number(budget);
  if (!Number.isFinite(numericBudget) || numericBudget <= 0) return res.status(400).json({ ok: false, message: 'กรุณาระบุงบประมาณให้ถูกต้อง' });
  TransactionModel.setBudget(numericBudget);
  res.json({ ok: true, budget: numericBudget });
};

exports.optimize = (req, res) => {
  const method = (req.body && req.body.method) || 'greedy';
  const all = TransactionModel.all();
  const expenses = all.filter(t => t.type === 'expense');
  const totalExpense = expenses.reduce((s, t) => s + t.amount, 0);
  const budget = TransactionModel.getBudget();

  const outcome = runOptimization(expenses, budget, totalExpense, method);
  res.json({ ok: true, data: outcome });
};
