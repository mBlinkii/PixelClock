const { test } = require('node:test');
const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');

function harness(fetchImpl) {
  const elements = new Map();
  const makeElement = (id) => ({ id, value: '', type: 'text', checked: false, hidden: false,
    textContent: '', children: [], classList: { toggle() {}, add() {} },
    append(...els) { this.children.push(...els); }, replaceChildren() { this.children = []; },
    addEventListener() {}, focus() {}, closest() { return { hidden: false }; } });
  const element = (id) => { if (!elements.has(id)) elements.set(id, makeElement(id)); return elements.get(id); };
  const timers = new Map(); let nextTimer = 1; const storage = new Map();
  const context = vm.createContext({
    window: {}, document: { hidden: false, getElementById: element, createElement: makeElement },
    navigator: { language: 'de' }, sessionStorage: { getItem: (key) => storage.get(key), setItem: (key, value) => storage.set(key, value), removeItem: (key) => storage.delete(key) },
    localStorage: { getItem() { return null; } }, storedLanguage: 'de',
    tr: (text) => text, fetch: fetchImpl, URLSearchParams, Headers, AbortController, TextEncoder, TextDecoder, atob, btoa,
    setTimeout(fn, ms) { const id = nextTimer++; timers.set(id, { fn, ms }); return id; },
    clearTimeout(id) { timers.delete(id); }, console
  });
  const source = fs.readFileSync('data/app.js', 'utf8');
  vm.runInContext(source.slice(0, source.indexOf('\ninitUi();')), context);
  return { context, element, timers, run: (code) => vm.runInContext(code, context) };
}

test('API requests are authenticated, uncached and time bounded', async () => {
  let request;
  const h = harness(async (url, options) => { request = { url, options }; return { status: 200 }; });
  h.run('setAuthHeader("Basic test-only")');
  await h.run('apiFetch("/api/status")');
  assert.equal(request.options.headers.get('Authorization'), 'Basic test-only');
  assert.equal(request.options.cache, 'no-store');
  assert.ok(request.options.signal instanceof AbortSignal);
  assert.equal(h.timers.size, 0);
});

test('restart diagnostics show uptime/reason and tolerate older firmware', () => {
  const h = harness();
  h.run('updateRestartDiagnostics({ uptimeMs: 90061000, resetReason: "Task-Watchdog", minFreeHeap: 20480, networkStackFreeBytes: 4096 })');
  assert.equal(h.element('restartStats').hidden, false);
  assert.equal(h.element('restartLine').textContent, '1 d 01:01:01 · Task-Watchdog');
  assert.equal(h.element('memoryLine').textContent, 'Min. freier Speicher: 20 KB · Min. freier Wetter-Stack: 4096 B');
  h.run('updateRestartDiagnostics({})');
  assert.equal(h.element('restartStats').hidden, true);
  assert.equal(h.element('restartLine').textContent, '');
  assert.equal(h.run('formatUptime(0)'), '00:00:00');
  assert.equal(h.run('formatUptime(undefined)'), '-');
});

test('status requests never overlap and failures release the in-flight guard', async () => {
  const h = harness();
  h.run('let count = 0; let finish; fetchStatus = () => { count++; return new Promise(resolve => { finish = resolve; }); };');
  const first = h.run('loadStatus()');
  const second = h.run('loadStatus()');
  assert.equal(first, second);
  assert.equal(h.run('count'), 1);
  h.run('finish()'); await first;
  h.run('fetchStatus = () => Promise.reject(new Error("offline"))');
  await assert.rejects(h.run('loadStatus()'));
  assert.equal(h.run('statusInFlight'), null);
  assert.equal(h.element('connectionState').textContent, 'Uhr nicht erreichbar');
});

test('an expired login stops polling and returns to login', async () => {
  const h = harness(async () => ({ status: 401 }));
  h.element('restartOverlay').hidden = true;
  h.run('setAuthHeader("Basic test-only"); scheduleStatusRefresh()');
  await assert.rejects(h.run('apiFetch("/api/status")'), /Unauthorized/);
  assert.equal(h.run('authHeaderValue()'), '');
  assert.equal(h.element('appShell').hidden, true);
  assert.equal(h.element('loginView').hidden, false);
  assert.equal(h.timers.size, 0);
});

