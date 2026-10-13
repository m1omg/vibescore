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
#include "context/iglobalcontext.h"
#include "global/iinteractive.h"
#include "playback/iplaybackcontroller.h"
#include "dockwindow/idockwindow.h"
#include "dockwindow/idockwindowprovider.h"

namespace mu::mobile {
//! The Back key (Android): goes one step back, and never quits the app.
//! 1. closes the topmost popup, menu or dialog, as Escape does;
//! 2. closes the panel opened last when panels are shown over the score (phones);
//! 3. on the score, leaves note input or text editing, or clears the selection (as Escape does);
//! 4. from the other pages, returns to Home;
//! 5. on Home, moves the app to the background, like the Home button.
class BackNavigationController : public QObject, public muse::Contextable
{
    Q_OBJECT

    muse::ContextInject<muse::IInteractive> interactive = { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };
    muse::ContextInject<playback::IPlaybackController> playbackController = { this };
    muse::ContextInject<muse::dock::IDockWindowProvider> dockWindowProvider = { this };

public:
    explicit BackNavigationController(const muse::modularity::ContextPtr& iocCtx);

    void init();

    void goBack();

private:
    bool eventFilter(QObject* watched, QEvent* event) override;

    bool dismissTopView();
    bool closeOverlayPanel();
    bool escapeScoreState();
    bool isHomeOpened() const;
    void moveAppToBackground();
};
}
