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
#include "androidappdata.h"

#include <QCoreApplication>
#include <QtCore/qcoreapplication_platform.h>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJniObject>
#include <QStandardPaths>
#include <QTextStream>

#include "log.h"

//! The file list is generated when the APK is packaged (see buildscripts/cmake/SetupAndroidPackaging.cmake),
//! because the Android asset manager can not list subdirectories
static const QString ASSETS_ROOT = QStringLiteral("assets:/share/");
static const QString ASSETS_FILE_LIST = QStringLiteral("vibescore-files.txt");
static const QString STAMP_FILE = QStringLiteral(".extracted");

static QString installedApkStamp()
{
    // Changes with every installed build, so updated data is extracted again
    QString apkPath;
    QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (context.isValid()) {
        apkPath = context.callObjectMethod<jstring>("getPackageCodePath").toString();
    }

    const QFileInfo apk(apkPath);
    return QStringLiteral("%1|%2|%3").arg(QCoreApplication::applicationVersion(), apkPath)
           .arg(apk.lastModified().toMSecsSinceEpoch());
}

QString muse::android::extractAppDataFromAssets()
{
    const QString target = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QStringLiteral("/share/");
    const QString stamp = installedApkStamp();

    QFile stampFile(target + STAMP_FILE);
    if (stampFile.open(QIODevice::ReadOnly)) {
        const bool upToDate = QString::fromUtf8(stampFile.readAll()) == stamp;
        stampFile.close();
        if (upToDate) {
            return target;
        }
    }

    QElapsedTimer timer;
    timer.start();

    QFile list(ASSETS_ROOT + ASSETS_FILE_LIST);
    if (!list.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOGE() << "Application data is missing from the package: " << list.fileName();
        return target;
    }

    QDir(target).removeRecursively();
    QDir().mkpath(target);

    int count = 0;
    qint64 bytes = 0;
    QTextStream in(&list);
    while (!in.atEnd()) {
        const QString relativePath = in.readLine().trimmed();
        if (relativePath.isEmpty()) {
            continue;
        }

        const QString dst = target + relativePath;
        QDir().mkpath(QFileInfo(dst).absolutePath());

        if (!QFile::copy(ASSETS_ROOT + relativePath, dst)) {
            LOGE() << "Failed to extract: " << relativePath;
            continue;
        }

        QFile::setPermissions(dst, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        bytes += QFileInfo(dst).size();
        ++count;
    }

    if (stampFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        stampFile.write(stamp.toUtf8());
        stampFile.close();
    }

    LOGI() << "Extracted " << count << " files (" << (bytes / (1024 * 1024)) << " MB) of application data in "
           << timer.elapsed() << " ms to " << target;

    return target;
}
