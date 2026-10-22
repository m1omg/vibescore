/*
 * SPDX-License-Identifier: GPL-3.0-only
 * VibeScore - MuseScore Studio for mobile devices
 *
 * Copyright (C) 2026 VibeScore contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "webaudiodriver.h"
#include "webaudiodriver.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

#include <emscripten.h>

#include "global/defer.h"
#include "global/runtime.h"

#include "log.h"

using namespace muse;
using namespace muse::audio;

static const AudioDeviceID DEFAULT_DEVICE_ID = "default";

#ifdef MUSE_THREADS_SUPPORT
//! Frames in the ring (a power of two, so that the positions stay right when the counters wrap around)
static constexpr uint32_t RING_FRAMES = 16384;
//! Frames the engine renders per call
static constexpr samples_t BLOCK_FRAMES = 512;
//! How far ahead of the player the ring is kept filled, beyond what the browser takes at once (its output buffer,
//! AudioContext.baseLatency): more is safer against a thread that runs late, less makes the sound follow the user
//! sooner (~64 ms)
static constexpr uint32_t AHEAD_FRAMES = 3072;
//! When the player still runs out while music plays (a slow or busy device), the ring is kept fuller, by a step
//! each time, up to this (~170 ms): the sound then follows the user (and the cursor leads it) a bit later
static constexpr uint32_t MAX_AHEAD_FRAMES = 8192;
static constexpr uint32_t AHEAD_STEP_FRAMES = 1024;
//! A block is music when a sample is louder than this; music played recently when a block was within this
static constexpr float SOUND_LEVEL = 1e-5f;
static constexpr double SOUND_RECENT_SECONDS = 0.5;
//! How often the thread looks whether the player took audio (a block of 512 frames lasts ~11 ms)
static constexpr int RENDER_INTERVAL_MS = 3;
#else
//! How far ahead of the playback the engine renders (seconds): more is safer against a busy main thread,
//! less makes playback start and stop sooner
static constexpr double RENDER_AHEAD_SECONDS = 0.15;
//! When the engine fell behind the player (at the start, or after the page was busy), it goes on this far ahead
static constexpr double RESTART_AHEAD_SECONDS = 0.06;
//! A block must reach the player before its frame: the margin for the message
static constexpr int SEND_MARGIN_FRAMES = 256;
//! Each wake-up of the page costs: a block (1024 frames) lasts ~21 ms
static constexpr int RENDER_INTERVAL_MS = 20;
#endif

// Creates the AudioContext and the player (an AudioWorklet) from the given code (a class VibeScorePlayer), and
// returns the sample rate of the browser's output
EM_JS(int, vs_audio_create, (const char* playerCodePtr), {
    if (!Module.vibescoreAudio) {
        const AudioContextClass = window.AudioContext || window.webkitAudioContext;
        const ctx = new AudioContextClass({ latencyHint: 'playback' });
        const audio = { ctx: ctx, node: null, onNodeCreated: [] };
        Module.vibescoreAudio = audio;

        const processor = UTF8ToString(playerCodePtr) + "\nregisterProcessor('vibescore-player', VibeScorePlayer);";
        const url = URL.createObjectURL(new Blob([processor], { type: 'application/javascript' }));
        ctx.audioWorklet.addModule(url).then(() => {
            audio.node = new AudioWorkletNode(ctx, 'vibescore-player',
                                              { numberOfInputs: 0, numberOfOutputs: 1, outputChannelCount: [2] });
            audio.node.connect(ctx.destination);
            for (const callback of audio.onNodeCreated) {
                callback(audio.node);
            }
        }).catch((error) => console.error('VibeScore: no audio output', error));

        // Browsers let a page play audio after the user interacted with it (on a touch screen, Chrome counts the end of
        // a tap, not its start)
        const resume = () => {
            if (ctx.state !== 'running') {
                ctx.resume().catch(() => {});
            }
        };
        for (const type of ['pointerdown', 'pointerup', 'touchend', 'click', 'keydown']) {
            document.addEventListener(type, resume, { capture: true });
        }
        // The page tells the user when the browser still does not play the audio (see vibescore-web.js)
        if (window.vibescoreAudioCreated) {
            window.vibescoreAudioCreated(ctx);
        }
    }

    return Module.vibescoreAudio.ctx.sampleRate;
});

#ifdef MUSE_THREADS_SUPPORT
// What the browser takes from the player at once (frames), when it says
EM_JS(int, vs_audio_base_latency_frames, (), {
    const ctx = Module.vibescoreAudio.ctx;
    return ctx.baseLatency > 0 ? Math.round(ctx.baseLatency * ctx.sampleRate) : 0;
});

// For comparing on a device: the page's address can set how far ahead the ring is kept filled (?audioahead=frames)
EM_JS(int, vs_audio_ahead_override, (), {
    const value = parseInt(new URLSearchParams(location.search).get('audioahead'), 10);
    return value > 0 ? value : 0;
});
#endif

EM_JS(void, vs_audio_suspend, (), {
    const audio = Module.vibescoreAudio;
    if (audio) {
        audio.ctx.suspend();
    }
});

#ifdef MUSE_THREADS_SUPPORT

// Plays the ring of interleaved stereo audio in the shared memory: takes what the render thread wrote (the counters
// are frames: [0] written, [1] taken), and plays silence where there is nothing yet ([2] counts those frames: the
// render thread was late)
static const char* RING_PLAYER_CODE = R"JS(
class VibeScorePlayer extends AudioWorkletProcessor {
    constructor() {
        super();
        this.ring = null;
        this.port.onmessage = (event) => {
            const d = event.data;
            this.ring = new Float32Array(d.memory, d.ring, d.frames * 2);
            this.counters = new Uint32Array(d.memory, d.counters, 3);
            this.mask = d.frames - 1;
        };
    }
    process(inputs, outputs) {
        const left = outputs[0][0];
        const right = outputs[0][1] || left;
        const n = left.length;
        let i = 0;
        if (this.ring) {
            const written = Atomics.load(this.counters, 0);
            const taken = Atomics.load(this.counters, 1);
            const count = Math.min(n, (written - taken) >>> 0);
            for (; i < count; ++i) {
                const p = ((taken + i) & this.mask) * 2;
                left[i] = this.ring[p];
                right[i] = this.ring[p + 1];
            }
            Atomics.store(this.counters, 1, (taken + count) >>> 0);
            if (count < n) {
                Atomics.add(this.counters, 2, n - count);
            }
        }
        for (; i < n; ++i) {
            left[i] = 0;
            right[i] = 0;
        }
        return true;
    }
}
)JS";

// Gives the player the ring, and tells the render thread whether the browser plays the audio
EM_JS(void, vs_audio_share_ring, (const float* ring, int frames, const uint32_t* counters, int32_t* isPlaying), {
    const audio = Module.vibescoreAudio;
    // The memory of the app as it is now (HEAPU8 may be a view of it from before it grew)
    const memory = typeof wasmMemory !== 'undefined' ? wasmMemory.buffer : HEAPU8.buffer;
    const playing = new Int32Array(memory, isPlaying, 1);
    const update = () => Atomics.store(playing, 0, audio.node && audio.ctx.state === 'running' ? 1 : 0);

    const share = (node) => {
        node.port.postMessage({ memory: memory, ring: ring, frames: frames, counters: counters });
        update();
    };
    if (audio.node) {
        share(audio.node);
    } else {
        audio.onNodeCreated.push(share);
    }
    // For diagnostics (the browser's console): the frames of silence the player played because the ring was empty,
    // and how the ring fared (see WebAudioDriver::th_fillRing)
    const ringCounters = new Uint32Array(memory, counters, 7);
    audio.underrunFrames = () => Atomics.load(ringCounters, 2);
    audio.stats = () => {
        const ms = (frames) => Math.round(frames * 10000 / audio.ctx.sampleRate) / 10;
        const lowest = Atomics.load(ringCounters, 4);
        return {
            underrunMs: ms(Atomics.load(ringCounters, 2)),
            reserveMs: ms(Atomics.load(ringCounters, 3)),
            lowestFillMs: lowest === 0xFFFFFFFF ? null : ms(lowest),
            longestBlockMs: Atomics.load(ringCounters, 5) / 1000,
            reserveIncreases: Atomics.load(ringCounters, 6),
            outputLatencyMs: Math.round((audio.ctx.baseLatency || 0) * 10000) / 10,
            sampleRate: audio.ctx.sampleRate,
        };
    };
    audio.resetStats = () => {
        Atomics.store(ringCounters, 4, 0xFFFFFFFF);
        Atomics.store(ringCounters, 5, 0);
    };
    audio.ctx.addEventListener('statechange', update);
    update();
});

#else

// Plays the blocks of interleaved stereo audio it is sent, each at its frame of the AudioContext's time
// (currentFrame), and silence where it has none: the page sends no silence, and the player sends nothing
// back (the page follows its time with the context's currentTime). A block that comes too late is dropped.
static const char* BLOCK_PLAYER_CODE = R"JS(
class VibeScorePlayer extends AudioWorkletProcessor {
    constructor() {
        super();
        this.queue = [];
        this.port.onmessage = (event) => this.queue.push(event.data);
    }
    process(inputs, outputs) {
        const left = outputs[0][0];
        const right = outputs[0][1] || left;
        const n = left.length;
        left.fill(0);
        right.fill(0);
        let i = 0;
        while (i < n && this.queue.length > 0) {
            const block = this.queue[0];
            const frames = block.data.length / 2;
            const frame = currentFrame + i;
            if (block.frame + frames <= frame) {
                this.queue.shift();
                continue;
            }
            if (block.frame > frame) {
                i += Math.min(n - i, block.frame - frame);
                continue;
            }
            let offset = frame - block.frame;
            for (; i < n && offset < frames; ++i, ++offset) {
                left[i] = block.data[2 * offset];
                right[i] = block.data[2 * offset + 1];
            }
            if (offset >= frames) {
                this.queue.shift();
            }
        }
        return true;
    }
}
)JS";

// The frame that the browser plays now (of the AudioContext's time), or -1 when the browser does not play audio yet
EM_JS(double, vs_audio_current_frame, (), {
    const audio = Module.vibescoreAudio;
    if (!audio || !audio.node || audio.ctx.state !== 'running') {
        return -1;
    }
    return Math.floor(audio.ctx.currentTime * audio.ctx.sampleRate);
});

// The player plays the block at the given frame
EM_JS(void, vs_audio_play, (const float* data, int frames, double frame), {
    const block = HEAPF32.slice(data >> 2, (data >> 2) + frames * 2);
    Module.vibescoreAudio.node.port.postMessage({ frame: frame, data: block }, [block.buffer]);
});

#endif

WebAudioDriver::WebAudioDriver()
{
#ifndef MUSE_THREADS_SUPPORT
    m_timer.setInterval(RENDER_INTERVAL_MS);
    QObject::connect(&m_timer, &QTimer::timeout, [this]() {
        renderAhead();
    });
#endif
}

WebAudioDriver::~WebAudioDriver()
{
    close();
}

void WebAudioDriver::init()
{
}

std::string WebAudioDriver::name() const
{
    return "WASM";
}

AudioDeviceID WebAudioDriver::defaultDevice() const
{
    return DEFAULT_DEVICE_ID;
}

bool WebAudioDriver::open(const Spec& spec, Spec* activeSpec)
{
#ifdef MUSE_THREADS_SUPPORT
    const int sampleRate = vs_audio_create(RING_PLAYER_CODE);
#else
    const int sampleRate = vs_audio_create(BLOCK_PLAYER_CODE);
#endif
    if (sampleRate <= 0) {
        LOGE() << "No audio output in this browser";
        return false;
    }

    m_activeSpec = spec;
    m_activeSpec.deviceId = DEFAULT_DEVICE_ID;
    m_activeSpec.output.sampleRate = static_cast<sample_rate_t>(sampleRate);
    m_activeSpec.output.audioChannelCount = 2;
#ifdef MUSE_THREADS_SUPPORT
    m_activeSpec.output.samplesPerChannel = BLOCK_FRAMES;
#endif

    m_block.resize(static_cast<size_t>(m_activeSpec.output.samplesPerChannel) * 2);

    if (activeSpec) {
        *activeSpec = m_activeSpec;
    }

    m_opened = true;

#ifdef MUSE_THREADS_SUPPORT
    m_ring.assign(static_cast<size_t>(RING_FRAMES) * 2, 0.f);
    const uint32_t burstFrames = (static_cast<uint32_t>(std::max(vs_audio_base_latency_frames(), 0)) + BLOCK_FRAMES - 1)
                                 / BLOCK_FRAMES * BLOCK_FRAMES;
    m_aheadFrames = std::clamp<uint32_t>(burstFrames + AHEAD_FRAMES, AHEAD_FRAMES, MAX_AHEAD_FRAMES);
    if (const int override = vs_audio_ahead_override(); override > 0) {
        // At least two blocks: with one, the ring is only refilled once the player has taken all of it
        m_aheadFrames = std::clamp<uint32_t>(static_cast<uint32_t>(override), 2 * BLOCK_FRAMES, MAX_AHEAD_FRAMES);
    }
    m_lastUnderrunFrames = 0;
    m_soundWritten = false;
    m_ringCounters[0] = 0;
    m_ringCounters[1] = 0;
    m_ringCounters[2] = 0;
    m_ringCounters[3] = m_aheadFrames;
    m_ringCounters[4] = UINT32_MAX;
    m_ringCounters[5] = 0;
    m_ringCounters[6] = 0;
    vs_audio_share_ring(m_ring.data(), static_cast<int>(RING_FRAMES),
                        reinterpret_cast<const uint32_t*>(m_ringCounters), reinterpret_cast<int32_t*>(&m_isPlaying));

    m_running = true;
    m_thread = std::make_unique<std::thread>([this]() {
        th_render();
    });
#else
    m_timer.start();
#endif

    m_activeSpecChanged.send(m_activeSpec);

    LOGI() << "Opened web audio output: " << m_activeSpec.output;
    return true;
}

void WebAudioDriver::close()
{
    if (!m_opened) {
        return;
    }

#ifdef MUSE_THREADS_SUPPORT
    m_running = false;
    if (m_thread) {
        m_thread->join();
        m_thread.reset();
    }
#else
    m_timer.stop();
#endif

    vs_audio_suspend();
    m_opened = false;
}

bool WebAudioDriver::isOpened() const
{
    return m_opened;
}

const IAudioDriver::Spec& WebAudioDriver::activeSpec() const
{
    return m_activeSpec;
}

async::Channel<IAudioDriver::Spec> WebAudioDriver::activeSpecChanged() const
{
    return m_activeSpecChanged;
}

std::vector<samples_t> WebAudioDriver::availableOutputDeviceBufferSizes() const
{
#ifdef MUSE_THREADS_SUPPORT
    return { BLOCK_FRAMES };
#else
    return { 256, 512, 1024, 2048 };
#endif
}

std::vector<sample_rate_t> WebAudioDriver::availableOutputDeviceSampleRates() const
{
    // The browser's output decides
    return { m_activeSpec.output.sampleRate };
}

AudioDeviceList WebAudioDriver::availableOutputDevices() const
{
    return { { DEFAULT_DEVICE_ID, "Browser" } };
}

async::Notification WebAudioDriver::availableOutputDevicesChanged() const
{
    return m_availableOutputDevicesChanged;
}

#ifdef MUSE_THREADS_SUPPORT

void WebAudioDriver::th_render()
{
    runtime::setThreadName("audio_driver");

    using clock = std::chrono::steady_clock;
    clock::time_point lastTime = clock::now();
    double unheardFrames = 0;

    while (m_running) {
        const clock::time_point now = clock::now();

        if (m_isPlaying.load(std::memory_order_acquire)) {
            unheardFrames = 0;
            th_fillRing();
        } else {
            // The browser does not play audio (yet): the engine goes on in real time (it also handles its messages
            // in the callback), and what it renders is dropped
            unheardFrames += std::chrono::duration<double>(now - lastTime).count() * m_activeSpec.output.sampleRate;
            th_renderUnheard(unheardFrames);
        }

        lastTime = now;
        std::this_thread::sleep_for(std::chrono::milliseconds(RENDER_INTERVAL_MS));
    }
}

void WebAudioDriver::th_fillRing()
{
    const uint32_t blockFrames = static_cast<uint32_t>(m_block.size() / 2);
    const uint32_t recentFrames = static_cast<uint32_t>(SOUND_RECENT_SECONDS * m_activeSpec.output.sampleRate);
    uint32_t written = m_ringCounters[0].load(std::memory_order_relaxed);
    const bool musicPlayed = m_soundWritten && written - m_lastSoundWritten <= recentFrames;

    // The player ran out of audio while music played (not e.g. while the sounds of a score load, when nothing plays):
    // the ring is kept fuller from now on
    const uint32_t underrunFrames = m_ringCounters[2].load(std::memory_order_relaxed);
    if (underrunFrames != m_lastUnderrunFrames) {
        m_lastUnderrunFrames = underrunFrames;
        if (musicPlayed && m_aheadFrames < MAX_AHEAD_FRAMES) {
            m_aheadFrames = std::min(m_aheadFrames + AHEAD_STEP_FRAMES, MAX_AHEAD_FRAMES);
            m_ringCounters[3].store(m_aheadFrames, std::memory_order_relaxed);
            m_ringCounters[6].fetch_add(1, std::memory_order_relaxed);
            LOGI() << "The player ran out of audio: " << m_aheadFrames << " frames are kept ready now";
        }
    }

    const uint32_t fill = written - m_ringCounters[1].load(std::memory_order_acquire);
    if (musicPlayed && fill < m_ringCounters[4].load(std::memory_order_relaxed)) {
        m_ringCounters[4].store(fill, std::memory_order_relaxed);
    }

    while (written - m_ringCounters[1].load(std::memory_order_acquire) + blockFrames <= m_aheadFrames) {
        const auto renderStart = std::chrono::steady_clock::now();
        m_activeSpec.callback(reinterpret_cast<uint8_t*>(m_block.data()), static_cast<int>(m_block.size() * sizeof(float)));
        const auto renderMicros = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - renderStart).count());
        if (renderMicros > m_ringCounters[5].load(std::memory_order_relaxed)) {
            m_ringCounters[5].store(renderMicros, std::memory_order_relaxed);
        }

        // Into the ring, in two parts where it wraps around
        const uint32_t pos = written & (RING_FRAMES - 1);
        const uint32_t firstFrames = std::min(blockFrames, RING_FRAMES - pos);
        std::memcpy(m_ring.data() + pos * 2, m_block.data(), firstFrames * 2 * sizeof(float));
        std::memcpy(m_ring.data(), m_block.data() + firstFrames * 2, (blockFrames - firstFrames) * 2 * sizeof(float));

        written += blockFrames;
        m_ringCounters[0].store(written, std::memory_order_release);

        if (std::any_of(m_block.cbegin(), m_block.cend(), [](float v) { return std::abs(v) > SOUND_LEVEL; })) {
            m_lastSoundWritten = written;
            m_soundWritten = true;
        }
    }

    // What the player ran short of while the ring was refilled belongs to the same hiccup: only the next one, that
    // empties a full ring, makes the ring fuller again
    m_lastUnderrunFrames = m_ringCounters[2].load(std::memory_order_relaxed);
}

void WebAudioDriver::th_renderUnheard(double& framesDue)
{
    const double blockFrames = static_cast<double>(m_block.size() / 2);
    framesDue = std::min(framesDue, 4 * blockFrames); // after a long pause, not all at once

    while (framesDue >= blockFrames) {
        m_activeSpec.callback(reinterpret_cast<uint8_t*>(m_block.data()), static_cast<int>(m_block.size() * sizeof(float)));
        framesDue -= blockFrames;
    }
}

#else

void WebAudioDriver::renderAhead()
{
    if (!m_opened || !m_activeSpec.callback || m_block.empty()) {
        return;
    }

    // The engine may let the app run inside the callback (an audio export, see SoundTrackWriter): the timer then
    // calls this again, and the engine is not called again until the callback returns
    if (m_inCallback) {
        return;
    }
    m_inCallback = true;
    DEFER {
        m_inCallback = false;
    };

    const int blockFrames = static_cast<int>(m_block.size() / 2);
    const double now = vs_audio_current_frame();

    if (now < 0) {
        // No audio yet (until the user interacts with the page): the engine still runs, because it handles
        // its messages in the callback; the result is dropped
        m_activeSpec.callback(reinterpret_cast<uint8_t*>(m_block.data()), static_cast<int>(m_block.size() * sizeof(float)));
        return;
    }

    const double sampleRate = m_activeSpec.output.sampleRate;
    if (m_nextFrame < now + SEND_MARGIN_FRAMES) {
        // Behind the player (at the start, or the page was busy): a gap, and on from a little ahead
        m_nextFrame = now + std::ceil(RESTART_AHEAD_SECONDS * sampleRate);
    }

    // The engine renders as much as the time of the player went on, also silence; silence is not sent to the player
    const double target = now + RENDER_AHEAD_SECONDS * sampleRate;
    while (m_nextFrame < target) {
        m_activeSpec.callback(reinterpret_cast<uint8_t*>(m_block.data()), static_cast<int>(m_block.size() * sizeof(float)));

        const bool silent = std::all_of(m_block.cbegin(), m_block.cend(), [](float sample) { return sample == 0.f; });
        if (!silent) {
            vs_audio_play(m_block.data(), blockFrames, m_nextFrame);
        }
        m_nextFrame += blockFrames;
    }
}

#endif
