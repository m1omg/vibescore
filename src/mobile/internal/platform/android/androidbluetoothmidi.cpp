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
#include "androidbluetoothmidi.h"

#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/qnativeinterface.h>

#include "log.h"

static const char* BLUETOOTH_MIDI_CLASS = "io/github/m1omg/vibescore/VibeScoreBluetoothMidi";

bool mu::mobile::android::isBluetoothMidiAvailable()
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    const jboolean available = QJniObject::callStaticMethod<jboolean>(BLUETOOTH_MIDI_CLASS, "isAvailable",
                                                                      "(Landroid/content/Context;)Z", context.object());

    QJniEnvironment env;
    if (env.checkAndClearExceptions()) {
        return false;
    }

    return available;
}

void mu::mobile::android::chooseBluetoothMidiDevice()
{
    // The context is the activity, which shows the chooser
    const QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>(BLUETOOTH_MIDI_CLASS, "chooseDevice", "(Landroid/app/Activity;)V", activity.object());

    QJniEnvironment env;
    if (env.checkAndClearExceptions()) {
        LOGE() << "Can not show the Bluetooth MIDI device chooser";
    }
}

void mu::mobile::android::connectChosenBluetoothMidiDevices()
{
    const QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject::callStaticMethod<void>(BLUETOOTH_MIDI_CLASS, "connectChosenDevices", "(Landroid/app/Activity;)V",
                                       activity.object());

    QJniEnvironment env;
    if (env.checkAndClearExceptions()) {
        LOGE() << "Can not connect the Bluetooth MIDI devices chosen earlier";
    }
}
