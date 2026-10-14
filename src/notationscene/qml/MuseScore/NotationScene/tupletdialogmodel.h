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
#include "actions/iactionsdispatcher.h"
#include "context/iglobalcontext.h"

namespace mu::notation {
//! "Create tuplet" (other tuplet) in QML, for touch screens: the options of the QWidget TupletDialog
class TupletDialogModel : public QObject, public muse::Contextable
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int actualNotes READ actualNotes WRITE setActualNotes NOTIFY changed)
    Q_PROPERTY(int normalNotes READ normalNotes WRITE setNormalNotes NOTIFY changed)
    //! engraving::TupletNumberType: number, ratio, none
    Q_PROPERTY(int numberType READ numberType WRITE setNumberType NOTIFY changed)
    //! engraving::TupletBracketType: auto, bracket, none
    Q_PROPERTY(int bracketType READ bracketType WRITE setBracketType NOTIFY changed)

    muse::ContextInject<context::IGlobalContext> context = { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };

public:
    explicit TupletDialogModel(QObject* parent = nullptr);

    //! The number and bracket of the score's style
    Q_INVOKABLE void load();
    Q_INVOKABLE void apply();

    int actualNotes() const;
    void setActualNotes(int notes);
    int normalNotes() const;
    void setNormalNotes(int notes);
    int numberType() const;
    void setNumberType(int type);
    int bracketType() const;
    void setBracketType(int type);

signals:
    void changed();

private:
    int m_actualNotes = 3;
    int m_normalNotes = 2;
    int m_numberType = 0;
    int m_bracketType = 0;
};
}
