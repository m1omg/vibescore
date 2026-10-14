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
#include <QStringList>
#include <qqmlintegration.h>

#include "modularity/ioc.h"
#include "context/iglobalcontext.h"

namespace mu::notation {
//! The Transpose dialog in QML (mobile devices and the web: the touch screen version of the QWidget TransposeDialog,
//! with the same options and the same result)
class TransposeDialogModel : public QObject, public muse::Contextable
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool chromatic READ chromatic WRITE setChromatic NOTIFY changed)
    Q_PROPERTY(bool toKey READ toKey WRITE setToKey NOTIFY changed)
    Q_PROPERTY(int keyIndex READ keyIndex WRITE setKeyIndex NOTIFY changed)
    Q_PROPERTY(int keyDirection READ keyDirection WRITE setKeyDirection NOTIFY changed)
    Q_PROPERTY(int intervalIndex READ intervalIndex WRITE setIntervalIndex NOTIFY changed)
    Q_PROPERTY(bool intervalUp READ intervalUp WRITE setIntervalUp NOTIFY changed)
    Q_PROPERTY(int degreeIndex READ degreeIndex WRITE setDegreeIndex NOTIFY changed)
    Q_PROPERTY(bool degreeUp READ degreeUp WRITE setDegreeUp NOTIFY changed)
    Q_PROPERTY(bool keepDegreeAlterations READ keepDegreeAlterations WRITE setKeepDegreeAlterations NOTIFY changed)
    Q_PROPERTY(bool transposeKeys READ transposeKeys WRITE setTransposeKeys NOTIFY changed)
    Q_PROPERTY(bool transposeChordNames READ transposeChordNames WRITE setTransposeChordNames NOTIFY changed)
    Q_PROPERTY(bool doubleSharpsFlats READ doubleSharpsFlats WRITE setDoubleSharpsFlats NOTIFY changed)

    //! Transposing to a key and transposing key signatures need a range selection
    Q_PROPERTY(bool canTransposeToKey READ canTransposeToKey NOTIFY changed)
    Q_PROPERTY(bool canTransposeChordNames READ canTransposeChordNames NOTIFY changed)

    Q_PROPERTY(QStringList keyNames READ keyNames CONSTANT)
    Q_PROPERTY(QStringList intervalNames READ intervalNames CONSTANT)
    Q_PROPERTY(QStringList degreeNames READ degreeNames CONSTANT)

    muse::ContextInject<context::IGlobalContext> context = { this };

public:
    enum KeyDirection {
        Closest,
        Up,
        Down
    };
    Q_ENUM(KeyDirection)

    explicit TransposeDialogModel(QObject* parent = nullptr);

    //! Selects the whole score when nothing is selected (as the QWidget dialog does), and restores the last options
    Q_INVOKABLE void load();
    Q_INVOKABLE void apply();
    Q_INVOKABLE void cancel();

    bool chromatic() const;
    void setChromatic(bool chromatic);
    bool toKey() const;
    void setToKey(bool toKey);
    int keyIndex() const;
    void setKeyIndex(int index);
    int keyDirection() const;
    void setKeyDirection(int direction);
    int intervalIndex() const;
    void setIntervalIndex(int index);
    bool intervalUp() const;
    void setIntervalUp(bool up);
    int degreeIndex() const;
    void setDegreeIndex(int index);
    bool degreeUp() const;
    void setDegreeUp(bool up);
    bool keepDegreeAlterations() const;
    void setKeepDegreeAlterations(bool keep);
    bool transposeKeys() const;
    void setTransposeKeys(bool transpose);
    bool transposeChordNames() const;
    void setTransposeChordNames(bool transpose);
    bool doubleSharpsFlats() const;
    void setDoubleSharpsFlats(bool use);

    bool canTransposeToKey() const;
    bool canTransposeChordNames() const;

    QStringList keyNames() const;
    QStringList intervalNames() const;
    QStringList degreeNames() const;

signals:
    void changed();

private:
    //! The options (the same defaults as the QWidget dialog); the last ones applied are used again
    struct State {
        bool chromatic = true;
        bool toKey = true;
        int keyIndex = 7; // C major / A minor
        int keyDirection = Closest;
        int intervalIndex = 0;
        bool intervalUp = true;
        int degreeIndex = 0;
        bool degreeUp = true;
        bool keepDegreeAlterations = true;
        bool transposeKeys = true;
        bool transposeChordNames = true;
        bool doubleSharpsFlats = true;
    };

    static State& lastState();

    INotationInteractionPtr interaction() const;

    template<typename T>
    void set(T& member, const T& value);

    State m_state;
    bool m_allSelected = false;
    bool m_canTransposeToKey = true;
    bool m_canTransposeChordNames = true;
};
}
