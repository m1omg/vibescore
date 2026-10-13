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

#include <Qt>

#include "global/modularity/imoduleinterface.h"
#include "global/async/notification.h"

namespace muse::ui {
//! Shift and Ctrl for touch screens, where there is no key to hold:
//! the app shows toggles for them, and touch input acts as if the keys that are on were held.
class ITouchModifiers : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(ITouchModifiers)

public:
    virtual ~ITouchModifiers() = default;

    virtual Qt::KeyboardModifiers modifiers() const = 0;
    virtual void setModifiers(Qt::KeyboardModifiers modifiers) = 0;
    virtual async::Notification modifiersChanged() const = 0;
};
}
