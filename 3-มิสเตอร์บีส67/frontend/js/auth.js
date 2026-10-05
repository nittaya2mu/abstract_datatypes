/**
 * auth.js
 * ระบบ Login แบบ Demo (เก็บ session ใน localStorage)
 * บัญชี Demo: username = student, password = 1234
 */

const AUTH_KEY = 'smartbudget_session';
const DEMO_USER = { username: 'student', password: '1234', name: 'นิสิต Demo' };

const Auth = {
  login(username, password) {
    if (username === DEMO_USER.username && password === DEMO_USER.password) {
      const session = { username, name: DEMO_USER.name, loginAt: new Date().toISOString() };
      localStorage.setItem(AUTH_KEY, JSON.stringify(session));
      return { ok: true, session };
    }
    return { ok: false, message: 'Username หรือ Password ไม่ถูกต้อง' };
  },

  logout() {
    localStorage.removeItem(AUTH_KEY);
  },

  currentSession() {
    const raw = localStorage.getItem(AUTH_KEY);
    return raw ? JSON.parse(raw) : null;
  },

  isLoggedIn() {
    return !!this.currentSession();
  },

  requireAuth() {
    if (!this.isLoggedIn()) {
      window.location.href = 'index.html';
    }
  }
};
