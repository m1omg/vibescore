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
#include "mobileplatformtheme.h"

#include <QGuiApplication>
#include <QStyleHints>

using namespace muse::ui;
using namespace muse::async;

void MobilePlatformTheme::startListening()
{
    if (m_colorSchemeConnection) {
        return;
    }

    m_colorSchemeConnection = QObject::connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged,
                                               this, [this](Qt::ColorScheme) {
        m_platformThemeChanged.notify();
    });
}

void MobilePlatformTheme::stopListening()
{
    QObject::disconnect(m_colorSchemeConnection);
    m_colorSchemeConnection = {};
}

bool MobilePlatformTheme::isFollowSystemThemeAvailable() const
{
    return true;
}

bool MobilePlatformTheme::isSystemThemeDark() const
{
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
}

Notification MobilePlatformTheme::platformThemeChanged() const
{
    return m_platformThemeChanged;
}

bool MobilePlatformTheme::isGlobalMenuAvailable() const
{
    return false;
}

void MobilePlatformTheme::applyPlatformStyleOnAppForTheme(const ThemeCode&)
{
}

void MobilePlatformTheme::applyPlatformStyleOnWindowForTheme(QWindow*, const ThemeCode&)
{
}
