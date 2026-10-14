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
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents
import MuseScore.NotationScene

//! Create tuplet (other tuplet), for touch screens: the options of the QWidget TupletDialog, with the same texts
StyledDialogView {
    id: root

    title: qsTrc("TupletDialog", "Create tuplet")

    contentWidth: 420
    contentHeight: 330
    margins: 16
    fillsCompactWindow: true

    TupletDialogModel {
        id: tupletModel
    }

    Component.onCompleted: {
        tupletModel.load()
    }

    NavigationPanel {
        id: navPanel
        name: "TupletDialog"
        section: root.navigationSection
        direction: NavigationPanel.Vertical
        order: 1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        Column {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12

            StyledTextLabel {
                text: qsTrc("TupletDialog", "Ratio:")
                font: ui.theme.bodyBoldFont
            }

            Row {
                spacing: 12

                IncrementalPropertyControl {
                    width: 120

                    currentValue: tupletModel.actualNotes
                    step: 1
                    decimals: 0
                    minValue: 1
                    maxValue: 9999

                    navigation.panel: navPanel
                    navigation.row: 0

                    onValueEdited: function(newValue) {
                        tupletModel.actualNotes = newValue
                    }
                }

                StyledTextLabel {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "/"
                    font: ui.theme.largeBodyFont
                }

                IncrementalPropertyControl {
                    width: 120

                    currentValue: tupletModel.normalNotes
                    step: 1
                    decimals: 0
                    minValue: 1
                    maxValue: 9999

                    navigation.panel: navPanel
                    navigation.row: 1

                    onValueEdited: function(newValue) {
                        tupletModel.normalNotes = newValue
                    }
                }
            }

            StyledTextLabel {
                text: qsTrc("TupletDialog", "Number")
                font: ui.theme.bodyBoldFont
            }

            // The values of engraving::TupletNumberType
            FlatRadioButtonList {
                width: parent.width
                height: 40

                model: [
                    { "text": qsTrc("TupletDialog", "Number"), "value": 0 },
                    { "text": qsTrc("TupletDialog", "Ratio"), "value": 1 },
                    { "text": qsTrc("TupletDialog", "None"), "value": 2 }
                ]
                currentValue: tupletModel.numberType

                navigationPanel: navPanel
                navigationRowStart: 10

                onToggled: function(newValue) {
                    tupletModel.numberType = newValue
                }
            }

            StyledTextLabel {
                text: qsTrc("TupletDialog", "Bracket")
                font: ui.theme.bodyBoldFont
            }

            // The values of engraving::TupletBracketType
            FlatRadioButtonList {
                width: parent.width
                height: 40

                model: [
                    { "text": qsTrc("TupletDialog", "Auto"), "value": 0 },
                    { "text": qsTrc("TupletDialog", "Bracket"), "value": 1 },
                    { "text": qsTrc("TupletDialog", "None"), "value": 2 }
                ]
                currentValue: tupletModel.bracketType

                navigationPanel: navPanel
                navigationRowStart: 20

                onToggled: function(newValue) {
                    tupletModel.bracketType = newValue
                }
            }
        }

        ButtonBox {
            Layout.fillWidth: true

            buttons: [ ButtonBoxModel.Cancel, ButtonBoxModel.Ok ]

            navigationPanel.section: root.navigationSection
            navigationPanel.order: 2

            onStandardButtonClicked: function(buttonId) {
                if (buttonId === ButtonBoxModel.Ok) {
                    tupletModel.apply()
                    root.accept()
                } else {
                    root.reject()
                }
            }
        }
    }
}
