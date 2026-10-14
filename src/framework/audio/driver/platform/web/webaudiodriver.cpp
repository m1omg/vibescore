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

#include <emscripten.h>

#include "log.h"

using namespace muse;
using namespace muse::audio;

static const AudioDeviceID DEFAULT_DEVICE_ID = "default";

//! How far ahead of the playback the engine renders (seconds): more is safer against a busy main thread,
//! less makes playback start and stop sooner
static constexpr double RENDER_AHEAD_SECONDS = 0.15;
static constexpr int RENDER_INTERVAL_MS = 10;

// Creates the AudioContext and the player (an AudioWorklet), and returns the sample rate of the browser's output
EM_JS(int, vs_audio_open, (), {
    if (!Module.vibescoreAudio) {
        const AudioContextClass = window.AudioContext || window.webkitAudioContext;
        const ctx = new AudioContextClass({ latencyHint: 'playback' });
        const audio = { ctx: ctx, node: null, sent: 0, played: 0 };
        Module.vibescoreAudio = audio;

        // Plays the interleaved stereo blocks it is sent, in order, and reports how many frames it played
        const processor = `
            class VibeScorePlayer extends AudioWorkletProcessor {
                constructor() {
                    super();
                    this.queue = [];
                    this.offset = 0;
                    this.played = 0;
                    this.port.onmessage = (event) => this.queue.push(event.data);
                }
                process(inputs, outputs) {
                    const left = outputs[0][0];
                    const right = outputs[0][1] || left;
                    let i = 0;
                    while (i < left.length && this.queue.length > 0) {
                        const block = this.queue[0];
                        const frames = block.length / 2;
                        while (i < left.length && this.offset < frames) {
                            left[i] = block[2 * this.offset];
                            right[i] = block[2 * this.offset + 1];
                            ++i;
                            ++this.offset;
                        }
                        if (this.offset >= frames) {
                            this.queue.shift();
                            this.offset = 0;
                        }
                    }
                    this.played += i;
                    for (; i < left.length; ++i) {
                        left[i] = 0;
                        right[i] = 0;
                    }
                    this.port.postMessage(this.played);
                    return true;
                }
            }
            registerProcessor('vibescore-player', VibeScorePlayer);
        `;
        const url = URL.createObjectURL(new Blob([processor], { type: 'application/javascript' }));
        ctx.audioWorklet.addModule(url).then(() => {
            audio.node = new AudioWorkletNode(ctx, 'vibescore-player',
                                              { numberOfInputs: 0, numberOfOutputs: 1, outputChannelCount: [2] });
            audio.node.port.onmessage = (event) => { audio.played = event.data; };
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

// The frames sent to the player that it did not play yet, or -1 when the browser does not play audio yet
EM_JS(int, vs_audio_queued_frames, (), {
    const audio = Module.vibescoreAudio;
    if (!audio || !audio.node || audio.ctx.state !== 'running') {
        return -1;
    }
    return audio.sent - audio.played;
});

EM_JS(void, vs_audio_play, (const float* data, int frames), {
    const audio = Module.vibescoreAudio;
    const block = HEAPF32.slice(data >> 2, (data >> 2) + frames * 2);
    audio.sent += frames;
    audio.node.port.postMessage(block, [block.buffer]);
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

    const int blockFrames = static_cast<int>(m_block.size() / 2);
    const int queued = vs_audio_queued_frames();

    if (queued < 0) {
        // No audio yet (until the user interacts with the page): the engine still runs, because it handles
        // its messages in the callback; the result is dropped
        m_activeSpec.callback(reinterpret_cast<uint8_t*>(m_block.data()), static_cast<int>(m_block.size() * sizeof(float)));
        return;
    }

    const int target = static_cast<int>(RENDER_AHEAD_SECONDS * m_activeSpec.output.sampleRate);
    for (int frames = queued; frames < target; frames += blockFrames) {
        m_activeSpec.callback(reinterpret_cast<uint8_t*>(m_block.data()), static_cast<int>(m_block.size() * sizeof(float)));
        vs_audio_play(m_block.data(), blockFrames);
    }
}
