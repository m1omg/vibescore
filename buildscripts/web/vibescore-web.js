// SPDX-License-Identifier: GPL-3.0-only
// VibeScore in the browser: starts the app and prepares its file system (see index.html).
//
// - The loader (vibescoreLoader) downloads and compiles the app with a progress bar, tells what it is doing, and
//   says what went wrong or which step takes too long (instead of showing "Loading…" for good).
// - /home/web_user (settings, the Scores library in Documents) and /files/data (the app's own data) are kept in
//   the browser's IndexedDB (Emscripten IDBFS): they are read before the app starts and written back regularly,
//   when the page is hidden, and when the app asks for it (vibescoreSyncFileSystem, after saving a score).
// - /files/share (templates, styles, ...) is unpacked from share.data (see buildscripts/web/pack_share.py).
// - The sound font is downloaded once into /files/share/sound, which is kept in IndexedDB too: later it is read from
//   there after the app started (instruments without a sound font wait for one, see EventAudioSource).
// - The service worker (vibescore-sw.js) makes the page cross-origin isolated, for the app with threads (in threads/),
//   and keeps the app's files; the page uses the app without threads where that does not work (vibescoreChooseApp).

const VS_SHARE_FOLDER = '/files/share';
const VS_SOUND_FOLDER = VS_SHARE_FOLDER + '/sound';
const VS_PERSISTENT_FOLDERS = ['/home/web_user', '/files/data', VS_SOUND_FOLDER];
// Read from IndexedDB before the app starts; the sound font (~50 MB) is read after it started (vibescoreLoadSoundFont)
const VS_STARTUP_FOLDERS = ['/home/web_user', '/files/data'];
const VS_SYNC_INTERVAL_MS = 30000;
// Opening an IndexedDB database takes a moment; when the browser does not answer (seen in some browser profiles),
// the app starts without it (without the saved data) instead of waiting for good
const VS_INDEXEDDB_TIMEOUT_MS = 10000;
// Reading the saved data from IndexedDB, likewise
const VS_RESTORE_TIMEOUT_MS = 20000;
// A step that shows no progress for this long is reported to the user
const VS_STALL_MS = 30000;
// The download of the app is started again when no data arrives for this long
const VS_DOWNLOAD_STALL_MS = 20000;
const VS_DOWNLOAD_ATTEMPTS = 3;
const VS_PROGRESS_INTERVAL_MS = 150;
// Preparing the app (the browser compiles it) can take minutes on phones and tablets: the user is told so after this
// time, and that something is wrong only after the longer one
const VS_PREPARE_HINT_MS = 10000;
const VS_PREPARE_STALL_MS = 240000;

const vsMB = bytes => (bytes / 1e6).toFixed(0);

const VS_STORAGE_PROBLEM = 'The browser\'s storage does not answer: your saved scores and settings are not available, '
                           + 'and what you do now is not kept after the page is closed.';

// Makes the IndexedDB databases that the browser does not open in time fail, instead of waiting for them for good:
// the app's runtime (Emscripten's Fetch library, IDBFS) waits for them before the app starts
function vsGuardIndexedDB(onTimeout) {
    const factory = window.indexedDB;
    if (!factory) {
        return;
    }

    // After one database did not open, the others are not waited for as long
    let timeout = VS_INDEXEDDB_TIMEOUT_MS;

    const open = factory.open.bind(factory);
    factory.open = (...args) => {
        const request = open(...args);
        const timer = setTimeout(() => {
            if (request.readyState === 'done') {
                return;
            }
            timeout = 1000;
            onTimeout(args[0]);

            // The answer that may still come is dropped; the waiting code gets an error
            const onerror = request.onerror;
            request.onupgradeneeded = null;
            request.onsuccess = event => event.target.result.close();
            request.onerror = null;
            const error = new DOMException(`The browser did not open the database ${args[0]}`, 'TimeoutError');
            onerror?.call(request, { target: { error }, preventDefault() {}, stopPropagation() {} });
        }, timeout);
        request.addEventListener('success', () => clearTimeout(timer));
        request.addEventListener('error', () => clearTimeout(timer));
        return request;
    };
}

