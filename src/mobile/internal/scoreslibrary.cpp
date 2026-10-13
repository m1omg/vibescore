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
#include "scoreslibrary.h"

#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include "log.h"

using namespace mu::mobile;
using namespace muse;

static QByteArray fileHash(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }

    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(&file);
    return hash.result();
}

io::path_t ScoresLibrary::libraryPath() const
{
    return globalConfiguration()->userDataPath() + "/Scores";
}

io::path_t ScoresLibrary::importedFilesPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/Imported";
}

bool ScoresLibrary::isNativeScoreFile(const QString& fileName)
{
    static const QStringList NATIVE_SUFFIXES { "mscz", "mscx", "mscs" };
    return NATIVE_SUFFIXES.contains(QFileInfo(fileName).suffix().toLower());
}

io::path_t ScoresLibrary::targetDir(const QString& fileName) const
{
    return isNativeScoreFile(fileName) ? libraryPath() : importedFilesPath();
}

io::path_t ScoresLibrary::uniquePath(const io::path_t& dir, const QString& fileName) const
{
    const QFileInfo info(fileName);
    const QString baseName = info.completeBaseName();
    const QString suffix = info.suffix().isEmpty() ? QString() : "." + info.suffix();

    io::path_t candidate = dir + "/" + fileName;
    for (int i = 2; fileSystem()->exists(candidate); ++i) {
        candidate = dir + "/" + QString("%1 (%2)%3").arg(baseName).arg(i).arg(suffix);
    }

    return candidate;
}

io::path_t ScoresLibrary::existingSameFile(const io::path_t& dir, const QString& fileName, const QString& source) const
{
    const io::path_t existing = dir + "/" + fileName;
    if (!fileSystem()->exists(existing)) {
        return io::path_t();
    }

    const QFileInfo existingInfo(existing.toQString());
    if (existingInfo.size() != QFileInfo(source).size()) {
        return io::path_t();
    }

    const QByteArray existingHash = fileHash(existing.toQString());
    if (existingHash.isEmpty() || existingHash != fileHash(source)) {
        return io::path_t();
    }

    return existing;
}

io::path_t ScoresLibrary::importFile(const QUrl& sourceUrl, const QString& givenFileName) const
{
    // QFile reads content:// URLs on Android
    const QString source = sourceUrl.isLocalFile() ? sourceUrl.toLocalFile() : sourceUrl.toString();

    QString fileName = givenFileName.isEmpty() ? QFileInfo(source).fileName() : givenFileName;
    if (fileName.isEmpty()) {
        fileName = QStringLiteral("Imported file");
    }

    const io::path_t dir = targetDir(fileName);
    fileSystem()->makePath(dir);

    const io::path_t existing = existingSameFile(dir, fileName, source);
    if (!existing.empty()) {
        LOGI() << "Already imported: " << existing;
        return existing;
    }

    const io::path_t target = uniquePath(dir, fileName);
    if (!QFile::copy(source, target.toQString())) {
        LOGE() << "Failed to import " << source << " to " << target;
        return io::path_t();
    }

    // Copies of read-only documents would be read-only too
    QFile::setPermissions(target.toQString(), QFile::ReadOwner | QFile::WriteOwner);

    LOGI() << "Imported " << source << " to " << target;
    return target;
}

io::path_t ScoresLibrary::importLocalFileByMoving(const QString& path) const
{
    const QFileInfo info(path);
    const io::path_t dir = targetDir(info.fileName());
    fileSystem()->makePath(dir);

    const io::path_t existing = existingSameFile(dir, info.fileName(), path);
    if (!existing.empty()) {
        fileSystem()->remove(path);
        return existing;
    }

    const io::path_t target = uniquePath(dir, info.fileName());
    Ret ret = fileSystem()->move(path, target);
    if (!ret) {
        LOGE() << "Failed to import " << path << ": " << ret.toString();
        return io::path_t();
    }

    return target;
}
