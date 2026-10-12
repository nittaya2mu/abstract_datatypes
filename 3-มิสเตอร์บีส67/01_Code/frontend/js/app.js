/**
 * app.js
 * ตัวควบคุมหลักของแอป: routing แบบ hash, render แต่ละหน้า, ผูก event ของ UI ทั้งหมด
 * เชื่อมต่อกับ Store (store.js) ซึ่งคุมโครงสร้างข้อมูลทั้งหมดอยู่เบื้องหลัง
 */

Auth.requireAuth();

const store = new Store();
const wishlistStore = new WishlistStore();
const session = Auth.currentSession();
document.getElementById('userName').textContent = session?.name || 'นิสิต';
document.getElementById('userAvatar').textContent = (session?.name || 'น')[0];

const fmt = (n) => Number(n || 0).toLocaleString('th-TH', { maximumFractionDigits: 0 });
const fmtDateTime = (t) => `${t.date} • ${t.time || '-'}`;

// ---------------------------------------------------------------------------
// Toast
// ---------------------------------------------------------------------------
function toast(msg) {
  const el = document.getElementById('toast');
  el.textContent = msg;
  el.classList.add('show');
  clearTimeout(toast._t);
  toast._t = setTimeout(() => el.classList.remove('show'), 2200);
}

// ---------------------------------------------------------------------------
// Router
// ---------------------------------------------------------------------------
const PAGES = {
  dashboard: { title: 'Dashboard', sub: 'ภาพรวมการเงินของคุณเดือนนี้', render: renderDashboard },
  transactions: { title: 'รายรับ-รายจ่าย', sub: 'จัดการรายการธุรกรรมทั้งหมดของคุณ', render: renderTransactions },
  search: { title: 'ค้นหาธุรกรรมย้อนหลัง', sub: 'ค้นหาด้วยคำค้น ช่วงเวลา หมวดหมู่ และเรียงลำดับผลลัพธ์', render: renderSearch },
  optimization: { title: 'Budget Optimization', sub: 'วิเคราะห์และแนะนำการลดค่าใช้จ่ายเมื่อเกินงบ', render: renderOptimization },
  wishlist: { title: 'ของที่อยากได้', sub: 'ตั้งเป้าหมายและวางแผนเก็บเงินซื้อของที่อยากได้', render: renderWishlist },
  datastructure: { title: 'Data Structure & Algorithm', sub: 'ดูการทำงานของโครงสร้างข้อมูลและอัลกอริทึมเบื้องหลังระบบ', render: renderDataStructure }
};

function navigate() {
  const hash = (window.location.hash || '#dashboard').replace('#', '');
  const page = PAGES[hash] ? hash : 'dashboard';

  document.querySelectorAll('.nav-item').forEach(a => a.classList.toggle('active', a.dataset.view === page));
  document.querySelectorAll('.view-inner').forEach(v => v.classList.toggle('active', v.id === `view-${page}`));
  document.getElementById('pageTitle').textContent = PAGES[page].title;
  document.getElementById('pageSub').textContent = PAGES[page].sub;
  PAGES[page].render();

  document.getElementById('sidebar').classList.remove('open');
}
window.addEventListener('hashchange', navigate);

document.getElementById('menuToggle').addEventListener('click', () => {
  document.getElementById('sidebar').classList.toggle('open');
});
document.getElementById('logoutBtn').addEventListener('click', (e) => {
  e.preventDefault();
  Auth.logout();
  window.location.href = 'index.html';
});
document.getElementById('themeToggleBtn').addEventListener('click', () => Theme.toggle());

