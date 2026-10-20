// SPDX-License-Identifier: GPL-3.0-only
// The service worker of VibeScore in the browser (registered by index.html):
//
// - It makes the page cross-origin isolated: it adds the headers Cross-Origin-Opener-Policy and
//   Cross-Origin-Embedder-Policy to the page and to the scripts of its workers (GitHub Pages cannot send headers).
//   Browsers give shared memory, which the app's threads need, only to such pages; without it the page starts the
//   app without threads.
// - It gives the files of the app from the browser's Cache Storage: later starts do not download them again (the
//   browser's own cache may not keep files this big, especially on phones), and the app also starts offline.
//   It keeps the scripts there; the page keeps the files that it fetches itself (the wasm, share.data: vsFetchKept in
//   vibescore-web.js), because the browser stops a service worker that works too long, and a download of the app can
//   take longer (Firefox stops it a minute after the page's last request): it was not kept then. The page downloads
//   them past the service worker (cache: 'no-store').
//   The address of a file has its version (?v=..., see stamp_page.py); the other versions of a file are removed when
//   it is kept. The page itself is taken from the network when there is one.
// - The sound font is not given from here: the page takes it from Cache Storage itself (vibescoreLoadSoundFont).

const VS_CACHE = 'vibescore-app';

self.addEventListener('install', () => self.skipWaiting());
self.addEventListener('activate', event => event.waitUntil(self.clients.claim()));

// The response with the headers that make the page cross-origin isolated (same-origin content only)
function vsIsolated(response) {
    if (!response || response.type === 'opaque' || response.type === 'opaqueredirect') {
        return response;
    }
    const headers = new Headers(response.headers);
    headers.set('Cross-Origin-Opener-Policy', 'same-origin');
    headers.set('Cross-Origin-Embedder-Policy', 'require-corp');
    headers.set('Cross-Origin-Resource-Policy', 'same-origin');
    return new Response(response.body, { status: response.status, statusText: response.statusText, headers });
}

// Removes the cached files of other builds at the same path
async function vsRemoveOtherVersions(cache, url) {
    for (const request of await cache.keys()) {
        const cached = new URL(request.url);
        if (cached.pathname === url.pathname && cached.search !== url.search) {
            await cache.delete(request);
        }
    }
}

// A file of a build: from the cache, or downloaded (and kept, if it is a script: the page keeps what it fetches)
async function vsVersioned(event, url) {
    const cache = await caches.open(VS_CACHE);
    const cached = await cache.match(event.request, { ignoreVary: true });
    if (cached) {
        return cached;
    }

    const response = await fetch(event.request);
    if (response.status === 200 && event.request.destination !== '') {
        // Kept when it arrived completely (the page reads the response meanwhile)
        const copy = response.clone();
        event.waitUntil(cache.put(event.request, copy)
            .then(() => vsRemoveOtherVersions(cache, url))
            .catch(error => console.warn('VibeScore: could not keep', url.pathname, error)));
    }
    return response;
}

// A slow network: the page that was kept is used after this long
const VS_NETWORK_TIMEOUT_MS = 5000;

// The page (and the other files without a build id): from the network, or from the cache when offline (or when the
// network takes too long, if there is a copy)
async function vsLatest(event) {
    const cache = await caches.open(VS_CACHE);
    const cached = await cache.match(event.request, { ignoreSearch: event.request.mode === 'navigate' });

    const fromNetwork = fetch(event.request).then(response => {
        if (response.ok && response.status === 200) {
            event.waitUntil(cache.put(event.request, response.clone()).catch(() => {}));
        }
        return response;
    });
    if (!cached) {
        return fromNetwork;
    }

    fromNetwork.catch(() => {}); // the copy is used then
    const timeout = new Promise(resolve => setTimeout(() => resolve(null), VS_NETWORK_TIMEOUT_MS));
    try {
        return (await Promise.race([fromNetwork, timeout])) ?? cached;
    } catch (error) {
        return cached;
    }
}

self.addEventListener('fetch', event => {
    const request = event.request;
    const url = new URL(request.url);
    if (request.method !== 'GET' || url.origin !== self.location.origin) {
        return;
    }
    if (url.pathname.includes('/sound/')) {
        return; // the sound font: kept by the page
    }
    if (request.cache === 'no-store') {
        return; // a download that the page keeps itself
    }

    const isVersioned = url.searchParams.has('v');
    // The page and the scripts of workers get the headers; the app's other files are given as they came (the
    // browser keeps the compiled code of the wasm only for a response as it was fetched or cached)
    const needsHeaders = request.mode === 'navigate' || request.destination === 'worker'
                         || request.destination === 'script';

    event.respondWith((async () => {
        const response = isVersioned ? await vsVersioned(event, url) : await vsLatest(event);
        return needsHeaders ? vsIsolated(response) : response;
    })());
});
