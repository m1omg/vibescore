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
import QtQuick.Templates as T

import Muse.Ui
import Muse.UiComponents

//! Hosts the content of a WindowView (popup, menu, dropdown, tooltip or dialog) inside the
//! main window, for platforms with a single window (mobile). Created by WindowView in C++.
T.Popup {
    id: root

    //! The WindowView's content item, shown inside this popup
    property Item viewContent: null

    //! Dialogs get a title bar (unless frameless), are modal and are centered
    property bool isDialog: false
    property bool frameless: true
    property string title: ""

    //! Maximum area available for the content (the window minus margins)
    property real maxContentWidth: parent ? parent.width : 0
    property real maxContentHeight: parent ? parent.height : 0

    //! The parts of the window that system bars and display cut-outs cover (edge-to-edge windows on mobile)
    readonly property real safeTop: parent ? parent.SafeArea.margins.top : 0
    readonly property real safeBottom: parent ? parent.SafeArea.margins.bottom : 0
    readonly property real safeLeft: parent ? parent.SafeArea.margins.left : 0
    readonly property real safeRight: parent ? parent.SafeArea.margins.right : 0

    readonly property real spaceWidth: maxContentWidth - safeLeft - safeRight
    readonly property real spaceHeight: maxContentHeight - safeTop - safeBottom

    //! Dialogs that are made for it (a "fillsCompactWindow" property on their content that is true)
    //! take the whole of a narrow window (phones)
    readonly property bool fillsWindow: isDialog && Boolean(viewContent) && Boolean(viewContent.fillsCompactWindow)
                                        && spaceWidth < 600

    //! The close button of a dialog was pressed; the view decides whether to close
    signal closeRequested()

    function attachContent(item) {
        root.viewContent = item
        if (item) {
            item.parent = flickable.contentItem
            item.x = 0
            item.y = 0

            // Like the window this host replaces, which resizes its content to the content's implicit size
            // (the content's own size bindings may depend on that size, e.g. StyledDialogView)
            item.width = Qt.binding(function() { return root.fillsWindow ? root.spaceWidth : item.implicitWidth })
            item.height = Qt.binding(function() {
                return root.fillsWindow ? root.spaceHeight - titleBar.height : item.implicitHeight
            })
        }
    }

    // The templates have no size of their own: size to the content, like the styled Popup
    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    padding: 0
    topMargin: root.isDialog ? root.safeTop : 0
    bottomMargin: root.isDialog ? root.safeBottom : 0
    leftMargin: root.isDialog ? root.safeLeft : 0
    rightMargin: root.isDialog ? root.safeRight : 0
    background: null
    focus: true

    // Dialogs are centered in the window; popups are positioned by the view
    anchors.centerIn: root.isDialog ? root.parent : undefined

    // Without animations, like the separate windows they replace
    enter: null
    exit: null

    T.Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.4)
    }

    contentItem: Item {
        id: container

        implicitWidth: Math.min(column.implicitWidth, root.isDialog ? root.spaceWidth : root.maxContentWidth)
        implicitHeight: Math.min(column.implicitHeight, root.isDialog ? root.spaceHeight : root.maxContentHeight)

        Rectangle {
            anchors.fill: parent
            visible: root.isDialog && !root.frameless
            color: ui.theme.backgroundPrimaryColor
            border.width: 1
            border.color: ui.theme.strokeColor
        }

        Item {
            id: column

            width: parent.width
            height: parent.height

            implicitWidth: Math.max(titleBar.implicitWidth, root.viewContent ? root.viewContent.width : 0)
            implicitHeight: titleBar.height + (root.viewContent ? root.viewContent.height : 0)

            Item {
                id: titleBar

                width: column.width
                height: visible ? 48 : 0
                // Only dialogs with a title of their own (the default title is the app name)
                visible: root.isDialog && !root.frameless && root.title !== ""
                implicitWidth: titleLabel.implicitWidth + closeButton.width + 32

                StyledTextLabel {
                    id: titleLabel

                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.right: closeButton.left
                    anchors.verticalCenter: parent.verticalCenter

                    text: root.title
                    font: ui.theme.bodyBoldFont
                    horizontalAlignment: Text.AlignLeft
                }

                FlatButton {
                    id: closeButton

                    anchors.right: parent.right
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter

                    width: 40
                    height: 40

                    icon: IconCode.CLOSE_X_ROUNDED
                    transparent: true

                    onClicked: root.closeRequested()
                }

                SeparatorLine {
                    anchors.bottom: parent.bottom
                }
            }

            //! Scrolls the content when it is larger than the window (e.g. big dialogs on phones)
            Flickable {
                id: flickable

                y: titleBar.height

                width: Math.min(contentWidth, root.isDialog ? root.spaceWidth : root.maxContentWidth)
                height: Math.min(contentHeight, (root.isDialog ? root.spaceHeight : root.maxContentHeight) - titleBar.height)

                contentWidth: root.viewContent ? root.viewContent.width : 0
                contentHeight: root.viewContent ? root.viewContent.height : 0

                clip: contentWidth > width || contentHeight > height
                interactive: clip
                boundsBehavior: Flickable.StopAtBounds
            }
        }
    }
}