// ---------------------------------------------------------------------------
// DASHBOARD VIEW
// ---------------------------------------------------------------------------
function renderDashboard() {
  const s = store.summary();
  const usedPct = s.budget > 0 ? Math.min((s.totalExpense / s.budget) * 100, 100) : 0;
  const isOver = s.budget > 0 && s.totalExpense > s.budget;
  const isNear = s.budget > 0 && !isOver && s.totalExpense / s.budget >= 0.8;

  document.getElementById('summaryCards').innerHTML = `
    ${card('รายรับทั้งหมด', '฿' + fmt(s.totalIncome), 'var(--success-bg)', 'var(--success)', '⬆')}
    ${card('รายจ่ายทั้งหมด', '฿' + fmt(s.totalExpense), 'var(--danger-bg)', 'var(--danger)', '⬇')}
    ${card('เงินคงเหลือ', '฿' + fmt(s.balance), 'var(--blue-100)', 'var(--blue-600)', '฿')}
    ${card('จำนวนธุรกรรม', s.count + ' รายการ', 'var(--gray-200)', 'var(--gray-700)', '📄')}
  `;

  document.getElementById('budgetHint').textContent = s.budget ? `฿${fmt(s.totalExpense)} / ฿${fmt(s.budget)}` : 'ยังไม่ได้ตั้งงบประมาณ';
  const fill = document.getElementById('dashProgressFill');
  fill.style.width = usedPct + '%';
  fill.className = 'progress-fill' + (isOver ? ' over' : isNear ? ' warn' : '');

  let alertHtml = '';
  if (isOver) alertHtml = alertBox('danger', '⚠', `ใช้จ่ายเกินงบประมาณไปแล้ว ฿${fmt(s.totalExpense - s.budget)} — ไปที่หน้า Budget Optimization เพื่อดูคำแนะนำ`);
  else if (isNear) alertHtml = alertBox('warn', '⏱', `ใช้งบไปแล้ว ${usedPct.toFixed(0)}% ใกล้เกินงบประมาณแล้ว ควรระวังการใช้จ่ายในหมวดที่ไม่จำเป็น`);
  else if (s.budget) alertHtml = alertBox('ok', '✓', `การใช้จ่ายยังอยู่ในงบประมาณที่ตั้งไว้ (ใช้ไป ${usedPct.toFixed(0)}%)`);
  document.getElementById('dashBudgetAlert').innerHTML = alertHtml;

  const catTotals = store.getCategoryTotals();
  const top = catTotals[0];
  document.getElementById('topCategoryBox').innerHTML = top ? `
    <div style="display:flex; align-items:center; gap:14px;">
      <div class="stat-icon" style="background:var(--blue-100); color:var(--blue-600);">🏷</div>
      <div>
        <div style="font-size:17px; font-weight:700;">${top.category}</div>
        <div class="hint">฿${fmt(top.total)} จาก ${top.count} รายการ</div>
      </div>
    </div>` : `<div class="hint">ยังไม่มีข้อมูลรายจ่าย</div>`;

  document.getElementById('categoryBreakdown').innerHTML = catTotals.length ? catTotals.map(c => {
    const pct = s.totalExpense ? (c.total / s.totalExpense) * 100 : 0;
    return `<div class="cat-row">
      <div class="cat-name">${c.category}</div>
      <div class="cat-bar-track"><div class="cat-bar-fill" style="width:${pct}%"></div></div>
      <div class="cat-amount">฿${fmt(c.total)}</div>
    </div>`;
  }).join('') : `<div class="hint">ยังไม่มีข้อมูล</div>`;

  const recent = store.getAll(false).slice(0, 6);
  document.getElementById('recentList').innerHTML = recent.length ? recent.map(t => txMiniRow(t)).join('') :
    `<div class="hint">ยังไม่มีรายการ</div>`;
}

function card(title, value, iconBg, iconColor, icon) {
  return `<div class="card">
    <div class="stat-row-top">
      <div class="stat-icon" style="background:${iconBg}; color:${iconColor};">${icon}</div>
    </div>
    <div class="card-title">${title}</div>
    <div class="card-value">${value}</div>
  </div>`;
}

function alertBox(type, icon, msg) {
  return `<div class="alert alert-${type === 'ok' ? 'ok' : type === 'warn' ? 'warn' : 'danger'}"><span>${icon}</span><span>${msg}</span></div>`;
}

function necessityChip(n) {
  if (!n) return '<span class="badge badge-gray">-</span>';
  const label = n <= 2 ? 'จำเป็น' : n === 3 ? 'ปานกลาง' : 'ตัดได้ง่าย';
  return `<span class="necessity-chip n${n}">${n} · ${label}</span>`;
}

function txMiniRow(t) {
  const sign = t.type === 'income' ? '+' : '−';
  const cls = t.type === 'income' ? 'amount-income' : 'amount-expense';
  return `<div style="display:flex; align-items:center; justify-content:space-between; padding:9px 0; border-bottom:1px solid var(--gray-100);">
    <div>
      <div style="font-size:13.5px; font-weight:500;">${t.category}</div>
      <div class="hint">${fmtDateTime(t)}</div>
    </div>
    <div class="${cls}">${sign}฿${fmt(t.amount)}</div>
  </div>`;
}

// ---------------------------------------------------------------------------
// TRANSACTIONS VIEW
// ---------------------------------------------------------------------------
function renderTransactions() {
  const all = store.getAll(false);
  document.getElementById('txCountHint').textContent = `${all.length} รายการทั้งหมด`;
  document.getElementById('txTableBody').innerHTML = all.map(txRow).join('');
  document.getElementById('txEmptyState').innerHTML = all.length ? '' : emptyState('ยังไม่มีรายการธุรกรรม', 'กดปุ่ม "+ เพิ่มรายการ" ด้านบนเพื่อเริ่มบันทึก');
  bindRowActions('#txTableBody');
}

