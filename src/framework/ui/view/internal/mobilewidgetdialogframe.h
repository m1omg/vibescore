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

#include <QPointer>
#include <QWidget>

class QDialog;

namespace muse::ui {
//! Shows a QWidget dialog on touch screens with a single window (mobile): the dialog takes the whole window,
//! scrolls (with the finger) when it does not fit, and gets a title bar with a close button.
//! Closing the frame (e.g. with the Back key) rejects the dialog; the frame closes when the dialog is done.
class MobileWidgetDialogFrame : public QWidget
{
public:
    //! The dialog becomes a child of the returned frame, which deletes itself when it is closed
    static MobileWidgetDialogFrame* wrap(QDialog* dialog);

private:
    explicit MobileWidgetDialogFrame(QDialog* dialog);

    bool eventFilter(QObject* watched, QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

    QPointer<QDialog> m_dialog;
};
}
