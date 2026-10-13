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

#include <gmock/gmock.h>

#include "notation/inotationnoteinput.h"

namespace mu::notation {
class NotationNoteInputMock : public INotationNoteInput
{
public:
    MOCK_METHOD(bool, isNoteInputMode, (), (const, override));

    MOCK_METHOD(const NoteInputState&, state, (), (const, override));

    MOCK_METHOD(void, startNoteInput, (NoteInputMethod, bool), (override));
    MOCK_METHOD(void, endNoteInput, (bool), (override));

    MOCK_METHOD(muse::async::Channel<bool>, noteInputStarted, (), (const, override));
    MOCK_METHOD(muse::async::Notification, noteInputEnded, (), (const, override));

    MOCK_METHOD(bool, usingNoteInputMethod, (NoteInputMethod), (const, override));
    MOCK_METHOD(void, setNoteInputMethod, (NoteInputMethod), (override));

    MOCK_METHOD(void, addNote, (const NoteInputParams&, NoteAddingMode), (override));
    MOCK_METHOD(void, padNote, (const Pad&), (override));
    MOCK_METHOD(muse::Ret, putNote, (const muse::PointF&, bool, bool), (override));
    MOCK_METHOD(void, removeNote, (const muse::PointF&), (override));

    MOCK_METHOD(void, addTuplet, (const TupletOptions&), (override));

    MOCK_METHOD(void, doubleNoteInputDuration, (), (override));
    MOCK_METHOD(void, halveNoteInputDuration, (), (override));

    MOCK_METHOD(void, addSlur, (mu::engraving::Slur*), (override));
    MOCK_METHOD(void, resetSlur, (), (override));

    MOCK_METHOD(void, addTie, (), (override));

    MOCK_METHOD(void, addLaissezVib, (), (override));

    MOCK_METHOD(void, setInputNote, (const NoteInputParams&), (override));
    MOCK_METHOD(void, setInputNotes, (const NoteValList&), (override));
    MOCK_METHOD(void, moveInputNotes, (bool, PitchMode), (override));

    MOCK_METHOD(void, setRestMode, (bool), (override));
    MOCK_METHOD(void, setAccidental, (AccidentalType), (override));
    MOCK_METHOD(void, setArticulation, (SymbolId), (override));
    MOCK_METHOD(void, setDrumNote, (int), (override));
    MOCK_METHOD(void, setCurrentVoice, (voice_idx_t), (override));
    MOCK_METHOD(void, setCurrentTrack, (track_idx_t), (override));

    MOCK_METHOD(muse::RectF, cursorRect, (), (const, override));

    MOCK_METHOD(muse::async::Notification, noteAdded, (), (const, override));
    MOCK_METHOD(muse::async::Notification, stateChanged, (), (const, override));
};
}
