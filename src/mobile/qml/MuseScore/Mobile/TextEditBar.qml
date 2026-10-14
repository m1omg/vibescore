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

//! Over the keyboard while text on the score is edited: what the keys of a hardware keyboard do besides typing
//! (the next syllable or word of the lyrics, the next chord symbol, another verse, bold...), and Done.
//! The buttons run the same actions as the keys. They are not in the keyboard navigation (a hardware keyboard has the
//! keys): a tap on them must not move the focus away from the score, which gets the typed text.
Rectangle {
    id: root

    property var shell: null

    readonly property int kind: root.shell ? root.shell.editedTextKind : MobileShellView.OtherText

    height: 48
    color: ui.theme.backgroundSecondaryColor

    //! { action, icon, text, title }: a button shows the icon, the text or both; the title is its tooltip
    readonly property var buttons: {
        switch (root.kind) {
        case MobileShellView.Lyrics:
            return [
                { "action": "prev-text-element", "icon": IconCode.ARROW_LEFT, "title": qsTrc("mobile", "Previous syllable") },
                { "action": "next-word", "icon": IconCode.ARROW_RIGHT, "text": qsTrc("mobile", "Word"),
                  "title": qsTrc("mobile", "Next word (space)") },
                { "action": "next-syllable", "text": qsTrc("mobile", "Syllable") + " -",
                  "title": qsTrc("mobile", "Next syllable, joined with a hyphen") },
                { "action": "add-melisma", "text": qsTrc("mobile", "Melisma") + " _",
                  "title": qsTrc("mobile", "Extend the syllable over the next note") },
                { "action": "prev-lyric-verse", "icon": IconCode.ARROW_UP, "title": qsTrc("mobile", "Previous verse") },
                { "action": "next-lyric-verse", "icon": IconCode.ARROW_DOWN, "title": qsTrc("mobile", "Next verse") }
            ]
        case MobileShellView.ChordSymbol:
            return [
                { "action": "prev-text-element", "icon": IconCode.ARROW_LEFT, "title": qsTrc("mobile", "Previous note") },
                { "action": "next-word", "icon": IconCode.ARROW_RIGHT, "text": qsTrc("mobile", "Note"),
                  "title": qsTrc("mobile", "Next note (space)") },
                { "action": "prev-beat-TEXT", "icon": IconCode.ARROW_LEFT, "text": qsTrc("mobile", "Beat"),
                  "title": qsTrc("mobile", "Previous beat") },
                { "action": "next-beat-TEXT", "icon": IconCode.ARROW_RIGHT, "text": qsTrc("mobile", "Beat"),
                  "title": qsTrc("mobile", "Next beat") }
            ]
        case MobileShellView.MusicalText:
            return [
                { "action": "prev-text-element", "icon": IconCode.ARROW_LEFT, "title": qsTrc("mobile", "Previous note") },
                { "action": "next-word", "icon": IconCode.ARROW_RIGHT, "text": qsTrc("mobile", "Note"),
                  "title": qsTrc("mobile", "Next note (space)") }
            ]
        default:
            return [
                { "action": "text-b", "icon": IconCode.TEXT_BOLD, "title": qsTrc("mobile", "Bold") },
                { "action": "text-i", "icon": IconCode.TEXT_ITALIC, "title": qsTrc("mobile", "Italic") },
                { "action": "text-u", "icon": IconCode.TEXT_UNDERLINE, "title": qsTrc("mobile", "Underline") }
            ]
        }
    }

    SeparatorLine {
        anchors.top: parent.top
    }

    Flickable {
        id: flickable

        anchors.left: parent.left
        anchors.right: doneButton.left
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter

        height: 40
        contentWidth: buttonRow.width
        clip: true
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds

        Row {
            id: buttonRow

            leftPadding: 8
            spacing: 4

            Repeater {
                model: root.buttons

                FlatButton {
                    required property var modelData

                    height: 40
                    width: modelData.text ? implicitWidth : 48
                    minWidth: 48

                    icon: modelData.icon ?? IconCode.NONE
                    text: modelData.text ?? ""
                    orientation: Qt.Horizontal
                    transparent: true

                    toolTipTitle: modelData.title

                    onClicked: {
                        root.shell.dispatch(modelData.action)
                    }
                }
            }
        }
    }

    FlatButton {
        id: doneButton

        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter

        height: 40
        minWidth: 80

        text: qsTrc("global", "Done")
        accentButton: true

        onClicked: {
            root.shell.dispatch("action://notation/cancel")
        }
    }
}
