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
#include "transposedialogmodel.h"

#include <algorithm>

#include "engraving/dom/engravingitem.h"
#include "notation/notationtypes.h"
#include "translation.h"

using namespace mu::notation;

TransposeDialogModel::TransposeDialogModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

TransposeDialogModel::State& TransposeDialogModel::lastState()
{
    static State state;
    return state;
}

INotationInteractionPtr TransposeDialogModel::interaction() const
{
    const INotationPtr notation = context() ? context()->currentNotation() : nullptr;
    return notation ? notation->interaction() : nullptr;
}

template<typename T>
void TransposeDialogModel::set(T& member, const T& value)
{
    if (member == value) {
        return;
    }

    member = value;
    emit changed();
}

void TransposeDialogModel::load()
{
    m_state = lastState();

    const INotationInteractionPtr interaction = this->interaction();
    if (!interaction) {
        return;
    }

    if (interaction->selection()->isNone()) {
        interaction->selectAll();
        m_allSelected = true;
    }

    // Transposing to a key and transposing key signatures are only possible with a range selection
    m_canTransposeToKey = interaction->selection()->isRange();
    if (!m_canTransposeToKey) {
        m_state.toKey = false;
    }

    const std::vector<EngravingItem*>& elements = interaction->selection()->elements();
    m_canTransposeChordNames = std::any_of(elements.cbegin(), elements.cend(), [](const EngravingItem* item) {
        return item->isHarmony() || item->isFretDiagram();
    });

    emit changed();
}

void TransposeDialogModel::apply()
{
    const INotationInteractionPtr interaction = this->interaction();
    if (!interaction) {
        return;
    }

    TransposeOptions options;

    if (!m_state.chromatic) {
        options.mode = TransposeMode::DIATONICALLY;
        options.direction = m_state.degreeUp ? TransposeDirection::UP : TransposeDirection::DOWN;
        options.interval = m_state.degreeIndex + 1;
        options.needTransposeKeys = m_state.keepDegreeAlterations;
    } else if (m_state.toKey && m_canTransposeToKey) {
        options.mode = TransposeMode::TO_KEY;
        options.direction = m_state.keyDirection == Closest ? TransposeDirection::CLOSEST
                            : m_state.keyDirection == Up ? TransposeDirection::UP : TransposeDirection::DOWN;
        options.key = Key(m_state.keyIndex - 7);
        options.interval = m_state.intervalIndex;
        options.needTransposeKeys = m_state.transposeKeys;
    } else {
        options.mode = TransposeMode::BY_INTERVAL;
        options.direction = m_state.intervalUp ? TransposeDirection::UP : TransposeDirection::DOWN;
        options.key = Key(m_state.keyIndex - 7);
        options.interval = m_state.intervalIndex;
        options.needTransposeKeys = m_state.transposeKeys;
    }

    options.needTransposeChordNames = m_state.transposeChordNames && m_canTransposeChordNames;
    options.needTransposeDoubleSharpsFlats = m_state.doubleSharpsFlats;

    lastState() = m_state;

    interaction->transpose(options);

    if (m_allSelected) {
        interaction->clearSelection();
        m_allSelected = false;
    }
}

void TransposeDialogModel::cancel()
{
    const INotationInteractionPtr interaction = this->interaction();
    if (interaction && m_allSelected) {
        interaction->clearSelection();
        m_allSelected = false;
    }
}

