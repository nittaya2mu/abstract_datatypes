/**
 * optimizer.js
 * Budget Optimization Engine
 *
 * โจทย์: เมื่อรายจ่ายรวมเกินงบประมาณที่ตั้งไว้ ระบบต้องแนะนำว่าควร "ลด/ตัด" รายการไหน
 * เพื่อประหยัดเงินให้ได้มากที่สุด โดยกระทบชีวิตประจำวันน้อยที่สุด
 *
 * แนวคิด: รายการที่ "necessity" สูง (4-5) คือรายการที่ไม่จำเป็น/อยากได้ ตัดได้ง่าย กระทบน้อย
 *          รายการที่ "necessity" ต่ำ (1-2) คือรายการจำเป็นต้องจ่าย ไม่ควรตัด
 *
 * มี 2 อัลกอริทึมให้เลือก:
 *  1. Greedy Algorithm  - เรียงรายการตาม necessity มากไปน้อย (ตัดง่ายสุดก่อน) แล้วไล่ตัด
 *                         จนกว่าจะประหยัดพอกับยอดที่เกินงบ เร็ว O(n log n) แต่ไม่รับประกันผลลัพธ์ดีที่สุดเสมอไป
 *  2. 0/1 Knapsack (DP) - หาชุดรายการที่ "ตัด" แล้วได้ผลรวมเงินประหยัด >= เป้าหมาย
 *                         โดยมี "ต้นทุนผลกระทบ" (necessity) รวมน้อยที่สุด รับประกันผลลัพธ์ดีที่สุด
 *                         แต่ใช้เวลา O(n * budget) จึงเหมาะกับข้อมูลไม่เยอะมาก
 */

// ---------- Greedy Algorithm ----------
function greedyOptimize(expenses, overBudgetAmount) {
  const steps = [];
  let remaining = overBudgetAmount;
  const cutList = [];

  // เรียงตาม necessity มาก -> น้อย, ถ้า necessity เท่ากันให้เรียงจำนวนเงินมาก -> น้อย
  // (ตัดรายการที่ "อยากได้มากที่สุด/จำเป็นน้อยที่สุด" และ "มูลค่าสูงที่สุด" ก่อน เพื่อประหยัดได้เร็วที่สุด)
  const sorted = [...expenses].sort((a, b) => {
    if (b.necessity !== a.necessity) return b.necessity - a.necessity;
    return b.amount - a.amount;
  });

  steps.push({
    title: 'ขั้นตอนที่ 1: เรียงลำดับรายจ่าย',
    detail: `เรียงรายการทั้งหมด ${expenses.length} รายการ ตามค่าความจำเป็นจากน้อยไปมาก (necessity สูง = ตัดง่าย มาก่อน) และถ้าความจำเป็นเท่ากัน ให้เรียงตามจำนวนเงินจากมากไปน้อย`
  });

  for (const item of sorted) {
    if (remaining <= 0) break;
    cutList.push(item);
    const before = remaining;
    remaining -= item.amount;
    steps.push({
      title: `พิจารณา: ${item.category} (${item.amount.toLocaleString()} บาท, necessity ${item.necessity})`,
      detail: `ยอดที่ยังต้องลดอีก ${before.toLocaleString()} บาท → เลือกตัดรายการนี้เพราะมีค่าความจำเป็นสูง (${item.necessity}/5) แปลว่ากระทบชีวิตประจำวันน้อยที่สุดในบรรดารายการที่เหลือ → ยอดที่ต้องลดคงเหลือ ${Math.max(remaining, 0).toLocaleString()} บาท`
    });
  }

  const totalSaved = cutList.reduce((s, i) => s + i.amount, 0);

  return {
    algorithm: 'Greedy Algorithm',
    cutList,
    totalSaved,
    stillOver: Math.max(remaining, 0),
    achieved: remaining <= 0,
    steps
  };
}

