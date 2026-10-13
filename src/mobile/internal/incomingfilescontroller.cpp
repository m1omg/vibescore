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
#include "incomingfilescontroller.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QUrl>

#ifdef Q_OS_ANDROID
#include "platform/android/androidincomingfiles.h"
#endif

#include "log.h"

using namespace mu::mobile;
using namespace muse;

IncomingFilesController::IncomingFilesController(const modularity::ContextPtr& iocCtx)
    : Contextable(iocCtx)
{
}

void IncomingFilesController::init()
{
    fileSystem()->makePath(m_library.libraryPath());

#ifdef Q_OS_ANDROID
    android::startReceivingIncomingFiles([this](const QString& path) {
        QMetaObject::invokeMethod(this, [this, path]() {
            onIncomingFile(path);
        }, Qt::QueuedConnection);
    });
#endif
}

void IncomingFilesController::onIncomingFile(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        LOGE() << "Incoming file does not exist: " << path;
        return;
    }

    const io::path_t target = m_library.importLocalFileByMoving(path);
    if (target.empty()) {
        return;
    }

    LOGI() << "Opening incoming file: " << target;

    // Handled like a file opened from the OS (see appshell's ApplicationActionController)
    QCoreApplication::postEvent(qApp, new QFileOpenEvent(QUrl::fromLocalFile(target.toQString())));
}
