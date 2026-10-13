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
#pragma once

#include "logger.h"

namespace muse::logger {
//! Writes log messages to the system log: logcat on Android, the unified log (os_log) on iOS.
//! Standard output is not visible on these platforms.
class PlatformLogDest : public LogDest
{
public:
    explicit PlatformLogDest(const LogLayout& l);

    std::string name() const override;
    void write(const LogMsg& logMsg) override;
};
}
