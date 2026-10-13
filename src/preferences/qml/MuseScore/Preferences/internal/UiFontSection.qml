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

import Muse.UiComponents

BaseSection {
    id: root

    title: qsTrc("preferences", "App text")

    property alias allFonts: selectFontControl.model

    property alias currentFontIndex: selectFontControl.currentIndex
    property alias bodyTextSize: bodyTextSizeControl.currentValue

    property bool isUiScaleAvailable: false
    property real uiScale: 1.0
    property bool isUiScaleRestartRequired: false

    signal fontChangeRequested(var newFontIndex)
    signal bodyTextSizeChangeRequested(var newBodyTextSize)
    signal uiScaleChangeRequested(var newUiScale)

    ComboBoxWithTitle {
        id: selectFontControl

        title: qsTrc("preferences", "Typeface")
        columnWidth: root.columnWidth

        navigation.name: "FontFaceBox"
        navigation.panel: root.navigation
        navigation.row: 1

        onValueEdited: function(newIndex, newValue) {
            root.fontChangeRequested(newIndex)
        }
    }

    IncrementalPropertyControlWithTitle {
        id: bodyTextSizeControl

        title: qsTrc("preferences", "Body text size")
        columnWidth: root.columnWidth
        control.width: 112

        minValue: 10
        maxValue: 18

        //: Abbreviation of "points", used to specify a font size
        measureUnitsSymbol: qsTrc("global", "pt")

        navigation.name: "BodyTextControl"
        navigation.panel: root.navigation
        navigation.row: 2

        onValueEdited: function(newValue) {
            root.bodyTextSizeChangeRequested(newValue)
        }
    }

    ComboBoxWithTitle {
        id: uiScaleControl

        visible: root.isUiScaleAvailable

        title: qsTrc("preferences", "Interface scale")
        columnWidth: root.columnWidth

        model: [
            { "text": "70%", "value": 0.7 },
            { "text": "80%", "value": 0.8 },
            { "text": "90%", "value": 0.9 },
            { "text": "100%", "value": 1.0 },
            { "text": "115%", "value": 1.15 },
            { "text": "130%", "value": 1.3 },
            { "text": "150%", "value": 1.5 }
        ]

        currentIndex: Math.max(0, indexOfValue(root.uiScale))

        navigation.name: "UiScaleBox"
        navigation.panel: root.navigation
        navigation.row: 3

        onValueEdited: function(newIndex, newValue) {
            root.uiScaleChangeRequested(newValue)
        }
    }

    StyledTextLabel {
        width: parent.width

        visible: root.isUiScaleAvailable && root.isUiScaleRestartRequired
        text: qsTrc("preferences", "The new interface scale is used when the app starts again.")
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
    }
}