// The browser and the device, for the reports of problems
function vsDeviceSummary() {
    const brands = navigator.userAgentData?.brands?.filter(b => !/Not.A.Brand/i.test(b.brand))
        .map(b => b.brand + ' ' + b.version).join(', ');
    return [brands || navigator.userAgent,
            navigator.hardwareConcurrency ? navigator.hardwareConcurrency + ' cores' : '',
            navigator.deviceMemory ? navigator.deviceMemory + ' GB memory' : ''].filter(Boolean).join(', ');
}

// The loading screen: what is being done, the progress, problems
function vibescoreLoader(elements) {
    let stage = '';
    let stageStartTime = Date.now();
    let lastProgressTime = Date.now();
    let progressText = '';
    // The step shows the seconds since it started (it has no progress of its own)
    let isCounting = false;
    let hint = '';
    let hintAfterMs = 0;
    let stallMs = VS_STALL_MS;
    let isStarted = false;
    let isFailed = false;
    let warnings = [];

    // When each step started, for the reports of problems (and the browser's console: vibescoreTimings)
    const loadStartTime = Date.now();
    const timings = window.vibescoreTimings = [];
    const noteTiming = text => timings.push(`${((Date.now() - loadStartTime) / 1000).toFixed(1)} s: ${text}`);

    const showStatus = () => {
        const elapsed = Date.now() - stageStartTime;
        let text = stage + (progressText ? ' ' + progressText : '')
                   + (isCounting ? ` ${Math.floor(elapsed / 1000)} s` : '');
        if (hint && elapsed >= hintAfterMs) {
            text += '\n' + hint;
        }
        elements.status.textContent = text;
    };

    const showProblems = extra => {
        elements.problem.textContent = [...warnings, extra].filter(Boolean).join('\n');
    };

    const loader = {
        // A step of the start, e.g. "Downloading VibeScore…". Options:
        // - counting: shows the seconds since the step started (for a step without a progress of its own);
        // - hint, hintAfterMs: a line shown under the step once it takes longer than hintAfterMs (default: at once);
        // - stallMs: how long the step may go without progress before it is reported as a problem.
        setStage(text, options = {}) {
            stage = text;
            stageStartTime = lastProgressTime = Date.now();
            progressText = '';
            isCounting = options.counting ?? false;
            hint = options.hint ?? '';
            hintAfterMs = options.hintAfterMs ?? 0;
            stallMs = options.stallMs ?? VS_STALL_MS;
            noteTiming(text);
            showStatus();
            showProblems();
        },

        // fraction: 0…1, or null for unknown
        setProgress(fraction, text) {
            lastProgressTime = Date.now();
            progressText = text ?? '';
            showStatus();
            const bar = elements.progress.firstElementChild;
            bar.style.width = fraction === null ? '100%' : Math.round(Math.min(1, fraction) * 100) + '%';
            bar.style.opacity = fraction === null ? '0.4' : '1';
            showProblems();
        },

        // Something that does not stop the app, e.g. no saved data
        warn(text) {
            console.warn('VibeScore: ' + text);
            warnings.push(text);
            showProblems();
        },

        failed(error) {
            if (isFailed) {
                return;
            }
            isFailed = true;
            console.error(error);
            const message = String(error?.message ?? error);
            noteTiming('failed: ' + message);
            elements.status.textContent = 'VibeScore could not start.';
            elements.progress.style.visibility = 'hidden';
            const advice = error instanceof RangeError || /memory/i.test(message)
                ? 'The browser did not have enough memory for VibeScore: close other tabs and apps, then reload the page.'
                : 'Reload the page to try again.';
            showProblems(`${message}\n${advice}\n(${vsDeviceSummary()})`);
        },

        started() {
            isStarted = true;
            noteTiming('started');
            elements.progress.style.visibility = 'hidden';
            if (warnings.length > 0 && elements.banner) {
                elements.banner.querySelector('span').textContent = warnings.join(' ');
                elements.banner.style.display = 'flex';
            }
        },

        stopped(exitData) {
            elements.status.textContent = 'VibeScore stopped' + (exitData.text ? ': ' + exitData.text : '') +
                '. Reload the page to start it again.';
            elements.progress.style.visibility = 'hidden';
            showProblems();
        },

        // What the browser needs to run the app
        checkBrowser() {
            if (typeof WebAssembly !== 'object' || typeof WebAssembly.instantiate !== 'function') {
                loader.failed('This browser does not run WebAssembly (it may be turned off in its settings).');
                return false;
            }
            const canvas = document.createElement('canvas');
            if (!canvas.getContext('webgl2') && !canvas.getContext('webgl')) {
                loader.failed('This browser does not show WebGL graphics (hardware acceleration may be turned off '
                              + 'in its settings).');
                return false;
            }
            return true;
        },

        // Downloads the app and compiles it (while it downloads, where the browser can), with progress.
        // A download that stops (no data for VS_DOWNLOAD_STALL_MS) is started again.
        async compile(url, expectedSize) {
            for (let attempt = 1; ; ++attempt) {
                let isStalled = false;
                try {
                    return await vsCompileWasm(url, expectedSize, loader, attempt > 1, () => { isStalled = true; });
                } catch (error) {
                    if (!isStalled) {
                        throw error;
                    }
                    if (attempt >= VS_DOWNLOAD_ATTEMPTS) {
                        throw new Error('The download of VibeScore stopped, several times. Check the connection, then '
                                        + 'reload the page.');
                    }
                    console.warn('VibeScore: the download stopped, downloading again', error);
                }
            }
        },

        // Module.instantiateWasm: downloads and compiles the app, then creates it (the download is ~45 MB compressed)
        instantiateWasm(url, expectedSize) {
            return (imports, successCallback) => {
                (async () => {
                    const module = await loader.compile(url, expectedSize);
                    noteTiming('compiled');
                    const instance = await WebAssembly.instantiate(module, imports);
                    noteTiming('created');
                    successCallback(instance, module);
                })().catch(error => loader.failed(error));

                return {}; // the exports come through successCallback
            };
        },

        // A note in the timings (for the reports of problems)
        note(text) {
            noteTiming(text);
        },

        // The last step: the app starts (the page cannot show anything else until the app shows itself)
        starting() {
            loader.setStage('Starting…', { hint: 'This can take a minute on phones and tablets.' });
            loader.setProgress(null);
        },
    };

    let hasStorageProblem = false;
    loader.storageProblem = () => {
        if (!hasStorageProblem) {
            hasStorageProblem = true;
            loader.warn(VS_STORAGE_PROBLEM);
        }
    };
    vsGuardIndexedDB(name => {
        console.warn(`VibeScore: the browser did not open the IndexedDB database ${name} in time`);
        loader.storageProblem();
    });

    // Errors before the app runs (afterwards the app reports its own)
    window.addEventListener('error', event => {
        if (!isStarted) {
            loader.failed(event.error ?? event.message);
        }
    });
    window.addEventListener('unhandledrejection', event => {
        if (!isStarted) {
            loader.failed(event.reason);
        }
    });

    // Every second: the seconds of a step that counts them, its hint, and a step that does not progress is reported,
    // so that the loading screen never stays without an explanation. (When the seconds stop, the page's thread is busy:
    // the browser shows nothing new until it is done.)
    const watchdog = setInterval(() => {
        if (isStarted || isFailed) {
            clearInterval(watchdog);
            return;
        }
        showStatus();
        if (Date.now() - lastProgressTime > stallMs) {
            showProblems(`"${stage || 'Loading'}" is taking longer than it should. If it stays like this, reload the `
                         + `page; the browser's console (F12) may tell why.\n(${vsDeviceSummary()})`);
        }
    }, 1000);

    return loader;
}

