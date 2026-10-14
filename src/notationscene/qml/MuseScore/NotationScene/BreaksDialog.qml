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

//! Measures per system (system locks), for touch screens: the options of the QWidget BreaksDialog, with the same texts
StyledDialogView {
    id: root

    title: qsTrc("BreaksDialog", "Measures per system")

    contentWidth: 420
    contentHeight: 260
    margins: 16
    fillsCompactWindow: true

    property int action: BreaksDialogModel.LockEveryNMeasures
    property int measuresPerSystem: 4

    //: `%1` will be replaced with a number input field.
    //: Text before it will appear before that number field, text after will appear after the field.
    readonly property var lockTextParts: qsTrc("notation/add-remove-system-breaks", "Lock layout with %1 measure(s) per system").split("%1")

    BreaksDialogModel {
        id: breaksModel
    }

    Component.onCompleted: {
        breaksModel.load()
    }

    onClosed: {
        breaksModel.cancel()
    }

    NavigationPanel {
        id: navPanel
        name: "BreaksDialog"
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

            Flow {
                width: parent.width
                spacing: 8

                RoundedRadioButton {
                    height: 44
                    text: root.lockTextParts[0].trim()
                    checked: root.action === BreaksDialogModel.LockEveryNMeasures

                    navigation.panel: navPanel
                    navigation.row: 0

                    onToggled: {
                        root.action = BreaksDialogModel.LockEveryNMeasures
                    }
                }

                IncrementalPropertyControl {
                    width: 100

                    enabled: root.action === BreaksDialogModel.LockEveryNMeasures
                    currentValue: root.measuresPerSystem
                    step: 1
                    decimals: 0
                    minValue: 1
                    maxValue: 99

                    navigation.panel: navPanel
                    navigation.row: 1

                    onValueEdited: function(newValue) {
                        root.measuresPerSystem = newValue
                    }
                }

                StyledTextLabel {
                    height: 44
                    verticalAlignment: Text.AlignVCenter
                    text: root.lockTextParts.length > 1 ? root.lockTextParts[1].trim() : ""
                }
            }

            RoundedRadioButton {
                width: parent.width
                height: 44
                text: qsTrc("BreaksDialog", "Add system locks to each system")
                checked: root.action === BreaksDialogModel.LockEachSystem

                navigation.panel: navPanel
                navigation.row: 2

                onToggled: {
                    root.action = BreaksDialogModel.LockEachSystem
                }
            }

            RoundedRadioButton {
                width: parent.width
                height: 44
                text: qsTrc("BreaksDialog", "Remove current system locks")
                checked: root.action === BreaksDialogModel.RemoveLocks

                navigation.panel: navPanel
                navigation.row: 3

                onToggled: {
                    root.action = BreaksDialogModel.RemoveLocks
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
                    breaksModel.apply(root.action, root.measuresPerSystem)
                    root.accept()
                } else {
                    root.reject()
                }
            }
        }
    }
}
