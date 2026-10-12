/**
 * linkedList.js
 * โครงสร้างข้อมูล Doubly Linked List สำหรับเก็บรายการธุรกรรม (Transaction)
 * แต่ละ Node เก็บข้อมูลธุรกรรม 1 รายการ และมีตัวชี้ prev / next
 * - prev: ใช้เดินย้อนกลับไปยังรายการก่อนหน้า (เช่น undo, เลื่อนดูย้อนหลัง)
 * - next: ใช้เดินไปข้างหน้าตามลำดับที่แทรกเข้ามา (เช่น แสดงรายการล่าสุดก่อน)
 */

function transactionTimestamp(date, time) {
  const normalizedTime = /^\d{2}:\d{2}$/.test(time || '') ? `${time}:00` : (time || '00:00:00');
  return new Date(`${date}T${normalizedTime}`).getTime();
}

class TransactionNode {
  constructor(data) {
    this.id = data.id;
    this.type = data.type;           // 'income' | 'expense'
    this.amount = data.amount;
    this.category = data.category;
    this.date = data.date;           // 'YYYY-MM-DD'
    this.time = data.time;           // 'HH:MM'
    this.note = data.note || '';
    this.necessity = data.necessity || null; // 1-5 (เฉพาะรายจ่าย)
    this.prev = null;
    this.next = null;
  }

  // timestamp ตัวเลขไว้ใช้เปรียบเทียบ/จัดเรียงตามวันเวลา
  get timestamp() {
    return transactionTimestamp(this.date, this.time);
  }

  toJSON() {
    return {
      id: this.id, type: this.type, amount: this.amount, category: this.category,
      date: this.date, time: this.time, note: this.note, necessity: this.necessity
    };
  }
}

class DoublyLinkedList {
  constructor() {
    this.head = null; // รายการที่เก่าที่สุด
    this.tail = null; // รายการที่ล่าสุด
    this.size = 0;
    this._index = new Map(); // id -> node สำหรับค้นหา O(1)
  }

  // เพิ่ม node ต่อท้าย (ต่อจาก tail) - O(1)
  append(data) {
    const node = new TransactionNode(data);
    if (!this.head) {
      this.head = node;
      this.tail = node;
    } else {
      node.prev = this.tail;
      this.tail.next = node;
      this.tail = node;
    }
    this._index.set(node.id, node);
    this.size++;
    return node;
  }

  // ค้นหา node ด้วย id - O(1) ผ่าน index map
  find(id) {
    return this._index.get(id) || null;
  }

  // ลบ node ด้วย id - O(1) เมื่อรู้ตำแหน่ง node แล้ว (ไม่ต้องไล่จากหัว)
  remove(id) {
    const node = this._index.get(id);
    if (!node) return false;

    if (node.prev) node.prev.next = node.next;
    else this.head = node.next; // ลบ head

    if (node.next) node.next.prev = node.prev;
    else this.tail = node.prev; // ลบ tail

    this._index.delete(id);
    this.size--;
    return true;
  }

  // แก้ไขข้อมูลของ node (คงตำแหน่งเดิมใน list ไว้)
  update(id, newData) {
    const node = this._index.get(id);
    if (!node) return false;
    Object.assign(node, {
      type: newData.type, amount: newData.amount, category: newData.category,
      date: newData.date, time: newData.time, note: newData.note, necessity: newData.necessity
    });
    return true;
  }

  // แปลงเป็น array
  // forward = true: เดินจาก head -> tail (เก่าสุด -> ใหม่สุด)
  // forward = false: เดินจาก tail -> head (ใหม่สุด -> เก่าสุด) โดยใช้ prev pointer
  toArray(forward = false) {
    const result = [];
    if (forward) {
      let cur = this.head;
      while (cur) { result.push(cur); cur = cur.next; }
    } else {
      let cur = this.tail;
      while (cur) { result.push(cur); cur = cur.prev; }
    }
    return result;
  }

  clear() {
    this.head = null;
    this.tail = null;
    this.size = 0;
    this._index.clear();
  }

  // สร้าง list จากอาเรย์ข้อมูล (ใช้ตอนโหลดจาก localStorage)
  static fromArray(arr) {
    const list = new DoublyLinkedList();
    arr.forEach(item => list.append(item));
    return list;
  }
}

// ทำให้เรียกใช้งานได้ทั้งแบบ module และแบบ global script
if (typeof module !== 'undefined') module.exports = { DoublyLinkedList, TransactionNode };
