/**
 * wishlistPlanner.js
 * ตรรกะคำนวณแผนการออมเงินเพื่อซื้อของที่อยากได้ (Wishlist)
 * รองรับ 3 รูปแบบ:
 *   1. Equal      - เก็บวันละเท่าๆ กัน โดยกำหนดจำนวนวันที่ต้องการเก็บให้ครบ
 *                   ระบบคำนวณ "เก็บวันละเท่าไหร่" ให้ (remaining / days)
 *   2. Multiplier - เก็บวันละทวีคูณ (เช่น วันแรก 10 บาท คูณ 2 ทุกวัน -> 10, 20, 40, 80, ...)
 *                   ระบบคำนวณ "ใช้เวลากี่วัน" ถึงจะครบเป้าหมาย โดยใช้ผลรวมอนุกรมเรขาคณิต
 *   3. Custom     - ผู้ใช้กำหนดเองว่าจะเก็บวันละเท่าไหร่ (คงที่)
 *                   ระบบคำนวณ "ใช้เวลากี่วัน" และวันที่คาดว่าจะเก็บครบ
 */

const MAX_SCHEDULE_DAYS = 3650; // กันไม่ให้คำนวณวนลูปไม่รู้จบ (สูงสุด ~10 ปี)

// ---------- โหมดที่ 1: Equal (เก็บวันละเท่ากัน) ----------
// input: remaining (ยอดที่ยังต้องเก็บ), days (จำนวนวันที่ต้องการเก็บให้ครบ)
// output: dailyAmount ที่ต้องเก็บต่อวัน (ปัดขึ้นเป็นจำนวนเต็มเพื่อให้เก็บง่าย)
function calcEqualPlan(remaining, days) {
  const d = Math.max(1, Math.floor(days));
  const dailyAmount = Math.ceil(remaining / d);
  return {
    planType: 'equal',
    days: d,
    dailyAmount,
    totalWhenDone: dailyAmount * d,
    explanation: `เก็บวันละ ${dailyAmount.toLocaleString()} บาท เท่ากันทุกวัน เป็นเวลา ${d} วัน จะได้ครบ ${(dailyAmount * d).toLocaleString()} บาท (เป้าหมาย ${remaining.toLocaleString()} บาท)`
  };
}

// ---------- โหมดที่ 2: Multiplier (เก็บวันละทวีคูณ) ----------
// input: remaining, startAmount (เงินวันแรก), multiplier (ตัวคูณ เช่น 2 = เพิ่มเป็น 2 เท่าทุกวัน)
// ใช้สูตรผลรวมอนุกรมเรขาคณิต S(n) = a * (r^n - 1) / (r - 1) เพื่อหาว่าต้องเก็บกี่วัน (n)
// คำนวณแบบวนสะสมทีละวันเพื่อความชัดเจนและรองรับ multiplier ที่ไม่ใช่จำนวนเต็ม
function calcMultiplierPlan(remaining, startAmount, multiplier) {
  const a = Math.max(1, startAmount);
  const r = Math.max(1.01, multiplier); // ต้องมากกว่า 1 ไม่งั้นจะไม่มีวันครบ
  const schedule = [];
  let cumulative = 0;
  let day = 0;
  let amount = a;

  while (cumulative < remaining && day < MAX_SCHEDULE_DAYS) {
    day++;
    cumulative += amount;
    schedule.push({ day, amount: Math.round(amount * 100) / 100, cumulative: Math.round(cumulative * 100) / 100 });
    amount *= r;
  }

  return {
    planType: 'multiplier',
    days: day,
    startAmount: a,
    multiplier: r,
    totalWhenDone: Math.round(cumulative * 100) / 100,
    schedule, // เก็บ schedule เต็มไว้ใช้คำนวณยอดที่แนะนำของแต่ละวันจริง
    explanation: day >= MAX_SCHEDULE_DAYS
      ? `ด้วยอัตราทวีคูณนี้ใช้เวลานานเกินไป (เกิน ${MAX_SCHEDULE_DAYS} วัน) ลองเพิ่มเงินวันแรกหรือตัวคูณให้มากขึ้น`
      : `เริ่มเก็บวันละ ${a.toLocaleString()} บาท แล้วเพิ่มเป็น ${r} เท่าทุกวัน จะครบเป้าหมายภายใน ${day} วัน (วันสุดท้ายเก็บประมาณ ${schedule[day - 1]?.amount.toLocaleString()} บาท)`
  };
}

// ---------- โหมดที่ 3: Custom (กำหนดเองว่าจะเก็บวันละเท่าไหร่) ----------
// input: remaining, dailyAmount (ผู้ใช้กำหนดเอง)
// output: days ที่ต้องใช้ และวันที่คาดว่าจะเก็บครบ
function calcCustomPlan(remaining, dailyAmount, startDate = new Date()) {
  const amt = Math.max(1, dailyAmount);
  const days = Math.ceil(remaining / amt);
  const completionDate = new Date(startDate);
  completionDate.setDate(completionDate.getDate() + days);

  return {
    planType: 'custom',
    days,
    dailyAmount: amt,
    completionDate: completionDate.toISOString().slice(0, 10),
    totalWhenDone: amt * days,
    explanation: `เก็บวันละ ${amt.toLocaleString()} บาท คงที่ จะใช้เวลาประมาณ ${days} วัน คาดว่าจะเก็บครบภายในวันที่ ${completionDate.toLocaleDateString('th-TH', { year: 'numeric', month: 'long', day: 'numeric' })}`
  };
}

// ---------- ยอดที่แนะนำให้เก็บ "วันนี้" ตามแผนที่เลือกไว้ และจำนวนครั้งที่ฝากไปแล้ว ----------
function suggestedAmountForNextDeposit(item, remaining) {
  const dayIndex = (item.depositLog?.length || 0) + 1; // ครั้งที่ฝากถัดไป (1-based)
  const p = item.planParams || {};
  let suggested;

  if (item.planType === 'equal') {
    suggested = p.dailyAmount;
  } else if (item.planType === 'multiplier') {
    suggested = Math.round(p.startAmount * Math.pow(p.multiplier, dayIndex - 1) * 100) / 100;
  } else {
    suggested = p.dailyAmount;
  }

  return Math.max(1, Math.min(suggested || remaining, remaining));
}

if (typeof module !== 'undefined') {
  module.exports = { calcEqualPlan, calcMultiplierPlan, calcCustomPlan, suggestedAmountForNextDeposit };
}
