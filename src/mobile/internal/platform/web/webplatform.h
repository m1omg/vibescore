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

//! Works around a bug of Qt 6.10 that breaks the event processing after a dialog that waits for an answer
//! (a nested event loop); call it once the application object exists
void fixNestedEventLoops();

//! The page tells the app that it added a sound font (it downloads it after the app started, see vibescore-web.js):
//! Module.vibescoreSoundFontsChanged()
void setSoundFontsChangedHandler(const std::function<void()>& handler);
}