function emptyState(title, sub) {
  return `<div class="empty-state"><div class="ico">📭</div><div style="font-weight:600;">${title}</div><div class="hint">${sub}</div></div>`;
}

function txRow(t) {
  const sign = t.type === 'income' ? '+' : '−';
  const cls = t.type === 'income' ? 'amount-income' : 'amount-expense';
  const typeBadge = t.type === 'income' ? '<span class="badge badge-success">รายรับ</span>' : '<span class="badge badge-danger">รายจ่าย</span>';
  return `<tr data-id="${t.id}">
    <td>${fmtDateTime(t)}</td>
    <td>${typeBadge}</td>
    <td>${t.category}</td>
    <td>${t.note || '-'}</td>
    <td>${necessityChip(t.necessity)}</td>
    <td style="text-align:right;" class="${cls}">${sign}฿${fmt(t.amount)}</td>
    <td>
      <div class="row-actions">
        <button class="icon-btn edit-btn" title="แก้ไข">✎</button>
        <button class="icon-btn danger del-btn" title="ลบ">🗑</button>
      </div>
    </td>
  </tr>`;
}

function bindRowActions(tableSelector) {
  document.querySelectorAll(`${tableSelector} .edit-btn`).forEach(btn => {
    btn.addEventListener('click', (e) => {
      const id = e.target.closest('tr').dataset.id;
      openTxModal(store.list.find(id));
    });
  });
  document.querySelectorAll(`${tableSelector} .del-btn`).forEach(btn => {
    btn.addEventListener('click', (e) => {
      const id = e.target.closest('tr').dataset.id;
      if (confirm('ต้องการลบรายการนี้หรือไม่?')) {
        store.deleteTransaction(id);
        toast('ลบรายการเรียบร้อยแล้ว');
        navigate();
      }
    });
  });
}

// ---------------------------------------------------------------------------
// TRANSACTION MODAL (Add / Edit)
// ---------------------------------------------------------------------------
const txModal = document.getElementById('txModalOverlay');
let currentTxType = 'income';
let currentNecessity = 3;

function openTxModal(existing) {
  document.getElementById('txForm').reset();
  document.getElementById('txId').value = existing ? existing.id : '';
  document.getElementById('txModalTitle').textContent = existing ? 'แก้ไขรายการ' : 'เพิ่มรายการ';

  currentTxType = existing ? existing.type : 'income';
  setTypeToggle(currentTxType);

  const now = new Date();
  document.getElementById('txAmount').value = existing ? existing.amount : '';
  document.getElementById('txCategory').value = existing ? existing.category : 'อาหาร';
  document.getElementById('txDate').value = existing ? existing.date : now.toISOString().slice(0, 10);
  document.getElementById('txTime').value = existing ? existing.time : now.toTimeString().slice(0, 5);
  document.getElementById('txNote').value = existing ? existing.note : '';

  currentNecessity = existing?.necessity || 3;
  setNecessityPicker(currentNecessity);

  txModal.classList.add('show');
}

function closeTxModal() { txModal.classList.remove('show'); }
document.getElementById('closeTxModal').addEventListener('click', closeTxModal);
document.getElementById('cancelTxBtn').addEventListener('click', closeTxModal);
document.getElementById('addTxTopBtn').addEventListener('click', () => openTxModal(null));

function setTypeToggle(type) {
  currentTxType = type;
  document.querySelectorAll('.type-toggle button').forEach(b => b.classList.toggle('active', b.dataset.type === type));
  document.getElementById('necessityField').style.display = type === 'expense' ? 'block' : 'none';
}
document.querySelectorAll('.type-toggle button').forEach(b => {
  b.addEventListener('click', () => setTypeToggle(b.dataset.type));
});

function setNecessityPicker(n) {
  currentNecessity = n;
  document.querySelectorAll('.necessity-picker button').forEach(b => b.classList.toggle('active', Number(b.dataset.n) === n));
}
document.querySelectorAll('.necessity-picker button').forEach(b => {
  b.addEventListener('click', () => setNecessityPicker(Number(b.dataset.n)));
});

document.getElementById('txForm').addEventListener('submit', (e) => {
  e.preventDefault();
  const id = document.getElementById('txId').value;
  const data = {
    type: currentTxType,
    amount: Number(document.getElementById('txAmount').value),
    category: document.getElementById('txCategory').value,
    date: document.getElementById('txDate').value,
    time: document.getElementById('txTime').value,
    note: document.getElementById('txNote').value.trim(),
    necessity: currentTxType === 'expense' ? currentNecessity : null
  };

  if (!data.amount || data.amount <= 0) { toast('กรุณาระบุจำนวนเงินให้ถูกต้อง'); return; }

  if (id) {
    store.updateTransaction(id, data);
    toast('แก้ไขรายการเรียบร้อยแล้ว');
  } else {
    store.addTransaction(data);
    toast('เพิ่มรายการเรียบร้อยแล้ว');
  }
  closeTxModal();
  navigate();
});

