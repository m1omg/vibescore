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
#include "mobilewindowsprovider.h"

#include <QUrl>

#include "async/async.h"

#include "log.h"

using namespace mu::mobile;
using namespace muse;
using namespace muse::actions;

MobileWindowsProvider::MobileWindowsProvider(const modularity::ContextPtr& iocCtx)
    : Contextable(iocCtx)
{
}

int MobileWindowsProvider::windowCount() const
{
    return 1;
}

bool MobileWindowsProvider::isProjectAlreadyOpened(const io::path_t&) const
{
    // The project of this window is checked before asking for other windows
    return false;
}

void MobileWindowsProvider::activateWindowWithProject(const io::path_t&)
{
}

bool MobileWindowsProvider::isHasWindowWithoutProject() const
{
    return false;
}

void MobileWindowsProvider::activateWindowWithoutProject(const QStringList&)
{
}

bool MobileWindowsProvider::openNewWindow(const QStringList& args)
{
    // The arguments are the command line of a new instance (see CommandLineParser)
    QString scorePath;
    QString displayNameOverride;
    bool newScore = false;

    for (int i = 0; i < args.size(); ++i) {
        const QString& arg = args.at(i);
        if (arg == "--session-type" && i + 1 < args.size()) {
            newScore = args.at(++i) == "start-with-new";
        } else if (arg == "--score-display-name-override" && i + 1 < args.size()) {
            displayNameOverride = args.at(++i);
        } else if (!arg.startsWith("--")) {
            scorePath = arg;
        }
    }

    if (!newScore && scorePath.isEmpty()) {
        LOGW() << "Nothing to open in a new window: " << args.join(' ');
        return false;
    }

    // This is called while the current request (e.g. opening a file) is still being processed
    async::Async::call(this, [this, newScore, scorePath, displayNameOverride]() {
        replaceProject(newScore, scorePath, displayNameOverride);
    });

    return true;
}

void MobileWindowsProvider::replaceProject(bool newScore, const QString& scorePath, const QString& displayNameOverride)
{
    // Asks to save the changes; the user can also cancel
    dispatcher()->dispatch("file-close");
    if (globalContext()->currentProject()) {
        return;
    }

    if (newScore) {
        dispatcher()->dispatch("file-new");
        return;
    }

    dispatcher()->dispatch("file-open", ActionData::make_arg2<QUrl, QString>(QUrl::fromUserInput(scorePath), displayNameOverride));
}

// Settings: there are no other instances to synchronize with

bool MobileWindowsProvider::isPreferencesAlreadyOpened() const
{
    return false;
}

void MobileWindowsProvider::activateWindowWithOpenedPreferences() const
{
}

void MobileWindowsProvider::settingsBeginTransaction()
{
}

void MobileWindowsProvider::settingsCommitTransaction()
{
}

void MobileWindowsProvider::settingsRollbackTransaction()
{
}

void MobileWindowsProvider::settingsReset()
{
}

void MobileWindowsProvider::settingsSetValue(const std::string&, const Val&)
{
}

// Resources (files): only this instance uses them

bool MobileWindowsProvider::lockResource(const std::string&)
{
    return true;
}

bool MobileWindowsProvider::unlockResource(const std::string&)
{
    return true;
}

void MobileWindowsProvider::notifyAboutResourceChanged(const std::string&)
{
}

async::Channel<std::string> MobileWindowsProvider::resourceChanged()
{
    return m_resourceChanged;
}

// Quit for all

void MobileWindowsProvider::notifyAboutWindowWasQuited()
{
}

void MobileWindowsProvider::quitForAll()
{
}

void MobileWindowsProvider::quitAllAndRestartLast()
{
}

void MobileWindowsProvider::quitAllAndRunInstallation(const io::path_t&)
{
}
