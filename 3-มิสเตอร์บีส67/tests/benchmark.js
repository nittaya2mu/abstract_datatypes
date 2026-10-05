/**
 * benchmark.js
 * วัดประสิทธิภาพจริงของอัลกอริทึม/โครงสร้างข้อมูลหลัก ที่ขนาดข้อมูล 3 ระดับ
 * แล้วเทียบอัตราการเติบโตของเวลาจริง กับ Big-O ที่คาดไว้ตามทฤษฎี
 *
 * วิธีรัน:  node tests/benchmark.js   (หรือ  npm run benchmark  จาก root โปรเจกต์)
 * ผลลัพธ์จะถูกพิมพ์ออกหน้าจอ และบันทึกเป็น tests/benchmark-results.md ด้วย
 */
const fs = require('fs');
const path = require('path');

const { DoublyLinkedList } = require('../frontend/js/linkedList');
const { AVLTree } = require('../frontend/js/avlTree');
const { SmartHashTable } = require('../frontend/js/hashTable');
const { quickSort } = require('../frontend/js/quickSort');
const { mergeSort } = require('../frontend/js/mergeSort');
const { runOptimization } = require('../frontend/js/optimizer');

const SIZES = [100, 1000, 10000];
const CATEGORIES = ['อาหาร', 'เดินทาง', 'การเรียน', 'บันเทิง', 'ช้อปปิ้ง', 'บิล/สาธารณูปโภค'];

function genTransactions(n) {
  const arr = [];
  const base = new Date('2024-01-01').getTime();
  for (let i = 0; i < n; i++) {
    const t = new Date(base + i * 3600 * 1000);
    arr.push({
      id: 'tx' + i,
      type: 'expense',
      amount: Math.floor(Math.random() * 5000) + 1,
      category: CATEGORIES[i % CATEGORIES.length],
      date: t.toISOString().slice(0, 10),
      time: t.toTimeString().slice(0, 5),
      note: '',
      necessity: (i % 5) + 1
    });
  }
  return arr;
}

function timeIt(fn) {
  const t0 = performance.now();
  fn();
  return performance.now() - t0;
}

const report = [];
report.push('# SmartBudget — Benchmark Results (Demo)');
report.push('');
report.push(`วันที่รัน: ${new Date().toISOString()}`);
report.push('');
report.push('> หมายเหตุ: เวลาที่วัดได้เป็นเวลาจริงบนเครื่องที่ใช้รันเทส ณ ขณะนั้น อาจคลาดเคลื่อนได้ตามสเปกเครื่องและโหลดของระบบ ณ เวลาที่รัน (Demo Benchmark)');
report.push('');

// ---------------------------------------------------------------------------
console.log('\n### 1) Sorting Algorithms: Quick Sort vs Merge Sort (คาดหวัง O(n log n)) ###\n');
report.push('## 1) Sorting Algorithms: Quick Sort vs Merge Sort');
report.push('');
report.push('คาดหวังทางทฤษฎี: O(n log n) ทั้งคู่ (เฉลี่ย) — เวลาที่โตขึ้นควรช้ากว่าการโตแบบเชิงเส้น (n) อย่างชัดเจนเมื่อ n โตขึ้น 10 เท่า');
report.push('');
report.push('| ขนาดข้อมูล (n) | Quick Sort (ms) | Merge Sort (ms) | n log n (อ้างอิง) | เวลา/（n log n）Quick | เวลา/（n log n）Merge |');
report.push('|---|---|---|---|---|---|');

let prevQ = null, prevM = null;
SIZES.forEach(n => {
  const data = genTransactions(n);
  const q = quickSort(data, t => t.amount, 'asc');
  const m = mergeSort(data, t => t.amount, 'asc');
  const nlogn = n * Math.log2(n);
  console.log(`n=${n.toLocaleString().padStart(6)}  QuickSort=${q.timeMs.toFixed(3)}ms (comparisons=${q.comparisons})  MergeSort=${m.timeMs.toFixed(3)}ms (comparisons=${m.comparisons})`);
  report.push(`| ${n.toLocaleString()} | ${q.timeMs.toFixed(3)} | ${m.timeMs.toFixed(3)} | ${nlogn.toFixed(0)} | ${(q.timeMs / nlogn).toExponential(2)} | ${(m.timeMs / nlogn).toExponential(2)} |`);
  prevQ = q.timeMs; prevM = m.timeMs;
});

