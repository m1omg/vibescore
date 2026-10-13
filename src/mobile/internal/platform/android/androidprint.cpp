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
#include "androidprint.h"

#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/qnativeinterface.h>

#include "log.h"

void mu::mobile::android::printPdf(const QString& pdfPath, const QString& jobName)
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();

    QJniObject::callStaticMethod<void>("io/github/m1omg/vibescore/VibeScorePrint", "printPdf",
                                       "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)V",
                                       context.object(), QJniObject::fromString(pdfPath).object<jstring>(),
                                       QJniObject::fromString(jobName).object<jstring>());

    QJniEnvironment env;
    if (env.checkAndClearExceptions()) {
        LOGE() << "Failed to print " << pdfPath;
    }
}
