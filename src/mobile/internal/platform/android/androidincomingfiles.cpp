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
#include "androidincomingfiles.h"

#include <QJniEnvironment>
#include <QJniObject>

#include "log.h"

using namespace mu::mobile;

static const char* ACTIVITY_CLASS = "io/github/m1omg/vibescore/VibeScoreActivity";

static android::IncomingFileHandler s_handler;

static void nativeIncomingFile(JNIEnv*, jclass, jstring path)
{
    const QString filePath = QJniObject(path).toString();
    if (s_handler) {
        s_handler(filePath);
    }
}

void android::startReceivingIncomingFiles(IncomingFileHandler handler)
{
    s_handler = std::move(handler);

    QJniEnvironment env;
    const JNINativeMethod methods[] = {
        { "nativeIncomingFile", "(Ljava/lang/String;)V", reinterpret_cast<void*>(nativeIncomingFile) },
    };

    if (!env.registerNativeMethods(ACTIVITY_CLASS, methods, 1)) {
        LOGE() << "Failed to register native methods of " << ACTIVITY_CLASS;
        return;
    }

    // Files that arrived while the app was starting
    QJniObject pending = QJniObject::callStaticObjectMethod(ACTIVITY_CLASS, "takePendingFiles", "()[Ljava/lang/String;");
    if (!pending.isValid()) {
        return;
    }

    auto array = static_cast<jobjectArray>(pending.object());
    const jsize count = env->GetArrayLength(array);
    for (jsize i = 0; i < count; ++i) {
        QJniObject item = QJniObject::fromLocalRef(env->GetObjectArrayElement(array, i));
        s_handler(item.toString());
    }
}
