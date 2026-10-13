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
#include "androidshare.h"

#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/qnativeinterface.h>

#include "log.h"

void mu::mobile::android::shareFiles(const QStringList& paths, const QString& chooserTitle)
{
    QJniEnvironment env;

    jobjectArray jpaths = env->NewObjectArray(static_cast<jsize>(paths.size()), env.findClass("java/lang/String"), nullptr);
    for (qsizetype i = 0; i < paths.size(); ++i) {
        const QJniObject path = QJniObject::fromString(paths.at(i));
        env->SetObjectArrayElement(jpaths, static_cast<jsize>(i), path.object<jstring>());
    }

    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    const QJniObject title = QJniObject::fromString(chooserTitle);

    QJniObject::callStaticMethod<void>("io/github/m1omg/vibescore/VibeScoreShare", "shareFiles",
                                       "(Landroid/content/Context;[Ljava/lang/String;Ljava/lang/String;)V",
                                       context.object(), jpaths, title.object<jstring>());

    if (env.checkAndClearExceptions()) {
        LOGE() << "Failed to share " << paths.join(", ");
    }

    env->DeleteLocalRef(jpaths);
}