test('polling pauses in hidden tabs, retries at 60 seconds and never runs logged out', () => {
  const h = harness();
  h.element('restartOverlay').hidden = true;
  h.run('setAuthHeader("Basic test-only"); scheduleStatusRefresh()');
  assert.equal([...h.timers.values()][0].ms, 15000);
  h.run('document.hidden = true; scheduleStatusRefresh()');
  assert.equal(h.timers.size, 0);
  h.run('document.hidden = false; statusFailures = 10; scheduleStatusRefresh()');
  assert.equal([...h.timers.values()][0].ms, 60000);
  h.run('setAuthHeader(""); scheduleStatusRefresh()');
  assert.equal(h.timers.size, 0);
});

test('secret values preserve password whitespace; empty keys are omitted', () => {
  const h = harness();
  h.element('brightnessPercentValue').value = '25';
  h.element('nightBrightnessPercentValue').value = '5';
  h.element('hourFormat').value = '24';
  h.element('password').value = ' test password ';
  h.element('adminPassword').value = ' test admin ';
  h.element('weatherApiKey').value = ' api-test-only ';
  const body = h.run('formBody()');
  assert.equal(body.get('password'), ' test password ');
  assert.equal(body.get('adminPassword'), ' test admin ');
  assert.equal(body.get('weatherApiKey'), 'api-test-only');
  assert.equal(body.has('openWeatherApiKey'), false);
});

test('async scan polls to completion, sorts, deduplicates and treats SSIDs as text', async () => {
  const h = harness();
  h.run(`let scans = 0;
    apiFetch = async () => ({ ok: true, json: async () => ++scans === 1 ? { scanning: true } :
      { networks: [{ssid: "<script>test</script>", rssi: -70}, {ssid: "Network", rssi: -40}, {ssid: "Network", rssi: -50}] } });
    setTimeout = (fn) => { fn(); return 1; };`);
  await h.run('scanNetworks()');
  assert.equal(h.run('scans'), 2);
  const items = h.element('networks').children.filter((item) => item.className.startsWith('networkItem'));
  assert.equal(items.length, 2);
  assert.equal(items[0].children[0].textContent, 'Network');
  assert.equal(items[0].children[1].textContent, '-40 dBm');
  assert.equal(items[1].children[0].textContent, '<script>test</script>');
  assert.equal(items[1].children[0].innerHTML, undefined);
});

test('open networks are labelled and signal strength maps to four levels', () => {
  const h = harness();
  h.run('renderNetworks(document.getElementById("list"), [{ ssid: "Guest", rssi: -80, secure: false }], () => {}, "")');
  const item = h.element('list').children[0];
  assert.equal(item.children[1].textContent, 'offen · -80 dBm');
  assert.equal(item.children[1].className, 'networkMeta signal1');
  assert.deepEqual([-50, -60, -70, -90].map((rssi) => h.run(`signalLevel(${rssi})`)), [4, 3, 2, 1]);
});

test('LED index mapping matches the firmware xy() for every wiring and origin', () => {
  const h = harness();
  const index = (x, y, origin, wiring) => h.run(`ledIndex(${x}, ${y}, 32, 8, ${origin}, ${wiring})`);
  // Default: columns, serpentine, top left.
  assert.deepEqual([index(0, 0, 0, 3), index(0, 7, 0, 3), index(1, 7, 0, 3), index(1, 0, 0, 3)], [0, 7, 8, 15]);
  assert.deepEqual([index(0, 0, 0, 0), index(31, 0, 0, 0), index(0, 1, 0, 0)], [0, 31, 32]);
  assert.deepEqual([index(31, 1, 0, 1), index(0, 1, 0, 1)], [32, 63]);
  assert.equal(index(31, 0, 1, 2), 0);
  assert.equal(index(0, 7, 2, 0), 0);
  assert.equal(index(31, 7, 3, 3), 0);
  for (let origin = 0; origin < 4; origin++) {
    for (let wiring = 0; wiring < 4; wiring++) {
      const seen = new Set();
      for (let y = 0; y < 8; y++) for (let x = 0; x < 32; x++) seen.add(index(x, y, origin, wiring));
      assert.equal(seen.size, 256, `origin ${origin}, wiring ${wiring} must be a permutation`);
    }
  }
});

