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

#include "modularity/ioc.h"
#include "actions/actionable.h"
#include "actions/iactionsdispatcher.h"
#include "global/iinteractive.h"

namespace mu::mobile {
//! Bluetooth MIDI devices (keyboards, ...). The action "midi-connect-bluetooth" (Preferences > Audio & MIDI) shows
//! the system's list of nearby devices: on Android the companion device chooser, on iOS Apple's Bluetooth MIDI panel.
//! A connected device is a MIDI device like the others. Android connects the chosen devices again when the app starts
//! (iOS keeps them connected by itself).
class BluetoothMidiController : public muse::Contextable, public muse::actions::Actionable
{
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };
    muse::ContextInject<muse::IInteractive> interactive = { this };

public:
    explicit BluetoothMidiController(const muse::modularity::ContextPtr& iocCtx);

    void init();

private:
    void chooseDevice();
};
}
