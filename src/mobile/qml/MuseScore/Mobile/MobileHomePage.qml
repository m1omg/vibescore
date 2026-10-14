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
import Muse.Learn
import MuseScore.AppShell
import MuseScore.Project

//! Home: the scores, plugins and learning resources, with a menu on the side (tablets) or at the bottom (phones in portrait)
Item {
    id: root

    property var params: ({})
    //! Phones (in either orientation)
    property bool isPhone: false
    //! Phones in portrait
    property bool isCompact: false

    property string section: "scores"
    property string subSection: ""

    onParamsChanged: {
        if (root.params && Boolean(root.params["section"])) {
            const section = root.params["section"]
            root.section = (section === "plugins") ? "extensions" : section
            root.subSection = Boolean(root.params["subSection"]) ? root.params["subSection"] : ""
        }
    }

    HomeMenuModel {
        id: homeMenuModel
    }

    function select(name) {
        root.subSection = ""
        root.section = name
    }

    function openPreferences() {
        api.launcher.open("muse://preferences")
    }

    Rectangle {
        id: menuPanel

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left

        width: visible ? (root.isPhone ? 76 : 240) : 0
        visible: !root.isCompact

        color: ui.theme.backgroundPrimaryColor

        HomeMenu {
            id: homeMenu

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: settingsButton.top

            currentPageName: root.section
            iconsOnly: root.isPhone

            onSelected: function(name) {
                root.select(name)
            }
        }

        FlatButton {
            id: settingsButton

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 12

            height: 48

            icon: IconCode.SETTINGS_COG
            text: root.isPhone ? "" : qsTrc("appshell/preferences", "Preferences")
            orientation: Qt.Horizontal
            transparent: true
            toolTipTitle: root.isPhone ? qsTrc("appshell/preferences", "Preferences") : ""

            onClicked: {
                root.openPreferences()
            }
        }
    }

    SeparatorLine {
        anchors.left: menuPanel.right
        orientation: Qt.Vertical
        visible: menuPanel.visible
    }

    Loader {
        anchors.top: parent.top
        anchors.bottom: bottomBar.top
        anchors.left: menuPanel.right
        anchors.right: parent.right

        sourceComponent: {
            switch (root.section) {
            case "extensions": return pluginsComp
            case "learn": return learnComp
            }

            return scoresComp
        }
    }

    // Phones in portrait: navigation at the bottom

    NavigationSection {
        id: bottomBarNavSec
        name: "HomeBottomBar"
        enabled: bottomBar.visible
        order: 2
    }

    NavigationPanel {
        id: bottomBarNavPanel
        name: "HomeBottomBarPanel"
        section: bottomBarNavSec
        enabled: bottomBar.visible
        order: 1
        direction: NavigationPanel.Horizontal
    }

    Rectangle {
        id: bottomBar

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        height: visible ? 64 : 0
        visible: root.isCompact

        color: ui.theme.backgroundPrimaryColor

        SeparatorLine {
            anchors.top: parent.top
        }

        Row {
            anchors.fill: parent
            anchors.topMargin: 1

            Repeater {
                id: bottomBarItems

                model: {
                    let items = [ { "name": "scores", "title": qsTrc("appshell", "Scores"), "icon": IconCode.MUSIC_NOTES } ]

                    if (homeMenuModel.isPluginsAvailable) {
                        items.push({ "name": "extensions", "title": qsTrc("appshell", "Plugins"), "icon": IconCode.PLUGIN })
                    }

                    if (homeMenuModel.isLearnAvailable) {
                        items.push({ "name": "learn", "title": qsTrc("appshell", "Learn"), "icon": IconCode.MORTAR_BOARD })
                    }

                    items.push({ "name": "preferences", "title": qsTrc("appshell/preferences", "Preferences"), "icon": IconCode.SETTINGS_COG })
                    return items
                }

                FlatButton {
                    required property var modelData
                    required property int index

                    readonly property bool isCurrent: modelData.name === root.section

                    width: bottomBar.width / bottomBarItems.count
                    height: bottomBar.height - 1

                    icon: modelData.icon
                    text: modelData.title
                    orientation: Qt.Vertical
                    transparent: !isCurrent
                    accentButton: isCurrent

                    navigation.panel: bottomBarNavPanel
                    navigation.column: index

                    onClicked: {
                        if (modelData.name === "preferences") {
                            root.openPreferences()
                        } else {
                            root.select(modelData.name)
                        }
                    }
                }
            }
        }
    }

    Component {
        id: scoresComp

        ScoresPage {
            subSection: root.subSection
        }
    }

    Component {
        id: pluginsComp

        PluginsPage {}
    }

    Component {
        id: learnComp

        LearnPage {
            section: root.subSection
        }
    }
}
