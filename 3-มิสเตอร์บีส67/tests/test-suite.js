/**
 * test-suite.js
 * ชุดทดสอบ (Unit Test) ครอบคลุมโครงสร้างข้อมูลและอัลกอริทึมทั้งหมดของ SmartBudget
 * รวมกรณีปกติและกรณีขอบ (edge cases) ตามเกณฑ์ "ความถูกต้องของโปรแกรม"
 *
 * วิธีรัน:  node tests/test-suite.js   (หรือ  npm test  จาก root โปรเจกต์)
 */
const { group, test, assertEqual, assertTrue, assertDeepEqual, summary } = require('./testFramework');

const { DoublyLinkedList } = require('../frontend/js/linkedList');
const { AVLTree } = require('../frontend/js/avlTree');
const { SmartHashTable } = require('../frontend/js/hashTable');
const { quickSort } = require('../frontend/js/quickSort');
const { mergeSort } = require('../frontend/js/mergeSort');
const { runOptimization, greedyOptimize, knapsackOptimize } = require('../frontend/js/optimizer');
const { calcEqualPlan, calcMultiplierPlan, calcCustomPlan, suggestedAmountForNextDeposit } = require('../frontend/js/wishlistPlanner');

const sampleTx = (over = {}) => ({
  type: 'expense', amount: 100, category: 'อาหาร', date: '2026-01-01', time: '08:00', note: '', necessity: 3, ...over
});

// ---------------------------------------------------------------------------
group('Doubly Linked List — กรณีปกติ');
// ---------------------------------------------------------------------------
test('append เพิ่มขนาดและตั้ง head/tail ถูกต้อง', () => {
  const dll = new DoublyLinkedList();
  dll.append({ id: 'a', ...sampleTx() });
  dll.append({ id: 'b', ...sampleTx() });
  assertEqual(dll.size, 2);
  assertEqual(dll.head.id, 'a');
  assertEqual(dll.tail.id, 'b');
});
test('find คืนค่า node ที่ถูกต้องด้วย id', () => {
  const dll = new DoublyLinkedList();
  dll.append({ id: 'a', ...sampleTx() });
  assertEqual(dll.find('a').id, 'a');
});
test('toArray(true) เรียงจาก head -> tail, toArray(false) เรียงจาก tail -> head', () => {
  const dll = new DoublyLinkedList();
  ['a', 'b', 'c'].forEach(id => dll.append({ id, ...sampleTx() }));
  assertDeepEqual(dll.toArray(true).map(n => n.id), ['a', 'b', 'c']);
  assertDeepEqual(dll.toArray(false).map(n => n.id), ['c', 'b', 'a']);
});

