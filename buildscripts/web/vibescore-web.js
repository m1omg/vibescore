// SPDX-License-Identifier: GPL-3.0-only
// VibeScore in the browser: prepares the file system of the app before it starts (see index.html).
//
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

async function vsLoadShare(FS, onProgress) {
    const [indexResponse, dataResponse] = await Promise.all([fetch('share.json'), fetch('share.data')]);
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

    onProgress?.(`${index.files.length} files`);
}

function vsSyncFileSystem(FS, populate) {
    return new Promise(resolve => {
        FS.syncfs(populate, error => {
            if (error) {
                console.error('VibeScore: could not sync the file system with IndexedDB', error);
            }
            resolve();
        });
    });
}

// The preRun step of the Emscripten module: runs before main(), and main() waits for the run dependencies
function vibescorePreRun(module, onStatus) {
    const FS = module.FS;
    const IDBFS = FS.filesystems.IDBFS;

    for (const folder of VS_PERSISTENT_FOLDERS) {
        vsMakeTree(FS, folder);
        FS.mount(IDBFS, {}, folder);
    }

    module.addRunDependency('vibescore-files');

    Promise.all([
        vsSyncFileSystem(FS, true),
        vsLoadShare(FS, onStatus),
    ]).then(() => {
        module.removeRunDependency('vibescore-files');
    }).catch(error => {
        onStatus?.(String(error));
        console.error(error);
    });

    // Written back to IndexedDB: regularly, when the page is hidden (a tab change, the app is closed)
    let syncing = false;
    const sync = async () => {
        if (syncing) {
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

    // Ask the browser to keep the data (it may otherwise evict it when storage runs low)
    navigator.storage?.persist?.();
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
