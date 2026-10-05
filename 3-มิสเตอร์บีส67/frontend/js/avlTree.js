/**
 * avlTree.js
 * AVL Tree (Self-Balancing Binary Search Tree)
 * Key = timestamp ของวันเวลาธุรกรรม (date + time)
 * ใช้สำหรับ:
 *   - Range Search: ค้นหาธุรกรรมทั้งหมดที่อยู่ในช่วงเวลาที่กำหนด ใน O(log n + k)
 *   - รักษาความสมดุลของต้นไม้ทุกครั้งที่ insert/remove ด้วย rotation
 *     เพื่อให้การค้นหายังคงเป็น O(log n) แม้ข้อมูลจะถูกแทรกแบบเรียงลำดับ
 */

function transactionTimestamp(date, time) {
  const normalizedTime = /^\d{2}:\d{2}$/.test(time || '') ? `${time}:00` : (time || '00:00:00');
  return new Date(`${date}T${normalizedTime}`).getTime();
}

class AVLNode {
  constructor(key, id) {
    this.key = key;       // timestamp
    this.ids = [id];      // เผื่อกรณี timestamp ซ้ำกัน เก็บหลาย id ใน node เดียว
    this.left = null;
    this.right = null;
    this.height = 1;
  }
}

class AVLTree {
  constructor() {
    this.root = null;
    this.rotationLog = []; // เก็บ log การหมุนต้นไม้ล่าสุด เพื่อใช้แสดงผลการทำงานของ Algorithm
  }

  _h(node) { return node ? node.height : 0; }
  _balanceFactor(node) { return node ? this._h(node.left) - this._h(node.right) : 0; }
  _updateHeight(node) { node.height = 1 + Math.max(this._h(node.left), this._h(node.right)); }

  _rotateRight(y) {
    const x = y.left;
    const T2 = x.right;
    x.right = y;
    y.left = T2;
    this._updateHeight(y);
    this._updateHeight(x);
    this.rotationLog.push(`Right Rotate ที่ key=${y.key}`);
    return x;
  }

  _rotateLeft(x) {
    const y = x.right;
    const T2 = y.left;
    y.left = x;
    x.right = T2;
    this._updateHeight(x);
    this._updateHeight(y);
    this.rotationLog.push(`Left Rotate ที่ key=${x.key}`);
    return y;
  }

  insert(key, id) {
    this.root = this._insert(this.root, key, id);
  }

  _insert(node, key, id) {
    if (!node) return new AVLNode(key, id);

    if (key < node.key) node.left = this._insert(node.left, key, id);
    else if (key > node.key) node.right = this._insert(node.right, key, id);
    else { node.ids.push(id); return node; } // key ซ้ำ: เพิ่มเข้า node เดิม

    this._updateHeight(node);
    const balance = this._balanceFactor(node);

    // Left Left
    if (balance > 1 && key < node.left.key) return this._rotateRight(node);
    // Right Right
    if (balance < -1 && key > node.right.key) return this._rotateLeft(node);
    // Left Right
    if (balance > 1 && key > node.left.key) {
      node.left = this._rotateLeft(node.left);
      return this._rotateRight(node);
    }
    // Right Left
    if (balance < -1 && key < node.right.key) {
      node.right = this._rotateRight(node.right);
      return this._rotateLeft(node);
    }
    return node;
  }

  remove(key, id) {
    this.root = this._remove(this.root, key, id);
  }

  _minValueNode(node) {
    let cur = node;
    while (cur.left) cur = cur.left;
    return cur;
  }

