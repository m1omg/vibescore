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

#include <mutex>

#include "async/asyncable.h"

#include "imidiinport.h"
#include "internal/midideviceslistener.h"

namespace muse::midi {
//! MIDI input from Android MIDI devices (USB, Bluetooth, virtual devices of other apps)
class AndroidMidiInPort : public IMidiInPort, public async::Asyncable
{
public:
    void init();
    void deinit();

    MidiDeviceList availableDevices() const override;
    async::Notification availableDevicesChanged() const override;

    Ret connect(const MidiDeviceID& deviceID) override;
    void disconnect() override;
    bool isConnected() const override;
    MidiDeviceID deviceID() const override;
    async::Notification deviceChanged() const override;

    async::Channel<tick_t, Event> eventReceived() const override;

private:
    //! Turns the MIDI 1.0 byte stream into events (with running status; system messages are skipped)
    void parse(const uint8_t* data, size_t size);

    MidiDeviceID m_deviceID;
    async::Notification m_deviceChanged;
    async::Notification m_availableDevicesChanged;
    MidiDevicesListener m_devicesListener;

    std::mutex m_parserMutex;
    uint8_t m_status = 0;
    uint8_t m_data[2] = { 0, 0 };
    int m_dataCount = 0;
    bool m_inSysex = false;

    async::Channel<tick_t, Event> m_eventReceived;
};
}
