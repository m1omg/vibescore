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

#include <QString>

namespace muse::android {
//! The application data (share/: templates, instruments, styles, SoundFont, translations, ...)
//! is packaged in the APK assets. Some of it is read with C stdio (e.g. the SoundFont loader),
//! which cannot read assets, so it is extracted into the app's private storage once per
//! installed APK. Returns the extracted directory, with a trailing slash.
QString extractAppDataFromAssets();
}
