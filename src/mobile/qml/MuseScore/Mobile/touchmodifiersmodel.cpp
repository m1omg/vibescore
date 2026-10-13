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
#include "touchmodifiersmodel.h"

using namespace mu::mobile;

TouchModifiersModel::TouchModifiersModel(QObject* parent)
    : QObject(parent)
{
    if (touchModifiers()) {
        touchModifiers()->modifiersChanged().onNotify(this, [this]() {
            emit modifiersChanged();
        });
    }
}

bool TouchModifiersModel::shift() const
{
    return isOn(Qt::ShiftModifier);
}

void TouchModifiersModel::setShift(bool on)
{
    setOn(Qt::ShiftModifier, on);
}

bool TouchModifiersModel::ctrl() const
{
    return isOn(Qt::ControlModifier);
}

void TouchModifiersModel::setCtrl(bool on)
{
    setOn(Qt::ControlModifier, on);
}

void TouchModifiersModel::clear()
{
    if (touchModifiers()) {
        touchModifiers()->setModifiers(Qt::NoModifier);
    }
}

bool TouchModifiersModel::isOn(Qt::KeyboardModifier modifier) const
{
    return touchModifiers() && touchModifiers()->modifiers().testFlag(modifier);
}

void TouchModifiersModel::setOn(Qt::KeyboardModifier modifier, bool on)
{
    if (!touchModifiers()) {
        return;
    }

    Qt::KeyboardModifiers modifiers = touchModifiers()->modifiers();
    modifiers.setFlag(modifier, on);
    touchModifiers()->setModifiers(modifiers);
}