// ---------- 0/1 Knapsack (DP) ----------
// เป้าหมาย: เลือกชุดรายการที่ "ตัด" แล้วผลรวมเงิน >= target โดยมีผลรวม necessity (impact cost) น้อยที่สุด
function knapsackOptimize(expenses, overBudgetAmount) {
  const steps = [];
  const toCents = (value) => Math.max(0, Math.round(Number(value) * 100));
  const targetCents = Math.ceil(Number(overBudgetAmount) * 100 - 1e-9);
  const n = expenses.length;

  if (targetCents <= 0 || n === 0) {
    return { algorithm: '0/1 Knapsack (DP)', cutList: [], totalSaved: 0, stillOver: 0, achieved: true,
      steps: [{ title: 'ไม่มีรายจ่ายเกินงบ', detail: 'ไม่จำเป็นต้องตัดรายการใด' }] };
  }

  // Work in integer satang/cents so decimal amounts such as 100.50 are valid DP indices.
  const amounts = expenses.map(e => toCents(e.amount));
  const maxSave = amounts.reduce((s, a) => s + a, 0);
  const cap = Math.min(targetCents, maxSave);
  const dp = new Array(cap + 1).fill(Infinity);
  dp[0] = 0;
  const choice = Array.from({ length: n }, () => new Array(cap + 1).fill(false));

  for (let i = 0; i < n; i++) {
    const amount = Math.min(amounts[i], cap);
    const impact = 6 - Number(expenses[i].necessity);
    if (amount <= 0) continue;
    for (let j = cap; j >= 0; j--) {
      const prevJ = Math.max(j - amount, 0);
      if (dp[prevJ] !== Infinity && dp[prevJ] + impact < dp[j]) {
        dp[j] = dp[prevJ] + impact;
        choice[i][j] = true;
      }
    }
  }

  let bestJ = cap;
  while (bestJ > 0 && dp[bestJ] === Infinity) bestJ--;

  const cutList = [];
  let j = bestJ;
  for (let i = n - 1; i >= 0; i--) {
    if (choice[i][j]) {
      cutList.push(expenses[i]);
      j = Math.max(j - Math.min(amounts[i], cap), 0);
    }
  }

  const totalSaved = cutList.reduce((s, i) => s + Number(i.amount), 0);
  const roundedSaved = Math.round((totalSaved + Number.EPSILON) * 100) / 100;
  const targetBaht = targetCents / 100;

  steps.push({ title: 'ขั้นตอนที่ 1: สร้างตาราง Dynamic Programming',
    detail: `สร้างตาราง dp ขนาด ${cap + 1} ช่อง (หน่วยสตางค์) แทนยอดเงินที่ต้องประหยัด โดย dp[j] = ผลรวมผลกระทบ (6 - necessity) ต่ำสุด` });
  steps.push({ title: 'ขั้นตอนที่ 2: พิจารณาแต่ละรายการ (0/1 - ตัดหรือไม่ตัด)',
    detail: `วนพิจารณารายจ่าย ${n} รายการทีละตัว และอัปเดตจากขวาไปซ้ายเพื่อไม่ให้ใช้รายการเดิมซ้ำ` });
  steps.push({ title: 'ขั้นตอนที่ 3: ย้อนรอย (Backtrack) หาคำตอบ',
    detail: `ย้อนจากช่อง dp[${bestJ}] ได้ ${cutList.length} รายการ รวมประหยัด ${roundedSaved.toLocaleString()} บาท จากเป้าหมาย ${targetBaht.toLocaleString()} บาท` });

  return {
    algorithm: '0/1 Knapsack (DP)', cutList, totalSaved: roundedSaved,
    stillOver: Math.max(Math.round((targetBaht - roundedSaved) * 100) / 100, 0),
    achieved: roundedSaved + 1e-9 >= targetBaht, steps
  };
}

function runOptimization(expenses, budget, totalExpense, method = 'greedy') {
  const overBudgetAmount = totalExpense - budget;
  if (overBudgetAmount <= 0) {
    return {
      overBudget: false,
      overBudgetAmount: 0,
      message: 'ยอดรายจ่ายยังอยู่ในงบประมาณ ไม่จำเป็นต้องตัดรายการใด',
      algorithm: method === 'greedy' ? 'Greedy Algorithm' : '0/1 Knapsack (DP)',
      cutList: [], totalSaved: 0, stillOver: 0, achieved: true, steps: []
    };
  }

  // เฉพาะรายจ่ายที่ necessity >= 3 เท่านั้นที่พิจารณาให้ตัด (necessity 1-2 = จำเป็นมาก ไม่แนะนำให้ตัด)
  const reducible = expenses.filter(e => (e.necessity || 3) >= 3);

  const outcome = method === 'knapsack'
    ? knapsackOptimize(reducible, overBudgetAmount)
    : greedyOptimize(reducible, overBudgetAmount);

  return { overBudget: true, overBudgetAmount, ...outcome };
}

if (typeof module !== 'undefined') module.exports = { runOptimization, greedyOptimize, knapsackOptimize };
