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
#pragma once

#include <functional>

#include <QByteArray>
#include <QString>
#include <QStringList>

//! The browser: what the system does for the mobile app (choosing, sharing and printing files)
namespace mu::mobile::web {
//! Asks the user for a file of their device (the browser's file chooser); the callback gets its name and content,
//! or an empty name when nothing was chosen
void openFile(const QString& nameFilter, const std::function<void(const QString& fileName, const QByteArray& content)>& done);

//! Gives the files to the user: the browser downloads them
void downloadFiles(const QStringList& paths);

//! Shows the browser's print dialog for a PDF
void printPdf(const QString& path, const QString& jobName);

//! Writes the folders kept in the browser's storage (IndexedDB) back now, e.g. after a score was saved
void syncFileSystem();

//! The page shows that the app is busy (e.g. "Opening “name”…", before a big score loads), with a spinner that the
//! browser keeps turning while the app's thread is busy and nothing of the app can be drawn (see vibescore-web.js);
//! an empty title takes it away again
void showBusy(const std::string& title);

//! Works around a bug of Qt 6.10 that breaks the event processing after a dialog that waits for an answer
//! (a nested event loop); call it once the application object exists
void fixNestedEventLoops();

//! The page tells the app when it is hidden or shown again (see vibescore-web.js): the application state becomes
//! inactive or active, as for an app that is left on Android or iOS (e.g. its changes are autosaved; the system may
//! close the page in the background without notice). The app writes its files to the browser's storage then.
//! Call it once the application object exists
void watchPageVisibility();

//! The page tells the app that it added a sound font (it downloads it after the app started, see vibescore-web.js):
//! Module.vibescoreSoundFontsChanged()
void setSoundFontsChangedHandler(const std::function<void()>& handler);
}
