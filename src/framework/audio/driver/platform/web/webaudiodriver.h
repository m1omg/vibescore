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
#pragma once

#include <vector>

#include "muse_framework_config.h"

#ifdef MUSE_THREADS_SUPPORT
#include <atomic>
#include <memory>
#include <thread>
#else
#include <QTimer>
#endif

#include "iaudiodriver.h"

namespace muse::audio {
//! Audio output in the browser: an AudioWorklet plays what the engine rendered.
//! - With threads: a thread of its own calls the engine (which renders in its threads) and keeps a ring of audio in
//!   the shared memory filled a little ahead; the player reads it there. The app's (main) thread does nothing for it.
//! - With one thread: the engine renders on the main thread, a little ahead of time (driven by a timer), and the
//!   blocks are sent to the player.
//! Browsers start audio after a user gesture; until then the rendered audio is dropped (the engine still runs).
class WebAudioDriver : public IAudioDriver
{
public:
    WebAudioDriver();
    ~WebAudioDriver() override;

    void init() override;

    std::string name() const override;

    AudioDeviceID defaultDevice() const override;

    bool open(const Spec& spec, Spec* activeSpec) override;
    void close() override;
    bool isOpened() const override;

    const Spec& activeSpec() const override;
    async::Channel<Spec> activeSpecChanged() const override;

    std::vector<samples_t> availableOutputDeviceBufferSizes() const override;
    std::vector<sample_rate_t> availableOutputDeviceSampleRates() const override;
    AudioDeviceList availableOutputDevices() const override;
    async::Notification availableOutputDevicesChanged() const override;

private:
#ifdef MUSE_THREADS_SUPPORT
    void th_render();
    void th_fillRing();
    void th_renderUnheard(double& framesDue);
#else
    void renderAhead();
#endif

    bool m_opened = false;
    Spec m_activeSpec;
    async::Channel<Spec> m_activeSpecChanged;
    async::Notification m_availableOutputDevicesChanged;

    std::vector<float> m_block;

#ifdef MUSE_THREADS_SUPPORT
    std::unique_ptr<std::thread> m_thread;
    std::atomic<bool> m_running = false;

    //! Rendered audio (interleaved stereo) that the player reads from the shared memory
    std::vector<float> m_ring;
    //! [0]: the frames written to the ring, [1]: the frames the player took from it (counters that wrap around),
    //! [2]: the frames of silence the player played because the ring was empty;
    //! for diagnostics (vibescoreAudio.stats() in the browser's console): [3] how far ahead the ring is kept filled,
    //! [4] the least it held while music played, [5] the longest rendering of a block (µs), [6] how often the
    //! reserve grew
    std::atomic<uint32_t> m_ringCounters[7] = { 0, 0, 0, 0, 0, 0, 0 };
    //! How far ahead of the player the ring is kept filled (frames), see th_fillRing()
    uint32_t m_aheadFrames = 0;
    //! The player's silence counter ([2]) when the render thread last looked
    uint32_t m_lastUnderrunFrames = 0;
    //! The ring's write counter after the last block with sound in it
    uint32_t m_lastSoundWritten = 0;
    bool m_soundWritten = false;
    //! Whether the browser plays the audio (set by the page)
    std::atomic<int32_t> m_isPlaying = 0;
#else
    bool m_inCallback = false;
    double m_nextFrame = 0; // the frame of the player's time at which the next rendered block plays
    QTimer m_timer;
#endif
};
}
