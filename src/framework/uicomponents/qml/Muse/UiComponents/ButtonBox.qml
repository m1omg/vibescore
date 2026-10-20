/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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
import QtQml
import QtQuick.Controls
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

Container {
    id: root

    property var buttons: []
    property alias buttonLayout: buttonBoxModel.buttonLayout

    //! The buttons are stacked, each as wide as the box, where their row does not fit the width the box is given
    //! (a narrow window: phones)
    readonly property bool stacked: root.width > 0 && root.width < prv.rowWidth

    // The width of the buttons in a row, also while they are stacked: what the box asks for
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            prv.rowWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             contentHeight + topPadding + bottomPadding)

    padding: 0
    spacing: 12

    property bool isAccessibilityDisabledWhenInit: false
    property NavigationPanel navigationPanel: NavigationPanel {
        name: "ButtonBox"
        enabled: root.enabled && root.visible
        direction: NavigationPanel.Horizontal

        onNavigationEvent: function(event) {
            if (event.type === NavigationEvent.AboutActive) {
                var btn = root.firstFocusBtn
                if (Boolean(btn) && btn.enabled) {
                    event.setData("controlIndex", [ btn.navigation.row, btn.navigation.column ])
                }
            }
        }
    }


    property var firstFocusBtn: {
        var btn = accentButton()
        if (!Boolean(btn)) {
            btn = root.itemAt(0)
        }

        return btn
    }

    signal standardButtonClicked(int buttonId)

    function accentButton() {
        for (var i = 0; i < root.count; i++) {
            var btn = root.itemAt(i) as FlatButton
            if (btn.accentButton) {
                return btn
            }
        }

        return null
    }

    function addButton(text, buttonId, buttonRole, isAccent, isLeftSide) {
        const button = Qt.createQmlObject("import Muse.UiComponents; FlatButton {}", root) as FlatButton
        button.text = text
        button.accentButton = isAccent

        button.buttonId = buttonId
        button.buttonRole = buttonRole
        button.isLeftSide = isLeftSide

        button.navigation.panel = root.navigationPanel
        button.navigation.column = root.count
        button.accessible.ignored = root.isAccessibilityDisabledWhenInit
        button.Layout.fillWidth = Qt.binding(function() { return root.stacked })

        root.addItem(button)

        return button
    }

    function restoreAccessibility() {
        for (var i = 0; i < root.count; i++) {
            var btn = root.itemAt(i) as FlatButton
            if (!Boolean(btn.navigation)) {
                continue
            }

            btn.accessible.ignored = false
        }
    }

    contentItem: GridLayout {
        // A row, or a column when stacked
        columns: root.stacked ? 1 : Math.max(root.count, 1)
        rowSpacing: root.spacing
        columnSpacing: root.spacing
        Repeater {
            model: root.contentModel
        }
    }

    ButtonBoxModel {
        id: buttonBoxModel

        buttonsItems: root.contentChildren
    }

    Connections {
        target: buttonBoxModel

        function onAddButton(text, buttonId, buttonRole, isAccent, isLeftSide) {
            const button = root.addButton(text, buttonId, buttonRole, isAccent, isLeftSide)

            button.clicked.connect(function() {
                root.standardButtonClicked(buttonId)
            })
        }

        function onReloadRequested() {
            prv.layoutButtons()
        }
    }

    Component.onCompleted: {
        buttonBoxModel.setButtons(root.buttons)
        prv.layoutButtons()
    }

    QtObject {
        id: prv

        //! The width of the buttons in a row (also while they are stacked)
        readonly property real rowWidth: {
            var width = 0
            var buttonCount = 0
            for (var i = 0; i < root.count; i++) {
                var item = root.itemAt(i)
                if (!Boolean(item) || Boolean(item.isSeparator) || !item.visible) {
                    continue
                }
                width += item.implicitWidth
                buttonCount++
            }
            return width + Math.max(buttonCount - 1, 0) * root.spacing
        }

        function layoutButtons() {
            var buttonsTypes = buttonBoxModel.load()

            removeSeparator()

            var lastLeftSideButtonType = -1
            var buttonsWidths = 0

            for (var i = buttonsTypes.length - 1; i >= 0; i--) {
                var buttonInfo = prv.buttonInfo(buttonsTypes[i])
                if (!Boolean(buttonInfo)) {
                    continue
                }

                var btn = root.itemAt(i) as FlatButton
                if (buttonInfo.button !== btn) {
                    root.moveItem(buttonInfo.index, i)
                }

                buttonInfo.button.navigation.panel = root.navigationPanel
                buttonInfo.button.navigation.column = i

                //! NOTE See description about AccessibleItem { id: accessibleInfo }
                buttonInfo.button.accessible.ignored = root.isAccessibilityDisabledWhenInit
                const _buttonId = buttonInfo.button.buttonId

                if (lastLeftSideButtonType === -1 && buttonInfo.button.isLeftSide) {
                    lastLeftSideButtonType = buttonsTypes[i]
                }

                if (buttonInfo.button.visible) {
                    buttonsWidths += buttonInfo.button.width
                }
            }

            if (buttonsWidths + buttonsTypes.length * root.spacing > root.width) {
                return
            }

            insertSeparator(lastLeftSideButtonType)
        }

        function buttonInfo(buttonType) {
            for (var i = 0; i < root.count; i++) {
                var btn = root.itemAt(i) as FlatButton
                if (!Boolean(btn)) {
                    continue
                }

                if (buttonType === btn.buttonId) {
                    return { index: i, button: btn }
                }
            }

            return null
        }

        function separatorInfo() {
            for (var i = 0; i < root.count; i++) {
                var item = root.itemAt(i)
                if (Boolean(item.isSeparator)) {
                    return { index: i, separator: item }
                }
            }

            return null
        }

        function removeSeparator() {
            var separatorInfo = prv.separatorInfo()
            if (Boolean(separatorInfo)) {
                root.removeItem(separatorInfo.index)
            }
        }

        function insertSeparator(lastLeftSideButtonType) {
            var index = 0
            if (lastLeftSideButtonType !== -1) {
                var lastLeftSideButtonInfo = buttonInfo(lastLeftSideButtonType)
                if (Boolean(lastLeftSideButtonInfo)) {
                    index = lastLeftSideButtonInfo.index + 1
                }
            }

            var separator = separatorComp.createObject(root)
            root.addItem(separator)

            for (var i = root.count - 2; i >= 0; i--) {
                var button = root.itemAt(i) as FlatButton
                if (button.buttonId === lastLeftSideButtonType) {
                    break;
                }

                root.moveItem(i, i + 1)
            }
        }
    }

    Component {
        id: separatorComp

        Item {
            property bool isSeparator: true
            property string text: "separator"
            // Between the buttons of the left side and the others; nothing to keep apart in a column
            visible: !root.stacked
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}
