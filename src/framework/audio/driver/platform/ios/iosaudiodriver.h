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

#include <QObject>

#include "iaudiodriver.h"

namespace muse::audio {
//! iOS / iPadOS audio output: AVAudioSession + RemoteIO audio unit.
//! The session mixes with other apps' audio, so opening the driver at startup
//! does not interrupt music the user is listening to.
class IosAudioDriver : public IAudioDriver
{
public:
    IosAudioDriver();
    ~IosAudioDriver() override;

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

    struct Data;

private:
    void addSessionObservers();
    void removeSessionObservers();
    void restartAfterInterruption();

    //! In the background, the audio unit is stopped (the app does not play in the background)
    void setSuspended(bool suspended);

    std::unique_ptr<Data> m_data;
    std::atomic<bool> m_opened = false;
    bool m_suspended = false;
    QObject m_appStateReceiver;

    Spec m_activeSpec;
    async::Channel<Spec> m_activeSpecChanged;
    async::Notification m_availableOutputDevicesChanged;
};
}
