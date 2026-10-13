/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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
#include "dummymidiinport.h"

using namespace muse;
using namespace muse::midi;

void DummyMidiInPort::init()
{
}

void DummyMidiInPort::deinit()
{
}

MidiDeviceList DummyMidiInPort::availableDevices() const
{
    return {};
}

async::Notification DummyMidiInPort::availableDevicesChanged() const
{
    return m_availableDevicesChanged;
}

Ret DummyMidiInPort::connect(const MidiDeviceID& deviceID)
{
    m_deviceID = deviceID;
    m_deviceChanged.notify();
    return true;
}

void DummyMidiInPort::disconnect()
{
    m_deviceID.clear();
    m_deviceChanged.notify();
}

bool DummyMidiInPort::isConnected() const
{
    return !m_deviceID.empty();
}

MidiDeviceID DummyMidiInPort::deviceID() const
{
    return m_deviceID;
}

async::Notification DummyMidiInPort::deviceChanged() const
{
    return m_deviceChanged;
}

async::Channel<tick_t, Event> DummyMidiInPort::eventReceived() const
{
    return m_eventReceived;
}
