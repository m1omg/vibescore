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
#include "mobilewidgetdialogframe.h"

#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QScroller>
#include <QToolButton>
#include <QVBoxLayout>

#include "translation.h"

using namespace muse::ui;

MobileWidgetDialogFrame* MobileWidgetDialogFrame::wrap(QDialog* dialog)
{
    return new MobileWidgetDialogFrame(dialog);
}

//! The button box of the dialog itself (not one of a part of it): in the dialog's own layout
static QDialogButtonBox* mainButtonBox(QDialog* dialog)
{
    QDialogButtonBox* result = nullptr;
    for (QDialogButtonBox* box : dialog->findChildren<QDialogButtonBox*>()) {
        if (box->parentWidget() == dialog) {
            result = box; // the last one: usually at the bottom
        }
    }
    return result;
}

// The frame has its own title bar: no window decoration (the web platform of Qt draws one)
MobileWidgetDialogFrame::MobileWidgetDialogFrame(QDialog* dialog)
    : QWidget(nullptr, Qt::Window | Qt::FramelessWindowHint), m_dialog(dialog)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(dialog->windowTitle());

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Title bar
    auto* titleBar = new QWidget(this);
    auto* titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(16, 4, 4, 4);

    auto* titleLabel = new QLabel(dialog->windowTitle(), titleBar);
    QFont titleFont = titleLabel->font();
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLayout->addWidget(titleLabel, 1);

    auto* closeButton = new QToolButton(titleBar);
    closeButton->setText(QStringLiteral("✕"));
    closeButton->setToolTip(muse::qtrc("global", "Close"));
    closeButton->setAutoRaise(true);
    closeButton->setMinimumSize(40, 40);
    titleLayout->addWidget(closeButton);
    connect(closeButton, &QToolButton::clicked, this, &QWidget::close);

    connect(dialog, &QWidget::windowTitleChanged, titleLabel, &QLabel::setText);

    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Plain);

    // The dialog, scrolled with the finger when it is larger than the window
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    scrollArea->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    dialog->setWindowFlags(Qt::Widget);
    scrollArea->setWidget(dialog);
    QScroller::grabGesture(scrollArea->viewport(), QScroller::TouchGesture);

    layout->addWidget(titleBar);
    layout->addWidget(separator);
    layout->addWidget(scrollArea, 1);

    // The buttons of the dialog (OK, Cancel...) always in sight, below the content, scrolled sideways if they do not fit
    if (QDialogButtonBox* buttons = mainButtonBox(dialog)) {
        auto* buttonsSeparator = new QFrame(this);
        buttonsSeparator->setFrameShape(QFrame::HLine);
        buttonsSeparator->setFrameShadow(QFrame::Plain);

        auto* buttonsArea = new QScrollArea(this);
        buttonsArea->setFrameShape(QFrame::NoFrame);
        buttonsArea->setWidgetResizable(true);
        buttonsArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        buttonsArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        buttonsArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto* buttonsBar = new QWidget(buttonsArea);
        auto* buttonsLayout = new QHBoxLayout(buttonsBar);
        buttonsLayout->setContentsMargins(12, 8, 12, 8);
        buttonsLayout->addWidget(buttons); // takes it out of the dialog's layout; its connections stay
        buttonsArea->setWidget(buttonsBar);
        buttonsArea->setFixedHeight(buttonsBar->sizeHint().height());
        QScroller::grabGesture(buttonsArea->viewport(), QScroller::TouchGesture);

        layout->addWidget(buttonsSeparator);
        layout->addWidget(buttonsArea);
    }

    dialog->installEventFilter(this);
}

bool MobileWidgetDialogFrame::eventFilter(QObject* watched, QEvent* event)
{
    // The dialog is done (accepted, rejected or closed)
    if (watched == m_dialog && event->type() == QEvent::Hide && isVisible()) {
        close();
    }

    return QWidget::eventFilter(watched, event);
}

void MobileWidgetDialogFrame::closeEvent(QCloseEvent* event)
{
    // Closed by the close button, the Back key or IInteractive::close: as the window's close button does
    if (m_dialog && m_dialog->isVisible()) {
        m_dialog->reject();
    }

    event->accept();
}