bool TransposeDialogModel::chromatic() const { return m_state.chromatic; }
void TransposeDialogModel::setChromatic(bool chromatic) { set(m_state.chromatic, chromatic); }
bool TransposeDialogModel::toKey() const { return m_state.toKey; }
void TransposeDialogModel::setToKey(bool toKey) { set(m_state.toKey, toKey); }
int TransposeDialogModel::keyIndex() const { return m_state.keyIndex; }
void TransposeDialogModel::setKeyIndex(int index) { set(m_state.keyIndex, index); }
int TransposeDialogModel::keyDirection() const { return m_state.keyDirection; }
void TransposeDialogModel::setKeyDirection(int direction) { set(m_state.keyDirection, direction); }
int TransposeDialogModel::intervalIndex() const { return m_state.intervalIndex; }
void TransposeDialogModel::setIntervalIndex(int index) { set(m_state.intervalIndex, index); }
bool TransposeDialogModel::intervalUp() const { return m_state.intervalUp; }
void TransposeDialogModel::setIntervalUp(bool up) { set(m_state.intervalUp, up); }
int TransposeDialogModel::degreeIndex() const { return m_state.degreeIndex; }
void TransposeDialogModel::setDegreeIndex(int index) { set(m_state.degreeIndex, index); }
bool TransposeDialogModel::degreeUp() const { return m_state.degreeUp; }
void TransposeDialogModel::setDegreeUp(bool up) { set(m_state.degreeUp, up); }
bool TransposeDialogModel::keepDegreeAlterations() const { return m_state.keepDegreeAlterations; }
void TransposeDialogModel::setKeepDegreeAlterations(bool keep) { set(m_state.keepDegreeAlterations, keep); }
bool TransposeDialogModel::transposeKeys() const { return m_state.transposeKeys; }
void TransposeDialogModel::setTransposeKeys(bool transpose) { set(m_state.transposeKeys, transpose); }
bool TransposeDialogModel::transposeChordNames() const { return m_state.transposeChordNames; }
void TransposeDialogModel::setTransposeChordNames(bool transpose) { set(m_state.transposeChordNames, transpose); }
bool TransposeDialogModel::doubleSharpsFlats() const { return m_state.doubleSharpsFlats; }
void TransposeDialogModel::setDoubleSharpsFlats(bool use) { set(m_state.doubleSharpsFlats, use); }

bool TransposeDialogModel::canTransposeToKey() const
{
    return m_canTransposeToKey;
}

bool TransposeDialogModel::canTransposeChordNames() const
{
    return m_canTransposeChordNames;
}

// The texts of the QWidget dialog (TransposeDialogBase): the same translations

QStringList TransposeDialogModel::keyNames() const
{
    static const char* keys[] = {
        "C♭ major / A♭ minor", "G♭ major / E♭ minor", "D♭ major / B♭ minor", "A♭ major / F minor", "E♭ major / C minor",
        "B♭ major / G minor", "F major / D minor", "C major / A minor", "G major / E minor", "D major / B minor",
        "A major / F♯ minor", "E major / C♯ minor", "B major / G♯ minor", "F♯ major / D♯ minor", "C♯ major / A♯ minor"
    };

    QStringList names;
    for (const char* key : keys) {
        names << muse::qtrc("TransposeDialogBase", key);
    }
    return names;
}

QStringList TransposeDialogModel::intervalNames() const
{
    static const char* intervals[] = {
        "Perfect unison", "Augmented unison", "Diminished second", "Minor second", "Major second", "Augmented second",
        "Diminished third", "Minor third", "Major third", "Augmented third", "Diminished fourth", "Perfect fourth",
        "Augmented fourth", "Diminished fifth", "Perfect fifth", "Augmented fifth", "Diminished sixth", "Minor sixth",
        "Major sixth", "Augmented sixth", "Diminished seventh", "Minor seventh", "Major seventh", "Augmented seventh",
        "Diminished octave", "Perfect octave"
    };

    QStringList names;
    for (const char* interval : intervals) {
        names << muse::qtrc("TransposeDialogBase", interval);
    }
    return names;
}

QStringList TransposeDialogModel::degreeNames() const
{
    static const char* degrees[] = { "Second", "Third", "Fourth", "Fifth", "Sixth", "Seventh" };

    QStringList names;
    for (const char* degree : degrees) {
        names << muse::qtrc("TransposeDialogBase", degree);
    }
    return names;
}