// ---------------------------------------------------------------------------
// SEARCH VIEW
// ---------------------------------------------------------------------------
let searchSort = { key: 'date', dir: 'desc' };

function populateCategoryOptions() {
  const sel = document.getElementById('searchCategory');
  const cats = [...new Set(store.getAll(true).map(t => t.category))];
  sel.innerHTML = '<option value="">ทั้งหมด</option>' + cats.map(c => `<option value="${c}">${c}</option>`).join('');
}

function renderSearch() {
  populateCategoryOptions();
  runSearch();
}

function runSearch() {
  const keyword = document.getElementById('searchKeyword').value.trim().toLowerCase();
  const start = document.getElementById('searchStart').value;
  const end = document.getElementById('searchEnd').value;
  const category = document.getElementById('searchCategory').value;
  const type = document.getElementById('searchType').value;

  // ใช้ AVL Tree range search เมื่อมีการระบุช่วงวันที่ ไม่เช่นนั้นดึงข้อมูลทั้งหมดจาก Linked List
  let results = (start || end) ? store.searchByDateRange(start, end) : store.getAll(true);

  if (keyword) {
    results = results.filter(t =>
      t.category.toLowerCase().includes(keyword) || (t.note || '').toLowerCase().includes(keyword)
    );
  }
  if (category) results = results.filter(t => t.category === category);
  if (type) results = results.filter(t => t.type === type);

  // เรียงลำดับด้วย Merge Sort (stable) ตามคีย์ที่เลือก
  const keyFn = searchSort.key === 'amount' ? (t => t.amount)
    : searchSort.key === 'category' ? (t => t.category)
      : (t => t.timestamp);
  const sortRes = mergeSort(results, keyFn, searchSort.dir === 'asc' ? 'asc' : 'desc');

  document.getElementById('searchResultHint').textContent = `พบ ${sortRes.result.length} รายการ`;
  document.getElementById('searchTableBody').innerHTML = sortRes.result.map(t => `
    <tr>
      <td>${fmtDateTime(t)}</td>
      <td>${t.type === 'income' ? '<span class="badge badge-success">รายรับ</span>' : '<span class="badge badge-danger">รายจ่าย</span>'}</td>
      <td>${t.category}</td>
      <td>${t.note || '-'}</td>
      <td>${necessityChip(t.necessity)}</td>
      <td style="text-align:right;" class="${t.type === 'income' ? 'amount-income' : 'amount-expense'}">${t.type === 'income' ? '+' : '−'}฿${fmt(t.amount)}</td>
    </tr>
  `).join('');
  document.getElementById('searchEmptyState').innerHTML = sortRes.result.length ? '' : emptyState('ไม่พบรายการที่ตรงกับเงื่อนไข', 'ลองปรับคำค้นหรือช่วงวันที่ใหม่อีกครั้ง');
}

document.getElementById('searchBtn').addEventListener('click', runSearch);
document.getElementById('searchResetBtn').addEventListener('click', () => {
  document.getElementById('searchKeyword').value = '';
  document.getElementById('searchStart').value = '';
  document.getElementById('searchEnd').value = '';
  document.getElementById('searchCategory').value = '';
  document.getElementById('searchType').value = '';
  runSearch();
});
document.querySelectorAll('.sort-btn[data-sort]').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.sort-btn[data-sort]').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    searchSort.key = btn.dataset.sort;
    runSearch();
  });
});
document.getElementById('sortDirBtn').addEventListener('click', (e) => {
  const dir = e.target.dataset.dir === 'desc' ? 'asc' : 'desc';
  e.target.dataset.dir = dir;
  e.target.textContent = dir === 'desc' ? 'มาก → น้อย' : 'น้อย → มาก';
  searchSort.dir = dir;
  runSearch();
});

// ---------------------------------------------------------------------------
// OPTIMIZATION VIEW
// ---------------------------------------------------------------------------
let optMethod = 'greedy';

function renderOptimization() {
  document.getElementById('budgetInput').value = store.getBudget() || '';
  runOptimizationView();
}

document.getElementById('saveBudgetBtn').addEventListener('click', () => {
  const val = Number(document.getElementById('budgetInput').value);
  if (!val || val <= 0) { toast('กรุณาระบุงบประมาณให้ถูกต้อง'); return; }
  store.setBudget(val);
  toast('บันทึกงบประมาณเรียบร้อยแล้ว');
  runOptimizationView();
});

