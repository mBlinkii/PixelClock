// Local browser QA only. All values and credentials are synthetic.
import http from 'node:http';
import { readFile } from 'node:fs/promises';
import { resolve, extname } from 'node:path';
const root = resolve('data');
const config = {
  ssid: 'PixelClock-Test', wifiCountry: 'DE', hostname: 'pixelclock', cityName: 'Berlin',
  locationLabel: 'Berlin, Deutschland', timezone: 'CET-1CEST,M3.5.0,M10.5.0/3', language: 'de',
  weatherProvider: 0, weatherProviderMax: 4, weatherIntervalHalfHours: 4,
  hasOpenWeatherApiKey: false, hasWeatherApiKey: false, wifiPowerSave: true,
  width: 32, height: 8, dataPin: 18, brightness: 64, fullBrightnessUnlocked: false,
  wiringMode: 3, origin: 0, displayMode: 0, colorOrder: 'GRB', temperatureUnit: 0,
  weatherIconEnabled: true, hourFormat: 24, colorWeekday: '#50B4FF', colorText: '#FFF5BE',
  colorPoint: '#50B4FF', colorColon: '#F8D66D', timePageSeconds: 8, pageSeconds: 8,
  colorGradientMode: 0, autoPage: true, selectedPage: 0, nightBrightness: 16, nightStart: 22, nightEnd: 7,
  adminUsername: 'admin', adminPasswordIsDefault: false, url: 'http://pixelclock.local'
};
let scans = 0;
http.createServer(async (req, res) => {
  const url = new URL(req.url, 'http://localhost');
  const reply = (code, value) => { res.writeHead(code, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' }); res.end(JSON.stringify(value)); };
  if (url.pathname.startsWith('/api/')) {
    if (!req.headers.authorization) { reply(401, { error: 'Admin-Anmeldung erforderlich.' }); return; }
    if (url.pathname === '/api/config' && req.method === 'GET') { reply(200, config); return; }
    if (url.pathname === '/api/status') {
      reply(200, { wifiConnected: true, setupMode: false, ip: '192.0.2.10', url: config.url,
        localTime: '2026-09-11 15:45:00', cityName: config.cityName, locationLabel: config.locationLabel,
        temperature: 22.4, temperatureMin: 16, temperatureMax: 25, temperatureUnit: 'C', weatherCode: 2,
        weatherProvider: ['Open-Meteo', 'OpenWeatherMap', 'DWD (Bright Sky)', 'MET Norway', 'WeatherAPI'][config.weatherProvider],
        weatherAgeMs: 360000, freeHeap: 156000, rssi: -48, wifiPowerSave: config.wifiPowerSave,
        firmwareVersion: '0.1.16', capabilities: { asyncWifiScan: true, weatherProviderMax: 4, wifiPowerSave: true } }); return;
    }
    if (url.pathname === '/api/networks') {
      reply(200, ++scans % 3 ? { scanning: true } : { networks: [{ ssid: 'Test-Netz', rssi: -45 }, { ssid: 'Test-Netz', rssi: -60 }, { ssid: '<Test & WiFi>', rssi: -75 }] }); return;
    }
    let body = ''; for await (const chunk of req) body += chunk;
    if (url.pathname === '/api/config') {
      for (const [key, value] of new URLSearchParams(body)) {
        if (/password|key/i.test(key)) continue;
        config[key] = typeof config[key] === 'boolean' ? value === '1' : typeof config[key] === 'number' ? Number(value) : value;
      }
      reply(200, { ok: true, weatherRefreshPending: true }); return;
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
}).listen(8765, '127.0.0.1', () => console.log('Mock UI: http://127.0.0.1:8765 (any test login)'));
