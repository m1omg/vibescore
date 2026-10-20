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

import Muse.Ui
import Muse.UiComponents
import MuseScore.Mobile

//! The content of the mobile window: the current page (Home or the score)
MobileShellView {
    id: root

    property string scoreTitle: ""

    //! Phones (in either orientation): the panels open over the score, one at a time; tablets: beside it
    readonly property bool isPhone: Math.min(width, height) < 600
    //! Narrow: phones in portrait
    readonly property bool isCompact: width < 600

    panelsOverlay: isPhone
    exclusiveGroups: isPhone ? [ "side", "bottom" ] : []

    InteractiveProvider {
        id: interactiveProvider

        topParent: root

        onRequestedDockPage: function(uri, params) {
            root.loadPage(uri, params)
        }
    }

    onPageLoaded: {
        interactiveProvider.onPageOpened()
    }

    //! Called by MobileShellView for the Back key: closes what the current page shows over its content
    function closeOverlay() {
        const page = notationLoader.visible ? notationLoader.item : null
        return Boolean(page) && page.closeOverlay()
    }

    Loader {
        id: homeLoader

        anchors.fill: parent

        property bool wasShown: false
        readonly property bool isCurrent: !root.isNotationPage(root.currentPageUri)

        active: isCurrent || wasShown
        visible: isCurrent
        onIsCurrentChanged: if (isCurrent) wasShown = true

        sourceComponent: MobileHomePage {
            params: root.currentPageParams
            isPhone: root.isPhone
            isCompact: root.isCompact
        }
    }

    Loader {
        id: notationLoader

        anchors.fill: parent

        property bool wasShown: false
        readonly property bool isCurrent: root.isNotationPage(root.currentPageUri)

        active: isCurrent || wasShown
        visible: isCurrent
        onIsCurrentChanged: if (isCurrent) wasShown = true

        sourceComponent: MobileNotationPage {
            shell: root
            scoreTitle: root.scoreTitle
            isPhone: root.isPhone
            isCompact: root.isCompact
        }
    }
}
