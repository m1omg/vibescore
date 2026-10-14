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

#include <algorithm>
#include <cmath>

#include <emscripten.h>

#include "global/defer.h"

#include "log.h"

using namespace muse;
using namespace muse::audio;

static const AudioDeviceID DEFAULT_DEVICE_ID = "default";

//! How far ahead of the playback the engine renders (seconds): more is safer against a busy main thread,
//! less makes playback start and stop sooner
static constexpr double RENDER_AHEAD_SECONDS = 0.15;
//! When the engine fell behind the player (at the start, or after the page was busy), it goes on this far ahead
static constexpr double RESTART_AHEAD_SECONDS = 0.06;
//! A block must reach the player before its frame: the margin for the message
static constexpr int SEND_MARGIN_FRAMES = 256;
//! Each wake-up of the page costs: a block (1024 frames) lasts ~21 ms
static constexpr int RENDER_INTERVAL_MS = 20;

// Creates the AudioContext and the player (an AudioWorklet), and returns the sample rate of the browser's output
EM_JS(int, vs_audio_open, (), {
    if (!Module.vibescoreAudio) {
        const AudioContextClass = window.AudioContext || window.webkitAudioContext;
        const ctx = new AudioContextClass({ latencyHint: 'playback' });
        const audio = { ctx: ctx, node: null };
        Module.vibescoreAudio = audio;

        // Plays the blocks of interleaved stereo audio it is sent, each at its frame of the AudioContext's time
        // (currentFrame), and silence where it has none: the page sends no silence, and the player sends nothing
        // back (the page follows its time with the context's currentTime). A block that comes too late is dropped.
        const processor = `
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
            registerProcessor('vibescore-player', VibeScorePlayer);
        `;
        const url = URL.createObjectURL(new Blob([processor], { type: 'application/javascript' }));
        ctx.audioWorklet.addModule(url).then(() => {
            audio.node = new AudioWorkletNode(ctx, 'vibescore-player',
                                              { numberOfInputs: 0, numberOfOutputs: 1, outputChannelCount: [2] });
            audio.node.connect(ctx.destination);
        }).catch((error) => console.error('VibeScore: no audio output', error));

        // Browsers let a page play audio after the user interacted with it
        const resume = () => {
            if (ctx.state !== 'running') {
                ctx.resume();
            }
        };
        for (const type of ['pointerdown', 'keydown', 'touchend']) {
            document.addEventListener(type, resume, { capture: true });
        }
    }

    return Module.vibescoreAudio.ctx.sampleRate;
});

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

EM_JS(void, vs_audio_suspend, (), {
    const audio = Module.vibescoreAudio;
    if (audio) {
        audio.ctx.suspend();
    }
});

WebAudioDriver::WebAudioDriver()
{
    m_timer.setInterval(RENDER_INTERVAL_MS);
    QObject::connect(&m_timer, &QTimer::timeout, [this]() {
        renderAhead();
    });
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
    const int sampleRate = vs_audio_open();
    if (sampleRate <= 0) {
        LOGE() << "No audio output in this browser";
        return false;
    }

    m_activeSpec = spec;
    m_activeSpec.deviceId = DEFAULT_DEVICE_ID;
    m_activeSpec.output.sampleRate = static_cast<sample_rate_t>(sampleRate);
    m_activeSpec.output.audioChannelCount = 2;

    m_block.resize(static_cast<size_t>(m_activeSpec.output.samplesPerChannel) * 2);

    if (activeSpec) {
        *activeSpec = m_activeSpec;
    }

    m_opened = true;
    m_timer.start();
    m_activeSpecChanged.send(m_activeSpec);

    LOGI() << "Opened web audio output: " << m_activeSpec.output;
    return true;
}

void WebAudioDriver::close()
{
    if (!m_opened) {
        return;
    }

    m_timer.stop();
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
    return { 256, 512, 1024, 2048 };
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
