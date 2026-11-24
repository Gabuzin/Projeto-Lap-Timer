// Run with: node test/web_track_routes.test.mjs
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';

const source = await readFile(new URL('../src/WEB/page.cpp', import.meta.url), 'utf8');
const routes = [...source.matchAll(
  /server\.on\("([^"]+)",\s*(HTTP_\w+),\s*(handle\w+)\);/g,
)].map(([, path, method, handler]) => ({ path, method, handler }));

// ESPAsyncWebServer's string URI matcher accepts the exact path AND its
// children. _attachHandler chooses the first matching handler in registration
// order. Model that behavior here; exact-only matching would hide this bug.
function dispatch(path, method = 'HTTP_POST') {
  return routes.find((route) => route.method === method &&
    (path === route.path || path.startsWith(route.path + '/')))?.handler;
}

assert.equal(dispatch('/api/tracks/update'), 'handleTrackUpdate',
  'Editing must reach the update handler, never create another track');
assert.equal(dispatch('/api/tracks/delete'), 'handleTrackDelete',
  'Deleting must reach the delete handler with only the track ID');
assert.equal(dispatch('/api/tracks'), 'handleTrackCreate');
assert.equal(dispatch('/api/settings'), 'handleSettingsUpdate');
assert.equal(dispatch('/api/tracks/update', 'HTTP_GET'), undefined);

console.log('PASS: track create/update/delete dispatch and settings dispatch.');