// Downloads and compiles the app (loader.compile). onStall is called before a download that stopped is cancelled.
async function vsCompileWasm(url, expectedSize, loader, isRetry, onStall) {
    loader.setStage(isRetry ? 'The download stopped. Downloading VibeScore again…' : 'Downloading VibeScore…');

    // No data for VS_DOWNLOAD_STALL_MS: the download is cancelled (the compilation then fails)
    const abort = new AbortController();
    let lastDataTime = Date.now();
    let isDownloaded = false;
    const stallCheck = setInterval(() => {
        if (!isDownloaded && Date.now() - lastDataTime > VS_DOWNLOAD_STALL_MS) {
            clearInterval(stallCheck);
            onStall();
            abort.abort();
        }
    }, 1000);

    try {
        const response = await fetch(url, { credentials: 'same-origin', signal: abort.signal,
                                            cache: isRetry ? 'reload' : 'default' });
        if (!response.ok) {
            throw new Error(`Could not download the app (${url}: ${response.status} ${response.statusText})`);
        }

        // The bytes are counted as they arrive (decompressed: the size of the wasm), on a copy of the response: the
        // browser compiles the response itself, which it can then keep compiled in its cache (Chromium's code cache
        // works only for a response as fetched), so that later starts skip most of the compilation
        const counting = response.clone();
        const counted = (async () => {
            const reader = counting.body.getReader();
            let received = 0;
            let lastShownTime = 0;
            for (;;) {
                const { done, value } = await reader.read();
                if (done) {
                    break;
                }
                received += value.byteLength;
                lastDataTime = Date.now();
                // Shown a few times a second (not for each of the thousands of pieces: that took the page's thread)
                if (lastDataTime - lastShownTime >= VS_PROGRESS_INTERVAL_MS) {
                    lastShownTime = lastDataTime;
                    loader.setProgress(expectedSize > 0 ? received / expectedSize : null,
                                       expectedSize > 0 ? `${vsMB(received)} of ${vsMB(expectedSize)} MB`
                                                        : `${vsMB(received)} MB`);
                }
            }
            // The browser finishes compiling it (it compiled what arrived while it was downloading)
            isDownloaded = true;
            loader.setStage('Preparing VibeScore…', {
                counting: true,
                hint: 'This can take a few minutes on phones and tablets.',
                hintAfterMs: VS_PREPARE_HINT_MS,
                stallMs: VS_PREPARE_STALL_MS,
            });
        })();
        counted.catch(() => {}); // a failed download fails the compilation too

        const isWasm = (response.headers.get('Content-Type') || '').startsWith('application/wasm');
        if (typeof WebAssembly.compileStreaming === 'function' && isWasm) {
            return await WebAssembly.compileStreaming(response);
        }
        return await WebAssembly.compile(await response.arrayBuffer());
    } finally {
        clearInterval(stallCheck);
    }
}