document.querySelectorAll('#methodToggle button').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('#methodToggle button').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    optMethod = btn.dataset.method;
    runOptimizationView();
  });
});

function runOptimizationView() {
  const s = store.summary();
  const expenses = store.getAll(true).filter(t => t.type === 'expense');
  const outcome = runOptimization(expenses, s.budget, s.totalExpense, optMethod);

  document.getElementById('optSummaryGrid').innerHTML = `
    ${miniStat('งบประมาณ', '฿' + fmt(s.budget))}
    ${miniStat('รายจ่ายรวม', '฿' + fmt(s.totalExpense))}
    ${miniStat('ยอดที่เกินงบ', '฿' + fmt(outcome.overBudgetAmount || 0))}
    ${miniStat('ประหยัดได้', '฿' + fmt(outcome.totalSaved || 0))}
  `;

  let alertHtml;
  if (!outcome.overBudget) {
    alertHtml = alertBox('ok', '✓', outcome.message);
  } else if (outcome.achieved) {
    alertHtml = alertBox('warn', '💡', `แนะนำให้ตัด ${outcome.cutList.length} รายการ รวม ฿${fmt(outcome.totalSaved)} จะทำให้กลับมาอยู่ในงบประมาณได้พอดี`);
  } else {
    alertHtml = alertBox('danger', '⚠', `แม้ตัดรายการที่ไม่จำเป็นทั้งหมดแล้ว (${outcome.cutList.length} รายการ) ก็ยังขาดอีก ฿${fmt(outcome.stillOver)} — ควรพิจารณาลดรายจ่ายจำเป็นหรือเพิ่มรายรับ`);
  }
  document.getElementById('optAlertBox').innerHTML = alertHtml;

  document.getElementById('cutListBox').innerHTML = outcome.cutList && outcome.cutList.length ? outcome.cutList.map(t => `
    <div class="cut-item">
      <div class="left">
        <b>${t.category}${t.note ? ' — ' + t.note : ''}</b>
        <span>${fmtDateTime(t)} ${necessityChipText(t.necessity)}</span>
      </div>
      <div class="amount-expense" style="font-weight:700;">−฿${fmt(t.amount)}</div>
    </div>
  `).join('') : emptyState('ไม่มีรายการที่ต้องตัด', outcome.overBudget ? 'ไม่มีรายจ่ายที่ necessity ≥ 3 ให้พิจารณา' : 'รายจ่ายยังอยู่ในงบประมาณ');

  document.getElementById('stepsBox').innerHTML = outcome.steps && outcome.steps.length ? outcome.steps.map(st => `
    <div class="step-item"><h4>${st.title}</h4><p>${st.detail}</p></div>
  `).join('') : `<div class="hint">ไม่มีขั้นตอนให้แสดง</div>`;
}

function necessityChipText(n) {
  if (!n) return '';
  const label = n <= 2 ? 'จำเป็น' : n === 3 ? 'ปานกลาง' : 'ตัดได้ง่าย';
  return `· ความจำเป็น ${n}/5 (${label})`;
}

function miniStat(label, value) {
  return `<div class="card"><div class="card-title">${label}</div><div class="card-value">${value}</div></div>`;
}

// ---------------------------------------------------------------------------
// WISHLIST / SAVINGS GOAL VIEW
// ---------------------------------------------------------------------------
let currentPlanType = 'equal';

function renderWishlist() {
  const items = wishlistStore.getAll();
  document.getElementById('wishlistEmptyState').innerHTML = items.length ? '' :
    emptyState('ยังไม่มีของที่อยากได้', 'กดปุ่ม "+ เพิ่มของที่อยากได้" เพื่อเริ่มตั้งเป้าหมายและวางแผนเก็บเงิน');
  document.getElementById('wishlistGrid').innerHTML = items.map(wishCard).join('');
  bindWishCardActions();
}

function planLabel(item) {
  if (item.planType === 'equal') return `เก็บวันละ ฿${fmt(item.planParams.dailyAmount)} เท่ากันทุกวัน (${item.planParams.days} วัน)`;
  if (item.planType === 'multiplier') return `เก็บวันละทวีคูณ เริ่ม ฿${fmt(item.planParams.startAmount)} × ${item.planParams.multiplier} เท่า/วัน`;
  return `กำหนดเอง เก็บวันละ ฿${fmt(item.planParams.dailyAmount)}`;
}

