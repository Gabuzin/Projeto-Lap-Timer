// Run with: node test/web_recording.test.mjs
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import vm from 'node:vm';

const source = await readFile(new URL('../include/web_assets.h', import.meta.url), 'utf8');
const script = source.match(/R"LAP_APP_JS\(([\s\S]*?)\)LAP_APP_JS"/)[1];
const sessionForm = source.match(/<form id="sessionForm"[\s\S]*?<\/form>/)[0];
const settingsForm = source.match(/<form id="settingsForm"[\s\S]*?<\/form>/)[0];
assert.match(sessionForm, /id="activeTrack"/);
assert.match(sessionForm, /id="minLap"/);
assert.doesNotMatch(settingsForm, /id="(?:activeTrack|minLap)"/);
assert.match(settingsForm, /id="devicePassword"/);

const elements = new Map();
function element(id) {
  if (!elements.has(id)) elements.set(id, {
    value: '', textContent: '', innerHTML: '', disabled: false, style: {},
    classList: { add() {}, remove() {}, toggle() {} },
    scrollIntoView() {}, events: {},
    addEventListener(name, handler) { this.events[name] = handler; },
  });
  return elements.get(id);
}
let status = { storageReady: true, gpsFix: false, sessionState: 'idle',
  recordingRequested: false, sessionId: '', device: 'Lap Timer' };
let settings = { activeTrack: 'track_1', minLapSeconds: 15, deviceName: 'Lap Timer' };
const requests = [];
let failStart = false;
let failStop = false;
let offline = false;
let poll;
const context = vm.createContext({
  URLSearchParams, console,
  document: { getElementById: element, querySelectorAll: () => [] },
  setTimeout() {}, setInterval(callback) { poll = callback; },
  fetch: async (url, options = {}) => {
    if (offline) throw Error('Sem conexão');
    const body = Object.fromEntries(new URLSearchParams(options.body));
    requests.push({ url, method: options.method || 'GET', body });
    let result;
    let error;
    if (url === '/api/recording/start') {
      if (failStart) error = 'Armazenamento indisponivel.';
      else {
        settings = { ...settings, ...body };
        status = { ...status, recordingRequested: true, sessionState: 'waiting_gps' };
      }
      result = status;
    } else if (url === '/api/recording/stop') {
      if (failStop) {
        error = 'Falha ao finalizar. Tente Parar novamente.';
        status = { ...status, recordingRequested: false, sessionState: 'stop_error' };
      } else status = { ...status, recordingRequested: false, sessionId: '', sessionState: 'idle' };
      result = status;
    } else if (url === '/api/status') result = status;
    else if (url === '/api/settings') {
      if (options.method === 'POST') settings = { ...settings, ...body };
      result = settings;
    } else if (url === '/api/tracks') result = [{ id: 'track_1', name: 'Pista de teste' }];
    else if (url === '/api/sessions') result = [];
    else throw Error('Unexpected endpoint: ' + url);
    return { ok: !error, status: error ? 409 : 200,
      json: async () => structuredClone(result),
      text: async () => JSON.stringify({ error }) };
  },
});
vm.runInContext(script, context);
await new Promise(setImmediate);
const submit = (id) => element(id).events.submit({ preventDefault() {} });
assert.equal(requests.filter((r) => r.method === 'POST').length, 0, 'Loading must not start recording');
assert.equal(element('startSession').disabled, false);
assert.equal(element('stopSession').disabled, true);

element('activeTrack').value = 'track_1';
element('minLap').value = '20';
await Promise.all([submit('sessionForm'), submit('sessionForm')]);
assert.equal(requests.filter((r) => r.url === '/api/recording/start').length, 1, 'Double click must send one start');
assert.deepEqual(requests.find((r) => r.url === '/api/recording/start').body,
  { activeTrack: 'track_1', minLapSeconds: '20' });
assert.match(element('recordingStatus').textContent, /Aguardando GPS/);
assert.equal(element('startSession').disabled, true);
assert.equal(element('stopSession').disabled, false, 'Waiting for GPS must be cancellable');
assert.equal(element('activeTrack').disabled, true);

status = { ...status, gpsFix: true, sessionId: 'session_001', sessionState: 'recording' };
await poll();
assert.match(element('recordingStatus').textContent, /Gravando.*session_001/);
element('deviceName').value = 'Novo nome';
element('devicePassword').value = '12345678';
await submit('settingsForm');
assert.deepEqual(requests.find((r) => r.url === '/api/settings' && r.method === 'POST').body,
  { deviceName: 'Novo nome', devicePassword: '12345678' }, 'Wi-Fi settings must not change session selection');
assert.equal(status.recordingRequested, true);

status = { ...status, gpsFix: false, sessionState: 'gps_lost' };
await poll();
assert.match(element('recordingStatus').textContent, /retorno do GPS/);
failStop = true;
await element('stopSession').events.click();
assert.match(element('recordingStatus').textContent, /finalização falhou/);
assert.equal(element('stopSession').disabled, false, 'Failed finalization must allow retry');
assert.equal(element('startSession').disabled, true);
failStop = false;
await element('stopSession').events.click();
assert.equal(element('stopSession').disabled, true);
assert.equal(element('startSession').disabled, false);
assert.equal(element('activeTrack').disabled, false);

failStart = true;
await submit('sessionForm');
assert.equal(element('message').textContent, 'Armazenamento indisponivel.');
assert.equal(element('startSession').disabled, false);
offline = true;
await poll();
assert.equal(element('startSession').disabled, true);
assert.equal(element('stopSession').disabled, true);
assert.match(element('recordingStatus').textContent, /Sem comunicação/);
console.log('PASS: separate forms; manual start/stop; waiting GPS; double click; Wi-Fi isolation; errors and reconnect controls.');