function vsMakeTree(FS, path) {
    let current = '';
    for (const part of path.split('/').filter(p => p.length > 0)) {
        current += '/' + part;
        try {
            FS.mkdir(current);
        } catch (e) {
            // exists
        }
    }
}

// The application data (share.data, ~6 MB) is downloaded while the app downloads (vibescorePrefetchShare)
let vsShareDownload = null;

function vibescorePrefetchShare() {
    const version = '?v=' + (window.VS_BUILD ?? '');
    vsShareDownload ??= Promise.all([fetch('share.json' + version), fetch('share.data' + version)])
        .then(async ([indexResponse, dataResponse]) => {
            if (!indexResponse.ok || !dataResponse.ok) {
                throw new Error('Could not download the application data (share.data)');
            }
            return [await indexResponse.json(), new Uint8Array(await dataResponse.arrayBuffer())];
        });
    vsShareDownload.catch(() => {}); // reported by vsLoadShare
    return vsShareDownload;
}

async function vsLoadShare(FS) {
    const [index, data] = await vibescorePrefetchShare();

    for (const file of index.files) {
        const path = VS_SHARE_FOLDER + '/' + file.path;
        vsMakeTree(FS, path.substring(0, path.lastIndexOf('/')));
        FS.writeFile(path, data.subarray(file.offset, file.offset + file.size));
    }
}