function wishCard(item) {
  const pct = Math.min((item.savedAmount / item.price) * 100, 100);
  const remaining = Math.max(item.price - item.savedAmount, 0);
  const planBadge = item.planType === 'equal' ? 'badge-blue' : item.planType === 'multiplier' ? 'badge-warning' : 'badge-gray';
  const planName = item.planType === 'equal' ? 'เก็บเท่ากันทุกวัน' : item.planType === 'multiplier' ? 'เก็บวันละทวีคูณ' : 'กำหนดเอง';

  return `<div class="wish-card ${item.completed ? 'completed' : ''}" data-id="${item.id}">
    <div class="wish-head">
      <div>
        <h4>${item.name}</h4>
        <div class="wish-price">เป้าหมาย ฿${fmt(item.price)}</div>
      </div>
      ${item.completed ? '<span class="badge badge-success">ครบเป้าหมาย 🎉</span>' : `<span class="badge ${planBadge}">${planName}</span>`}
    </div>

    <div>
      <div class="progress-track"><div class="progress-fill" style="width:${pct}%"></div></div>
      <div class="wish-progress-row" style="margin-top:6px;">
        <span>เก็บแล้ว <span class="wish-saved">฿${fmt(item.savedAmount)}</span></span>
        <span>${item.completed ? 'ครบแล้ว' : `เหลือ ฿${fmt(remaining)}`}</span>
      </div>
    </div>

    <div class="hint">${planLabel(item)}</div>

    <div class="wish-actions">
      ${item.completed
        ? `<button class="btn btn-ghost btn-sm del-wish-btn">ลบรายการ</button>`
        : `<button class="btn btn-primary btn-sm deposit-btn">ฝากเงิน</button>
           <button class="btn btn-ghost btn-sm del-wish-btn">ลบ</button>`}
    </div>
  </div>`;
}

function bindWishCardActions() {
  document.querySelectorAll('#wishlistGrid .deposit-btn').forEach(btn => {
    btn.addEventListener('click', (e) => openDepositModal(e.target.closest('.wish-card').dataset.id));
  });
  document.querySelectorAll('#wishlistGrid .del-wish-btn').forEach(btn => {
    btn.addEventListener('click', (e) => {
      const id = e.target.closest('.wish-card').dataset.id;
      if (confirm('ต้องการลบเป้าหมายนี้หรือไม่?')) {
        wishlistStore.remove(id);
        toast('ลบเป้าหมายเรียบร้อยแล้ว');
        renderWishlist();
      }
    });
  });
}

// ---------- Add Wishlist Modal ----------
const wishModal = document.getElementById('wishModalOverlay');
document.getElementById('addWishBtn').addEventListener('click', () => openWishModal());
document.getElementById('closeWishModal').addEventListener('click', closeWishModal);
document.getElementById('cancelWishBtn').addEventListener('click', closeWishModal);

function openWishModal() {
  document.getElementById('wishForm').reset();
  document.getElementById('equalDays').value = 30;
  document.getElementById('multStart').value = 10;
  document.getElementById('multFactor').value = 2;
  document.getElementById('customDaily').value = 50;
  setPlanTab('equal');
  updatePlanPreview();
  wishModal.classList.add('show');
}
function closeWishModal() { wishModal.classList.remove('show'); }

function setPlanTab(type) {
  currentPlanType = type;
  document.querySelectorAll('.plan-tab').forEach(b => b.classList.toggle('active', b.dataset.plan === type));
  document.querySelectorAll('.plan-fields').forEach(p => p.classList.toggle('active', p.id === 'plan-' + type));
  updatePlanPreview();
}
document.querySelectorAll('.plan-tab').forEach(b => b.addEventListener('click', () => setPlanTab(b.dataset.plan)));

// คำนวณตัวอย่างผลลัพธ์แบบ Live ทุกครั้งที่ผู้ใช้พิมพ์ค่าใหม่
['wishPrice', 'equalDays', 'multStart', 'multFactor', 'customDaily'].forEach(id => {
  document.getElementById(id).addEventListener('input', updatePlanPreview);
});

function updatePlanPreview() {
  const price = Number(document.getElementById('wishPrice').value) || 0;
  const box = document.getElementById('planPreview');
  if (price <= 0) { box.textContent = 'กรอกราคาของก่อน เพื่อดูผลการคำนวณ'; return; }

  if (currentPlanType === 'equal') {
    const days = Number(document.getElementById('equalDays').value) || 1;
    const plan = calcEqualPlan(price, days);
    box.innerHTML = plan.explanation;
  } else if (currentPlanType === 'multiplier') {
    const start = Number(document.getElementById('multStart').value) || 1;
    const factor = Number(document.getElementById('multFactor').value) || 2;
    const plan = calcMultiplierPlan(price, start, factor);
    const preview = plan.schedule.slice(0, 6).map(s => `<span class="schedule-chip">วัน ${s.day}: ฿${fmt(s.amount)}</span>`).join('');
    box.innerHTML = plan.explanation + (plan.schedule.length ? `<div class="schedule-mini">${preview}${plan.schedule.length > 6 ? '<span class="schedule-chip">...</span>' : ''}</div>` : '');
  } else {
    const daily = Number(document.getElementById('customDaily').value) || 1;
    const plan = calcCustomPlan(price, daily);
    box.innerHTML = plan.explanation;
  }
}

