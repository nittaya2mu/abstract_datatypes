/**
 * utils/optimizer.js
 * ตรรกะ Budget Optimization ฝั่ง Backend (เทียบเท่า frontend/js/optimizer.js)
 * ดูคำอธิบายอัลกอริทึมแบบเต็มได้ในไฟล์ฝั่ง frontend
 */
const { performance } = require('perf_hooks');

function greedyOptimize(expenses, overBudgetAmount) {
  const steps = [];
  let remaining = overBudgetAmount;
  const cutList = [];

  const sorted = [...expenses].sort((a, b) => {
    if (b.necessity !== a.necessity) return b.necessity - a.necessity;
    return b.amount - a.amount;
  });

  steps.push({ title: 'ขั้นตอนที่ 1: เรียงลำดับรายจ่าย', detail: `เรียงรายการทั้งหมด ${expenses.length} รายการ ตามค่าความจำเป็นจากมากไปน้อย (necessity สูง = ตัดง่าย มาก่อน)` });

  for (const item of sorted) {
    if (remaining <= 0) break;
    cutList.push(item);
    const before = remaining;
    remaining -= item.amount;
    steps.push({
      title: `พิจารณา: ${item.category} (${item.amount.toLocaleString()} บาท, necessity ${item.necessity})`,
      detail: `ยอดที่ยังต้องลดอีก ${before.toLocaleString()} บาท → เลือกตัดเพราะกระทบชีวิตประจำวันน้อยที่สุด → คงเหลือต้องลด ${Math.max(remaining, 0).toLocaleString()} บาท`
    });
  }

  const totalSaved = cutList.reduce((s, i) => s + i.amount, 0);
  return { algorithm: 'Greedy Algorithm', cutList, totalSaved, stillOver: Math.max(remaining, 0), achieved: remaining <= 0, steps };
}

function knapsackOptimize(expenses, overBudgetAmount) {
  const steps = [];
  const target = Math.ceil(overBudgetAmount);
  const n = expenses.length;

  if (target <= 0 || n === 0) {
    return { algorithm: '0/1 Knapsack (DP)', cutList: [], totalSaved: 0, stillOver: 0, achieved: true, steps: [{ title: 'ไม่มีรายจ่ายเกินงบ', detail: 'ไม่จำเป็นต้องตัดรายการใด' }] };
  }

  const maxSave = expenses.reduce((s, e) => s + e.amount, 0);
  const cap = Math.min(target, maxSave);
  const dp = new Array(cap + 1).fill(Infinity);
  dp[0] = 0;
  const choice = Array.from({ length: n }, () => new Array(cap + 1).fill(false));

  for (let i = 0; i < n; i++) {
    const amount = Math.min(expenses[i].amount, cap);
    const impact = 6 - expenses[i].necessity;
    for (let j = cap; j >= 0; j--) {
      const prevJ = Math.max(j - amount, 0);
      if (dp[prevJ] + impact < dp[j]) { dp[j] = dp[prevJ] + impact; choice[i][j] = true; }
    }
  }

  let bestJ = cap;
  while (bestJ > 0 && dp[bestJ] === Infinity) bestJ--;

  const cutList = [];
  let j = bestJ;
  for (let i = n - 1; i >= 0; i--) {
    if (choice[i][j]) {
      cutList.push(expenses[i]);
      const amount = Math.min(expenses[i].amount, cap);
      j = Math.max(j - amount, 0);
    }
  }

  const totalSaved = cutList.reduce((s, i) => s + i.amount, 0);
  steps.push({ title: 'ขั้นตอนที่ 1: สร้างตาราง DP', detail: `สร้างตาราง dp ขนาด ${cap + 1} ช่อง แทนยอดเงินที่ต้องประหยัด` });
  steps.push({ title: 'ขั้นตอนที่ 2: พิจารณาแต่ละรายการ (0/1)', detail: `วนพิจารณารายจ่ายทั้ง ${n} รายการทีละตัว อัปเดตตาราง dp จากขวาไปซ้าย` });
  steps.push({ title: 'ขั้นตอนที่ 3: ย้อนรอย (Backtrack)', detail: `ไล่ย้อนจาก dp[${bestJ}] ได้รายการที่ควรตัดทั้งหมด ${cutList.length} รายการ ซึ่งให้ผลกระทบรวมต่ำสุด` });

  return { algorithm: '0/1 Knapsack (DP)', cutList, totalSaved, stillOver: Math.max(target - totalSaved, 0), achieved: totalSaved >= target, steps };
}

function runOptimization(expenses, budget, totalExpense, method = 'greedy') {
  const overBudgetAmount = totalExpense - budget;
  if (overBudgetAmount <= 0) {
    return {
      overBudget: false, overBudgetAmount: 0,
      message: 'ยอดรายจ่ายยังอยู่ในงบประมาณ ไม่จำเป็นต้องตัดรายการใด',
      algorithm: method === 'greedy' ? 'Greedy Algorithm' : '0/1 Knapsack (DP)',
      cutList: [], totalSaved: 0, stillOver: 0, achieved: true, steps: []
    };
  }
  const reducible = expenses.filter(e => (e.necessity || 3) >= 3);
  const outcome = method === 'knapsack' ? knapsackOptimize(reducible, overBudgetAmount) : greedyOptimize(reducible, overBudgetAmount);
  return { overBudget: true, overBudgetAmount, ...outcome };
}

module.exports = { runOptimization, greedyOptimize, knapsackOptimize };