// ---------------------------------------------------------------------------
group('Doubly Linked List — กรณีขอบ (edge cases)');
// ---------------------------------------------------------------------------
test('list ว่าง: head/tail เป็น null, size เป็น 0, toArray คืน []', () => {
  const dll = new DoublyLinkedList();
  assertEqual(dll.head, null);
  assertEqual(dll.tail, null);
  assertEqual(dll.size, 0);
  assertDeepEqual(dll.toArray(true), []);
});
test('find id ที่ไม่มีอยู่จริง คืนค่า null', () => {
  const dll = new DoublyLinkedList();
  dll.append({ id: 'a', ...sampleTx() });
  assertEqual(dll.find('ghost'), null);
});
test('remove id ที่ไม่มีอยู่จริง คืนค่า false และไม่กระทบ list', () => {
  const dll = new DoublyLinkedList();
  dll.append({ id: 'a', ...sampleTx() });
  assertEqual(dll.remove('ghost'), false);
  assertEqual(dll.size, 1);
});
test('remove node เดียวใน list: head และ tail ต้องกลายเป็น null', () => {
  const dll = new DoublyLinkedList();
  dll.append({ id: 'a', ...sampleTx() });
  dll.remove('a');
  assertEqual(dll.head, null);
  assertEqual(dll.tail, null);
  assertEqual(dll.size, 0);
});
test('remove head: head ตัวใหม่ต้องถูกต้อง และ prev ของ node ใหม่เป็น null', () => {
  const dll = new DoublyLinkedList();
  ['a', 'b', 'c'].forEach(id => dll.append({ id, ...sampleTx() }));
  dll.remove('a');
  assertEqual(dll.head.id, 'b');
  assertEqual(dll.head.prev, null);
  assertEqual(dll.size, 2);
});
test('remove tail: tail ตัวใหม่ต้องถูกต้อง และ next ของ node ใหม่เป็น null', () => {
  const dll = new DoublyLinkedList();
  ['a', 'b', 'c'].forEach(id => dll.append({ id, ...sampleTx() }));
  dll.remove('c');
  assertEqual(dll.tail.id, 'b');
  assertEqual(dll.tail.next, null);
});
test('remove node ตรงกลาง: prev/next ของเพื่อนบ้านต้องเชื่อมกันใหม่', () => {
  const dll = new DoublyLinkedList();
  ['a', 'b', 'c'].forEach(id => dll.append({ id, ...sampleTx() }));
  dll.remove('b');
  assertEqual(dll.head.next.id, 'c');
  assertEqual(dll.tail.prev.id, 'a');
  assertDeepEqual(dll.toArray(true).map(n => n.id), ['a', 'c']);
});
test('update แก้ไขข้อมูลโดยตำแหน่งใน list ไม่เปลี่ยน', () => {
  const dll = new DoublyLinkedList();
  ['a', 'b'].forEach(id => dll.append({ id, ...sampleTx() }));
  dll.update('a', sampleTx({ amount: 999, category: 'เดินทาง' }));
  assertEqual(dll.find('a').amount, 999);
  assertEqual(dll.toArray(true)[0].id, 'a'); // ตำแหน่งยังเหมือนเดิม
});

// ---------------------------------------------------------------------------
group('AVL Tree — กรณีปกติและกรณีขอบ');
// ---------------------------------------------------------------------------
test('insert แล้วต้นไม้สมดุล (height <= 1.45 * log2(n+2))', () => {
  const tree = new AVLTree();
  for (let i = 0; i < 1000; i++) tree.insert(i, 'id' + i); // insert แบบเรียงลำดับ (worst case ของ BST ปกติ)
  const maxAllowedHeight = Math.ceil(1.45 * Math.log2(1002));
  assertTrue(tree.height() <= maxAllowedHeight, `height=${tree.height()} เกิน ${maxAllowedHeight} แปลว่าต้นไม้ไม่สมดุล`);
});
test('rangeSearch บนต้นไม้ว่าง คืนค่า [] โดยไม่ error', () => {
  const tree = new AVLTree();
  assertDeepEqual(tree.rangeSearch(0, 100), []);
});
test('rangeSearch ครอบคลุมทุกค่าต้องได้ทุก id กลับมา', () => {
  const tree = new AVLTree();
  for (let i = 0; i < 20; i++) tree.insert(i, 'id' + i);
  assertEqual(tree.rangeSearch(0, 19).length, 20);
});
test('insert key ซ้ำ: เก็บหลาย id ไว้ใน node เดียวกัน', () => {
  const tree = new AVLTree();
  tree.insert(100, 'a');
  tree.insert(100, 'b');
  const found = tree.rangeSearch(100, 100);
  assertDeepEqual(found.sort(), ['a', 'b']);
});
test('remove leaf node ไม่กระทบโครงสร้างส่วนอื่น', () => {
  const tree = new AVLTree();
  [50, 30, 70].forEach((k, i) => tree.insert(k, 'id' + i));
  tree.remove(30, 'id1');
  assertEqual(tree.rangeSearch(30, 30).length, 0);
  assertEqual(tree.rangeSearch(50, 50).length, 1);
});
test('remove node ที่มีลูก 2 ฝั่ง ยังคง rangeSearch ได้ถูกต้องครบ', () => {
  const tree = new AVLTree();
  const keys = [50, 30, 70, 20, 40, 60, 80];
  keys.forEach((k, i) => tree.insert(k, 'id' + i));
  tree.remove(50, 'id0'); // root มีลูกสองฝั่ง
  const remaining = tree.rangeSearch(0, 100);
  assertEqual(remaining.length, keys.length - 1);
});
test('remove AVL node ที่มีลูก 2 ฝั่งและ successor มีหลาย id ต้องไม่ทำให้ id ซ้ำ', () => {
  const tree = new AVLTree();
  tree.insert(50, 'root');
  tree.insert(30, 'left');
  tree.insert(70, 'right');
  tree.insert(60, 's1');
  tree.insert(60, 's2');
  tree.insert(80, 'r2');
  tree.remove(50, 'root');
  const all = tree.rangeSearch(0, 100).sort();
  assertDeepEqual(all, ['left', 'r2', 'right', 's1', 's2'].sort());
});
test('remove id/key ที่ไม่มีอยู่จริง ไม่ error และไม่ลบอะไรทิ้ง', () => {
  const tree = new AVLTree();
  tree.insert(10, 'a');
  tree.remove(999, 'ghost');
  assertEqual(tree.rangeSearch(0, 100).length, 1);
});

