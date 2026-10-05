const { app, BrowserWindow, dialog } = require('electron');
const path = require('path');

let server;

async function startServer() {
  process.env.PORT = '0';
  process.env.SMARTBUDGET_DB_PATH = path.join(app.getPath('userData'), 'smartbudget.db');
  // Load the existing Express server. Exporting the server is handled below.
  const serverModule = require('./backend/server');
  if (serverModule && typeof serverModule.start === 'function') {
    server = await serverModule.start(0);
  } else {
    throw new Error('Backend server ไม่สามารถเริ่มแบบ Desktop ได้');
  }
  return server;
}

async function createWindow() {
  await startServer();
  const port = server.address().port;

  const win = new BrowserWindow({
    width: 1400,
    height: 900,
    minWidth: 1000,
    minHeight: 700,
    show: false,
    autoHideMenuBar: true,
    backgroundColor: '#f7f7f5',
    webPreferences: {
      contextIsolation: true,
      nodeIntegration: false,
      sandbox: true
    }
  });

  win.once('ready-to-show', () => win.show());
  await win.loadURL(`http://127.0.0.1:${port}/index.html`);
}

app.whenReady().then(async () => {
  try {
    await createWindow();
  } catch (err) {
    console.error(err);
    dialog.showErrorBox('SmartBudget', `ไม่สามารถเปิดโปรแกรมได้\n\n${err.message}`);
    app.quit();
  }
});

app.on('window-all-closed', () => {
  if (server) server.close();
  if (process.platform !== 'darwin') app.quit();
});

app.on('before-quit', () => {
  if (server) server.close();
});
