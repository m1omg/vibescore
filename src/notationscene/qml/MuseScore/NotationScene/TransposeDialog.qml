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

//! Transpose, for touch screens (mobile devices and the web): the options of the QWidget TransposeDialog,
//! with the same texts, as large controls that fit a phone
StyledDialogView {
    id: root

    title: qsTrc("TransposeDialogBase", "Transpose")

    contentWidth: 480
    contentHeight: 560
    margins: 16
    fillsCompactWindow: true

    TransposeDialogModel {
        id: transposeModel
    }

    Component.onCompleted: {
        transposeModel.load()
    }

    onClosed: {
        // Undoes the selection of the whole score that load() made, unless the score was transposed
        transposeModel.cancel()
    }

    function choices(names) {
        return names.map((name, index) => ({ "text": name, "value": index }))
    }

    readonly property var upDown: [
        { "text": qsTrc("TransposeDialogBase", "Up"), "value": true },
        { "text": qsTrc("TransposeDialogBase", "Down"), "value": false }
    ]

    NavigationPanel {
        id: navPanel
        name: "TransposeDialog"
        section: root.navigationSection
        direction: NavigationPanel.Vertical
        order: 1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 16

        StyledFlickable {
            Layout.fillWidth: true
            Layout.fillHeight: true

            contentHeight: content.implicitHeight

            Column {
                id: content

                width: parent.width
                spacing: 16

                FlatRadioButtonList {
                    width: parent.width
                    height: 40

                    model: [
                        { "text": qsTrc("TransposeDialogBase", "Transpose chromatically"), "value": true },
                        { "text": qsTrc("TransposeDialogBase", "Transpose diatonically"), "value": false }
                    ]
                    currentValue: transposeModel.chromatic

                    navigationPanel: navPanel
                    navigationRowStart: 0

                    onToggled: function(newValue) {
                        transposeModel.chromatic = newValue
                    }
                }

                // Chromatically: to a key or by an interval

                Column {
                    width: parent.width
                    spacing: 12
                    visible: transposeModel.chromatic

                    FlatRadioButtonList {
                        width: parent.width
                        height: 40
                        // To a key: only with a range selection
                        visible: transposeModel.canTransposeToKey

                        model: [
                            { "text": qsTrc("TransposeDialogBase", "To Key"), "value": true },
                            { "text": qsTrc("TransposeDialogBase", "By interval"), "value": false }
                        ]
                        currentValue: transposeModel.toKey

                        navigationPanel: navPanel
                        navigationRowStart: 10

                        onToggled: function(newValue) {
                            transposeModel.toKey = newValue
                        }
                    }

                    StyledDropdown {
                        width: parent.width
                        visible: transposeModel.toKey && transposeModel.canTransposeToKey

                        model: root.choices(transposeModel.keyNames)
                        currentIndex: transposeModel.keyIndex

                        navigation.panel: navPanel
                        navigation.row: 20

                        onActivated: function(index, value) {
                            transposeModel.keyIndex = index
                        }
                    }

                    FlatRadioButtonList {
                        width: parent.width
                        height: 40
                        visible: transposeModel.toKey && transposeModel.canTransposeToKey

                        model: [
                            { "text": qsTrc("TransposeDialogBase", "Closest"), "value": TransposeDialogModel.Closest },
                            { "text": qsTrc("TransposeDialogBase", "Up"), "value": TransposeDialogModel.Up },
                            { "text": qsTrc("TransposeDialogBase", "Down"), "value": TransposeDialogModel.Down }
                        ]
                        currentValue: transposeModel.keyDirection

                        navigationPanel: navPanel
                        navigationRowStart: 30

                        onToggled: function(newValue) {
                            transposeModel.keyDirection = newValue
                        }
                    }

                    StyledDropdown {
                        width: parent.width
                        visible: !transposeModel.toKey || !transposeModel.canTransposeToKey

                        model: root.choices(transposeModel.intervalNames)
                        currentIndex: transposeModel.intervalIndex

                        navigation.panel: navPanel
                        navigation.row: 40

                        onActivated: function(index, value) {
                            transposeModel.intervalIndex = index
                        }
                    }

                    FlatRadioButtonList {
                        width: parent.width
                        height: 40
                        visible: !transposeModel.toKey || !transposeModel.canTransposeToKey

                        model: root.upDown
                        currentValue: transposeModel.intervalUp

                        navigationPanel: navPanel
                        navigationRowStart: 50

                        onToggled: function(newValue) {
                            transposeModel.intervalUp = newValue
                        }
                    }

                    CheckBox {
                        width: parent.width
                        enabled: transposeModel.canTransposeToKey

                        text: qsTrc("TransposeDialogBase", "Transpose key signatures")
                        checked: transposeModel.transposeKeys

                        navigation.panel: navPanel
                        navigation.row: 60

                        onClicked: {
                            transposeModel.transposeKeys = !checked
                        }
                    }
                }

                // Diatonically: by scale degrees

                Column {
                    width: parent.width
                    spacing: 12
                    visible: !transposeModel.chromatic

                    StyledDropdown {
                        width: parent.width

                        model: root.choices(transposeModel.degreeNames)
                        currentIndex: transposeModel.degreeIndex

                        navigation.panel: navPanel
                        navigation.row: 70

                        onActivated: function(index, value) {
                            transposeModel.degreeIndex = index
                        }
                    }

                    FlatRadioButtonList {
                        width: parent.width
                        height: 40

                        model: root.upDown
                        currentValue: transposeModel.degreeUp

                        navigationPanel: navPanel
                        navigationRowStart: 80

                        onToggled: function(newValue) {
                            transposeModel.degreeUp = newValue
                        }
                    }

                    CheckBox {
                        width: parent.width

                        text: qsTrc("TransposeDialogBase", "Keep degree alterations")
                        checked: transposeModel.keepDegreeAlterations

                        navigation.panel: navPanel
                        navigation.row: 90

                        onClicked: {
                            transposeModel.keepDegreeAlterations = !checked
                        }
                    }
                }

                SeparatorLine {}

                StyledTextLabel {
                    text: qsTrc("TransposeDialogBase", "Options")
                    font: ui.theme.bodyBoldFont
                }

                CheckBox {
                    width: parent.width
                    enabled: transposeModel.canTransposeChordNames

                    text: qsTrc("TransposeDialogBase", "Transpose chord symbols and fretboard diagrams")
                    checked: transposeModel.transposeChordNames

                    navigation.panel: navPanel
                    navigation.row: 100

                    onClicked: {
                        transposeModel.transposeChordNames = !checked
                    }
                }

                Column {
                    width: parent.width
                    spacing: 8

                    StyledTextLabel {
                        text: qsTrc("TransposeDialogBase", "Accidentals:")
                    }

                    StyledDropdown {
                        width: parent.width

                        model: [
                            { "text": qsTrc("TransposeDialogBase", "Single ♯ and ♭ only"), "value": false },
                            { "text": qsTrc("TransposeDialogBase", "Use double ♯ and ♭"), "value": true }
                        ]
                        currentIndex: transposeModel.doubleSharpsFlats ? 1 : 0

                        navigation.panel: navPanel
                        navigation.row: 110

                        onActivated: function(index, value) {
                            transposeModel.doubleSharpsFlats = value
                        }
                    }
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
                    transposeModel.apply()
                    root.accept()
                } else {
                    root.reject()
                }
            }
        }
    }
}