// ---------------------------------------------------------------------------
group('Hash Table — กรณีปกติและกรณีขอบ');
// ---------------------------------------------------------------------------
test('add สะสมยอดเงินในหมวดหมู่เดียวกันถูกต้อง', () => {
  const ht = new SmartHashTable();
  ht.add('อาหาร', 100);
  ht.add('อาหาร', 50);
  assertEqual(ht.get('อาหาร'), 150);
});
test('get หมวดหมู่ที่ไม่มีอยู่จริง คืนค่า 0 ไม่ error', () => {
  const ht = new SmartHashTable();
  assertEqual(ht.get('ไม่มีอยู่จริง'), 0);
});
test('has ตรวจสอบการมีอยู่ของ key ได้ถูกต้อง', () => {
  const ht = new SmartHashTable();
  ht.add('เดินทาง', 10);
  assertTrue(ht.has('เดินทาง'));
  assertTrue(!ht.has('ไม่มี'));
});
test('remove บนหมวดหมู่ที่ไม่มีอยู่จริง คืนค่า false', () => {
  const ht = new SmartHashTable();
  assertEqual(ht.remove('ไม่มี', 10), false);
});
test('entries() เรียงจากยอดมากไปน้อยถูกต้อง', () => {
  const ht = new SmartHashTable();
  ht.add('A', 10);
  ht.add('B', 100);
  ht.add('C', 50);
  assertDeepEqual(ht.entries().map(e => e.category), ['B', 'C', 'A']);
});
test('หลายหมวดหมู่ที่อาจชนกันใน bucket เดียวกันยังแยกยอดกันถูกต้อง (separate chaining)', () => {
  const ht = new SmartHashTable(2); // บังคับ bucket น้อยเพื่อให้ชนกันแน่นอน
  ht.add('อาหาร', 100);
  ht.add('เดินทาง', 200);
  ht.add('การเรียน', 300);
  assertEqual(ht.get('อาหาร'), 100);
  assertEqual(ht.get('เดินทาง'), 200);
  assertEqual(ht.get('การเรียน'), 300);
});
test('remove Hash Table เมื่อยอดหมด ต้องลบ entry ออกจาก table', () => {
  const ht = new SmartHashTable();
  ht.add('อาหาร', 100);
  assertTrue(ht.remove('อาหาร', 100));
  assertTrue(!ht.has('อาหาร'));
  assertEqual(ht.get('อาหาร'), 0);
  assertEqual(ht.size, 0);
  assertDeepEqual(ht.entries(), []);
});

