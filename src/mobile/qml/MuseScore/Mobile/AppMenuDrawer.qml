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

//! The app menu for phones: a drawer that shows one menu level at a time (submenus replace their parent menu,
//! with a way back), instead of menus that open beside each other.
Item {
    id: root

    //! The top-level menu items (MenuItem)
    property var menuModel: []
    property string title: ""

    signal handleMenuItem(string itemId)

    //! The submenus opened, from the top level: [{ title, items }]
    property var levels: []

    readonly property var currentItems: root.levels.length > 0 ? root.levels[root.levels.length - 1].items : root.menuModel
    readonly property string currentTitle: root.levels.length > 0 ? root.levels[root.levels.length - 1].title : root.title

    visible: false

    function open() {
        root.levels = []
        root.visible = true
        Qt.callLater(focusFirstItem)
    }

    function close() {
        root.visible = false
        root.levels = []
    }

    //! Back: the parent menu, or closes the drawer
    function back() {
        if (root.levels.length > 0) {
            root.levels = root.levels.slice(0, -1)
            Qt.callLater(focusFirstItem)
        } else {
            root.close()
        }
    }

    function focusFirstItem() {
        listView.positionViewAtBeginning()
        backButton.navigation.requestActive()
    }

    NavigationSection {
        id: navSec
        name: "AppMenuDrawer"
        enabled: root.visible
        order: 100
        type: NavigationSection.Exclusive

        onNavigationEvent: function(event) {
            if (event.type === NavigationEvent.Escape) {
                root.back()
                event.accepted = true
            }
        }
    }

    NavigationPanel {
        id: navPanel
        name: "AppMenuDrawerItems"
        section: navSec
        enabled: root.visible
        order: 1
        direction: NavigationPanel.Vertical
        accessible.name: root.currentTitle
    }

    // Dims the score; a tap outside the drawer closes it
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: 0.3

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    Rectangle {
        id: panel

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        width: Math.min(parent.width - 48, 360)

        color: ui.theme.backgroundPrimaryColor

        // Takes the touches that would go through to the dimmed area
        MouseArea {
            anchors.fill: parent
        }

        Rectangle {
            id: header

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            height: 52

            color: ui.theme.backgroundSecondaryColor

            FlatButton {
                id: backButton

                anchors.left: parent.left
                anchors.leftMargin: 4
                anchors.verticalCenter: parent.verticalCenter

                icon: root.levels.length > 0 ? IconCode.ARROW_LEFT : IconCode.CLOSE_X_ROUNDED
                transparent: true
                toolTipTitle: root.levels.length > 0 ? qsTrc("global", "Back") : qsTrc("global", "Close")

                navigation.panel: navPanel
                navigation.row: 0

                onClicked: {
                    root.back()
                }
            }

            StyledTextLabel {
                anchors.left: backButton.right
                anchors.leftMargin: 8
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter

                horizontalAlignment: Text.AlignLeft
                font: ui.theme.largeBodyBoldFont
                text: root.currentTitle
            }
        }

        SeparatorLine {
            anchors.top: header.bottom
        }

        StyledListView {
            id: listView

            anchors.top: header.bottom
            anchors.topMargin: 1
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            model: root.currentItems

            delegate: Loader {
                id: rowLoader

                required property var modelData
                required property int index

                width: ListView.view.width

                sourceComponent: Boolean(modelData) && Boolean(modelData.title) ? itemComp : separatorComp

                Component {
                    id: separatorComp

                    Item {
                        height: 9

                        SeparatorLine {
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }

                Component {
                    id: itemComp

                    ListItemBlank {
                        id: menuRow

                        readonly property var item: rowLoader.modelData
                        readonly property bool hasSubmenu: Boolean(item.subitems) && item.subitems.length > 0

                        height: 48
                        enabled: item.enabled

                        navigation.panel: navPanel
                        navigation.row: rowLoader.index + 1
                        navigation.accessible.name: item.title
                        navigation.accessible.checked: item.checkable && item.checked

                        StyledIconLabel {
                            id: checkIcon

                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            anchors.verticalCenter: parent.verticalCenter

                            width: 24

                            iconCode: menuRow.item.checkable && menuRow.item.checked
                                      ? IconCode.TICK_RIGHT_ANGLE
                                      : (menuRow.item.icon ?? IconCode.NONE)
                        }

                        StyledTextLabel {
                            anchors.left: checkIcon.right
                            anchors.leftMargin: 12
                            anchors.right: trailing.left
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter

                            horizontalAlignment: Text.AlignLeft
                            text: menuRow.item.title
                        }

                        Item {
                            id: trailing

                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter

                            width: menuRow.hasSubmenu ? submenuIcon.width : shortcutLabel.width
                            height: parent.height

                            StyledIconLabel {
                                id: submenuIcon

                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter

                                visible: menuRow.hasSubmenu
                                iconCode: IconCode.SMALL_ARROW_RIGHT
                            }

                            // For keyboards
                            StyledTextLabel {
                                id: shortcutLabel

                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter

                                visible: !menuRow.hasSubmenu && Boolean(text)
                                width: visible ? implicitWidth : 0
                                opacity: 0.7
                                text: menuRow.item.shortcuts ?? ""
                            }
                        }

                        onClicked: {
                            if (menuRow.hasSubmenu) {
                                root.levels = root.levels.concat([{ "title": menuRow.item.title, "items": menuRow.item.subitems }])
                                Qt.callLater(root.focusFirstItem)
                                return
                            }

                            const itemId = menuRow.item.id
                            root.close()
                            root.handleMenuItem(itemId)
                        }
                    }
                }
            }
        }
    }
}
