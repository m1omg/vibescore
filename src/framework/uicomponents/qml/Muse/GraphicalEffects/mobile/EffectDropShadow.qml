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

//! DropShadow of Qt5Compat.GraphicalEffects, on mobile platforms and the web (see EffectRectangularGlow.qml)
MultiEffect {
    property real horizontalOffset: 0
    property real verticalOffset: 0
    property real radius: 4
    property int samples: 0 // compat
    property color color: "black"
    property real spread: 0 // not supported
    property bool transparentBorder: true

    shadowEnabled: true
    shadowColor: color
    shadowHorizontalOffset: horizontalOffset
    shadowVerticalOffset: verticalOffset
    blurMax: 64
    shadowBlur: Math.min(1, radius / blurMax)
    autoPaddingEnabled: transparentBorder
}
