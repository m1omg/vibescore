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
#include "androidmidibridge.h"

#include <mutex>

#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/qnativeinterface.h>

#include "log.h"

using namespace muse::midi;

static const char* MIDI_CLASS = "io/github/m1omg/vibescore/VibeScoreMidi";

static std::mutex s_handlerMutex;
static android::BytesHandler s_handler;

static void nativeMidiReceived(JNIEnv* env, jclass, jbyteArray data)
{
    const jsize size = env->GetArrayLength(data);
    if (size <= 0) {
        return;
    }

    jbyte* bytes = env->GetByteArrayElements(data, nullptr);

    {
        std::lock_guard lock(s_handlerMutex);
        if (s_handler) {
            s_handler(reinterpret_cast<const uint8_t*>(bytes), static_cast<size_t>(size));
        }
    }

    env->ReleaseByteArrayElements(data, bytes, JNI_ABORT);
}

static MidiDeviceList portList(const char* method)
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    const QJniObject result = QJniObject::callStaticObjectMethod(MIDI_CLASS, method,
                                                                 "(Landroid/content/Context;)[Ljava/lang/String;",
                                                                 context.object());
    QJniEnvironment env;
    if (env.checkAndClearExceptions() || !result.isValid()) {
        return {};
    }

    MidiDeviceList devices;

    auto array = static_cast<jobjectArray>(result.object());
    const jsize count = env->GetArrayLength(array);
    for (jsize i = 0; i < count; ++i) {
        const QString entry = QJniObject::fromLocalRef(env->GetObjectArrayElement(array, i)).toString();
        const qsizetype separator = entry.indexOf('\n');
        if (separator < 0) {
            continue;
        }

        MidiDevice device;
        device.id = entry.left(separator).toStdString();
        device.name = entry.mid(separator + 1).toStdString();
        devices.push_back(device);
    }

    return devices;
}

static bool openPort(const char* method, const MidiDeviceID& id)
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    const jboolean ok = QJniObject::callStaticMethod<jboolean>(MIDI_CLASS, method,
                                                               "(Landroid/content/Context;Ljava/lang/String;)Z",
                                                               context.object(),
                                                               QJniObject::fromString(QString::fromStdString(id)).object<jstring>());
    QJniEnvironment env;
    return !env.checkAndClearExceptions() && ok;
}

MidiDeviceList android::sources()
{
    return portList("sources");
}

MidiDeviceList android::destinations()
{
    return portList("destinations");
}

bool android::openSource(const MidiDeviceID& id)
{
    return openPort("openSource", id);
}

void android::closeSource()
{
    QJniObject::callStaticMethod<void>(MIDI_CLASS, "closeSource", "()V");
}

bool android::openDestination(const MidiDeviceID& id)
{
    return openPort("openDestination", id);
}

void android::closeDestination()
{
    QJniObject::callStaticMethod<void>(MIDI_CLASS, "closeDestination", "()V");
}

bool android::send(const uint8_t* data, size_t size)
{
    QJniEnvironment env;
    jbyteArray array = env->NewByteArray(static_cast<jsize>(size));
    env->SetByteArrayRegion(array, 0, static_cast<jsize>(size), reinterpret_cast<const jbyte*>(data));

    const jboolean ok = QJniObject::callStaticMethod<jboolean>(MIDI_CLASS, "send", "([B)Z", array);
    env->DeleteLocalRef(array);

    return !env.checkAndClearExceptions() && ok;
}

void android::setSourceHandler(const BytesHandler& handler)
{
    static std::once_flag registered;
    std::call_once(registered, []() {
        QJniEnvironment env;
        const JNINativeMethod methods[] = {
            { "nativeMidiReceived", "([B)V", reinterpret_cast<void*>(nativeMidiReceived) },
        };

        if (!env.registerNativeMethods(MIDI_CLASS, methods, 1)) {
            LOGE() << "Failed to register native methods of " << MIDI_CLASS;
        }
    });

    std::lock_guard lock(s_handlerMutex);
    s_handler = handler;
}