// Reads the given persistent folders from IndexedDB (populate) or writes them to it, one after the other
function vsSyncFileSystem(FS, populate, folders) {
    const IDBFS = FS.filesystems.IDBFS;
    return folders.reduce((previous, folder) => previous.then(ok => new Promise(resolve => {
        IDBFS.syncfs(FS.lookupPath(folder).node.mount, populate, error => {
            if (error) {
                console.error('VibeScore: could not sync', folder, 'with IndexedDB', error);
            }
            resolve(ok && !error);
        });
    })), Promise.resolve(true));
}

// Reads the folder of the sound font from IndexedDB (set by vibescorePreRun): a promise of whether it worked
let vsRestoreSoundFolder = null;

// Writes the persistent folders to IndexedDB (set by vibescorePreRun)
let vsSyncFileSystemNow = null;

// Qt for WebAssembly arms one browser timer for its next timer, with the wait rounded down to whole milliseconds,
// and arms it again only if a timer of Qt was due when it fired (QEventDispatcherWasm::sendTimerEvents). Firefox's
// clock is coarser, so the browser's timer often fired just before Qt's timer was due: then Qt's timers stopped
// until the next input event (the audio stopped, deferred updates waited). The browser's timers that Qt asks for
// are made 2 ms longer, so that they are not early.
const VS_TIMER_MARGIN_MS = 2;
function vsLengthenQtTimers() {
    const setTimeoutOriginal = window.setTimeout.bind(window);
    window.setTimeout = (handler, timeout, ...args) =>
        setTimeoutOriginal(handler, timeout > 0 ? timeout + VS_TIMER_MARGIN_MS : timeout, ...args);
}

// The preRun step of the Emscripten module: runs before main(), and main() waits for the run dependencies
function vibescorePreRun(module, loader) {
    vsLengthenQtTimers();

    const FS = module.FS;
    const IDBFS = FS.filesystems.IDBFS;

    for (const folder of VS_PERSISTENT_FOLDERS) {
        vsMakeTree(FS, folder);
        FS.mount(IDBFS, {}, folder);
    }

    module.addRunDependency('vibescore-files');
    loader.setStage('Opening your scores…');

    // The browser may not answer (its storage is broken or turned off): then the app starts without the saved data,
    // and does not save it (that could overwrite the data that the browser still has)
    let canPersist = true;
    const restore = Promise.race([
        vsSyncFileSystem(FS, true, VS_STARTUP_FOLDERS),
        new Promise(resolve => setTimeout(() => resolve('timeout'), VS_RESTORE_TIMEOUT_MS)),
    ]).then(result => {
        if (result !== true) {
            canPersist = false;
            loader.storageProblem();
        }
    });

    Promise.all([restore, vsLoadShare(FS)]).then(() => {
        loader.starting();
        module.removeRunDependency('vibescore-files');
    }).catch(error => loader.failed(error));

    // The sound font's folder, after the start; until it was read, it is not written (that would remove the sound font
    // that IndexedDB has)
    let isSoundFolderRestored = false;
    vsRestoreSoundFolder = async () => {
        await restore;
        if (!canPersist) {
            return false;
        }
        const ok = await vsSyncFileSystem(FS, true, [VS_SOUND_FOLDER]);
        isSoundFolderRestored = ok;
        return ok;
    };

    // Written back to IndexedDB: regularly, when the page is hidden (a tab change, the app is closed)
    let syncing = false;
    const sync = async () => {
        if (syncing || !canPersist) {
            return;
        }
        syncing = true;
        await vsSyncFileSystem(FS, false, isSoundFolderRestored ? VS_PERSISTENT_FOLDERS : VS_STARTUP_FOLDERS);
        syncing = false;
    };

    vsSyncFileSystemNow = sync;
    setInterval(sync, VS_SYNC_INTERVAL_MS);

    // The app asks for it after a score was saved. Then the browser is also asked to keep the data (it may otherwise
    // evict it when storage runs low): some browsers ask the user, which makes more sense after a save than at the start
    let isPersistenceRequested = false;
    window.vibescoreSyncFileSystem = () => {
        if (!isPersistenceRequested) {
            isPersistenceRequested = true;
            navigator.storage?.persist?.().catch(() => {});
        }
        return sync();
    };
    document.addEventListener('visibilitychange', () => {
        if (document.visibilityState === 'hidden') {
            sync();
        }
    });
    window.addEventListener('pagehide', sync);
}

