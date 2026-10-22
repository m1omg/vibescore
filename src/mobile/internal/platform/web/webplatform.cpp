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
#include "webplatform.h"

#include <emscripten.h>
#include <emscripten/bind.h>

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QtCore/private/qwasmsuspendresumecontrol_p.h>
#include <qpa/qwindowsysteminterface.h>

#include "log.h"

// See fixNestedEventLoops()
EM_JS(void, vs_guard_pending_events, (int noopHandler), {
    const control = Module.qtSuspendResumeControl;
    if (!control || control.vibescoreFixed) {
        return;
    }
    // What Qt reads of a browser event that the browser only gives while it delivers the event: the windows are in a
    // shadow DOM, whose events have no target, related target and path afterwards (and their offset is then relative
    // to nothing). Qt handles a queued event after an earlier one of its batch returned, which can be much later (e.g.
    // that one opened a score, or a dialog), and a pointer event without a target throws in Qt (dom::mapPoint): the
    // exception stopped the event processing for good (the app froze). The values are kept on the event when it is
    // queued, during its delivery.
    const keep = (event) => {
        if (!(event instanceof Event) || event.vibescoreKept) {
            return;
        }
        const path = event.composedPath();
        const values = { target: event.target, currentTarget: event.currentTarget, relatedTarget: event.relatedTarget };
        if (event instanceof MouseEvent) {
            values.offsetX = event.offsetX;
            values.offsetY = event.offsetY;
        }
        for (const key of Object.keys(values)) {
            Object.defineProperty(event, key, { value: values[key], configurable: true });
        }
        Object.defineProperty(event, 'composedPath', { value: () => path, configurable: true });
        event.vibescoreKept = true;
    };
    const guard = (queue) => {
        if (queue && !queue.vibescoreGuarded) {
            const shift = queue.shift;
            queue.shift = function() {
                return this.length > 0 ? shift.call(this) : { index: noopHandler, arg: undefined };
            };
            const push = queue.push;
            queue.push = function(...items) {
                for (const item of items) {
                    keep(item && item.arg);
                }
                return push.apply(this, items);
            };
            queue.vibescoreGuarded = true;
        }
        return queue;
    };
    // Qt replaces the queue at times: guard each one
    let queue = guard(control.pendingEvents);
    Object.defineProperty(control, 'pendingEvents', {
        configurable: true,
        enumerable: true,
        get: () => queue,
        set: (value) => { queue = guard(value); },
    });
    control.vibescoreFixed = true;
});

// Shows the print dialog of the browser for a PDF of the file system, in a hidden frame of the page
EM_JS(void, vs_print_pdf, (const char* pathPtr, const char* titlePtr), {
    const path = UTF8ToString(pathPtr);
    const title = UTF8ToString(titlePtr);
    const data = FS.readFile(path);
    const url = URL.createObjectURL(new Blob([data], { type: 'application/pdf' }));

    const frame = document.createElement('iframe');
    frame.style.position = 'fixed';
    frame.style.width = '0';
    frame.style.height = '0';
    frame.style.border = '0';
    frame.title = title;
    frame.src = url;
    frame.onload = () => {
        frame.contentWindow.focus();
        frame.contentWindow.print();
        // The print dialog is modal in most browsers; remove the frame a while later
        setTimeout(() => { frame.remove(); URL.revokeObjectURL(url); }, 60000);
    };
    document.body.appendChild(frame);
});

namespace mu::mobile::web {
void openFile(const QString& nameFilter, const std::function<void(const QString& fileName, const QByteArray& content)>& done)
{
    QFileDialog::getOpenFileContent(nameFilter, done);
}

void downloadFiles(const QStringList& paths)
{
    for (const QString& path : paths) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            LOGE() << "Can not read " << path;
            continue;
        }

        QFileDialog::saveFileContent(file.readAll(), QFileInfo(path).fileName());
    }
}

void printPdf(const QString& path, const QString& jobName)
{
    // In a hidden frame of the page: the browser shows the PDF's print dialog
    const QByteArray filePath = path.toUtf8();
    const QByteArray title = jobName.toUtf8();

    vs_print_pdf(filePath.constData(), title.constData());
}

void fixNestedEventLoops()
{
    // QWasmSuspendResumeControl::sendPendingEvents() takes the number of pending (browser) events once, then takes
    // them one by one from a JavaScript queue. When the handler of one of them runs a nested event loop (e.g. a dialog
    // that waits for an answer), that loop handles the remaining events, and the outer loop then takes from an empty
    // queue: an exception that stops the event processing. An empty queue gives an event that does nothing instead.
    QWasmSuspendResumeControl* control = QWasmSuspendResumeControl::get();
    if (!control) {
        return;
    }

    const uint32_t noopHandler = control->registerEventHandler([](emscripten::val) {});

    vs_guard_pending_events(static_cast<int>(noopHandler));
}

void watchPageVisibility()
{
    // Through Qt's queue of browser events, which the event loop waiting for a dialog's answer also processes
    QWasmSuspendResumeControl* control = QWasmSuspendResumeControl::get();
    if (!control) {
        return;
    }

    const uint32_t handler = control->registerEventHandler([](emscripten::val visible) {
        const bool isVisible = visible.as<bool>();
        QWindowSystemInterface::handleApplicationStateChanged<QWindowSystemInterface::SynchronousDelivery>(
            isVisible ? Qt::ApplicationActive : Qt::ApplicationInactive);

        // E.g. the autosave of the open score
        if (!isVisible) {
            syncFileSystem();
        }
    });

    emscripten::val::global("window").set("vibescorePageVisibilityChanged", control->jsEventHandlerAt(handler));
}

static std::function<void()> s_soundFontsChangedHandler;

void setSoundFontsChangedHandler(const std::function<void()>& handler)
{
    s_soundFontsChangedHandler = handler;
}

static void soundFontsChanged()
{
    if (s_soundFontsChangedHandler) {
        s_soundFontsChangedHandler();
    }
}

void syncFileSystem()
{
    EM_ASM({
        if (window.vibescoreSyncFileSystem) {
            window.vibescoreSyncFileSystem();
        }
    });
}

void showBusy(const std::string& title)
{
    EM_ASM({
        if (window.vibescoreBusy) {
            window.vibescoreBusy(UTF8ToString($0));
        }
    }, title.c_str());
}
}

EMSCRIPTEN_BINDINGS(vibescore) {
    emscripten::function("vibescoreSoundFontsChanged", &mu::mobile::web::soundFontsChanged);
}
