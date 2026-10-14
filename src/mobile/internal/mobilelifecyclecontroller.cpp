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
#include "mobilelifecyclecontroller.h"

#include <QGuiApplication>

#include "log.h"

#ifdef Q_OS_WASM
#include "project/inotationproject.h"
#include "platform/web/webplatform.h"
#endif

using namespace mu::mobile;
using namespace muse;

MobileLifecycleController::MobileLifecycleController(const modularity::ContextPtr& iocCtx)
    : Contextable(iocCtx)
{
}

void MobileLifecycleController::init()
{
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, &MobileLifecycleController::onApplicationStateChanged);

#ifdef Q_OS_WASM
    keepSavedScoresInBrowserStorage();
#endif
}

void MobileLifecycleController::onApplicationStateChanged(Qt::ApplicationState state)
{
    LOGI() << "Application state: " << int(state);

    if (state == Qt::ApplicationSuspended && playbackController() && playbackController()->isPlaying()) {
        // Toggles playback: pauses at the current position
        dispatcher()->dispatch("play");
    }
}

#ifdef Q_OS_WASM
//! The files of the web app are in memory: the page writes them to the browser's storage (IndexedDB) regularly and
//! when the page is hidden, which can come too late when the page is closed right after saving
void MobileLifecycleController::keepSavedScoresInBrowserStorage()
{
    auto watchCurrentProject = [this]() {
        if (project::INotationProjectPtr project = globalContext()->currentProject()) {
            project->saveComplited().onReceive(this, [](const muse::io::path_t&, project::SaveMode) {
                web::syncFileSystem();
            });
        }
    };

    globalContext()->currentProjectChanged().onNotify(this, watchCurrentProject);
    watchCurrentProject();
}
#endif
