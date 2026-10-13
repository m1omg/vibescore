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

#include <QObject>
#include <QString>

#include "modularity/ioc.h"
#include "global/io/ifilesystem.h"

#include "scoreslibrary.h"

namespace mu::mobile {
//! Documents opened from other apps (file managers, mail, share sheet) arrive as local copies.
//! They are imported (see ScoresLibrary) and opened.
class IncomingFilesController : public QObject, public muse::Contextable
{
    Q_OBJECT

    muse::GlobalInject<muse::io::IFileSystem> fileSystem;

public:
    explicit IncomingFilesController(const muse::modularity::ContextPtr& iocCtx);

    void init();

    //! Must be called on the main thread
    void onIncomingFile(const QString& path);

private:
    ScoresLibrary m_library;
};
}
