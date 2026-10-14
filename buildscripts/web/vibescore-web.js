// SPDX-License-Identifier: GPL-3.0-only
// VibeScore in the browser: starts the app and prepares its file system (see index.html).
//
// - The loader (vibescoreLoader) downloads and compiles the app with a progress bar, tells what it is doing, and
//   says what went wrong or which step takes too long (instead of showing "Loading…" for good).
// - /home/web_user (settings, the Scores library in Documents) and /files/data (the app's own data) are kept in
//   the browser's IndexedDB (Emscripten IDBFS): they are read before the app starts and written back regularly,
//   when the page is hidden, and when the app asks for it (vibescoreSyncFileSystem, after saving a score).
// - /files/share (templates, styles, ...) is unpacked from share.data (see buildscripts/web/pack_share.py).
// - The sound font is downloaded once into /files/share/sound, which is kept in IndexedDB too: later the app
//   finds it when it starts (the instruments of a score that is opened at the start get their sounds).

const VS_SHARE_FOLDER = '/files/share';
const VS_SOUND_FOLDER = VS_SHARE_FOLDER + '/sound';
const VS_PERSISTENT_FOLDERS = ['/home/web_user', '/files/data', VS_SOUND_FOLDER];
const VS_SYNC_INTERVAL_MS = 30000;
// Opening an IndexedDB database takes a moment; when the browser does not answer (seen in some browser profiles),
// the app starts without it (without the saved data) instead of waiting for good
const VS_INDEXEDDB_TIMEOUT_MS = 10000;
// Reading the saved data from IndexedDB, likewise
const VS_RESTORE_TIMEOUT_MS = 20000;
// A step that shows no progress for this long is reported to the user
const VS_STALL_MS = 30000;

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

