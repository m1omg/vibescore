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

//! Select (Select > More…), for touch screens: the filters and actions of the QWidget SelectDialog, with the same texts
StyledDialogView {
    id: root

    title: qsTrc("SelectDialog", "Select")

    contentWidth: 440
    contentHeight: 560
    margins: 16
    fillsCompactWindow: true

    SelectDialogModel {
        id: selectModel
    }

    Component.onCompleted: {
        selectModel.load()
    }

    NavigationPanel {
        id: navPanel
        name: "SelectDialog"
        section: root.navigationSection
        direction: NavigationPanel.Vertical
        order: 1
    }

    //! { text, property, enabled }: the filters, as check boxes
    readonly property var filters: [
        { "text": qsTrc("SelectDialog", "Same staff"), "property": "sameStaff", "enabled": true },
        { "text": qsTrc("SelectDialog", "Same voice"), "property": "sameVoice", "enabled": true },
        { "text": qsTrc("SelectDialog", "Same duration"), "property": "sameDuration", "enabled": selectModel.isRest },
        { "text": qsTrc("SelectDialog", "Same beat"), "property": "sameBeat", "enabled": true },
        { "text": qsTrc("SelectDialog", "Same measure"), "property": "sameMeasure", "enabled": true },
        { "text": qsTrc("SelectDialog", "Same system"), "property": "sameSystem", "enabled": true },
        { "text": qsTrc("SelectDialog", "In selection"), "property": "inSelection", "enabled": !selectModel.isSingleSelection }
    ]

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
                spacing: 12

                StyledTextLabel {
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    text: qsTrc("SelectDialog", "Element type:") + " " + selectModel.elementType
                }

                CheckBox {
                    width: parent.width
                    enabled: selectModel.hasSubtype

                    text: qsTrc("SelectDialog", "Same subtype:") + " " + selectModel.elementSubtype
                    checked: selectModel.sameSubtype

                    navigation.panel: navPanel
                    navigation.row: 0

                    onClicked: {
                        selectModel.sameSubtype = !checked
                    }
                }

                SeparatorLine {}

                Repeater {
                    model: root.filters

                    CheckBox {
                        required property var modelData
                        required property int index

                        width: parent.width
                        enabled: modelData.enabled

                        text: modelData.text
                        checked: selectModel[modelData.property]

                        navigation.panel: navPanel
                        navigation.row: 1 + index

                        onClicked: {
                            selectModel[modelData.property] = !checked
                        }
                    }
                }

                SeparatorLine {}

                StyledTextLabel {
                    text: qsTrc("SelectDialog", "Action")
                    font: ui.theme.bodyBoldFont
                }

                Repeater {
                    model: [
                        { "text": qsTrc("SelectDialog", "Replace selection"), "value": SelectDialogModel.ReplaceSelection },
                        { "text": qsTrc("SelectDialog", "Add to selection"), "value": SelectDialogModel.AddToSelection },
                        { "text": qsTrc("SelectDialog", "Search in selection"), "value": SelectDialogModel.SearchInSelection },
                        { "text": qsTrc("SelectDialog", "Subtract from selection"), "value": SelectDialogModel.SubtractFromSelection }
                    ]

                    RoundedRadioButton {
                        required property var modelData
                        required property int index

                        width: parent.width
                        height: 40

                        text: modelData.text
                        checked: selectModel.action === modelData.value

                        navigation.panel: navPanel
                        navigation.row: 20 + index

                        onToggled: {
                            selectModel.action = modelData.value
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
                    selectModel.apply()
                    root.accept()
                } else {
                    root.reject()
                }
            }
        }
    }
}
