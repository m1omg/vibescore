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

#include <atomic>
#include <memory>
#include <mutex>

#include "iaudiodriver.h"

namespace muse::audio {
//! Android audio output based on Oboe (AAudio, with OpenSL ES fallback on old devices).
//! The stream delivers exactly spec.output.samplesPerChannel frames per callback,
//! so the engine always renders whole blocks.
class OboeAudioDriver : public IAudioDriver
{
public:
    OboeAudioDriver();
    ~OboeAudioDriver() override;

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
    struct Stream; // Oboe stream and callbacks, kept out of this header
    friend struct Stream;

    void closeStream();
    void onStreamDisconnected();

    std::unique_ptr<Stream> m_stream;
    mutable std::mutex m_streamMutex;

    //! "Opened" from the point of view of the audio subsystem; stays true when the stream
    //! is lost (e.g. headphones unplugged) so that the controller reopens it
    std::atomic<bool> m_opened = false;

    Spec m_activeSpec;
    async::Channel<Spec> m_activeSpecChanged;
    async::Notification m_availableOutputDevicesChanged;
};
}