document.getElementById('wishForm').addEventListener('submit', (e) => {
  e.preventDefault();
  const name = document.getElementById('wishName').value.trim();
  const price = Number(document.getElementById('wishPrice').value);
  if (!name || !price || price <= 0) { toast('กรุณากรอกชื่อและราคาให้ถูกต้อง'); return; }

  let planParams;
  if (currentPlanType === 'equal') {
    const days = Number(document.getElementById('equalDays').value) || 1;
    const plan = calcEqualPlan(price, days);
    planParams = { days: plan.days, dailyAmount: plan.dailyAmount };
  } else if (currentPlanType === 'multiplier') {
    const start = Number(document.getElementById('multStart').value) || 1;
    const factor = Number(document.getElementById('multFactor').value) || 2;
    planParams = { startAmount: start, multiplier: factor };
  } else {
    const daily = Number(document.getElementById('customDaily').value) || 1;
    planParams = { dailyAmount: daily };
  }

  wishlistStore.add({ name, price, planType: currentPlanType, planParams });
  toast('เพิ่มเป้าหมายเรียบร้อยแล้ว');
  closeWishModal();
  renderWishlist();
});

// ---------- Deposit Modal ----------
const depositModal = document.getElementById('depositModalOverlay');
document.getElementById('closeDepositModal').addEventListener('click', closeDepositModal);
document.getElementById('cancelDepositBtn').addEventListener('click', closeDepositModal);

function openDepositModal(id) {
  const item = wishlistStore.findById(id);
  if (!item) return;
  const remaining = Math.max(item.price - item.savedAmount, 0);
  const suggested = suggestedAmountForNextDeposit(item, remaining);

  document.getElementById('depositWishId').value = id;
  document.getElementById('depositWishName').textContent = `${item.name} — เหลืออีก ฿${fmt(remaining)}`;
  document.getElementById('depositAmount').value = suggested;
  document.getElementById('depositSuggestHint').textContent = `ระบบแนะนำ ฿${fmt(suggested)} ตามแผนที่ตั้งไว้ (แก้ไขจำนวนเองได้)`;
  depositModal.classList.add('show');
}
function closeDepositModal() { depositModal.classList.remove('show'); }

document.getElementById('depositForm').addEventListener('submit', (e) => {
  e.preventDefault();
  const id = document.getElementById('depositWishId').value;
  const amount = Number(document.getElementById('depositAmount').value);
  if (!amount || amount <= 0) { toast('กรุณาระบุจำนวนเงินให้ถูกต้อง'); return; }

  const item = wishlistStore.deposit(id, amount);
  toast(item.completed ? `🎉 เก็บเงินครบเป้าหมาย "${item.name}" แล้ว!` : 'บันทึกการฝากเงินเรียบร้อยแล้ว');
  closeDepositModal();
  renderWishlist();
});

// ---------------------------------------------------------------------------
// DATA STRUCTURE & ALGORITHM VIEW
// ---------------------------------------------------------------------------
function renderDataStructure() {
  document.querySelectorAll('.tab-btn').forEach(btn => {
    btn.onclick = () => {
      document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
      document.querySelectorAll('.tab-panel').forEach(p => p.classList.remove('active'));
      btn.classList.add('active');
      document.getElementById('tab-' + btn.dataset.tab).classList.add('active');
    };
  });

  renderDLLTab();
  renderAVLTab();
  renderHashTab();
  document.getElementById('benchGrid').innerHTML = `<div class="hint">กดปุ่ม "รันการทดสอบ" เพื่อเปรียบเทียบ Quick Sort และ Merge Sort</div>`;
}

function renderDLLTab() {
  const arr = store.getAll(true);
  document.getElementById('dllSize').textContent = arr.length;
  document.getElementById('dllHead').textContent = arr[0] ? `${arr[0].category} (${arr[0].date})` : '-';
  document.getElementById('dllTail').textContent = arr[arr.length - 1] ? `${arr[arr.length - 1].category} (${arr[arr.length - 1].date})` : '-';

  const shown = arr.slice(-8); // แสดงตัวอย่าง 8 node ล่าสุดเพื่อไม่ให้แน่นเกินไป
  document.getElementById('dllChain').innerHTML = shown.map((t, i) => `
    <div class="dll-node">
      <div class="cell p">prev</div>
      <div class="cell d">${t.category}<br><span style="color:var(--gray-500)">฿${fmt(t.amount)}</span></div>
      <div class="cell n">next</div>
    </div>
    ${i < shown.length - 1 ? '<span class="dll-arrow">⇄</span>' : ''}
  `).join('') + (arr.length > 8 ? `<span class="hint">…และอีก ${arr.length - 8} node</span>` : '');
}

