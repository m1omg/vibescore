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
#include "androidmidiinport.h"

#include "androidmidibridge.h"
#include "midierrors.h"
#include "global/translation.h"

#include "log.h"

using namespace muse;
using namespace muse::midi;

void AndroidMidiInPort::init()
{
    android::setSourceHandler([this](const uint8_t* data, size_t size) {
        parse(data, size);
    });

    m_devicesListener.startWithCallback([this]() {
        return availableDevices();
    });

    m_devicesListener.devicesChanged().onNotify(this, [this]() {
        if (isConnected()) {
            bool connectedDeviceRemoved = true;
            for (const MidiDevice& device : availableDevices()) {
                if (device.id == m_deviceID) {
                    connectedDeviceRemoved = false;
                    break;
                }
            }

            if (connectedDeviceRemoved) {
                disconnect();
            }
        }

        m_availableDevicesChanged.notify();
    });
}

void AndroidMidiInPort::deinit()
{
    if (isConnected()) {
        disconnect();
    }

    android::setSourceHandler(nullptr);
}

MidiDeviceList AndroidMidiInPort::availableDevices() const
{
    MidiDeviceList devices;
    devices.push_back({ NONE_DEVICE_ID, muse::trc("midi", "No device") });

    for (const MidiDevice& device : android::sources()) {
        devices.push_back(device);
    }

    return devices;
}

async::Notification AndroidMidiInPort::availableDevicesChanged() const
{
    return m_availableDevicesChanged;
}

Ret AndroidMidiInPort::connect(const MidiDeviceID& deviceID)
{
    if (isConnected()) {
        disconnect();
    }

    if (deviceID.empty() || deviceID == NONE_DEVICE_ID) {
        return make_ok();
    }

    if (!android::openSource(deviceID)) {
        return make_ret(Err::MidiFailedConnect, "failed to open MIDI device " + deviceID);
    }

    {
        std::lock_guard lock(m_parserMutex);
        m_status = 0;
        m_dataCount = 0;
        m_inSysex = false;
    }

    m_deviceID = deviceID;
    m_deviceChanged.notify();

    LOGI() << "Connected to MIDI input " << deviceID;
    return make_ok();
}

void AndroidMidiInPort::disconnect()
{
    if (!isConnected()) {
        return;
    }

    android::closeSource();

    m_deviceID.clear();
    m_deviceChanged.notify();
}

bool AndroidMidiInPort::isConnected() const
{
    return !m_deviceID.empty() && m_deviceID != NONE_DEVICE_ID;
}

MidiDeviceID AndroidMidiInPort::deviceID() const
{
    return m_deviceID;
}

async::Notification AndroidMidiInPort::deviceChanged() const
{
    return m_deviceChanged;
}

async::Channel<tick_t, Event> AndroidMidiInPort::eventReceived() const
{
    return m_eventReceived;
}

void AndroidMidiInPort::parse(const uint8_t* data, size_t size)
{
    std::lock_guard lock(m_parserMutex);

    for (size_t i = 0; i < size; ++i) {
        const uint8_t byte = data[i];

        if (byte >= 0xF8) {
            // Real-time messages (clock, start, stop, active sensing) may come in the middle of others
            continue;
        }

        if (byte & 0x80) {
            if (byte == 0xF0) {
                m_inSysex = true;
            } else if (byte == 0xF7) {
                m_inSysex = false;
            } else if (byte > 0xF0) {
                // Other system common messages: their data bytes are skipped
                m_status = 0;
                m_inSysex = false;
            } else {
                m_status = byte;
                m_inSysex = false;
            }

            m_dataCount = 0;
            continue;
        }

        if (m_inSysex || m_status == 0) {
            continue;
        }

        m_data[m_dataCount++] = byte;

        const uint8_t type = m_status & 0xF0;
        const int needed = (type == 0xC0 || type == 0xD0) ? 1 : 2;
        if (m_dataCount < needed) {
            continue;
        }

        uint32_t package = m_status | (uint32_t(m_data[0]) << 8);
        if (needed == 2) {
            package |= uint32_t(m_data[1]) << 16;
        }

        // Running status: further data bytes use the same status
        m_dataCount = 0;

        Event e = Event::fromMidi10Package(package).toMIDI20();
        if (e) {
            m_eventReceived.send(0, e);
        }
    }
}
