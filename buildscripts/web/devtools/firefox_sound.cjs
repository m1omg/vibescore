// SPDX-License-Identifier: GPL-3.0-only
// Firefox: does the app load the sound font and make sound? Two starts in one profile; the output's loudness is
// measured with an AnalyserNode on the player (as in sound_check.py). Usage: node firefox_sound.cjs URL PROFILE FIREFOX
// Needs puppeteer-core (npm i puppeteer-core), a Firefox release, a display (xvfb-run -a) and a sound output: in a
// container without one, start PulseAudio first (pulseaudio --start --exit-idle-time=-1), or the AudioContext never
// runs. VS_FF_AUTOPLAY sets media.autoplay.default (0 allowed, 1 blocked, 5 blocked for all media; default 1).
const path = require('path');
const puppeteer = require('puppeteer-core');
const DEMO = path.join(__dirname, '..', '..', '..', 'demos', 'Dawn.mscx');
const [url, profile, firefox] = process.argv.slice(2);
const sleep = ms => new Promise(r => setTimeout(r, ms));
const t0 = Date.now();
const note = text => console.log(`${((Date.now() - t0) / 1000).toFixed(1).padStart(6)}s ${text}`);

const TAP = `(() => {
    const connect = AudioNode.prototype.connect;
    window.__vsMaxRms = 0;
    AudioNode.prototype.connect = function (destination, ...rest) {
        const result = connect.call(this, destination, ...rest);
        if (destination instanceof AudioDestinationNode && !window.__vsAnalyser) {
            const analyser = this.context.createAnalyser();
            analyser.fftSize = 2048;
            connect.call(this, analyser);
            window.__vsAnalyser = analyser;
            const data = new Float32Array(analyser.fftSize);
            setInterval(() => {
                analyser.getFloatTimeDomainData(data);
                let sum = 0;
                for (const v of data) sum += v * v;
                window.__vsMaxRms = Math.max(window.__vsMaxRms, Math.sqrt(sum / data.length));
            }, 50);
        }
        return result;
    };
    Object.defineProperty(window, 'showOpenFilePicker', { value: undefined });
    const click = HTMLInputElement.prototype.click;
    HTMLInputElement.prototype.click = function () {
        if (this.type === 'file') { this.style.display = 'none'; document.body.appendChild(this); window.__vsFileInput = this; return; }
        return click.call(this);
    };
})();`;

const STATE = () => {
    const i = window.vibescoreInstance;
    const rms = window.__vsMaxRms;
    window.__vsMaxRms = 0;
    let sf = null;
    try { sf = i ? i.FS.analyzePath('/files/share/sound/MS Basic.sf3').exists : null; } catch (e) {}
    const line = document.querySelector('#sound');
    return JSON.stringify({ ctx: i?.vibescoreAudio?.ctx.state, rms: Number(rms.toFixed(4)), soundFont: sf,
                            line: line?.style.display === 'flex' ? line.textContent.trim() : '' });
};

(async () => {
    const browser = await puppeteer.launch({ browser: 'firefox', executablePath: firefox, headless: false,
        userDataDir: profile, extraPrefsFirefox: { 'webgl.force-enabled': true, 'webgl.forbid-software': false, 'media.autoplay.default': Number(process.env.VS_FF_AUTOPLAY ?? 1) } });
    for (const start of [1, 2]) {
        const page = (await browser.pages())[0] ?? await browser.newPage();
        await page.evaluateOnNewDocument(TAP);
        await page.goto(url, { waitUntil: 'domcontentloaded' }).catch(() => {});
        for (;;) {
            await sleep(1000);
            try {
                if (await page.evaluate(() => document.querySelector('#loading')?.style.display === 'none')) break;
            } catch (e) { /* loading again */ }
        }
        note(`start ${start}: started ${await page.evaluate(STATE)}`);
        for (let i = 0; i < 60 && !JSON.parse(await page.evaluate(STATE)).soundFont; ++i) await sleep(1000);
        note(`start ${start}: ${await page.evaluate(STATE)}`);
        if (start === 1) {
            await page.evaluate(() => { window.__vsFileInput = null; });
            await page.mouse.click(1039, 674); // Open other…
            await page.waitForFunction(() => window.__vsFileInput, { timeout: 30000 });
            const input = await page.evaluateHandle(() => window.__vsFileInput);
            await input.uploadFile(DEMO);
            await sleep(12000);
            await page.mouse.click(700, 450);
            await page.keyboard.press(' ');
            for (let i = 0; i < 6; ++i) {
                await sleep(500);
                note(`playing: ${await page.evaluate(STATE)}`);
            }
        }
        await sleep(3000);
    }
    await browser.close();
})().catch(e => { console.error(e); process.exit(1); });
