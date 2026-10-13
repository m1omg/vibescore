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

#include <QString>
#include <QUrl>

#include "modularity/ioc.h"
#include "global/iglobalconfiguration.h"
#include "global/io/ifilesystem.h"

namespace mu::mobile {
//! The Scores library: the folder where the scores of the app live (userDataPath()/Scores;
//! Android: app-specific external storage, reachable over USB; iOS: Documents, shown by the Files app).
//! Files that come from elsewhere (the system file picker, other apps) are imported as local copies:
//! native scores into the library, so that saving and autosave work like on desktop;
//! other files (MusicXML, MIDI, images, styles, ...) into a cache folder, as they are only read.
class ScoresLibrary
{
    muse::GlobalInject<muse::IGlobalConfiguration> globalConfiguration;
    muse::GlobalInject<muse::io::IFileSystem> fileSystem;

public:
    muse::io::path_t libraryPath() const;
    muse::io::path_t importedFilesPath() const;

    static bool isNativeScoreFile(const QString& fileName);

    //! Copies the file (a local path or a content:// URL on Android) into the library or the cache folder,
    //! and returns the copy. A file that is already in the library with the same content is reused.
    muse::io::path_t importFile(const QUrl& source, const QString& fileName = QString()) const;

    //! Moves a local file (e.g. a received copy) into the library or the cache folder
    muse::io::path_t importLocalFileByMoving(const QString& path) const;

private:
    muse::io::path_t targetDir(const QString& fileName) const;
    muse::io::path_t uniquePath(const muse::io::path_t& dir, const QString& fileName) const;
    muse::io::path_t existingSameFile(const muse::io::path_t& dir, const QString& fileName, const QString& source) const;
};
}