// The sound font (about 50 MB) is downloaded after the app started, so that the app shows up sooner;
// playback is silent until it is loaded
// localStorage, where the browser has it
function vsStorageGet(key) {
    try {
        return window.localStorage.getItem(key);
    } catch (e) {
        return null;
    }
}

function vsStorageSet(key, value) {
    try {
        if (value === null) {
            window.localStorage.removeItem(key);
        } else {
            window.localStorage.setItem(key, value);
        }
    } catch (e) {
        // no storage: nothing is remembered
    }
}

// The browser did not make the page cross-origin isolated with the service worker's headers (remembered per browser,
// so that the page is not loaded twice every time)
const VS_ISOLATION_FAILED_KEY = 'vibescore-isolation-failed';
// The app with threads did not start with this build: the page uses the one without threads
const VS_THREADS_FAILED_KEY = 'vibescore-threads-failed';
const VS_SERVICE_WORKER_WAIT_MS = 10000;

// Registers the service worker (vibescore-sw.js), which keeps the app's files and makes the page cross-origin isolated.
// Returns true when the page is loaded again to be isolated (the first time).
async function vibescoreIsolatePage() {
    if (!('serviceWorker' in navigator) || !window.isSecureContext) {
        return false;
    }

    let registration;
    try {
        registration = await navigator.serviceWorker.register('vibescore-sw.js');
    } catch (error) {
        console.warn('VibeScore: no service worker', error);
        return false;
    }

    if (window.crossOriginIsolated) {
        vsStorageSet(VS_ISOLATION_FAILED_KEY, null);
        try {
            sessionStorage.removeItem(VS_ISOLATION_FAILED_KEY);
        } catch (e) {
            // no session storage
        }
        return false;
    }
    if (!window.VS_THREADS_WASM_SIZE || vsStorageGet(VS_ISOLATION_FAILED_KEY) === navigator.userAgent) {
        return false;
    }

    // Loaded again already in this tab, and still not isolated: the browser does not do it this way
    let reloaded = false;
    try {
        reloaded = sessionStorage.getItem(VS_ISOLATION_FAILED_KEY) !== null;
        sessionStorage.setItem(VS_ISOLATION_FAILED_KEY, '1');
    } catch (e) {
        return false; // no session storage: the page could be loaded again and again
    }
    if (reloaded) {
        vsStorageSet(VS_ISOLATION_FAILED_KEY, navigator.userAgent);
        return false;
    }

    // The first time, the service worker is installed first
    await new Promise(resolve => {
        const worker = registration.installing || registration.waiting;
        if (registration.active || !worker) {
            resolve();
            return;
        }
        worker.addEventListener('statechange', () => {
            if (worker.state === 'activated' || worker.state === 'redundant') {
                resolve();
            }
        });
        setTimeout(resolve, VS_SERVICE_WORKER_WAIT_MS);
    });
    if (!registration.active) {
        return false;
    }

    location.reload();
    return true;
}

