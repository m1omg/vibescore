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
#include "platformlogdest.h"

#include "muse_framework_config.h"

#if defined(Q_OS_ANDROID)
#include <android/log.h>
#elif defined(Q_OS_IOS)
#include <os/log.h>
#endif

using namespace muse::logger;

static constexpr const char* SYSTEM_LOG_TAG = MUSE_APP_NAME_MACHINE_READABLE;

PlatformLogDest::PlatformLogDest(const LogLayout& l)
    : LogDest(l)
{
}

std::string PlatformLogDest::name() const
{
    return "PlatformLogDest";
}

void PlatformLogDest::write(const LogMsg& logMsg)
{
    const std::string msg = m_layout.output(logMsg);

#if defined(Q_OS_ANDROID)
    int priority = ANDROID_LOG_INFO;
    if (logMsg.type == Logger::ERRR) {
        priority = ANDROID_LOG_ERROR;
    } else if (logMsg.type == Logger::WARN) {
        priority = ANDROID_LOG_WARN;
    } else if (logMsg.type == Logger::DEBG) {
        priority = ANDROID_LOG_DEBUG;
    }
    __android_log_write(priority, SYSTEM_LOG_TAG, msg.c_str());
#elif defined(Q_OS_IOS)
    static os_log_t log = os_log_create(MUSE_APP_NAME_MACHINE_READABLE, "app");
    os_log_type_t type = OS_LOG_TYPE_DEFAULT;
    if (logMsg.type == Logger::ERRR) {
        type = OS_LOG_TYPE_ERROR;
    } else if (logMsg.type == Logger::DEBG) {
        type = OS_LOG_TYPE_DEBUG;
    }
    os_log_with_type(log, type, "%{public}s", msg.c_str());
#else
    (void)msg;
#endif
}
