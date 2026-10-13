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
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

//! Note input without a keyboard: note names, accidentals, pitch, durations and voices.
//! The buttons run the same actions as the keyboard shortcuts.
Rectangle {
    id: root

    property var shell: null
    property NavigationSection navigationSection: null
    property int navigationOrderStart: 1

    color: ui.theme.backgroundPrimaryColor

    readonly property int spacing: 6

    //! Wide sheets (phones in landscape, tablets) put two rows of keys side by side
    readonly property int columns: width >= 820 ? 16 : 8
    readonly property var keys: root.rows.reduce((all, row) => all.concat(row), [])
    readonly property int rowCount: Math.ceil(keys.length / columns)
    readonly property real keyWidth: (width - (columns + 1) * spacing) / columns
    readonly property real keyHeight: Math.max(44, Math.min(56, (height - (rowCount + 1) * spacing) / rowCount))

    //! Read by MobileSheet: the keypad does not need keys taller than 56
    readonly property real preferredSheetContentHeight: rowCount * 56 + (rowCount + 1) * spacing

    //! { action, text } or { action, icon, music }: music icons use the music toolbar font
    readonly property var rows: [
        [
            { "action": "note-c", "text": "C" },
            { "action": "note-d", "text": "D" },
            { "action": "note-e", "text": "E" },
            { "action": "note-f", "text": "F" },
            { "action": "note-g", "text": "G" },
            { "action": "note-a", "text": "A" },
            { "action": "note-b", "text": "B" },
            { "action": "action://notation/delete", "icon": IconCode.DELETE_TANK }
        ],
        [
            { "action": "flat", "icon": IconCode.FLAT, "music": true },
            { "action": "nat", "icon": IconCode.NATURAL, "music": true },
            { "action": "sharp", "icon": IconCode.SHARP, "music": true },
            { "action": "pitch-up", "icon": IconCode.ARROW_UP },
            { "action": "pitch-down", "icon": IconCode.ARROW_DOWN },
            { "action": "pitch-up-octave", "text": "8va ↑" },
            { "action": "pitch-down-octave", "text": "8vb ↓" },
            { "action": "pad-rest", "icon": IconCode.REST, "music": true }
        ],
        [
            { "action": "pad-note-1", "icon": IconCode.NOTE_WHOLE, "music": true },
            { "action": "pad-note-2", "icon": IconCode.NOTE_HALF, "music": true },
            { "action": "pad-note-4", "icon": IconCode.NOTE_QUARTER, "music": true },
            { "action": "pad-note-8", "icon": IconCode.NOTE_8TH, "music": true },
            { "action": "pad-note-16", "icon": IconCode.NOTE_16TH, "music": true },
            { "action": "pad-note-32", "icon": IconCode.NOTE_32ND, "music": true },
            { "action": "pad-dot", "icon": IconCode.NOTE_DOTTED, "music": true },
            { "action": "tie", "icon": IconCode.NOTE_TIE, "music": true }
        ],
        [
            { "action": "voice-1", "icon": IconCode.VOICE_1 },
            { "action": "voice-2", "icon": IconCode.VOICE_2 },
            { "action": "voice-3", "icon": IconCode.VOICE_3 },
            { "action": "voice-4", "icon": IconCode.VOICE_4 },
            { "action": "triplet", "icon": IconCode.NOTE_TUPLET, "music": true },
            { "action": "notation-move-left", "icon": IconCode.ARROW_LEFT },
            { "action": "notation-move-right", "icon": IconCode.ARROW_RIGHT },
            { "action": "note-input", "icon": IconCode.EDIT }
        ]
    ]

    NavigationPanel {
        id: navPanel
        name: "NoteInputKeypad"
        section: root.navigationSection
        order: root.navigationOrderStart
        direction: NavigationPanel.Both
    }

    Grid {
        anchors.fill: parent
        anchors.margins: root.spacing

        columns: root.columns
        spacing: root.spacing

        Repeater {
            model: root.keys

            FlatButton {
                required property var modelData
                required property int index

                width: root.keyWidth
                height: root.keyHeight

                text: modelData.text ?? ""
                icon: modelData.icon ?? IconCode.NONE
                iconFont: modelData.music ? ui.theme.toolbarIconsFont : ui.theme.iconsFont
                textFont: ui.theme.largeBodyBoldFont

                navigation.panel: navPanel
                navigation.row: Math.floor(index / root.columns)
                navigation.column: index % root.columns

                onClicked: {
                    root.shell.dispatch(modelData.action)
                }
            }
        }
    }
}