// The app with threads (in threads/) where the browser gives the page shared memory, else the one without threads.
// ?threads=0 or ?threads=1 in the page's address chooses.
function vibescoreChooseApp(loader) {
    const wanted = new URLSearchParams(location.search).get('threads');
    const canUseThreads = window.crossOriginIsolated === true && typeof SharedArrayBuffer === 'function'
                          && window.VS_THREADS_WASM_SIZE > 0;
    const failedBefore = vsStorageGet(VS_THREADS_FAILED_KEY) === window.VS_BUILD;
    const threads = canUseThreads && wanted !== '0' && (!failedBefore || wanted === '1');

    loader.note(threads ? 'with threads' : `without threads (isolated: ${window.crossOriginIsolated === true}`
                + `${failedBefore ? ', failed before' : ''})`);
    return {
        threads: threads,
        dir: threads ? 'threads/' : '',
        wasmSize: threads ? window.VS_THREADS_WASM_SIZE : window.VS_WASM_SIZE,
    };
}

// The app with threads did not start: the page is loaded again without them (for this build)
function vibescoreThreadsFailed(app, loader, error) {
    if (!app.threads) {
        return false;
    }
    console.error('VibeScore: the app with threads did not start; starting the one without threads', error);
    vsStorageSet(VS_THREADS_FAILED_KEY, window.VS_BUILD);
    location.reload();
    return true;
}

function vibescoreLoadScript(src) {
    return new Promise((resolve, reject) => {
        const script = document.createElement('script');
        script.src = src;
        script.onload = () => resolve();
        script.onerror = () => reject(new Error('Could not load ' + src));
        document.head.appendChild(script);
    });
}

async function vibescoreLoadSoundFont(instance, onStatus) {
    const name = 'MS Basic.sf3';
    // Kept from an earlier visit: read from the browser's storage now (after the start, which does not need it)
    if (vsRestoreSoundFolder && await vsRestoreSoundFolder()
        && instance.FS.analyzePath(VS_SOUND_FOLDER + '/' + name).exists) {
        instance.vibescoreSoundFontsChanged();
        return;
    }

    try {
        const response = await fetch('sound/' + encodeURIComponent(name));
        if (!response.ok) {
            throw new Error(response.status + ' ' + response.statusText);
        }
        const data = new Uint8Array(await response.arrayBuffer());
        instance.FS.writeFile(VS_SOUND_FOLDER + '/' + name, data);
        instance.vibescoreSoundFontsChanged();
        vsSyncFileSystemNow?.();
    } catch (error) {
        console.error('VibeScore: could not download the sound font', error);
        onStatus?.('No sound: the sound font could not be downloaded');
    }
}

// Qt for WebAssembly can leave the keyboard focus on the page itself, outside the app, when its windows change (seen
// when the Properties panel was opened): then the app gets no keys, not even after a click into the score. A click
// or tap into the app gives the focus back to the window that was clicked, and focus that a window of the app lost to
// no element goes back to it.
function vibescoreKeepKeyboardFocus(screen) {
    const isLost = () => document.activeElement === null || document.activeElement === document.body;
    const focusHelperOf = element => {
        for (let e = element; e; e = e.parentElement) {
            if (e.classList?.contains('qt-window')) {
                return e.querySelector(':scope > .qt-window-focus-helper');
            }
        }
        return null;
    };

    screen.addEventListener('pointerdown', event => {
        if (isLost()) {
            focusHelperOf(event.composedPath()[0])?.focus({ preventScroll: true });
        }
    }, true);

    document.addEventListener('focusout', event => {
        const helper = event.composedPath()[0];
        if (event.relatedTarget || !helper.classList?.contains('qt-window-focus-helper')) {
            return; // the focus moved somewhere
        }
        setTimeout(() => {
            if (isLost() && document.hasFocus() && helper.isConnected) {
                helper.focus({ preventScroll: true });
            }
        }, 0);
    }, true);
}
