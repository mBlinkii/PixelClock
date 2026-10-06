// Local browser QA only. All values and credentials are synthetic.
import http from 'node:http';
import { readFile } from 'node:fs/promises';
import { resolve, extname } from 'node:path';
const root = resolve('data');
const portArg = process.argv.find((arg) => arg.startsWith('--port='));
const port = Number(portArg?.slice(7) || process.env.PIXEL_CLOCK_MOCK_PORT || 8765);
// --first-run (or PIXEL_CLOCK_MOCK_FIRST_RUN=1) simulates a fresh clock: setup AP, no Wi-Fi, default login.
const firstRun = process.argv.includes('--first-run') || process.env.PIXEL_CLOCK_MOCK_FIRST_RUN === '1';
const restartKeys = new Set(['ssid', 'password', 'wifiCountry', 'hostname', 'width', 'height', 'dataPin', 'colorOrder']);
let setupMode = firstRun;
const config = {
  ssid: firstRun ? '' : 'PixelClock-Test', hasPassword: !firstRun, wifiCountry: 'DE', hostname: 'pixelclock', cityName: 'Berlin',
  locationLabel: 'Berlin, Deutschland', timezone: 'CET-1CEST,M3.5.0,M10.5.0/3', language: 'de',
  weatherProvider: 0, weatherModel: '', weatherProviderMax: 4, weatherIntervalHalfHours: 4,
  hasOpenWeatherApiKey: false, hasWeatherApiKey: false, wifiPowerSave: true,
  width: 32, height: 8, dataPin: 18, brightness: 64, fullBrightnessUnlocked: false,
  wiringMode: 3, origin: 0, displayMode: 0, colorOrder: 'GRB', temperatureUnit: 0,
  weatherIconEnabled: true, hourFormat: 24, colorWeekday: '#50B4FF', colorText: '#FFF5BE',
  colorPoint: '#50B4FF', colorColon: '#F8D66D', timePageSeconds: 16, pageSeconds: 8,
  colorGradientMode: 0, autoPage: true, selectedPage: 0, nightBrightness: 16, nightStart: 22, nightEnd: 7,
  adminUsername: 'admin', adminPasswordIsDefault: firstRun, minAdminPasswordLength: 8, maxAdminPasswordLength: 64,
  url: 'http://pixelclock.local', restartRequired: false, setupApSsid: 'PixelClock-Setup-A1B2C3',
  setupApPasswordIsDefault: true, routerHostname: 'pixelclock-A1B2C3'
};
let scans = 0;
let preview = null;
const glyphs = {
  0: '111101101101111', 1: '010110010010111', 2: '111001111100111', 3: '111001111001111', 4: '101101111001001',
  5: '111100111001111', 6: '111100111101111', 7: '111001001010010', 8: '111101111101111', 9: '111101111001111',
  ':': '000010000010000'
};
// Same physical order as xy() in src/display.cpp.
function ledIndex(x, y, width, height, origin, wiring) {
  let px = origin === 1 || origin === 3 ? width - 1 - x : x;
  let py = origin === 2 || origin === 3 ? height - 1 - y : y;
  if (wiring === 2 || wiring === 3) {
    if (wiring === 3 && px % 2 === 1) py = height - 1 - py;
    return px * height + py;
  }
  if (wiring === 1 && py % 2 === 1) px = width - 1 - px;
  return py * width + px;
}
// Draws a clock page roughly like the firmware so the live view has real content.
function renderFrame() {
  const c = { ...config, ...(preview || {}) };
  const width = Number(c.width), height = Number(c.height), count = Math.min(512, width * height);
  const pixels = new Array(count).fill('000000');
  const hex = (color) => String(color).replace('#', '').toLowerCase();
  const set = (x, y, color) => {
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    const index = ledIndex(x, y, width, height, Number(c.origin), Number(c.wiringMode));
    if (index < count) pixels[index] = color;
  };
  for (const [x, y] of [[4, 1], [3, 2], [4, 2], [5, 2], [2, 3], [3, 3], [4, 3], [5, 3], [6, 3], [3, 4], [4, 4], [5, 4], [4, 5]]) set(x, y, 'ffbe14');
  const time = Number(c.hourFormat) === 12 ? ' 3:45' : '15:45';
  let cursor = 13;
  for (const char of time) {
    const glyph = glyphs[char];
    if (glyph) {
      for (let i = 0; i < 15; i++) {
        if (glyph[i] === '1') set(cursor + (i % 3), 1 + Math.floor(i / 3), hex(char === ':' ? c.colorColon : c.colorText));
      }
    }
    cursor += char === ':' || char === ' ' ? 2 : 4;
  }
  for (let i = 0; i < 3; i++) for (let x = 0; x < 3; x++) set(15 + i * 5 + x, height - 1, i === 0 ? hex(c.colorPoint) : '121212');
  return { width, height, count, origin: Number(c.origin), wiring: Number(c.wiringMode), brightness: Number(c.brightness),
    preview: Boolean(preview), pixels: pixels.join('') };
}
let locationPending = false, locationError = '';
http.createServer(async (req, res) => {
  const url = new URL(req.url, 'http://localhost');
  const reply = (code, value) => { res.writeHead(code, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' }); res.end(JSON.stringify(value)); };
  if (url.pathname === '/api/recovery/start' && req.method === 'POST') {
    reply(200, { ok: true, expiresInSeconds: 300 }); return;
  }
  if (url.pathname === '/api/recovery/finish' && req.method === 'POST') {
    let body = ''; for await (const chunk of req) body += chunk;
    // The mock always shows code 123456.
    if (new URLSearchParams(body).get('code') !== '123456') { reply(403, { ok: false, error: 'Der Code stimmt nicht.' }); return; }
    config.adminPasswordIsDefault = false;
    reply(200, { ok: true, adminUsername: config.adminUsername }); return;
  }
  if (url.pathname === '/api/setup') {
    const firstSetup = !config.ssid && config.adminPasswordIsDefault;
    reply(200, firstSetup ? { firstSetup, adminUsername: config.adminUsername } : { firstSetup }); return;
  }
  if (url.pathname.startsWith('/api/')) {
    if (!req.headers.authorization) { reply(401, { error: 'Admin-Anmeldung erforderlich.' }); return; }
    // A login with password "test-throttle" simulates the firmware's failed-login lock.
    if (Buffer.from(req.headers.authorization.replace(/^Basic /, ''), 'base64').toString().endsWith(':test-throttle')) {
      reply(429, { ok: false, error: 'Zu viele Fehlversuche. Bitte später erneut versuchen.', retryAfterSeconds: 30 }); return;
    }
    if (url.pathname === '/api/config' && req.method === 'GET') { reply(200, config); return; }
    if (url.pathname === '/api/status') {
      reply(200, { wifiConnected: !setupMode, setupMode, ip: setupMode ? '192.168.4.1' : '192.0.2.10', url: config.url, lastNtpMs: setupMode ? 0 : 1000,
        localTime: '2026-09-11 15:45:00', cityName: config.cityName, locationLabel: config.locationLabel, locationPending, ...(locationError ? { locationError } : {}),
        temperature: 22.4, temperatureMin: 16, temperatureMax: 25, temperatureUnit: 'C', weatherCode: 2,
        weatherProvider: ['Open-Meteo', 'OpenWeatherMap', 'DWD (Bright Sky)', 'MET Norway', 'WeatherAPI'][config.weatherProvider],
        weatherAgeMs: 360000, freeHeap: 156000, rssi: -48, wifiPowerSave: config.wifiPowerSave,
        firmwareVersion: '0.1.22', uptimeMs: 90061000, resetReason: 'Task-Watchdog',
        minFreeHeap: 84000, networkStackFreeBytes: 4096, setupApSsid: config.setupApSsid,
        routerHostname: config.routerHostname, displayPreviewActive: Boolean(preview), weatherModel: config.weatherModel,
        capabilities: { asyncWifiScan: true, weatherProviderMax: 4, wifiPowerSave: true, captivePortal: true,
          setupApPassword: true, setupTestPattern: true, displayFrame: true, displayPreview: true,
          fullFactoryReset: true, resetButton: true, loginThrottle: true } }); return;
    }
    if (url.pathname === '/api/networks') {
      reply(200, ++scans % 3 ? { scanning: true } : { networks: [{ ssid: 'Test-Netz', rssi: -45, secure: true }, { ssid: 'Test-Netz', rssi: -60, secure: true },
        { ssid: '<Test & WiFi>', rssi: -75, secure: true }, { ssid: 'Gast-Netz', rssi: -68, secure: false }] }); return;
    }
    if (url.pathname === '/api/display/frame') { reply(200, renderFrame()); return; }
    let body = ''; for await (const chunk of req) body += chunk;
    if (url.pathname === '/api/display/preview/cancel') { preview = null; reply(200, { ok: true }); return; }
    if (url.pathname === '/api/display/preview') {
      preview = Object.fromEntries(new URLSearchParams(body));
      reply(200, { ok: true, previewSeconds: 120 }); return;
    }
    if (url.pathname === '/api/reset/factory') {
      reply(200, { ok: true, fullWipe: true, setupApSsid: config.setupApSsid }); return;
    }
    if (url.pathname === '/api/config') {
      preview = null;
      let authChanged = false;
      const oldCity = config.cityName;
      for (const [key, value] of new URLSearchParams(body)) {
        const changed = String(config[key]) !== value;
        if (key === 'adminPassword') { config.adminPasswordIsDefault = false; authChanged = true; }
        if (key === 'adminUsername' && value !== config.adminUsername) authChanged = true;
        if (key === 'password') config.hasPassword = true;
        if (key === 'openWeatherApiKey') config.hasOpenWeatherApiKey = true;
        if (key === 'weatherApiKey') config.hasWeatherApiKey = true;
        if (key === 'setupApPassword') config.setupApPasswordIsDefault = false;
        if (restartKeys.has(key) && (key === 'password' || changed)) config.restartRequired = true;
        if (/password|key/i.test(key)) continue;
        config[key] = typeof config[key] === 'boolean' ? value === '1' : typeof config[key] === 'number' ? Number(value) : value;
      }
      config.url = `http://${config.hostname}.local`;
      const cityChanged = config.cityName !== oldCity;
      if (cityChanged) {
        locationPending = true; locationError = '';
        setTimeout(() => {
          const code = config.cityName.match(/\b\d{4,5}\b/)?.[0];
          if (code === '00000') { locationError = 'Postleitzahl nicht gefunden'; return; }
          config.locationLabel = code ? `${code} Teststadt, Deutschland` : `${config.cityName}, Deutschland`;
          locationPending = false;
        }, 3000);
      }
      reply(200, { ok: true, weatherRefreshPending: true, cityResolutionPending: cityChanged, restartRequired: config.restartRequired, authChanged,
        hostname: config.hostname, url: config.url, locationLabel: config.locationLabel,
        adminPasswordIsDefault: config.adminPasswordIsDefault, setupApPasswordIsDefault: config.setupApPasswordIsDefault }); return;
    }
    if (url.pathname === '/api/restart') {
      setupMode = !config.ssid; config.restartRequired = false;
      reply(200, { ok: true }); return;
    }
    reply(200, { ok: true }); return;
  }
  const path = resolve(root, '.' + (url.pathname === '/' ? '/index.html' : url.pathname));
  if (!path.startsWith(root + '/') && !path.startsWith(root + '\\')) { res.writeHead(403); res.end(); return; }
  try {
    const data = await readFile(path);
    res.writeHead(200, { 'Content-Type': ({ '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8', '.css': 'text/css; charset=utf-8', '.svg': 'image/svg+xml' })[extname(path)] || 'application/octet-stream' });
    res.end(data);
  } catch { res.writeHead(404); res.end(); }
}).listen(port, '127.0.0.1', () => console.log(`Mock UI: http://127.0.0.1:${port} (any test login)`));
