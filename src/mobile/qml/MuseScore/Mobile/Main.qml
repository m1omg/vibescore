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
import QtQuick.Controls

import Muse.Ui
import Muse.UiComponents
import Muse.Shortcuts
import Muse.Toast
import MuseScore.AppShell

//! The main window of the mobile app (see GuiApp; MUSE_DESKTOP_SHELL=1 loads the desktop window instead)
ApplicationWindow {
    id: root

    objectName: "ApplicationWindow"

    title: titleProvider.title

    visible: false

    color: ui.theme.backgroundPrimaryColor

    Component.onCompleted: {
        ui.rootItem = root.contentItem
        titleProvider.load()
    }

    MainWindowTitleProvider {
        id: titleProvider
    }

    MainWindowBridge {
        id: bridge

        window: root

        filePath: titleProvider.filePath
        fileModified: titleProvider.fileModified
    }

    ToolTipProvider { }

    //! NOTE Need only create (keyboard shortcuts, e.g. of a hardware keyboard)
    Shortcuts { }

    MobileShell {
        anchors.fill: parent

        scoreTitle: titleProvider.title

        onPageLoaded: {
            // The window is transparent until its content is ready (see GuiApp)
            root.opacity = 1.0
        }
    }

    ToastProvider {
        navigationOrder: 100000
    }
}