// ---------------------------------------------------------------------------
group('Sorting Algorithms — กรณีปกติและกรณีขอบ');
// ---------------------------------------------------------------------------
test('quickSort และ mergeSort เรียง array ว่างได้โดยไม่ error', () => {
  assertDeepEqual(quickSort([], x => x, 'asc').result, []);
  assertDeepEqual(mergeSort([], x => x, 'asc').result, []);
});
test('quickSort และ mergeSort เรียง array ที่มีตัวเดียวได้ถูกต้อง', () => {
  assertDeepEqual(quickSort([5], x => x, 'asc').result, [5]);
  assertDeepEqual(mergeSort([5], x => x, 'asc').result, [5]);
});
test('เรียง array ที่เรียงอยู่แล้ว (already sorted)', () => {
  const data = [1, 2, 3, 4, 5];
  assertDeepEqual(quickSort(data, x => x, 'asc').result, [1, 2, 3, 4, 5]);
  assertDeepEqual(mergeSort(data, x => x, 'asc').result, [1, 2, 3, 4, 5]);
});
test('เรียง array ที่เรียงกลับด้าน (reverse sorted)', () => {
  const data = [5, 4, 3, 2, 1];
  assertDeepEqual(quickSort(data, x => x, 'asc').result, [1, 2, 3, 4, 5]);
  assertDeepEqual(mergeSort(data, x => x, 'asc').result, [1, 2, 3, 4, 5]);
});
test('เรียงแบบ desc ได้ถูกต้อง', () => {
  const data = [1, 3, 2];
  assertDeepEqual(quickSort(data, x => x, 'desc').result, [3, 2, 1]);
  assertDeepEqual(mergeSort(data, x => x, 'desc').result, [3, 2, 1]);
});
test('mergeSort เป็น stable sort: รายการ key ซ้ำกันต้องคงลำดับเดิม', () => {
  const data = [
    { k: 1, tag: 'A' }, { k: 1, tag: 'B' }, { k: 1, tag: 'C' }
  ];
  const res = mergeSort(data, x => x.k, 'asc').result;
  assertDeepEqual(res.map(x => x.tag), ['A', 'B', 'C']); // ลำดับเดิมต้องไม่สลับ
});
test('quickSort และ mergeSort ให้ผลลัพธ์ตรงกันบนข้อมูลสุ่มขนาดใหญ่ (cross-check ความถูกต้อง)', () => {
  const data = Array.from({ length: 500 }, () => Math.floor(Math.random() * 10000));
  const q = quickSort(data, x => x, 'asc').result;
  const m = mergeSort(data, x => x, 'asc').result;
  assertDeepEqual(q, m);
  // ตรวจว่าเรียงจริง (ไม่มีคู่ที่สลับลำดับ)
  for (let i = 1; i < q.length; i++) assertTrue(q[i] >= q[i - 1], 'ผลลัพธ์ไม่ได้เรียงจริง');
});

