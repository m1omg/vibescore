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

//! Says what the app is busy with, e.g. opening a big score (see MobileInteractive::showProgress). It has no buttons
//! and nothing moves in it: the app cannot respond or draw anything until it is done, then the dialog is closed.
StyledDialogView {
    id: root

    property string text: ""

    title: root.text

    contentWidth: Math.min(Math.max(label.implicitWidth, 240), 440)
    contentHeight: label.implicitHeight
    margins: 24

    // No title bar: its close button would suggest that the work can be cancelled
    frameless: true
    closeOnEscape: false

    StyledTextLabel {
        id: label

        anchors.left: parent.left
        anchors.right: parent.right

        text: root.text
        font: ui.theme.largeBodyFont
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }
}
