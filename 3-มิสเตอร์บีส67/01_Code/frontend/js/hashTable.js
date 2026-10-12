/**
 * hashTable.js
 * Hash Table แบบ Separate Chaining
 * ใช้ Map หมวดหมู่ (category) -> ยอดรวมเงิน (amount) เพื่อให้การรวมยอดแต่ละหมวดหมู่
 * ทำได้ใน O(1) โดยเฉลี่ย แทนที่จะต้องวนลูปสรุปยอดทุกครั้ง O(n)
 */

class SmartHashTable {
  constructor(bucketCount = 32) {
    this.bucketCount = bucketCount;
    this.buckets = Array.from({ length: bucketCount }, () => []);
    this.size = 0;
  }

  // Hash function: DJB2-like string hashing
  _hash(key) {
    let hash = 5381;
    const str = String(key);
    for (let i = 0; i < str.length; i++) {
      hash = ((hash << 5) + hash) + str.charCodeAt(i);
      hash = hash & 0xFFFFFFFF;
    }
    return Math.abs(hash) % this.bucketCount;
  }

  _findEntry(key) {
    const idx = this._hash(key);
    const bucket = this.buckets[idx];
    for (const entry of bucket) {
      if (entry.key === key) return { idx, entry };
    }
    return { idx, entry: null };
  }

  // เพิ่มยอดเงินเข้าไปในหมวดหมู่ (ถ้ายังไม่มี key จะสร้างใหม่)
  add(key, amount) {
    const { idx, entry } = this._findEntry(key);
    if (entry) {
      entry.value += amount;
      entry.count += 1;
    } else {
      this.buckets[idx].push({ key, value: amount, count: 1 });
      this.size++;
    }
  }

  get(key) {
    const { entry } = this._findEntry(key);
    return entry ? entry.value : 0;
  }

  has(key) {
    return this._findEntry(key).entry !== null;
  }

  remove(key, amount) {
    const { idx, entry } = this._findEntry(key);
    if (!entry) return false;

    const value = Number(amount);
    if (!Number.isFinite(value) || value < 0) return false;

    entry.value -= value;
    entry.count -= 1;

    // เมื่อไม่มี transaction เหลือแล้ว ต้องลบ entry ออกจาก bucket
    // เพื่อให้ has()/size()/entries() สะท้อนข้อมูลจริง
    if (entry.count <= 0 || entry.value <= 0) {
      const bucket = this.buckets[idx];
      const pos = bucket.indexOf(entry);
      if (pos !== -1) bucket.splice(pos, 1);
      this.size--;
    }
    return true;
  }

  clear() {
    this.buckets = Array.from({ length: this.bucketCount }, () => []);
    this.size = 0;
  }

  // คืนค่าทุกคู่ key-value ที่มีอยู่ (เรียงจากมากไปน้อยตามยอดเงิน)
  entries() {
    const out = [];
    this.buckets.forEach(bucket => bucket.forEach(e => out.push({ category: e.key, total: e.value, count: e.count })));
    return out.sort((a, b) => b.total - a.total);
  }

  // ใช้สำหรับหน้า Data Structure & Algorithm: แสดง bucket distribution
  getBucketSnapshot() {
    return this.buckets.map((bucket, i) => ({
      index: i,
      items: bucket.map(e => `${e.key} (${e.value.toLocaleString()} บาท)`)
    })).filter(b => b.items.length > 0);
  }

  static buildFromTransactions(transactions) {
    const table = new SmartHashTable();
    transactions.filter(t => t.type === 'expense').forEach(t => table.add(t.category, t.amount));
    return table;
  }
}

if (typeof module !== 'undefined') module.exports = { SmartHashTable };
