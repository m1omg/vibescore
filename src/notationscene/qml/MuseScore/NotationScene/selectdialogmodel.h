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
#include "notation/notationtypes.h"

namespace mu::notation {
//! "Select" (Select > More…) in QML, for touch screens: selects the elements like the one the selection started from,
//! with the filters and actions of the QWidget SelectDialog
class SelectDialogModel : public QObject, public muse::Contextable
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString elementType READ elementType NOTIFY loaded)
    Q_PROPERTY(QString elementSubtype READ elementSubtype NOTIFY loaded)
    Q_PROPERTY(bool hasSubtype READ hasSubtype NOTIFY loaded)
    Q_PROPERTY(bool isRest READ isRest NOTIFY loaded)
    Q_PROPERTY(bool isSingleSelection READ isSingleSelection NOTIFY loaded)

    Q_PROPERTY(bool sameSubtype MEMBER m_sameSubtype NOTIFY changed)
    Q_PROPERTY(bool sameStaff MEMBER m_sameStaff NOTIFY changed)
    Q_PROPERTY(bool sameVoice MEMBER m_sameVoice NOTIFY changed)
    Q_PROPERTY(bool sameDuration MEMBER m_sameDuration NOTIFY changed)
    Q_PROPERTY(bool sameBeat MEMBER m_sameBeat NOTIFY changed)
    Q_PROPERTY(bool sameMeasure MEMBER m_sameMeasure NOTIFY changed)
    Q_PROPERTY(bool sameSystem MEMBER m_sameSystem NOTIFY changed)
    Q_PROPERTY(bool inSelection MEMBER m_inSelection NOTIFY changed)
    Q_PROPERTY(int action MEMBER m_action NOTIFY changed)

    muse::ContextInject<context::IGlobalContext> context = { this };

public:
    enum Action {
        ReplaceSelection,
        AddToSelection,
        SearchInSelection,
        SubtractFromSelection
    };
    Q_ENUM(Action)

    explicit SelectDialogModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();
    Q_INVOKABLE void apply();

    QString elementType() const;
    QString elementSubtype() const;
    bool hasSubtype() const;
    bool isRest() const;
    bool isSingleSelection() const;

signals:
    void loaded();
    void changed();

private:
    INotationPtr notation() const;
    FilterElementsOptions elementOptions() const;

    engraving::EngravingItem* m_element = nullptr;
    bool m_isSingleSelection = true;

    bool m_sameSubtype = false;
    bool m_sameStaff = false;
    bool m_sameVoice = false;
    bool m_sameDuration = false;
    bool m_sameBeat = false;
    bool m_sameMeasure = false;
    bool m_sameSystem = false;
    bool m_inSelection = false;
    int m_action = ReplaceSelection;
};
}
