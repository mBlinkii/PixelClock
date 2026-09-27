const { test } = require('node:test');
const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');

function harness(fetchImpl) {
  const elements = new Map();
  const makeElement = (id) => ({ id, value: '', type: 'text', checked: false, hidden: false,
    textContent: '', children: [], classList: { toggle() {}, add() {} },
    append(el) { this.children.push(el); }, replaceChildren() { this.children = []; },
    addEventListener() {}, focus() {}, closest() { return { hidden: false }; } });
  const element = (id) => { if (!elements.has(id)) elements.set(id, makeElement(id)); return elements.get(id); };
  const timers = new Map(); let nextTimer = 1; const storage = new Map();
  const context = vm.createContext({
    window: {}, document: { hidden: false, getElementById: element, createElement: makeElement },
    navigator: { language: 'de' }, sessionStorage: { getItem: (key) => storage.get(key), setItem: (key, value) => storage.set(key, value), removeItem: (key) => storage.delete(key) },
    localStorage: { getItem() { return null; } }, storedLanguage: 'de',
    tr: (text) => text, fetch: fetchImpl, URLSearchParams, Headers, AbortController, TextEncoder,
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
  assert.equal(h.element('networks').children.length, 2);
  assert.equal(h.element('networks').children[0].textContent, 'Network · -40 dBm');
  assert.equal(h.element('networks').children[1].textContent, '<script>test</script> · -70 dBm');
});
