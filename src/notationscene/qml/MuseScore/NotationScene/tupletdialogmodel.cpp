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
#include "tupletdialogmodel.h"

#include "engraving/dom/tuplet.h"
#include "notation/notationtypes.h"

using namespace mu::notation;
using namespace muse::actions;

TupletDialogModel::TupletDialogModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

void TupletDialogModel::load()
{
    const INotationPtr notation = context() ? context()->currentNotation() : nullptr;
    if (!notation) {
        return;
    }

    m_numberType = notation->style()->styleValue(mu::engraving::Sid::tupletNumberType).toInt();
    m_bracketType = notation->style()->styleValue(mu::engraving::Sid::tupletBracketType).toInt();
    emit changed();
}

void TupletDialogModel::apply()
{
    TupletOptions options;
    options.ratio = Fraction(m_actualNotes, m_normalNotes);
    options.numberType = TupletNumberType(m_numberType);
    options.bracketType = TupletBracketType(m_bracketType);

    dispatcher()->dispatch("custom-tuplet", ActionData::make_arg1<TupletOptions>(options));
}

int TupletDialogModel::actualNotes() const
{
    return m_actualNotes;
}

void TupletDialogModel::setActualNotes(int notes)
{
    if (m_actualNotes != notes) {
        m_actualNotes = notes;
        emit changed();
    }
}

int TupletDialogModel::normalNotes() const
{
    return m_normalNotes;
}

void TupletDialogModel::setNormalNotes(int notes)
{
    if (m_normalNotes != notes) {
        m_normalNotes = notes;
        emit changed();
    }
}

int TupletDialogModel::numberType() const
{
    return m_numberType;
}

void TupletDialogModel::setNumberType(int type)
{
    if (m_numberType != type) {
        m_numberType = type;
        emit changed();
    }
}

int TupletDialogModel::bracketType() const
{
    return m_bracketType;
}

void TupletDialogModel::setBracketType(int type)
{
    if (m_bracketType != type) {
        m_bracketType = type;
        emit changed();
    }
}