test('names are normalized like the firmware hostname sanitizer', () => {
  const h = harness();
  assert.equal(h.run('sanitizeName("  Pixel Clock-Küche! ")'), 'pixelclock-kche');
  assert.equal(h.run('sanitizeName("--Wohnzimmer--")'), 'wohnzimmer');
  assert.equal(h.run('sanitizeName("")'), 'pixelclock');
  assert.equal(h.run('sanitizeName("", "")'), '');
  assert.equal(h.run('sanitizeName("a".repeat(40)).length'), 31);
});

test('a changed admin login keeps the session with the new credentials', () => {
  const h = harness();
  h.run('savedConfig = {}; setAuthHeader(basicAuthValue("admin", "test-old:pw"))');
  assert.deepEqual({ ...h.run('decodeBasicAuth(authHeaderValue())') }, { username: 'admin', password: 'test-old:pw' });
  h.run('adoptChangedLogin(new URLSearchParams("adminUsername=Admin&adminPassword=test-new-pw"))');
  assert.deepEqual({ ...h.run('decodeBasicAuth(authHeaderValue())') }, { username: 'admin', password: 'test-new-pw' });
  h.run('adoptChangedLogin(new URLSearchParams("adminUsername=Kitchen"))');
  assert.deepEqual({ ...h.run('decodeBasicAuth(authHeaderValue())') }, { username: 'kitchen', password: 'test-new-pw' });
  assert.equal(h.element('adminUsername').value, 'kitchen');
});

test('live frames map physical LED order back to rows and columns', () => {
  const h = harness();
  // 4x2 matrix, column serpentine from the top left.
  const pixels = ['aa0000', 'bb0000', 'cc0000', 'dd0000', 'ee0000', 'ff0000', '110000', '220000'].join('');
  const grid = h.run(`frameToGrid({ width: 4, height: 2, count: 8, origin: 0, wiring: 3, pixels: "${pixels}" })`);
  assert.deepEqual([...grid.colors], ['aa0000', 'dd0000', 'ee0000', '220000', 'bb0000', 'cc0000', 'ff0000', '110000']);
  const partial = h.run(`frameToGrid({ width: 4, height: 2, count: 3, origin: 0, wiring: 3, pixels: "${pixels.slice(0, 18)}" })`);
  assert.deepEqual([...partial.colors], ['aa0000', '000000', '000000', '000000', 'bb0000', 'cc0000', '000000', '000000']);
});

test('settings export leaves out secrets and import accepts only known fields', () => {
  const h = harness();
  h.run('savedForm = "ssid=Home&brightness=64&password=test-secret&setupApPassword=test-ap&language=de"; currentLanguage = "en"');
  const exported = JSON.parse(h.run('JSON.stringify(buildSettingsExport())'));
  assert.equal(exported.format, 'pixel-clock-settings');
  assert.deepEqual(exported.settings, { ssid: 'Home', brightness: '64', language: 'en' });
  const file = JSON.stringify({ format: 'pixel-clock-settings', settings: {
    brightness: 80, autoPage: true, password: 'x', adminPassword: 'y', unknown: 1, colorText: { nested: true }, language: 'en' } });
  const parsed = JSON.parse(h.run(`JSON.stringify(parseSettingsImport(${JSON.stringify(file)}))`));
  assert.deepEqual(parsed, { values: { brightness: '80', autoPage: 'true' }, language: 'en' });
  assert.throws(() => h.run('parseSettingsImport("{}")'), /gültige/);
  assert.throws(() => h.run('parseSettingsImport("not json")'), /gültige/);
});

