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

#include "global/modularity/imoduleinterface.h"

namespace muse::ui {
//! A short vibration that confirms a touch gesture (mobile devices; does nothing where there is none).
//! It follows the system's setting for touch feedback.
class IHapticFeedback : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(IHapticFeedback)

public:
    virtual ~IHapticFeedback() = default;

    enum class Type {
        LongPress, // a long press was recognized (context menu, preview)
        Tick       // something small happened (a step, a snap)
    };

    virtual void perform(Type type) = 0;
};
}
