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

//! A sheet that shows the panels of a group one at a time, with tabs to switch between them.
//! Which panel is shown comes from the shell's open docks (see MobileShellView).
Rectangle {
    id: root

    //! [{ name, title, component }]
    property var panels: []
    property var shell: null
    property NavigationSection navigationSection: null
    //! The panels are created over a few frames, after what shows them is on the screen: e.g. the palettes, open
    //! beside the score on tablets, were most of the time it took to open a small score
    property bool asynchronousPanels: false

    readonly property int currentIndex: {
        if (!root.shell) {
            return -1
        }

        for (let i = 0; i < root.panels.length; ++i) {
            if (root.shell.openDocks.indexOf(root.panels[i].name) >= 0) {
                return i
            }
        }

        return -1
    }

    readonly property bool isOpen: currentIndex >= 0
    readonly property var currentPanelItem: {
        const loader = contentRepeater.itemAt(root.currentIndex)
        return loader ? loader.item : null
    }

    //! The height the sheet needs for the current panel, or 0 when the panel can take any height
    readonly property real preferredHeight: currentPanelItem && currentPanelItem.preferredSheetContentHeight
                                            ? header.height + 1 + currentPanelItem.preferredSheetContentHeight : 0

    color: ui.theme.backgroundPrimaryColor

    NavigationPanel {
        id: tabsNavPanel
        name: "SheetTabs"
        section: root.navigationSection
        order: 0
        direction: NavigationPanel.Horizontal
    }

    Rectangle {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        height: 44

        color: ui.theme.backgroundSecondaryColor

        ListView {
            id: tabs

            anchors.left: parent.left
            anchors.right: menuButton.visible ? menuButton.left : closeButton.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 4

            orientation: ListView.Horizontal
            spacing: 2
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            model: root.panels

            delegate: FlatButton {
                required property var modelData
                required property int index

                anchors.verticalCenter: parent ? parent.verticalCenter : undefined
                height: 36

                text: modelData.title
                orientation: Qt.Horizontal
                isNarrow: true
                transparent: index !== root.currentIndex
                accentButton: index === root.currentIndex

                navigation.panel: tabsNavPanel
                navigation.column: index

                onClicked: {
                    root.shell.setDockOpen(modelData.name, true)
                }
            }
        }

        MenuButton {
            id: menuButton

            anchors.right: closeButton.left
            anchors.verticalCenter: parent.verticalCenter

            readonly property var menuModel_: root.currentPanelItem && root.currentPanelItem.contextMenuModel
                                               ? root.currentPanelItem.contextMenuModel : null

            visible: Boolean(menuModel_) && menuModel_.items && menuModel_.items.length > 0
            menuModel: menuModel_ ? menuModel_.items : []

            icon: IconCode.MENU_THREE_DOTS
            transparent: true

            navigation.panel: tabsNavPanel
            navigation.column: 1000

            onHandleMenuItem: function(itemId) {
                menuModel_.handleMenuItem(itemId)
            }
        }

        FlatButton {
            id: closeButton

            anchors.right: parent.right
            anchors.rightMargin: 4
            anchors.verticalCenter: parent.verticalCenter

            icon: IconCode.CLOSE_X_ROUNDED
            transparent: true
            toolTipTitle: qsTrc("global", "Close")

            navigation.panel: tabsNavPanel
            navigation.column: 1001

            onClicked: {
                if (root.isOpen) {
                    root.shell.setDockOpen(root.panels[root.currentIndex].name, false)
                }
            }
        }
    }

    SeparatorLine {
        anchors.top: header.bottom
    }

    Item {
        id: content

        anchors.top: header.bottom
        anchors.topMargin: 1
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Repeater {
            id: contentRepeater

            model: root.panels

            Loader {
                id: panelLoader

                required property var modelData
                required property int index

                readonly property bool isCurrent: index === root.currentIndex
                property bool wasShown: false

                anchors.fill: parent

                // Created when first shown, then kept (with its state)
                active: isCurrent || wasShown
                asynchronous: root.asynchronousPanels
                // Not while it is created over a few frames, half done
                visible: isCurrent && status === Loader.Ready
                onIsCurrentChanged: if (isCurrent) wasShown = true

                sourceComponent: modelData.component
            }
        }
    }
}
