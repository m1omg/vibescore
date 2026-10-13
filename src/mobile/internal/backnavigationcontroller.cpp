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
#include "backnavigationcontroller.h"

#include <QApplication>
#include <QDialog>
#include <QKeyEvent>

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QtCore/qnativeinterface.h>
#endif

#include "uicomponents/qml/Muse/UiComponents/windowview.h"

using namespace mu::mobile;
using namespace muse;
using namespace muse::uicomponents;

static const Uri HOME_PAGE_URI("musescore://home");
static const Uri NOTATION_PAGE_URI("musescore://notation");

BackNavigationController::BackNavigationController(const modularity::ContextPtr& iocCtx)
    : Contextable(iocCtx)
{
}

void BackNavigationController::init()
{
    qApp->installEventFilter(this);
}

bool BackNavigationController::eventFilter(QObject* watched, QEvent* event)
{
    if (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease) {
        return false;
    }

    const QKeyEvent* keyEvent = static_cast<const QKeyEvent*>(event);
    if (keyEvent->key() != Qt::Key_Back) {
        return false;
    }

    // Key events go to the window first, then to the focused item: handle them once, at the window.
    // Consuming both press and release also stops Android from finishing the activity (quitting the app)
    if (!watched->isWindowType()) {
        return false;
    }

    if (event->type() == QEvent::KeyRelease && !keyEvent->isAutoRepeat()) {
        goBack();
    }

    return true;
}

void BackNavigationController::goBack()
{
    if (dismissTopView()) {
        return;
    }

    if (closeOverlayPanel()) {
        return;
    }

    if (escapeScoreState()) {
        return;
    }

    if (!isHomeOpened()) {
        interactive()->open(UriQuery(HOME_PAGE_URI));
        return;
    }

    moveAppToBackground();
}

bool BackNavigationController::dismissTopView()
{
    // Popups, menus and dialogs shown inside the main window
    const QList<WindowView*>& views = WindowView::openInWindowViews();
    if (!views.isEmpty()) {
        // A view that cannot be dismissed (e.g. a progress dialog) still takes the Back key
        views.last()->dismiss();
        return true;
    }

    // QWidget dialogs and menus are separate windows
    if (QWidget* popup = QApplication::activePopupWidget()) {
        popup->close();
        return true;
    }

    if (QWidget* modal = QApplication::activeModalWidget()) {
        if (QDialog* dialog = qobject_cast<QDialog*>(modal)) {
            dialog->reject();
        } else {
            modal->close();
        }
        return true;
    }

    return false;
}

bool BackNavigationController::closeOverlayPanel()
{
    // The mobile shell (MobileShellView) is the dock window
    dock::IDockWindow* window = dockWindowProvider() ? dockWindowProvider()->window() : nullptr;
    if (!window) {
        return false;
    }

    bool closed = false;
    QMetaObject::invokeMethod(&window->asItem(), "closeLastOpenedPanel", Q_RETURN_ARG(bool, closed));
    return closed;
}

bool BackNavigationController::escapeScoreState()
{
    if (interactive()->currentUri().val != NOTATION_PAGE_URI) {
        return false;
    }

    const notation::INotationPtr notation = globalContext()->currentNotation();
    if (!notation) {
        return false;
    }

    const notation::INotationInteractionPtr interaction = notation->interaction();
    const bool hasStateToLeave = playbackController()->isPlaying()
                                 || interaction->noteInput()->isNoteInputMode()
                                 || interaction->isTextEditingStarted()
                                 || interaction->isGripEditStarted()
                                 || interaction->isEditingElement()
                                 || !interaction->selection()->isNone();
    if (!hasStateToLeave) {
        return false;
    }

    // What Escape does on the score
    dispatcher()->dispatch("action://notation/cancel");
    return true;
}

bool BackNavigationController::isHomeOpened() const
{
    return interactive()->currentUri().val == HOME_PAGE_URI;
}

void BackNavigationController::moveAppToBackground()
{
#ifdef Q_OS_ANDROID
    // Like the Home button: the app keeps running, with the score and the undo history as they are
    QNativeInterface::QAndroidApplication::runOnAndroidMainThread([]() {
        QJniObject activity = QNativeInterface::QAndroidApplication::context();
        if (activity.isValid()) {
            activity.callMethod<jboolean>("moveTaskToBack", jboolean(true));
        }
    });
#endif
}
