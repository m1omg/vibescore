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

#include <cstdint>
#include <functional>

#include "miditypes.h"

//! Calls into the Java side of Android MIDI support (VibeScoreMidi, android.media.midi)
namespace muse::midi::android {
MidiDeviceList sources();
MidiDeviceList destinations();

bool openSource(const MidiDeviceID& id);
void closeSource();

bool openDestination(const MidiDeviceID& id);
void closeDestination();
bool send(const uint8_t* data, size_t size);

//! Bytes received from the open source, called on a MIDI thread
using BytesHandler = std::function<void (const uint8_t* data, size_t size)>;
void setSourceHandler(const BytesHandler& handler);
}
