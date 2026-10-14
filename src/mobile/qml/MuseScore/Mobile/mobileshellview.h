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

#include <QHash>
#include <QQuickItem>
#include <QStringList>
#include <QVariantMap>
#include <qqmlintegration.h>

#include "async/asyncable.h"
#include "modularity/ioc.h"
#include "dockwindow/idockwindow.h"
#include "dockwindow/idockwindowprovider.h"
#include "actions/iactionsdispatcher.h"
#include "context/iglobalcontext.h"

namespace mu::mobile {
//! The window content of the mobile shell: shows one page at a time (Home, score, ...) and keeps track of
//! the panels ("docks") that are open. The panels are shown in sheets; the panels of a group (e.g. those of
//! the left sheet) are shown one at a time. As the IDockWindow, it serves the dock actions (View menu,
//! "dock-toggle", panels opened automatically, ...) like the docking window of the desktop app.
class MobileShellView : public QQuickItem, public muse::dock::IDockWindow, public muse::Contextable, public muse::async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(QString currentPageUri READ currentPageUri NOTIFY currentPageChanged)
    Q_PROPERTY(QVariantMap currentPageParams READ currentPageParams NOTIFY currentPageChanged)
    Q_PROPERTY(QStringList openDocks READ openDocks NOTIFY openDocksChanged)

    //! Phones: panels are shown over the score, so the sheets of these groups are shown one at a time,
    //! and the Back key closes them
    Q_PROPERTY(bool panelsOverlay READ panelsOverlay WRITE setPanelsOverlay NOTIFY panelsOverlayChanged)
    Q_PROPERTY(QStringList exclusiveGroups READ exclusiveGroups WRITE setExclusiveGroups NOTIFY panelsOverlayChanged)

    //! Whether the current score is in note input mode (e.g. to show the note input keypad on phones)
    Q_PROPERTY(bool isNoteInputMode READ isNoteInputMode NOTIFY isNoteInputModeChanged)

    //! Whether text on the score is being edited, and which kind of text (see EditedTextKind),
    //! for the bar of text editing over the keyboard
    Q_PROPERTY(bool isEditingText READ isEditingText NOTIFY editedTextChanged)
    Q_PROPERTY(int editedTextKind READ editedTextKind NOTIFY editedTextChanged)

    QML_ELEMENT

    muse::ContextInject<muse::dock::IDockWindowProvider> dockWindowProvider = { this };
    muse::ContextInject<muse::actions::IActionsDispatcher> dispatcher = { this };
    muse::ContextInject<context::IGlobalContext> globalContext = { this };

public:
    enum EditedTextKind {
        OtherText,
        Lyrics,
        ChordSymbol,
        MusicalText // fingerings, figured bass, sticking: one per note, like lyrics
    };
    Q_ENUM(EditedTextKind)

    explicit MobileShellView(QQuickItem* parent = nullptr);
    ~MobileShellView() override;

    QString currentPageUri() const;
    QVariantMap currentPageParams() const;
    QStringList openDocks() const;

    bool panelsOverlay() const;
    void setPanelsOverlay(bool overlay);
    QStringList exclusiveGroups() const;
    void setExclusiveGroups(const QStringList& groups);

    bool isNoteInputMode() const;
    bool isEditingText() const;
    int editedTextKind() const;

    //! Back key: closes the shell's overlay (see closeOverlay() in MobileShell.qml),
    //! or the panel opened last when panels are shown over the score
    Q_INVOKABLE bool closeLastOpenedPanel();

    //! Runs an action (e.g. "play") from a button of the shell
    Q_INVOKABLE void dispatch(const QString& actionCode);

    //! Called by the interactive provider for primary pages (musescore://home, musescore://notation, ...)
    Q_INVOKABLE void loadPage(const QString& uri, const QVariantMap& params);

    //! Declares a panel: its group (panels of a group are shown one at a time) and whether it is open by default
    Q_INVOKABLE void registerDock(const QString& dockName, const QString& group, bool openByDefault);

    // IDockWindow
    Q_INVOKABLE bool isDockOpen(const QString& dockName) const override;
    Q_INVOKABLE void setDockOpen(const QString& dockName, bool open) override;
    Q_INVOKABLE void toggleDock(const QString& dockName) override;
    muse::async::Channel<QStringList> docksOpenStatusChanged() const override;

    bool isDockFloating(const QString& dockName) const override;
    void toggleDockFloating(const QString& dockName) override;

    muse::dock::DockPageView* currentPage() const override;
    QQuickItem& asItem() const override;

    Q_INVOKABLE void restoreDefaultLayout() override;

signals:
    void currentPageChanged();
    void openDocksChanged();
    void panelsOverlayChanged();
    void isNoteInputModeChanged();
    void editedTextChanged();

    //! A page is shown, the first one or another one
    void pageLoaded();

protected:
    void componentComplete() override;

private:
    struct Dock {
        QString group;
        bool openByDefault = false;
    };

    void notifyAboutDocksChanged(const QStringList& changed);
    void listenNotation();
    void updateNoteInputMode();
    void updateEditedText();

    QString m_currentPageUri;
    QVariantMap m_currentPageParams;

    QHash<QString, Dock> m_docks;
    QStringList m_openDocks; // in the order they were opened
    bool m_panelsOverlay = false;
    QStringList m_exclusiveGroups;
    bool m_isNoteInputMode = false;
    bool m_isEditingText = false;
    EditedTextKind m_editedTextKind = OtherText;
    muse::async::Channel<QStringList> m_docksOpenStatusChanged;
};
}
