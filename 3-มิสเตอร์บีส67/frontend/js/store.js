/**
 * store.js
 * ศูนย์กลางจัดการข้อมูลของแอป เชื่อมโยงโครงสร้างข้อมูลทั้ง 3 ชนิดเข้าด้วยกัน:
 *   - DoublyLinkedList : เก็บลำดับธุรกรรมทั้งหมด (insert/delete O(1), เดินหน้า/ถอยหลัง)
 *   - AVLTree          : index ตาม timestamp สำหรับ Range Search แบบเร็ว
 *   - SmartHashTable   : สรุปยอดตามหมวดหมู่แบบเร็ว
 * ข้อมูลถูก persist ลง localStorage ทุกครั้งที่มีการเปลี่ยนแปลง เพื่อให้ Refresh แล้วข้อมูลไม่หาย
 */

const STORAGE_KEY = 'smartbudget_transactions';
const BUDGET_KEY = 'smartbudget_budget';

class Store {
  constructor() {
    this.list = new DoublyLinkedList();
    this.tree = new AVLTree();
    this.hash = new SmartHashTable();
    this._load();
  }

  _load() {
    const raw = localStorage.getItem(STORAGE_KEY);
    let data;
    if (!raw) {
      data = generateSampleData();
      localStorage.setItem(STORAGE_KEY, JSON.stringify(data));
      localStorage.setItem(BUDGET_KEY, String(SAMPLE_BUDGET));
    } else {
      data = JSON.parse(raw);
    }
    this._rebuildStructures(data);
  }

  _rebuildStructures(data) {
    this.list.clear();
    this.tree = new AVLTree();
    this.hash.clear();
    data.forEach(t => {
      this.list.append(t);
      this.tree.insert(transactionTimestamp(t.date, t.time), t.id);
      if (t.type === 'expense') this.hash.add(t.category, t.amount);
    });
  }

  _persist() {
    const data = this.list.toArray(true).map(n => n.toJSON());
    localStorage.setItem(STORAGE_KEY, JSON.stringify(data));
  }

  getBudget() {
    return Number(localStorage.getItem(BUDGET_KEY)) || 0;
  }

  setBudget(amount) {
    localStorage.setItem(BUDGET_KEY, String(amount));
  }

  genId() {
    return 'tx' + Date.now().toString(36) + Math.floor(Math.random() * 1000);
  }

  addTransaction(data) {
    const record = { ...data, id: this.genId() };
    this.list.append(record);
    this.tree.insert(transactionTimestamp(record.date, record.time), record.id);
    if (record.type === 'expense') this.hash.add(record.category, record.amount);
    this._persist();
    return record;
  }

  updateTransaction(id, newData) {
    const node = this.list.find(id);
    if (!node) return false;

    // ปรับ hash table: หักยอดเก่าออก แล้วบวกยอดใหม่เข้า (ถ้าเป็นรายจ่าย)
    if (node.type === 'expense') this.hash.remove(node.category, node.amount);
    if (newData.type === 'expense') this.hash.add(newData.category, newData.amount);

    // ปรับ AVL tree ถ้าวันเวลาเปลี่ยน
    const oldTs = node.timestamp;
    const newTs = transactionTimestamp(newData.date, newData.time);
    if (oldTs !== newTs) {
      this.tree.remove(oldTs, id);
      this.tree.insert(newTs, id);
    }

    this.list.update(id, newData);
    this._persist();
    return true;
  }

  deleteTransaction(id) {
    const node = this.list.find(id);
    if (!node) return false;
    if (node.type === 'expense') this.hash.remove(node.category, node.amount);
    this.tree.remove(node.timestamp, id);
    this.list.remove(id);
    this._persist();
    return true;
  }

  getAll(forward = false) {
    return this.list.toArray(forward);
  }

  // ค้นหาด้วยช่วงเวลาโดยใช้ AVL Tree range search แล้วดึงรายละเอียดจาก linked list index
  searchByDateRange(startDate, endDate) {
    const startTs = startDate ? new Date(`${startDate}T00:00:00`).getTime() : -Infinity;
    const endTs = endDate ? new Date(`${endDate}T23:59:59`).getTime() : Infinity;
    const ids = this.tree.rangeSearch(startTs, endTs);
    return ids.map(id => this.list.find(id)).filter(Boolean);
  }

  getCategoryTotals() {
    return this.hash.entries();
  }

  summary() {
    const all = this.getAll(true);
    const income = all.filter(t => t.type === 'income').reduce((s, t) => s + t.amount, 0);
    const expense = all.filter(t => t.type === 'expense').reduce((s, t) => s + t.amount, 0);
    return {
      totalIncome: income,
      totalExpense: expense,
      balance: income - expense,
      budget: this.getBudget(),
      count: all.length
    };
  }

  resetToSample() {
    localStorage.removeItem(STORAGE_KEY);
    localStorage.removeItem(BUDGET_KEY);
    this._load();
    this._persist();
  }
}

// รองรับการ require() จากสคริปต์ทดสอบ (frontend/tests) โดยไม่กระทบการใช้งานในเบราว์เซอร์
if (typeof module !== 'undefined') module.exports = { Store };