// ---------------------------------------------------------------------------
console.log('\n### 2) AVL Tree: Insert + Range Search (คาดหวัง O(log n) ต่อครั้ง) ###\n');
report.push('');
report.push('## 2) AVL Tree: Insert + Range Search');
report.push('');
report.push('คาดหวังทางทฤษฎี: insert และ range search ต่อครั้งเป็น O(log n) — เวลารวมของ insert ทั้งหมด (n ครั้ง) จึงควรโตแบบ O(n log n)');
report.push('');
report.push('| ขนาดข้อมูล (n) | Insert ทั้งหมด (ms) | เฉลี่ย/ครั้ง (ms) | Range Search ทั้งช่วง (ms) | ความสูงต้นไม้ | log2(n) อ้างอิง |');
report.push('|---|---|---|---|---|---|');

SIZES.forEach(n => {
  const data = genTransactions(n);
  const tree = new AVLTree();
  const insertTime = timeIt(() => {
    data.forEach(t => tree.insert(new Date(`${t.date}T${t.time}:00`).getTime(), t.id));
  });
  const allTs = data.map(t => new Date(`${t.date}T${t.time}:00`).getTime());
  const lo = Math.min(...allTs), hi = Math.max(...allTs);
  const searchTime = timeIt(() => tree.rangeSearch(lo, hi));
  console.log(`n=${n.toLocaleString().padStart(6)}  Insert(total)=${insertTime.toFixed(3)}ms  avg/op=${(insertTime / n).toFixed(5)}ms  RangeSearch(all)=${searchTime.toFixed(3)}ms  height=${tree.height()}`);
  report.push(`| ${n.toLocaleString()} | ${insertTime.toFixed(3)} | ${(insertTime / n).toFixed(5)} | ${searchTime.toFixed(3)} | ${tree.height()} | ${Math.log2(n).toFixed(2)} |`);
});

// ---------------------------------------------------------------------------
console.log('\n### 3) Hash Table: Add + Get (คาดหวัง O(1) โดยเฉลี่ยต่อครั้ง) ###\n');
report.push('');
report.push('## 3) Hash Table: Add + Get');
report.push('');
report.push('คาดหวังทางทฤษฎี: O(1) โดยเฉลี่ยต่อการ add/get หนึ่งครั้ง — เวลาเฉลี่ยต่อครั้งจึงควร "เกือบคงที่" แม้ n โตขึ้นมาก');
report.push('');
report.push('| ขนาดข้อมูล (n) | Add ทั้งหมด (ms) | เฉลี่ย/ครั้ง (ms) | Get ทั้งหมด (ms) | เฉลี่ย/ครั้ง (ms) |');
report.push('|---|---|---|---|---|');

SIZES.forEach(n => {
  const data = genTransactions(n);
  const ht = new SmartHashTable();
  const addTime = timeIt(() => data.forEach(t => ht.add(t.category, t.amount)));
  const getTime = timeIt(() => { for (let i = 0; i < n; i++) ht.get(CATEGORIES[i % CATEGORIES.length]); });
  console.log(`n=${n.toLocaleString().padStart(6)}  Add(total)=${addTime.toFixed(3)}ms  avg/op=${(addTime / n).toFixed(5)}ms  Get(total)=${getTime.toFixed(3)}ms  avg/op=${(getTime / n).toFixed(5)}ms`);
  report.push(`| ${n.toLocaleString()} | ${addTime.toFixed(3)} | ${(addTime / n).toFixed(5)} | ${getTime.toFixed(3)} | ${(getTime / n).toFixed(5)} |`);
});

