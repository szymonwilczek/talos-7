const { app, BrowserWindow, shell } = require("electron");
const path = require("path");

let mainWindow;

const isDev = !app.isPackaged;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1280,
    height: 800,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true,
      preload: path.join(__dirname, "preload.js"),
    },
    autoHideMenuBar: true,
    show: false,
  });

  const productionURL = "https://talos-7.vercel.app/";
  const devURL = "http://localhost:3000";

  const startUrl =
    process.env.ELECTRON_START_URL || (isDev ? devURL : productionURL);

  console.log(`[Talos] Loading URL: ${startUrl}`);
  mainWindow.loadURL(startUrl);

  mainWindow.once("ready-to-show", () => {
    mainWindow.show();
  });

  if (isDev) {
    mainWindow.webContents.openDevTools();
  } else {
    mainWindow.webContents.on("before-input-event", (event, input) => {
      if (input.control && input.shift && input.key.toLowerCase() === "i") {
        event.preventDefault();
      }
    });
  }

  mainWindow.webContents.setWindowOpenHandler(({ url }) => {
    if (url.startsWith("https:") || url.startsWith("http:")) {
      shell.openExternal(url);
    }
    return { action: "deny" };
  });

  // --- WEB SERIAL API ---

  // Raspberry Pi Vendor ID = 0x2E8A = 11914
  mainWindow.webContents.session.on(
    "select-serial-port",
    (event, portList, callback) => {
      event.preventDefault();

      // Pico
      const talosPort = portList.find((device) => {
        return device.vendorId === "11914";
      });

      if (talosPort) {
        callback(talosPort.portId);
      } else {
        callback("");
      }
    },
  );

  mainWindow.webContents.session.setPermissionCheckHandler(
    (permission, requestingOrigin) => {
      // Serial API for localhost AND production URL
      const allowedOrigins = [
        "http://localhost:3000",
        "https://talos-7.vercel.app",
      ];
      if (
        permission === "serial" &&
        allowedOrigins.some((origin) => requestingOrigin.startsWith(origin))
      ) {
        return true;
      }
      return false;
    },
  );

  mainWindow.webContents.session.setDevicePermissionHandler((details) => {
    return details.deviceType === "serial";
  });

  mainWindow.on("closed", function () {
    mainWindow = null;
  });
}

app.on("ready", createWindow);

app.on("window-all-closed", function () {
  if (process.platform !== "darwin") {
    app.quit();
  }
});

app.on("activate", function () {
  if (mainWindow === null) {
    createWindow();
  }
});
