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

#include "modularity/ioc.h"
#include "context/iglobalcontext.h"

namespace mu::notation {
//! "Measures per system" (add or remove system locks) in QML, for touch screens: the QWidget BreaksDialog's options
class BreaksDialogModel : public QObject, public muse::Contextable
{
    Q_OBJECT
    QML_ELEMENT

    muse::ContextInject<context::IGlobalContext> context = { this };

public:
    enum Action {
        LockEveryNMeasures,
        LockEachSystem,
        RemoveLocks
    };
    Q_ENUM(Action)

    explicit BreaksDialogModel(QObject* parent = nullptr);

    //! Selects the whole score unless a range is selected (as the QWidget dialog does)
    Q_INVOKABLE void load();
    Q_INVOKABLE void apply(int action, int measuresPerSystem);
    Q_INVOKABLE void cancel();

private:
    INotationInteractionPtr interaction() const;

    bool m_allSelected = false;
};
}
