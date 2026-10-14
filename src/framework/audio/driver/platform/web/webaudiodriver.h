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

#include <QTimer>

#include "iaudiodriver.h"

namespace muse::audio {
//! Audio output in the browser (single-threaded WebAssembly): the engine renders on the main thread, a little ahead
//! of time (driven by a timer), and the blocks are sent to an AudioWorklet that plays them.
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
    void renderAhead();

    bool m_opened = false;
    bool m_inCallback = false;
    Spec m_activeSpec;
    async::Channel<Spec> m_activeSpecChanged;
    async::Notification m_availableOutputDevicesChanged;

    QTimer m_timer;
    std::vector<float> m_block;
};
}