// ---------------------------------------------------------------------------
group('Budget Optimization (Greedy + Knapsack) — กรณีปกติและกรณีขอบ');
// ---------------------------------------------------------------------------
test('งบไม่เกิน: คืนค่า overBudget=false และไม่มีรายการให้ตัด', () => {
  const expenses = [sampleTx({ amount: 1000, necessity: 5 })];
  const res = runOptimization(expenses, 5000, 1000, 'greedy');
  assertEqual(res.overBudget, false);
  assertDeepEqual(res.cutList, []);
});
test('งบ = 0 และมีรายจ่าย: ต้องเกินงบทันทีเท่ากับยอดรายจ่ายทั้งหมด', () => {
  const expenses = [sampleTx({ amount: 500, necessity: 5 })];
  const res = runOptimization(expenses, 0, 500, 'greedy');
  assertEqual(res.overBudget, true);
  assertEqual(res.overBudgetAmount, 500);
});
test('ไม่มีรายการที่ necessity >= 3 ให้ตัด: cutList ว่าง และ stillOver เท่ากับยอดที่เกิน', () => {
  const expenses = [sampleTx({ amount: 1000, necessity: 1 }), sampleTx({ amount: 500, necessity: 2 })];
  const res = runOptimization(expenses, 1000, 1500, 'greedy');
  assertDeepEqual(res.cutList, []);
  assertEqual(res.stillOver, 500);
  assertEqual(res.achieved, false);
});
test('Greedy: necessity เท่ากันต้องเรียงตามจำนวนเงินมาก -> น้อยก่อน', () => {
  const expenses = [
    sampleTx({ amount: 100, necessity: 5, category: 'เล็ก' }),
    sampleTx({ amount: 500, necessity: 5, category: 'ใหญ่' })
  ];
  const res = greedyOptimize(expenses, 300);
  assertEqual(res.cutList[0].category, 'ใหญ่'); // ตัดตัวที่มูลค่าสูงกว่าก่อนเพื่อถึงเป้าหมายเร็วสุด
});
test('Greedy: ตัดครบพอดีจนถึงงบ ต้อง achieved=true และ stillOver=0', () => {
  const expenses = [sampleTx({ amount: 300, necessity: 5 }), sampleTx({ amount: 200, necessity: 1 })];
  const res = greedyOptimize(expenses.filter(e => e.necessity >= 3), 300);
  assertEqual(res.achieved, true);
  assertEqual(res.stillOver, 0);
});
test('Knapsack บน overBudgetAmount <= 0 คืนค่า cutList ว่างทันที', () => {
  const res = knapsackOptimize([sampleTx({ amount: 100, necessity: 5 })], 0);
  assertDeepEqual(res.cutList, []);
  assertEqual(res.achieved, true);
});
test('Knapsack: totalSaved ต้อง >= overBudgetAmount เมื่อทำได้ (ตัดได้พอ)', () => {
  const expenses = [
    sampleTx({ amount: 400, necessity: 5 }), sampleTx({ amount: 300, necessity: 4 }), sampleTx({ amount: 250, necessity: 3 })
  ];
  const res = knapsackOptimize(expenses, 500);
  assertTrue(res.totalSaved >= 500, `totalSaved=${res.totalSaved} ควร >= 500`);
});
test('Knapsack ให้ผลประหยัดดีกว่าหรือเท่ากับ Greedy เสมอ (ความเหมาะสมที่สุดของ DP)', () => {
  // สถานการณ์ที่ Greedy ไม่ใช่คำตอบที่ดีที่สุด: necessity สูงสุดมีมูลค่าน้อย ทำให้ Greedy ตัดเกินความจำเป็น
  const expenses = [
    sampleTx({ amount: 600, necessity: 5, category: 'A' }),
    sampleTx({ amount: 350, necessity: 4, category: 'B' }),
    sampleTx({ amount: 350, necessity: 4, category: 'C' })
  ];
  const target = 650;
  const g = greedyOptimize(expenses, target);
  const k = knapsackOptimize(expenses, target);
  assertTrue(k.totalSaved >= target, 'Knapsack ควรประหยัดได้ถึงเป้าหมายเสมอถ้าเป็นไปได้');
  // impact รวม (6-necessity) ของ Knapsack ต้องไม่แย่กว่า Greedy
  const impact = (list) => list.reduce((s, i) => s + (6 - i.necessity), 0);
  assertTrue(impact(k.cutList) <= impact(g.cutList), 'Knapsack ควรให้ผลกระทบรวมต่ำกว่าหรือเท่ากับ Greedy เสมอ');
});

