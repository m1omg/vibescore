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
#include "mobileprintprovider.h"

#include <QDir>
#include <QStandardPaths>

#include "io/file.h"

#if defined(Q_OS_ANDROID)
#include "platform/android/androidprint.h"
#elif defined(Q_OS_IOS)
#include "platform/ios/iosplatform.h"
#endif

#include "log.h"

using namespace mu::mobile;
using namespace muse;

MobilePrintProvider::MobilePrintProvider(const modularity::ContextPtr& iocCtx)
    : Contextable(iocCtx)
{
}

Ret MobilePrintProvider::printNotation(notation::INotationPtr notation)
{
    IF_ASSERT_FAILED(notation) {
        return make_ret(Ret::Code::InternalError);
    }

    project::INotationWriterPtr writer = writers()->writer("pdf");
    if (!writer) {
        LOGE() << "No PDF writer";
        return make_ret(Ret::Code::InternalError);
    }

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/Print";
    QDir().mkpath(dir);

    QString jobName = notation->name();
    if (jobName.isEmpty()) {
        jobName = QStringLiteral("Score");
    }

    const io::path_t pdfPath = dir + "/" + io::escapeFileName(jobName).toQString() + ".pdf";

    io::File file(pdfPath);
    if (!file.open(io::IODevice::WriteOnly)) {
        LOGE() << "Can not write " << pdfPath;
        return make_ret(Ret::Code::InternalError);
    }

    Ret ret = writer->write(notation, file);
    file.close();

    if (!ret) {
        LOGE() << "Failed to write the PDF to print: " << ret.toString();
        return ret;
    }

#if defined(Q_OS_ANDROID)
    android::printPdf(pdfPath.toQString(), jobName);
#elif defined(Q_OS_IOS)
    ios::printPdf(pdfPath.toQString(), jobName);
#endif

    return ret;
}
