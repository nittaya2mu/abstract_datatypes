/**
 * theme.js
 * สลับโหมดสว่าง (light) / โหมดมืด (dark)
 * บันทึกค่าไว้ใน localStorage เพื่อให้จำโหมดที่เลือกไว้ข้ามการเปิดใช้งานครั้งถัดไป
 * ไฟล์นี้ต้องถูกโหลดเร็วที่สุด (วางไว้ใน <head>) เพื่อตั้งค่า theme ก่อนหน้าเว็บ render
 * ป้องกันอาการหน้าจอกะพริบสลับสีตอนโหลด (Flash of Unstyled Theme)
 */

const THEME_KEY = 'smartbudget_theme';

const Theme = {
  get() {
    return localStorage.getItem(THEME_KEY) || 'light';
  },
  set(theme) {
    localStorage.setItem(THEME_KEY, theme);
    document.documentElement.setAttribute('data-theme', theme);
    this.syncUI();
  },
  toggle() {
    const next = this.get() === 'dark' ? 'light' : 'dark';
    this.set(next);
    return next;
  },
  // อัปเดตไอคอน/ข้อความบนปุ่มสลับโหมดให้ตรงกับ theme ปัจจุบัน (เรียกหลัง DOM พร้อมแล้ว)
  syncUI() {
    const theme = this.get();
    document.querySelectorAll('[data-theme-icon]').forEach(el => {
      el.textContent = theme === 'dark' ? '☀' : '☾';
    });
    document.querySelectorAll('[data-theme-label]').forEach(el => {
      el.textContent = theme === 'dark' ? 'โหมดสว่าง' : 'โหมดมืด';
    });
  },
  init() {
    // ตั้งค่า attribute ทันทีตอนโหลด (ก่อน body render) เพื่อไม่ให้จอกะพริบ
    document.documentElement.setAttribute('data-theme', this.get());
    document.addEventListener('DOMContentLoaded', () => this.syncUI());
  }
};

Theme.init();