  // ลบ node ทั้งก้อนโดยไม่สนใจ ids ภายใน ใช้สำหรับย้าย successor
  // ตอนลบ node ที่มีลูก 2 ฝั่ง
  _removeWholeNode(node, key) {
    if (!node) return null;
    if (key < node.key) node.left = this._removeWholeNode(node.left, key);
    else if (key > node.key) node.right = this._removeWholeNode(node.right, key);
    else {
      if (!node.left) return node.right;
      if (!node.right) return node.left;
      const successor = this._minValueNode(node.right);
      node.key = successor.key;
      node.ids = successor.ids.slice();
      node.right = this._removeWholeNode(node.right, successor.key);
    }

    this._updateHeight(node);
    const balance = this._balanceFactor(node);
    if (balance > 1 && this._balanceFactor(node.left) >= 0) return this._rotateRight(node);
    if (balance > 1 && this._balanceFactor(node.left) < 0) {
      node.left = this._rotateLeft(node.left);
      return this._rotateRight(node);
    }
    if (balance < -1 && this._balanceFactor(node.right) <= 0) return this._rotateLeft(node);
    if (balance < -1 && this._balanceFactor(node.right) > 0) {
      node.right = this._rotateRight(node.right);
      return this._rotateLeft(node);
    }
    return node;
  }

  _remove(node, key, id) {
    if (!node) return null;
    if (key < node.key) node.left = this._remove(node.left, key, id);
    else if (key > node.key) node.right = this._remove(node.right, key, id);
    else {
      node.ids = node.ids.filter(x => x !== id);
      if (node.ids.length > 0) return node; // ยังมี id อื่นอยู่ใน node นี้

      if (!node.left || !node.right) {
        node = node.left || node.right;
      } else {
        const successor = this._minValueNode(node.right);
        // ย้ายข้อมูลของ successor ทั้ง node มาแทน node ที่ถูกลบ
        // แล้วลบ successor ออกจาก subtree ทั้ง node เพื่อไม่ให้ ids ซ้ำ
        // ในกรณีที่ successor มีหลาย transaction ที่ timestamp เดียวกัน
        node.key = successor.key;
        node.ids = successor.ids.slice();
        node.right = this._removeWholeNode(node.right, successor.key);
      }
    }
    if (!node) return null;

    this._updateHeight(node);
    const balance = this._balanceFactor(node);

    if (balance > 1 && this._balanceFactor(node.left) >= 0) return this._rotateRight(node);
    if (balance > 1 && this._balanceFactor(node.left) < 0) {
      node.left = this._rotateLeft(node.left);
      return this._rotateRight(node);
    }
    if (balance < -1 && this._balanceFactor(node.right) <= 0) return this._rotateLeft(node);
    if (balance < -1 && this._balanceFactor(node.right) > 0) {
      node.right = this._rotateRight(node.right);
      return this._rotateLeft(node);
    }
    return node;
  }

  // Range Search: คืนค่า id ทั้งหมดที่ timestamp อยู่ในช่วง [startTs, endTs]
  rangeSearch(startTs, endTs) {
    if (startTs > endTs) return [];
    const result = [];
    this._rangeSearch(this.root, startTs, endTs, result);
    return result;
  }

  _rangeSearch(node, lo, hi, result) {
    if (!node) return;
    // ตัดกิ่งที่ไม่จำเป็นออก (pruning) เพื่อลดเวลาค้นหา
    if (node.key > lo) this._rangeSearch(node.left, lo, hi, result);
    if (node.key >= lo && node.key <= hi) result.push(...node.ids);
    if (node.key < hi) this._rangeSearch(node.right, lo, hi, result);
  }

  height() { return this._h(this.root); }

  // In-order traversal สำหรับตรวจสอบ/แสดงผล
  inOrder() {
    const out = [];
    const walk = (n) => { if (!n) return; walk(n.left); out.push({ key: n.key, ids: n.ids }); walk(n.right); };
    walk(this.root);
    return out;
  }

  static buildFromTransactions(transactions) {
    const tree = new AVLTree();
    transactions.forEach(t => tree.insert(transactionTimestamp(t.date, t.time), t.id));
    return tree;
  }
}

if (typeof module !== 'undefined') module.exports = { AVLTree };
