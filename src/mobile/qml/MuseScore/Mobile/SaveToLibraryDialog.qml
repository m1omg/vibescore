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

//! Asks for the name of a score that is saved into the Scores library (see MobileInteractive)
StyledDialogView {
    id: root

    property string name: ""
    property string folderName: ""

    title: qsTrc("mobile", "Save score")

    contentWidth: 440
    contentHeight: content.implicitHeight
    margins: 20

    function save() {
        const trimmedName = root.name.trim()
        if (trimmedName === "") {
            return
        }

        root.ret = { errcode: 0, value: trimmedName }
        root.hide()
    }

    onNavigationActivateRequested: {
        nameField.navigation.requestActive()
        nameField.ensureActiveFocus()
    }

    ColumnLayout {
        id: content

        anchors.left: parent.left
        anchors.right: parent.right

        spacing: 16

        NavigationPanel {
            id: navPanel
            name: "SaveToLibrary"
            section: root.navigationSection
            order: 1
        }

        StyledTextLabel {
            Layout.fillWidth: true

            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap

            text: qsTrc("mobile", "Name of the score in the “%1” folder:").arg(root.folderName)
        }

        TextInputField {
            id: nameField

            Layout.fillWidth: true

            currentText: root.name
            hint: qsTrc("mobile", "Score name")

            navigation.panel: navPanel
            navigation.row: 1

            onTextChanged: function(newTextValue) {
                root.name = newTextValue
            }

            onAccepted: {
                root.save()
            }
        }

        ButtonBox {
            Layout.fillWidth: true

            buttons: [ ButtonBoxModel.Cancel, ButtonBoxModel.Save ]

            navigationPanel.section: root.navigationSection
            navigationPanel.order: 2

            onStandardButtonClicked: function(buttonId) {
                if (buttonId === ButtonBoxModel.Save) {
                    root.save()
                } else {
                    root.reject()
                }
            }
        }
    }
}
