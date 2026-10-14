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
#include "hapticfeedback.h"

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QtCore/qnativeinterface.h>
#elif defined(Q_OS_IOS)
#include "platform/ios/ioshapticfeedback.h"
#elif defined(Q_OS_WASM)
#include <emscripten.h>
#endif

using namespace muse::ui;

#ifdef Q_OS_WASM
// The Vibration API: Android browsers (other browsers have none and ignore it)
EM_JS(void, muse_vibrate, (int milliseconds), {
    if (navigator.vibrate) {
        navigator.vibrate(milliseconds);
    }
});
#endif

void HapticFeedback::perform(Type type)
{
#if defined(Q_OS_ANDROID)
    // View.performHapticFeedback on the activity's view, on the Android UI thread (HapticFeedbackConstants)
    static constexpr jint LONG_PRESS = 0;
    static constexpr jint CLOCK_TICK = 4;
    const jint constant = type == Type::LongPress ? LONG_PRESS : CLOCK_TICK;

    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([constant]() {
        const QJniObject activity = QNativeInterface::QAndroidApplication::context();
        const QJniObject window = activity.callObjectMethod("getWindow", "()Landroid/view/Window;");
        if (!window.isValid()) {
            return;
        }
        const QJniObject view = window.callObjectMethod("getDecorView", "()Landroid/view/View;");
        if (view.isValid()) {
            view.callMethod<jboolean>("performHapticFeedback", "(I)Z", constant);
        }
    });
#elif defined(Q_OS_IOS)
    ios::performHapticFeedback(type == Type::LongPress);
#elif defined(Q_OS_WASM)
    muse_vibrate(type == Type::LongPress ? 25 : 10);
#else
    Q_UNUSED(type);
#endif
}
