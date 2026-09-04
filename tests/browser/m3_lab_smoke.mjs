import http from 'node:http';
import { readFile } from 'node:fs/promises';
import { extname, join, normalize } from 'node:path';
import { chromium, firefox, webkit } from 'playwright';

const root = process.argv[2];
if (!root) throw new Error('Usage: node m3_lab_smoke.mjs <built-m3-lab-directory>');
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

async function waitFor(description, predicate, timeout = 30000) {
  try {
    await page.waitForFunction(predicate, null, { timeout });
  } catch (error) {
    const snapshot = await page.evaluate(() => ({
      status: document.querySelector('#status')?.textContent ?? null,
      runner: document.querySelector('#runner-status')?.textContent ?? null,
      voices: document.querySelector('#metric-voices')?.textContent ?? null,
      cpu: document.querySelector('#metric-cpu')?.textContent ?? null,
    }));
    throw new Error(`${description} failed: ${error.message}\n${JSON.stringify({ browserName, snapshot, pageErrors, consoleMessages }, null, 2)}`);
  }
}

async function runScenario(id) {
  await page.selectOption('#test-select', id);
  await page.click('#run-test');
  await waitFor(`${id} scenario completion`, () => document.querySelector('#runner-status')?.textContent.includes('automated'));
  return JSON.parse(await page.textContent('#evidence-preview'));
}

try {
  browser = await launcher.launch(options);
  page = await browser.newPage();
  page.on('pageerror', (error) => pageErrors.push(String(error)));
  page.on('console', (message) => {
    if (['error', 'warning'].includes(message.type())) consoleMessages.push(`${message.type()}: ${message.text()}`);
  });

  await page.goto(`http://127.0.0.1:${port}/`, { waitUntil: 'networkidle' });
  await page.click('#start');
  await waitFor('M3 WASM AudioWorklet ready', () => document.querySelector('#status')?.dataset.kind === 'ok', 20000);

  const overblow = await runScenario('B10');
  if (overblow.TEST !== 'B10 — Overblow') throw new Error(`Unexpected B10 evidence TEST: ${overblow.TEST}`);
  if (overblow.OBSERVED?.hardFailures?.length) throw new Error('B10 recorded a hard failure');
  if ((overblow.OBSERVED?.maxima?.activeVoices ?? 0) < 1) throw new Error('B10 never observed an active voice');
  if ((overblow.OBSERVED?.maxima?.corePeak ?? 0) <= 1e-4) throw new Error('B10 did not produce measurable resonant output');
  if (overblow.OBSERVED?.finalTelemetry?.protectedState) throw new Error('B10 ended protected');

  const polyphony = await runScenario('B16');
  if (polyphony.TEST !== 'B16 — Polyphony') throw new Error(`Unexpected B16 evidence TEST: ${polyphony.TEST}`);
  if (polyphony.OBSERVED?.hardFailures?.length) throw new Error('B16 recorded a hard failure');
  if ((polyphony.OBSERVED?.maxima?.activeVoices ?? 0) < 8) throw new Error('B16 did not reach eight-voice stress');
  const one = polyphony.OBSERVED?.marks?.find((mark) => mark.kind === 'polyphony' && mark.count === 1);
  const four = polyphony.OBSERVED?.marks?.find((mark) => mark.kind === 'polyphony' && mark.count === 4);
  const oneCpu = Number(one?.telemetry?.cpuLoadSmoothed);
  const fourCpu = Number(four?.telemetry?.cpuLoadSmoothed);
  if (!Number.isFinite(oneCpu) || oneCpu >= 25) throw new Error(`M3 one-voice smoothed CPU budget failed: ${oneCpu}`);
  if (!Number.isFinite(fourCpu) || fourCpu >= 70) throw new Error(`M3 four-voice smoothed CPU budget failed: ${fourCpu}`);
  if (polyphony.OBSERVED?.finalTelemetry?.protectedState) throw new Error('B16 ended protected');

  if (pageErrors.length) throw new Error(`Page errors: ${pageErrors.join(' | ')}`);
  console.log(`${browserName} Resonant Engine M3 Breath Pipe realtime smoke PASS (1v=${oneCpu.toFixed(2)}%, 4v=${fourCpu.toFixed(2)}%)`);
} finally {
  if (browser) await browser.close();
  await new Promise((resolve) => server.close(resolve));
}
