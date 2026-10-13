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

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "actions/iactionsdispatcher.h"
#include "context/iglobalcontext.h"
#include "multiwindows/imultiwindowsprovider.h"

namespace mu::mobile {
//! Mobile apps have a single window. Where the desktop app opens another score (or a new one)
//! in a new window, the score in this window is replaced, after the usual "save changes?" question.
class MobileWindowsProvider : public muse::mi::IMultiWindowsProvider, public muse::Contextable, public muse::async::Asyncable
{
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };

public:
    explicit MobileWindowsProvider(const muse::modularity::ContextPtr& iocCtx);

    // Windows info
    int windowCount() const override;

    // Project opening
    bool isProjectAlreadyOpened(const muse::io::path_t& projectPath) const override;
    void activateWindowWithProject(const muse::io::path_t& projectPath) override;
    bool isHasWindowWithoutProject() const override;
    void activateWindowWithoutProject(const QStringList& args = {}) override;
    bool openNewWindow(const QStringList& args) override;

    // Settings
    bool isPreferencesAlreadyOpened() const override;
    void activateWindowWithOpenedPreferences() const override;
    void settingsBeginTransaction() override;
    void settingsCommitTransaction() override;
    void settingsRollbackTransaction() override;
    void settingsReset() override;
    void settingsSetValue(const std::string& key, const muse::Val& value) override;

    // Resources (files)
    bool lockResource(const std::string& name) override;
    bool unlockResource(const std::string& name) override;
    void notifyAboutResourceChanged(const std::string& name) override;
    muse::async::Channel<std::string> resourceChanged() override;

    // Quit for all
    void notifyAboutWindowWasQuited() override;
    void quitForAll() override;
    void quitAllAndRestartLast() override;
    void quitAllAndRunInstallation(const muse::io::path_t& installerPath) override;

private:
    void replaceProject(bool newScore, const QString& scorePath, const QString& displayNameOverride);

    muse::async::Channel<std::string> m_resourceChanged;
};
}