function renderAVLTab() {
  document.getElementById('avlSize').textContent = store.tree.inOrder().length;
  document.getElementById('avlHeight').textContent = store.tree.height();

  document.getElementById('avlDemoBtn').onclick = () => {
    const start = document.getElementById('avlDemoStart').value;
    const end = document.getElementById('avlDemoEnd').value;
    if (!start && !end) { toast('กรุณาระบุช่วงวันที่'); return; }
    const t0 = performance.now();
    const results = store.searchByDateRange(start, end);
    const t1 = performance.now();
    document.getElementById('avlDemoResult').innerHTML = `
      <div class="hint" style="margin-bottom:10px;">พบ ${results.length} รายการ ใช้เวลา ${(t1 - t0).toFixed(3)} ms (ค้นหาผ่าน AVL Tree range search)</div>
      ${results.slice(0, 10).map(t => txMiniRow(t)).join('') || emptyState('ไม่พบรายการ', 'ลองเลือกช่วงวันที่อื่น')}
    `;
  };

  // Visualization: แสดง 2 ระดับบนของต้นไม้แบบง่าย
  const root = store.tree.root;
  const fmtNode = (n) => n ? new Date(n.key).toLocaleDateString('th-TH', { day: '2-digit', month: '2-digit' }) : '';
  let html = '';
  if (root) {
    html += `<div class="tree-level"><div class="tree-node">${fmtNode(root)}</div></div>`;
    const l2 = [root.left, root.right].filter(Boolean);
    if (l2.length) html += `<div class="tree-level">${l2.map(n => `<div class="tree-node">${fmtNode(n)}</div>`).join('')}</div>`;
    const l3 = [];
    l2.forEach(n => { if (n.left) l3.push(n.left); if (n.right) l3.push(n.right); });
    if (l3.length) html += `<div class="tree-level">${l3.map(n => `<div class="tree-node">${fmtNode(n)}</div>`).join('')}</div>`;
  } else {
    html = '<div class="hint">ไม่มีข้อมูล</div>';
  }
  document.getElementById('avlViz').innerHTML = html;
}

function renderHashTab() {
  const entries = store.hash.entries();
  document.getElementById('hashKeys').textContent = entries.length;
  document.getElementById('hashLoad').textContent = (store.hash.size / store.hash.bucketCount).toFixed(2);

  const snapshot = store.hash.getBucketSnapshot();
  document.getElementById('hashBucketViz').innerHTML = snapshot.length ? snapshot.map(b => `
    <div class="bucket-row">
      <div class="bucket-idx">Bucket ${b.index}</div>
      <div style="display:flex; flex-wrap:wrap; gap:6px;">${b.items.map(i => `<span class="bucket-chip">${i}</span>`).join('')}</div>
    </div>
  `).join('') : `<div class="hint">ไม่มีข้อมูล</div>`;
}

document.getElementById('runBenchBtn').addEventListener('click', () => {
  const keySel = document.getElementById('benchKey').value;
  const data = store.getAll(true);
  const keyFn = keySel === 'amount' ? (t => t.amount) : keySel === 'category' ? (t => t.category) : (t => t.timestamp);

  const qs = quickSort(data, keyFn, 'asc');
  const ms = mergeSort(data, keyFn, 'asc');

  document.getElementById('benchGrid').innerHTML = `
    ${benchCard(qs, 'comparisons', 'swaps', 'Swaps')}
    ${benchCard(ms, 'comparisons', 'merges', 'Merge operations')}
  `;
});

function benchCard(res, cKey, oKey, oLabel) {
  return `<div class="bench-card">
    <h4>${res.algorithm}</h4>
    <div class="bench-row"><span>จำนวนข้อมูล</span><b>${res.n.toLocaleString()}</b></div>
    <div class="bench-row"><span>จำนวนการเปรียบเทียบ</span><b>${res[cKey].toLocaleString()}</b></div>
    <div class="bench-row"><span>${oLabel}</span><b>${res[oKey].toLocaleString()}</b></div>
    <div class="bench-row"><span>เวลาที่ใช้ (Demo Benchmark)</span><b>${res.timeMs.toFixed(3)} ms</b></div>
  </div>`;
}

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
navigate();