test('setup network password is only sent when entered; preview detects visual changes', () => {
  const h = harness();
  assert.equal(h.run('formBody()').has('setupApPassword'), false);
  h.element('setupApPassword').value = 'test-ap-pass';
  assert.equal(h.run('formBody()').get('setupApPassword'), 'test-ap-pass');
  h.run('savedForm = formBody().toString()');
  assert.equal(h.run('previewDiffersFromSaved(formBody())'), false);
  h.element('ssid').value = 'Other';
  assert.equal(h.run('previewDiffersFromSaved(formBody())'), false);
  h.element('colorText').value = '#ff0000';
  assert.equal(h.run('previewDiffersFromSaved(formBody())'), true);
});

test('a locked login shows the remaining wait time', async () => {
  const h = harness(async () => ({ ok: false, status: 429, json: async () => ({ retryAfterSeconds: 42 }) }));
  h.element('loginUsername').value = 'admin';
  h.element('loginPassword').value = 'x';
  await h.run('login({ preventDefault() {} })');
  assert.equal(h.element('loginMessage').textContent, 'Zu viele Fehlversuche. Bitte in 42 Sekunden erneut versuchen.');
});

test('the assistant opens on its own only while the clock has no saved Wi-Fi', () => {
  const h = harness();
  h.run('savedConfig = { ssid: "", adminPasswordIsDefault: true }');
  assert.equal(h.run('isFirstSetup()'), true);
  h.run('savedConfig = { ssid: "Home", adminPasswordIsDefault: true }; lastStatus = { setupMode: true }');
  assert.equal(h.run('isFirstSetup()'), false);
  h.run('savedConfig = { ssid: "Home", adminPasswordIsDefault: false }; lastStatus = { setupMode: false }');
  assert.equal(h.run('isFirstSetup()'), false);
  assert.equal(h.run('setupIncomplete()'), false);
});

test('the Open-Meteo model is saved with the form and named in the weather card', () => {
  const h = harness();
  h.element('weatherModel').value = 'icon_seamless';
  assert.equal(h.run('formBody()').get('weatherModel'), 'icon_seamless');
  assert.equal(h.run('weatherModelLabel("icon_seamless")'), 'DWD ICON');
  assert.equal(h.run('weatherModelLabel("")'), '');
  assert.equal(h.run('weatherModelLabel("unknown")'), '');
  const ids = h.run('openMeteoModels.map(([id]) => id).join()');
  const firmware = fs.readFileSync('src/weather_models.h', 'utf8');
  const firmwareIds = [...firmware.matchAll(/^\s*"([a-z0-9_]+)",/gm)].map((match) => match[1]);
  assert.deepEqual(ids.split(',').slice(1), firmwareIds);
});

test('a new clock logs in with the default credentials; others keep the login page', async () => {
  const fresh = harness(async () => ({ ok: true, json: async () => ({ firstSetup: true, adminUsername: 'admin' }) }));
  assert.equal(await fresh.run('tryFirstSetupLogin()'), true);
  assert.deepEqual({ ...fresh.run('decodeBasicAuth(authHeaderValue())') }, { username: 'admin', password: 'pixelclock' });
  const configured = harness(async () => ({ ok: true, json: async () => ({ firstSetup: false }) }));
  assert.equal(await configured.run('tryFirstSetupLogin()'), false);
  assert.equal(configured.run('authHeaderValue()'), '');
  const oldFirmware = harness(async () => ({ ok: false, status: 401 }));
  assert.equal(await oldFirmware.run('tryFirstSetupLogin()'), false);
});

test('password recovery checks code and new password before sending', () => {
  const { run } = harness(async () => ({ ok: true, json: async () => ({}) }));
  assert.match(run('recoveryInputError("12345", "longenough", "longenough")'), /6-stelligen Code/);
  assert.match(run('recoveryInputError("123456", "short", "short")'), /mindestens 8/);
  assert.match(run('recoveryInputError("123456", "pixelclock", "pixelclock")'), /Standardpasswort/);
  assert.match(run('recoveryInputError("123456", "longenough", "different1")'), /nicht überein/);
  assert.equal(run('recoveryInputError("123456", "longenough", "longenough")'), '');
});