// ---------------------------------------------------------------------------
group('Wishlist Savings Planner — กรณีปกติและกรณีขอบ');
// ---------------------------------------------------------------------------
test('Equal plan: จำนวนเงินต่อวัน x จำนวนวัน ต้อง >= ราคาสินค้า', () => {
  const plan = calcEqualPlan(1000, 7);
  assertTrue(plan.dailyAmount * plan.days >= 1000);
});
test('Equal plan: days เป็น 0 หรือติดลบต้องถูกบังคับเป็นอย่างน้อย 1 วัน (กันหารด้วยศูนย์)', () => {
  const plan = calcEqualPlan(1000, 0);
  assertEqual(plan.days, 1);
  assertEqual(plan.dailyAmount, 1000);
});
test('Multiplier plan: ผลรวมสะสมวันสุดท้ายต้อง >= ราคาสินค้า', () => {
  const plan = calcMultiplierPlan(1000, 10, 2);
  assertTrue(plan.totalWhenDone >= 1000);
  assertTrue(plan.schedule[plan.schedule.length - 1].cumulative >= 1000);
});
test('Multiplier plan: ราคาต่ำกว่าเงินวันแรก ต้องใช้แค่ 1 วัน', () => {
  const plan = calcMultiplierPlan(5, 10, 2);
  assertEqual(plan.days, 1);
});
test('Custom plan: จำนวนวันคำนวณถูกต้อง (ceil ของ remaining/dailyAmount)', () => {
  const plan = calcCustomPlan(1000, 300);
  assertEqual(plan.days, 4); // 300*3=900 ไม่พอ ต้องวันที่ 4
});
test('suggestedAmountForNextDeposit ไม่แนะนำเกินยอดที่เหลือ (ป้องกันฝากเกินเป้าหมาย)', () => {
  const item = { planType: 'multiplier', planParams: { startAmount: 100, multiplier: 3 }, depositLog: [] };
  const suggested = suggestedAmountForNextDeposit(item, 50); // เหลือแค่ 50 แต่สูตรจะแนะนำ 100
  assertEqual(suggested, 50);
});

// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
group('Regression Tests — บั๊กที่เคยหลุดจากชุดทดสอบเดิม');
// ---------------------------------------------------------------------------
test('Merge Sort รองรับ key แบบ string/category', () => {
  const data = [{ c: 'z' }, { c: 'a' }, { c: 'm' }];
  const r = mergeSort(data, x => x.c, 'asc').result.map(x => x.c);
  assertDeepEqual(r, ['a', 'm', 'z']);
});
test('Quick Sort ไม่ stack overflow กับข้อมูลเรียง 20,000 รายการ', () => {
  const data = Array.from({ length: 20000 }, (_, i) => ({ v: i }));
  const r = quickSort(data, x => x.v, 'asc').result;
  assertEqual(r[0].v, 0);
  assertEqual(r[r.length - 1].v, 19999);
});
test('Knapsack รองรับจำนวนเงินทศนิยม', () => {
  const r = knapsackOptimize([
    { id: 'a', amount: 100.50, necessity: 5 },
    { id: 'b', amount: 50.25, necessity: 4 }
  ], 100.50);
  assertEqual(r.achieved, true);
  assertEqual(r.cutList.length, 1);
  assertEqual(Number(r.totalSaved.toFixed(2)), 100.50);
});
test('Timestamp รองรับ HH:MM และ HH:MM:SS', () => {
  const a = new DoublyLinkedList();
  a.append({ id: 'minute', ...sampleTx({ date: '2026-10-04', time: '12:34' }) });
  a.append({ id: 'second', ...sampleTx({ date: '2026-10-04', time: '12:34:56' }) });
  assertTrue(Number.isFinite(a.find('minute').timestamp));
  assertTrue(Number.isFinite(a.find('second').timestamp));
  assertTrue(a.find('second').timestamp > a.find('minute').timestamp);
});
test('AVL rangeSearch ช่วงกลับด้านคืน []', () => {
  const tree = new AVLTree();
  tree.insert(10, 'a'); tree.insert(20, 'b');
  assertDeepEqual(tree.rangeSearch(20, 10), []);
});

const result = summary();
process.exit(result.fail > 0 ? 1 : 0);
