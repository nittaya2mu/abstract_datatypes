/**
 * testFramework.js
 * เฟรมเวิร์คทดสอบขนาดเล็ก ไม่พึ่งพา library ภายนอก ใช้รันผ่าน Node โดยตรง
 * รองรับ: test(name, fn), assertEqual, assertTrue, assertDeepEqual
 * เมื่อรันจบจะสรุปผล PASS/FAIL ทั้งหมด และคืนค่าผลลัพธ์เป็น array เพื่อนำไปเขียนเป็นรายงานต่อได้
 */

const results = [];
let currentGroup = '';

function group(name) {
  currentGroup = name;
  console.log(`\n=== ${name} ===`);
}

function test(name, fn) {
  const fullName = currentGroup ? `${currentGroup} > ${name}` : name;
  try {
    fn();
    results.push({ name: fullName, pass: true });
    console.log(`  ✓ ${name}`);
  } catch (err) {
    results.push({ name: fullName, pass: false, error: err.message });
    console.log(`  ✗ ${name}  →  ${err.message}`);
  }
}

function assertEqual(actual, expected, msg = '') {
  if (actual !== expected) {
    throw new Error(`${msg} expected ${JSON.stringify(expected)} but got ${JSON.stringify(actual)}`);
  }
}

function assertTrue(cond, msg = 'expected condition to be true') {
  if (!cond) throw new Error(msg);
}

function assertDeepEqual(actual, expected, msg = '') {
  const a = JSON.stringify(actual);
  const b = JSON.stringify(expected);
  if (a !== b) throw new Error(`${msg} expected ${b} but got ${a}`);
}

function summary() {
  const pass = results.filter(r => r.pass).length;
  const fail = results.filter(r => !r.pass).length;
  console.log(`\n${'='.repeat(50)}`);
  console.log(`ผลรวม: ผ่าน ${pass} / ล้มเหลว ${fail} / ทั้งหมด ${results.length}`);
  console.log('='.repeat(50));
  return { pass, fail, total: results.length, results };
}

module.exports = { group, test, assertEqual, assertTrue, assertDeepEqual, summary };
