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
#include "androidmidioutport.h"

#include "androidmidibridge.h"
#include "midierrors.h"
#include "global/translation.h"

#include "log.h"

using namespace muse;
using namespace muse::midi;

void AndroidMidiOutPort::init()
{
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

void AndroidMidiOutPort::deinit()
{
    if (isConnected()) {
        disconnect();
    }
}

MidiDeviceList AndroidMidiOutPort::availableDevices() const
{
    MidiDeviceList devices;
    devices.push_back({ NONE_DEVICE_ID, muse::trc("midi", "No device") });

    for (const MidiDevice& device : android::destinations()) {
        devices.push_back(device);
    }

    return devices;
}

async::Notification AndroidMidiOutPort::availableDevicesChanged() const
{
    return m_availableDevicesChanged;
}

Ret AndroidMidiOutPort::connect(const MidiDeviceID& deviceID)
{
    if (isConnected()) {
        disconnect();
    }

    if (deviceID.empty() || deviceID == NONE_DEVICE_ID) {
        return make_ok();
    }

    if (!android::openDestination(deviceID)) {
        return make_ret(Err::MidiFailedConnect, "failed to open MIDI device " + deviceID);
    }

    m_deviceID = deviceID;
    m_deviceChanged.notify();

    LOGI() << "Connected to MIDI output " << deviceID;
    return make_ok();
}

void AndroidMidiOutPort::disconnect()
{
    if (!isConnected()) {
        return;
    }

    android::closeDestination();

    m_deviceID.clear();
    m_deviceChanged.notify();
}

bool AndroidMidiOutPort::isConnected() const
{
    return !m_deviceID.empty() && m_deviceID != NONE_DEVICE_ID;
}

MidiDeviceID AndroidMidiOutPort::deviceID() const
{
    return m_deviceID;
}

async::Notification AndroidMidiOutPort::deviceChanged() const
{
    return m_deviceChanged;
}

bool AndroidMidiOutPort::supportsMIDI20Output() const
{
    return false;
}

Ret AndroidMidiOutPort::sendEvent(const Event& e)
{
    if (!isConnected()) {
        return make_ret(Err::MidiNotConnected);
    }

    for (const Event& event : e.toMIDI10()) {
        uint8_t bytes[4];
        const int length = event.toMidi10Bytes(bytes);
        if (length > 0 && !android::send(bytes, static_cast<size_t>(length))) {
            return make_ret(Err::MidiSendError, "failed to send MIDI");
        }
    }

    return make_ok();
}
