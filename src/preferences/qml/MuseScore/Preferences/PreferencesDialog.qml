/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
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
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents
import MuseScore.Preferences

import "internal"

StyledDialogView {
    id: root

    title: qsTrc("preferences", "Preferences")

    //! Phones: the dialog takes the whole window, and the list of pages and the pages are shown one at a time
    fillsCompactWindow: true
    readonly property bool isCompact: (Qt.platform.os === "android" || Qt.platform.os === "ios")
                                      && mainLayout.width > 0 && mainLayout.width < 600

    contentWidth: 880
    contentHeight: 640
    resizable: true

    property string currentPageId: ""
    property var params: null

    property QtObject prv: QtObject {
        property var pagesObjects: (new Map())

        //! Compact layout: a page is shown (instead of the list of pages)
        property bool isPageShown: false

        function currentPageTitle() {
            const pages = preferencesModel.availablePages()
            for (const page of pages) {
                if (page.id === preferencesModel.currentPageId) {
                    return page.title
                }
            }
            return ""
        }

        function resolveStackCurrentIndex() {
            var keys = Object.keys(root.prv.pagesObjects)
            return keys.indexOf(preferencesModel.currentPageId)
        }

        function updateStackCurrentIndex() {
            stack.currentIndex = resolveStackCurrentIndex()
        }
    }

    Component.onCompleted: {
        preferencesModel.load(root.currentPageId)

        initPagesObjects()

        prv.updateStackCurrentIndex()

        // A page that was asked for is shown straight away
        prv.isPageShown = root.currentPageId !== ""
    }

    function initPagesObjects() {
        var pages = preferencesModel.availablePages()
        for (var i in pages) {
            var pageInfo = pages[i]

            if (!Boolean(pageInfo.path)) {
                continue
            }

            var pageComponent = Qt.createComponent("../" + pageInfo.path)
            if (pageComponent.status === Component.Error) {
                console.error("Failed to load preferences page component:", pageComponent.errorString())
                continue
            }

            var properties = {
                navigationSection: root.navigationSection,
                navigationOrderStart: (i + 1) * 100
            }

            if (root.currentPageId === pageInfo.id) {
                var params = root.params
                for (var key in params) {
                    var value = params[key]
                    properties[key] = value
                }
            }

            var obj = pageComponent.createObject(stack, properties)

            if (!Boolean(obj)) {
                continue
            }

            obj.hideRequested.connect(function() {
                root.hide()
            })

            root.prv.pagesObjects[pageInfo.id] = obj
        }
    }

    PreferencesModel {
        id: preferencesModel

        onCurrentPageIdChanged: function(currentPageId) {
            prv.updateStackCurrentIndex()
        }

        onUserAgreesToResetPreferences: {
            var pages = preferencesModel.availablePages()

            for (var i in pages) {
                var page = pages[i]
                var obj = root.prv.pagesObjects[page.id]
                obj.reset()
            }

            Qt.callLater(preferencesModel.resetFactorySettings)
        }
    }

    ColumnLayout {
        id: mainLayout

        anchors.fill: parent

        spacing: 0

        // Compact layout: the title of the page, with the way back to the list of pages
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 48

            visible: root.isCompact && prv.isPageShown
            spacing: 4

            FlatButton {
                Layout.leftMargin: 4

                icon: IconCode.ARROW_LEFT
                transparent: true
                toolTipTitle: qsTrc("global", "Back")

                navigation.panel: menu.navigation
                navigation.row: -1

                onClicked: {
                    prv.isPageShown = false
                }
            }

            StyledTextLabel {
                Layout.fillWidth: true

                horizontalAlignment: Text.AlignLeft
                font: ui.theme.largeBodyBoldFont
                text: prv.isPageShown ? prv.currentPageTitle() : ""
            }
        }

        SeparatorLine {
            visible: root.isCompact && prv.isPageShown
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            spacing: 0

            PreferencesMenu {
                id: menu

                Layout.fillHeight: true
                Layout.fillWidth: root.isCompact
                Layout.preferredWidth: 220

                visible: !root.isCompact || !prv.isPageShown

                navigation.section: root.navigationSection
                navigation.order: 1

                model: preferencesModel

                onPageClicked: {
                    prv.isPageShown = true
                }
            }

            SeparatorLine {
                orientation: Qt.Vertical
                visible: !root.isCompact
            }

            StackLayout {
                id: stack

                visible: !root.isCompact || prv.isPageShown
            }
        }

        SeparatorLine { }

        PreferencesButtonsPanel {
            id: buttonsPanel

            Layout.fillWidth: true
            Layout.preferredHeight: 70

            isCompact: root.isCompact

            navigation.section: root.navigationSection
            navigation.order: 100000

            onRevertFactorySettingsRequested: {
                preferencesModel.askForConfirmationOfPreferencesReset()
            }

            onRejectRequested: {
                preferencesModel.cancel()
                root.reject()
            }

            onApplyRequested: {
                preferencesModel.apply()

                var ok = true
                var pages = preferencesModel.availablePages()

                for (var i in pages) {
                    var page = pages[i]
                    var obj = root.prv.pagesObjects[page.id]
                    ok &= obj.apply()
                }

                if (ok) {
                    root.hide()
                }
            }
        }
    }
}