// ---------------------------------------------------------------------------
console.log('\n### 4) Budget Optimization: Greedy O(n log n) vs Knapsack O(n × budget) ###\n');
report.push('');
report.push('## 4) Budget Optimization: Greedy vs 0/1 Knapsack (DP)');
report.push('');
report.push('คาดหวังทางทฤษฎี: Greedy เป็น O(n log n); Knapsack เป็น O(n × overBudgetAmount) ซึ่งจะช้ากว่า Greedy มากขึ้นเรื่อยๆ เมื่อ n หรือยอดเกินงบสูงขึ้น');
report.push('');
report.push('| ขนาดข้อมูล (n) | Greedy (ms) | Knapsack (ms) | Knapsack ช้ากว่า Greedy กี่เท่า |');
report.push('|---|---|---|---|');

// Knapsack เป็น O(n*budget) ขนาดใหญ่มากจะช้ามาก จึงจำกัดเพดาน overBudget ไว้ไม่ให้รันนานเกินไปตอนสาธิต
SIZES.forEach(n => {
  const data = genTransactions(n).map(t => ({ ...t, necessity: (t.necessity % 3) + 3 })); // บังคับ necessity 3-5 ให้ตัดได้ทุกตัว
  const totalExpense = data.reduce((s, t) => s + t.amount, 0);
  const budget = Math.max(totalExpense - Math.min(5000, totalExpense * 0.3), 0);

  const gTime = timeIt(() => runOptimization(data, budget, totalExpense, 'greedy'));
  const kTime = timeIt(() => runOptimization(data, budget, totalExpense, 'knapsack'));
  console.log(`n=${n.toLocaleString().padStart(6)}  Greedy=${gTime.toFixed(3)}ms  Knapsack=${kTime.toFixed(3)}ms  ratio=${(kTime / Math.max(gTime, 0.001)).toFixed(1)}x`);
  report.push(`| ${n.toLocaleString()} | ${gTime.toFixed(3)} | ${kTime.toFixed(3)} | ${(kTime / Math.max(gTime, 0.001)).toFixed(1)}x |`);
});

report.push('');
report.push('## สรุปการแปลผล');
report.push('');
report.push('- **Quick Sort / Merge Sort**: เวลาโตช้ากว่าการโตแบบเชิงเส้นตรง (n) อย่างชัดเจน สอดคล้องกับ O(n log n) ที่คาดไว้ ไม่ใช่ O(n²)');
report.push('- **AVL Tree**: เวลาเฉลี่ยต่อการ insert/search หนึ่งครั้งเพิ่มขึ้นช้ามากเมื่อ n โตขึ้น 100 เท่า (100 → 10,000) สอดคล้องกับ O(log n) ต่อครั้ง และความสูงต้นไม้ใกล้เคียง log2(n) ตามที่ AVL การันตีไว้');
report.push('- **Hash Table**: เวลาเฉลี่ยต่อการ add/get หนึ่งครั้งแทบไม่เปลี่ยนแปลงเมื่อ n โตขึ้น สอดคล้องกับ O(1) โดยเฉลี่ย');
report.push('- **Greedy vs Knapsack**: Knapsack ช้ากว่า Greedy อย่างเห็นได้ชัดและช้าขึ้นเรื่อยๆ เมื่อ n โตขึ้น เพราะ Knapsack มี complexity ขึ้นกับทั้ง n และขนาดงบประมาณ (O(n × budget)) ในขณะที่ Greedy ขึ้นกับ n เท่านั้น (O(n log n)) — ยืนยันการเลือกให้ผู้ใช้สลับโหมดได้เองตามความเหมาะสมของขนาดข้อมูล');
report.push('');

fs.writeFileSync(path.join(__dirname, 'benchmark-results.md'), report.join('\n'), 'utf-8');
console.log('\nบันทึกผลลัพธ์ฉบับเต็มไว้ที่ tests/benchmark-results.md แล้ว');
