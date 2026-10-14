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
#include "breaksdialogmodel.h"

#include "notation/notationtypes.h"

using namespace mu::notation;

BreaksDialogModel::BreaksDialogModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

INotationInteractionPtr BreaksDialogModel::interaction() const
{
    const INotationPtr notation = context() ? context()->currentNotation() : nullptr;
    return notation ? notation->interaction() : nullptr;
}

void BreaksDialogModel::load()
{
    const INotationInteractionPtr interaction = this->interaction();
    if (!interaction) {
        return;
    }

    if (!interaction->selection()->isRange()) {
        interaction->selectAll();
        m_allSelected = true;
    }
}

void BreaksDialogModel::apply(int action, int measuresPerSystem)
{
    const INotationInteractionPtr interaction = this->interaction();
    if (!interaction) {
        return;
    }

    switch (action) {
    case LockEveryNMeasures:
        interaction->addRemoveSystemLocks(AddRemoveSystemLockType::MeasuresInterval, measuresPerSystem);
        break;
    case LockEachSystem:
        interaction->addRemoveSystemLocks(AddRemoveSystemLockType::AfterEachSystem, 0);
        break;
    case RemoveLocks:
        interaction->addRemoveSystemLocks(AddRemoveSystemLockType::None, 0);
        break;
    }

    cancel();
}

void BreaksDialogModel::cancel()
{
    const INotationInteractionPtr interaction = this->interaction();
    if (interaction && m_allSelected) {
        interaction->clearSelection();
        m_allSelected = false;
    }
}