// The loading screen: what is being done, the progress, problems
function vibescoreLoader(elements) {
    let stage = '';
    let lastProgressTime = Date.now();
    let isStarted = false;
    let isFailed = false;
    let warnings = [];

    const showProblems = extra => {
        elements.problem.textContent = [...warnings, extra].filter(Boolean).join('\n');
    };

    const loader = {
        // A step of the start, e.g. "Downloading VibeScore…"
        setStage(text) {
            stage = text;
            lastProgressTime = Date.now();
            elements.status.textContent = text;
            showProblems();
        },

        // fraction: 0…1, or null for unknown
        setProgress(fraction, text) {
            lastProgressTime = Date.now();
            if (text) {
                elements.status.textContent = stage + ' ' + text;
            }
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
            elements.status.textContent = 'VibeScore could not start.';
            elements.progress.style.visibility = 'hidden';
            showProblems(String(error?.message ?? error) + '\nReload the page to try again.');
        },

        started() {
            isStarted = true;
            elements.progress.style.visibility = 'hidden';
            if (warnings.length > 0 && elements.banner) {
                elements.banner.querySelector('span').textContent = warnings.join(' ');
                elements.banner.style.display = 'flex';
            }
            // Ask the browser to keep the data (it may otherwise evict it when storage runs low)
            navigator.storage?.persist?.().catch(() => {});
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

        // Module.instantiateWasm: downloads and compiles the app, with progress (the download is ~45 MB compressed)
        instantiateWasm(url, expectedSize) {
            return (imports, successCallback) => {
                (async () => {
                    loader.setStage('Downloading VibeScore…');
                    const response = await fetch(url, { credentials: 'same-origin' });
                    if (!response.ok) {
                        throw new Error(`Could not download the app (${url}: ${response.status} ${response.statusText})`);
                    }

                    // The bytes are counted as they arrive (decompressed: the size of the wasm)
                    let received = 0;
                    const counter = new TransformStream({
                        transform(chunk, controller) {
                            received += chunk.byteLength;
                            loader.setProgress(expectedSize > 0 ? received / expectedSize : null,
                                               expectedSize > 0 ? `${vsMB(received)} of ${vsMB(expectedSize)} MB`
                                                                : `${vsMB(received)} MB`);
                            controller.enqueue(chunk);
                        },
                    });
                    const counted = new Response(response.body.pipeThrough(counter),
                                                 { headers: { 'Content-Type': 'application/wasm' } });

                    // Compiled while it downloads, where the browser can
                    let result;
                    if (typeof WebAssembly.instantiateStreaming === 'function') {
                        result = await WebAssembly.instantiateStreaming(counted, imports);
                    } else {
                        result = await WebAssembly.instantiate(await counted.arrayBuffer(), imports);
                    }

                    loader.setStage('Starting…');
                    loader.setProgress(null);
                    successCallback(result.instance, result.module);
                })().catch(error => loader.failed(error));

                return {}; // the exports come through successCallback
            };
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

    // A step that does not progress is reported, so that "Loading…" never stays without an explanation
    const watchdog = setInterval(() => {
        if (isStarted || isFailed) {
            clearInterval(watchdog);
            return;
        }
        if (Date.now() - lastProgressTime > VS_STALL_MS) {
            showProblems(`"${stage || 'Loading'}" is taking longer than it should. If it stays like this, reload `
                         + 'the page; the browser\'s console (F12) may tell why.');
        }
    }, 5000);

    return loader;
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

async function vsLoadShare(FS) {
    const version = '?v=' + (window.VS_BUILD ?? '');
    const [indexResponse, dataResponse] = await Promise.all([fetch('share.json' + version), fetch('share.data' + version)]);
    if (!indexResponse.ok || !dataResponse.ok) {
        throw new Error('Could not download the application data (share.data)');
    }

    const index = await indexResponse.json();
    const data = new Uint8Array(await dataResponse.arrayBuffer());

    for (const file of index.files) {
        const path = VS_SHARE_FOLDER + '/' + file.path;
        vsMakeTree(FS, path.substring(0, path.lastIndexOf('/')));
        FS.writeFile(path, data.subarray(file.offset, file.offset + file.size));
    }
}

function vsSyncFileSystem(FS, populate) {
    return new Promise(resolve => {
        FS.syncfs(populate, error => {
            if (error) {
                console.error('VibeScore: could not sync the file system with IndexedDB', error);
            }
            resolve(!error);
        });
    });
}

// The preRun step of the Emscripten module: runs before main(), and main() waits for the run dependencies
function vibescorePreRun(module, loader) {
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
        vsSyncFileSystem(FS, true),
        new Promise(resolve => setTimeout(() => resolve('timeout'), VS_RESTORE_TIMEOUT_MS)),
    ]).then(result => {
        if (result !== true) {
            canPersist = false;
            loader.storageProblem();
        }
    });

    Promise.all([restore, vsLoadShare(FS)]).then(() => {
        loader.setStage('Starting…');
        module.removeRunDependency('vibescore-files');
    }).catch(error => loader.failed(error));

    // Written back to IndexedDB: regularly, when the page is hidden (a tab change, the app is closed)
    let syncing = false;
    const sync = async () => {
        if (syncing || !canPersist) {
            return;
        }
        syncing = true;
        await vsSyncFileSystem(FS, false);
        syncing = false;
    };

    window.vibescoreSyncFileSystem = sync;
    setInterval(sync, VS_SYNC_INTERVAL_MS);
    document.addEventListener('visibilitychange', () => {
        if (document.visibilityState === 'hidden') {
            sync();
        }
    });
    window.addEventListener('pagehide', sync);
}

// The sound font (about 50 MB) is downloaded after the app started, so that the app shows up sooner;
// playback is silent until it is loaded
async function vibescoreLoadSoundFont(instance, onStatus) {
    const name = 'MS Basic.sf3';
    if (instance.FS.analyzePath(VS_SOUND_FOLDER + '/' + name).exists) {
        return; // kept from an earlier visit
    }

    try {
        const response = await fetch('sound/' + encodeURIComponent(name));
        if (!response.ok) {
            throw new Error(response.status + ' ' + response.statusText);
        }
        const data = new Uint8Array(await response.arrayBuffer());
        instance.FS.writeFile(VS_SOUND_FOLDER + '/' + name, data);
        instance.vibescoreSoundFontsChanged();
        window.vibescoreSyncFileSystem?.();
    } catch (error) {
        console.error('VibeScore: could not download the sound font', error);
        onStatus?.('No sound: the sound font could not be downloaded');
    }
}
