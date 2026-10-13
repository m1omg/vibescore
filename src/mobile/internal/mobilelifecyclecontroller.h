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

#include "modularity/ioc.h"
#include "actions/iactionsdispatcher.h"
#include "playback/iplaybackcontroller.h"

namespace mu::mobile {
//! The app on mobile platforms goes to the background and comes back: playback pauses in the background
//! (a notation app is not a media player). The score is autosaved (see ProjectAutoSaver) and the audio
//! stream is paused (see the audio drivers) by the modules concerned.
class MobileLifecycleController : public QObject, public muse::Contextable
{
    Q_OBJECT

    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };

public:
    explicit MobileLifecycleController(const muse::modularity::ContextPtr& iocCtx);

    void init();

private:
    void onApplicationStateChanged(Qt::ApplicationState state);
};
}
