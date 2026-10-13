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

#include <QObject>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "ui/itouchmodifiers.h"

namespace mu::mobile {
//! The Shift and Ctrl toggles for the score: while one is on, touches on the score act as if the key were held
class TouchModifiersModel : public QObject, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool shift READ shift WRITE setShift NOTIFY modifiersChanged)
    Q_PROPERTY(bool ctrl READ ctrl WRITE setCtrl NOTIFY modifiersChanged)

    QML_ELEMENT

    muse::GlobalInject<muse::ui::ITouchModifiers> touchModifiers;

public:
    explicit TouchModifiersModel(QObject* parent = nullptr);

    bool shift() const;
    void setShift(bool on);
    bool ctrl() const;
    void setCtrl(bool on);

    Q_INVOKABLE void clear();

signals:
    void modifiersChanged();

private:
    bool isOn(Qt::KeyboardModifier modifier) const;
    void setOn(Qt::KeyboardModifier modifier, bool on);
};
}
