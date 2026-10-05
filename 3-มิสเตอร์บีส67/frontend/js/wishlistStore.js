/**
 * wishlistStore.js
 * จัดการข้อมูล "ของที่อยากได้" (Wishlist / Savings Goal) แยกจากธุรกรรมหลัก
 * เก็บลง localStorage ภายใต้ key แยกต่างหาก เพื่อไม่ให้กระทบ Store ของธุรกรรมเดิม
 */

const WISHLIST_KEY = 'smartbudget_wishlist';

class WishlistStore {
  constructor() {
    this.items = this._load();
  }

  _load() {
    const raw = localStorage.getItem(WISHLIST_KEY);
    return raw ? JSON.parse(raw) : [];
  }

  _persist() {
    localStorage.setItem(WISHLIST_KEY, JSON.stringify(this.items));
  }

  getAll() {
    return this.items;
  }

  findById(id) {
    return this.items.find(i => i.id === id) || null;
  }

  add({ name, price, planType, planParams }) {
    const item = {
      id: 'wl' + Date.now().toString(36) + Math.floor(Math.random() * 1000),
      name,
      price: Number(price),
      createdDate: new Date().toISOString().slice(0, 10),
      planType,
      planParams,
      savedAmount: 0,
      depositLog: [],
      completed: false
    };
    this.items.unshift(item);
    this._persist();
    return item;
  }

  update(id, data) {
    const item = this.findById(id);
    if (!item) return false;
    Object.assign(item, data);
    this._persist();
    return true;
  }

  remove(id) {
    const before = this.items.length;
    this.items = this.items.filter(i => i.id !== id);
    this._persist();
    return this.items.length < before;
  }

  deposit(id, amount) {
    const item = this.findById(id);
    if (!item) return null;
    item.savedAmount += amount;
    item.depositLog.push({ date: new Date().toISOString().slice(0, 10), amount });
    if (item.savedAmount >= item.price) item.completed = true;
    this._persist();
    return item;
  }
}

// รองรับการ require() จากสคริปต์ทดสอบ (frontend/tests) โดยไม่กระทบการใช้งานในเบราว์เซอร์
if (typeof module !== 'undefined') module.exports = { WishlistStore };
