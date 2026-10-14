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
import QtQuick
import QtQuick.Effects

//! RectangularGlow of Qt5Compat.GraphicalEffects, on mobile platforms and the web: Qt5Compat's effects compile their
//! shaders when the app runs, which fails on OpenGL ES 3 and WebGL 2 (and needs Qt Shader Tools in the app).
//! A glow around the item: glowRadius wide, its outer corners rounded by cornerRadius.
RectangularShadow {
    property real glowRadius: 0
    property real cornerRadius: 0

    blur: glowRadius
    radius: Math.max(0, cornerRadius - glowRadius)
}
