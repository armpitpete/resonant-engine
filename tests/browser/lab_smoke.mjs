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
const chromiumArgs = ['--autoplay-policy=no-user-gesture-required'];
let options = { headless: true };
if (browserName === 'chromium') options.args = chromiumArgs;
if (browserName === 'firefox') launcher = firefox;
if (browserName === 'webkit') launcher = webkit;
if (browserName === 'msedge') options = { headless: true, channel: 'msedge', args: chromiumArgs };

let browser;
let page;
const pageErrors = [];
const consoleMessages = [];

async function snapshot() {
  if (!page) return {};
  return page.evaluate(() => ({
    status: document.querySelector('#status')?.textContent ?? null,
    statusKind: document.querySelector('#status')?.dataset.kind ?? null,
    voices: document.querySelector('#metric-voices')?.textContent ?? null,
    cpu: document.querySelector('#metric-cpu')?.textContent ?? null,
    runner: document.querySelector('#runner-status')?.textContent ?? null,
    startDisabled: document.querySelector('#start')?.disabled ?? null,
    noteOnDisabled: document.querySelector('#note-on')?.disabled ?? null,
  }));
}

async function waitFor(description, predicate, timeout) {
  try {
    await page.waitForFunction(predicate, null, { timeout });
  } catch (error) {
    const diagnostic = {
      browserName,
      description,
      snapshot: await snapshot(),
      pageErrors,
      consoleMessages,
    };
    throw new Error(`${description} failed: ${error.message}\n${JSON.stringify(diagnostic, null, 2)}`);
  }
}

try {
  browser = await launcher.launch(options);
  page = await browser.newPage();
  page.on('pageerror', (error) => pageErrors.push(String(error)));
  page.on('console', (message) => {
    if (['error', 'warning'].includes(message.type())) {
      consoleMessages.push(`${message.type()}: ${message.text()}`);
    }
  });

  await page.goto(`http://127.0.0.1:${port}/`, { waitUntil: 'networkidle' });
  await page.click('#start');
  await waitFor(
    'WASM AudioWorklet ready',
    () => document.querySelector('#status')?.dataset.kind === 'ok',
    20000,
  );

  await page.click('#note-on');
  await waitFor(
    'realtime audio quantum and active-voice telemetry',
    () => !document.querySelector('#metric-voices')?.textContent.startsWith('0 '),
    15000,
  );

  await page.click('#note-off');
  await page.selectOption('#test-select', 'A01');
  await page.click('#run-test');
  await waitFor(
    'A01 deterministic scenario completion',
    () => document.querySelector('#runner-status')?.textContent.includes('automated'),
    20000,
  );

  const evidence = await page.textContent('#evidence-preview');
  const parsed = JSON.parse(evidence);
  if (parsed.TEST !== 'A01 — Pluck') throw new Error(`Unexpected evidence TEST: ${parsed.TEST}`);
  if (!['PASS', 'INVESTIGATE'].includes(parsed.AUTOMATED_RESULT)) {
    throw new Error(`Unexpected automated result: ${parsed.AUTOMATED_RESULT}`);
  }
  if (!Number.isFinite(parsed.OBSERVED?.maxima?.cpuLoad)) {
    throw new Error('A01 evidence does not contain finite CPU telemetry');
  }
  if ((parsed.OBSERVED?.maxima?.activeVoices ?? 0) < 1) {
    throw new Error('A01 evidence never observed an active voice');
  }
  const decay = parsed.OBSERVED?.marks?.find((mark) => mark.kind === 'decay');
  const decayRms = Number(decay?.analysis?.rms);
  if (!Number.isFinite(decayRms) || decayRms < 0.006) {
    throw new Error(`A01 pluck decay is too quiet at 700 ms: RMS=${decayRms}`);
  }
  if (pageErrors.length) throw new Error(`Page errors: ${pageErrors.join(' | ')}`);
  console.log(`${browserName} Resonant Engine Lab realtime smoke PASS`);
} finally {
  if (browser) await browser.close();
  await new Promise((resolve) => server.close(resolve));
}
