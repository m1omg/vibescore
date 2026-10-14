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

#include <QObject>
#include <qqmlintegration.h>

namespace mu::appshell {
//! Which sections of the Home page this build has
class HomeMenuModel : public QObject
{
    Q_OBJECT

    //! MuseScore.com account (cloud module)
    Q_PROPERTY(bool isAccountAvailable READ isAccountAvailable CONSTANT)

    //! Muse Sounds (needs the proprietary MuseSampler library)
    Q_PROPERTY(bool isMuseSoundsAvailable READ isMuseSoundsAvailable CONSTANT)

    //! Plugins (extensions module) and Learn (videos; learn module): not in every build (e.g. the web app)
    Q_PROPERTY(bool isPluginsAvailable READ isPluginsAvailable CONSTANT)
    Q_PROPERTY(bool isLearnAvailable READ isLearnAvailable CONSTANT)

    QML_ELEMENT

public:
    explicit HomeMenuModel(QObject* parent = nullptr);

    bool isAccountAvailable() const;
    bool isMuseSoundsAvailable() const;
    bool isPluginsAvailable() const;
    bool isLearnAvailable() const;
};
}
