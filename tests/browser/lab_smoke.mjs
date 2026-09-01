import http from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, normalize } from 'node:path';
import { chromium, firefox, webkit } from 'playwright';

const root = process.argv[2];
if (!root) throw new Error('Usage: node lab_smoke.mjs <built-lab-directory>');
const browserName = process.env.LAB_BROWSER ?? 'chromium';
const mime = new Map([
  ['.html', 'text/html; charset=utf-8'],
  ['.js', 'text/javascript; charset=utf-8'],
  ['.json', 'application/json; charset=utf-8'],
  ['.css', 'text/css; charset=utf-8'],
]);

const server = http.createServer(async (request, response) => {
  try {
    const requestPath = new URL(request.url, 'http://127.0.0.1').pathname;
    const relative = requestPath === '/' ? 'index.html' : requestPath.slice(1);
    const safe = normalize(relative).replace(/^([.][.][/\\])+/, '');
    const path = join(root, safe);
    const data = await readFile(path);
    response.writeHead(200, { 'content-type': mime.get(extname(path)) ?? 'application/octet-stream' });
    response.end(data);
  } catch {
    response.writeHead(404);
    response.end('not found');
  }
});
await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
const { port } = server.address();

let launcher = chromium;
let options = { headless: true };
if (browserName === 'firefox') launcher = firefox;
if (browserName === 'webkit') launcher = webkit;
if (browserName === 'msedge') options = { headless: true, channel: 'msedge' };

let browser;
try {
  browser = await launcher.launch(options);
  const page = await browser.newPage();
  const pageErrors = [];
  page.on('pageerror', (error) => pageErrors.push(String(error)));
  await page.goto(`http://127.0.0.1:${port}/`, { waitUntil: 'networkidle' });
  await page.click('#start');
  await page.waitForFunction(() => document.querySelector('#status')?.dataset.kind === 'ok', null, { timeout: 15000 });
  await page.click('#note-on');
  await page.waitForFunction(() => !document.querySelector('#metric-voices')?.textContent.startsWith('0 '), null, { timeout: 10000 });
  await page.click('#note-off');
  await page.selectOption('#test-select', 'A01');
  await page.click('#run-test');
  await page.waitForFunction(() => document.querySelector('#runner-status')?.textContent.includes('automated'), null, { timeout: 15000 });
  const evidence = await page.textContent('#evidence-preview');
  const parsed = JSON.parse(evidence);
  if (parsed.TEST !== 'A01 — Pluck') throw new Error(`Unexpected evidence TEST: ${parsed.TEST}`);
  if (!['PASS', 'INVESTIGATE'].includes(parsed.AUTOMATED_RESULT)) {
    throw new Error(`Unexpected automated result: ${parsed.AUTOMATED_RESULT}`);
  }
  if (pageErrors.length) throw new Error(`Page errors: ${pageErrors.join(' | ')}`);
  console.log(`${browserName} Resonant Engine Lab smoke PASS`);
} finally {
  if (browser) await browser.close();
  await new Promise((resolve) => server.close(resolve));
}
