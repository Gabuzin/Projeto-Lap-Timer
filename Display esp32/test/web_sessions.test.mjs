// Run with: node test/web_sessions.test.mjs
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import vm from 'node:vm';

const source = await readFile(new URL('../include/web_assets.h', import.meta.url), 'utf8');
const script = source.match(/R"LAP_APP_JS\(([\s\S]*?)\)LAP_APP_JS"/)[1];
new vm.Script(script); // Check syntax of the complete embedded application.
const escape = script.slice(script.indexOf('const safe ='), script.indexOf('function msg('));
const render = script.slice(script.indexOf('function renderSessions('), script.indexOf('function renderSettings('));
const list = { innerHTML: '' };
const context = vm.createContext({ sessions: [], $: () => list });
vm.runInContext(escape + render, context);

vm.runInContext('renderSessions()', context);
assert.match(list.innerHTML, /Nenhuma sessão registrada/);
assert.doesNotMatch(list.innerHTML, /<a /);

context.sessions = [{ id: 'session_001', hasLaps: true, name: 'Treino <1>', track: 'A & B', size: 123 }];
vm.runInContext('renderSessions()', context);
assert.equal((list.innerHTML.match(/<a /g) || []).length, 3);
assert.match(list.innerHTML, /href="\/api\/session\?id=session_001"/);
assert.match(list.innerHTML, /id=session_001&file=metadata.json/);
assert.match(list.innerHTML, /id=session_001&file=laps.json/);
assert.match(list.innerHTML, /Treino &lt;1&gt;/);
assert.match(list.innerHTML, /A &amp; B/);

context.sessions = [{ id: 'session_20261118_120000', legacy: true }];
vm.runInContext('renderSessions()', context);
assert.equal((list.innerHTML.match(/<a /g) || []).length, 2);
assert.doesNotMatch(list.innerHTML, /file=laps.json/);
assert.match(list.innerHTML, /id=session_20261118_120000&file=metadata.json/);

context.sessions = [{ id: 'a&file=settings.json', hasLaps: false }];
vm.runInContext('renderSessions()', context);
assert.match(list.innerHTML, /id=a%26file%3Dsettings.json/);
assert.doesNotMatch(list.innerHTML, /id=a&file=settings.json/);

console.log('PASS: JavaScript syntax; empty/new/legacy sessions; escaping and URL encoding.');
