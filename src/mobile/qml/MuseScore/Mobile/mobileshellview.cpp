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
#include "mobileshellview.h"

#include "log.h"

using namespace mu::mobile;
using namespace muse;

MobileShellView::MobileShellView(QQuickItem* parent)
    : QQuickItem(parent), Contextable(iocCtxForQmlObject(this))
{
}

MobileShellView::~MobileShellView()
{
    if (dockWindowProvider() && dockWindowProvider()->window() == this) {
        dockWindowProvider()->deinit();
    }
}

void MobileShellView::componentComplete()
{
    QQuickItem::componentComplete();

    if (dockWindowProvider()) {
        dockWindowProvider()->init(this);
    }

    if (globalContext()) {
        globalContext()->currentNotationChanged().onNotify(this, [this]() {
            listenNoteInputMode();
        });
        listenNoteInputMode();
    }
}

void MobileShellView::listenNoteInputMode()
{
    const notation::INotationPtr notation = globalContext()->currentNotation();
    if (notation) {
        notation->interaction()->noteInput()->stateChanged().onNotify(this, [this]() {
            updateNoteInputMode();
        });
    }

    updateNoteInputMode();
}

void MobileShellView::updateNoteInputMode()
{
    const notation::INotationPtr notation = globalContext()->currentNotation();
    const bool isNoteInputMode = notation && notation->interaction()->noteInput()->isNoteInputMode();

    if (m_isNoteInputMode != isNoteInputMode) {
        m_isNoteInputMode = isNoteInputMode;
        emit isNoteInputModeChanged();
    }
}

bool MobileShellView::isNoteInputMode() const
{
    return m_isNoteInputMode;
}

QString MobileShellView::currentPageUri() const
{
    return m_currentPageUri;
}

QVariantMap MobileShellView::currentPageParams() const
{
    return m_currentPageParams;
}

QStringList MobileShellView::openDocks() const
{
    return m_openDocks;
}

bool MobileShellView::panelsOverlay() const
{
    return m_panelsOverlay;
}

void MobileShellView::setPanelsOverlay(bool overlay)
{
    if (m_panelsOverlay == overlay) {
        return;
    }

    m_panelsOverlay = overlay;
    emit panelsOverlayChanged();
}

QStringList MobileShellView::exclusiveGroups() const
{
    return m_exclusiveGroups;
}

void MobileShellView::setExclusiveGroups(const QStringList& groups)
{
    if (m_exclusiveGroups == groups) {
        return;
    }

    m_exclusiveGroups = groups;
    emit panelsOverlayChanged();
}

bool MobileShellView::closeLastOpenedPanel()
{
    // The shell's own overlays first (e.g. the app menu drawer): a QML function "closeOverlay()"
    if (metaObject()->indexOfMethod("closeOverlay()") >= 0) {
        QVariant closed;
        QMetaObject::invokeMethod(this, "closeOverlay", Q_RETURN_ARG(QVariant, closed));
        if (closed.toBool()) {
            return true;
        }
    }

    if (!m_panelsOverlay) {
        return false;
    }

    for (auto it = m_openDocks.crbegin(); it != m_openDocks.crend(); ++it) {
        if (!m_docks.value(*it).group.isEmpty()) {
            setDockOpen(*it, false);
            return true;
        }
    }

    return false;
}

void MobileShellView::dispatch(const QString& actionCode)
{
    dispatcher()->dispatch(actionCode.toStdString());
}

void MobileShellView::loadPage(const QString& uri, const QVariantMap& params)
{
    if (m_currentPageUri == uri && m_currentPageParams == params) {
        return;
    }

    m_currentPageUri = uri;
    m_currentPageParams = params;
    emit currentPageChanged();
    emit pageLoaded();
}

void MobileShellView::registerDock(const QString& dockName, const QString& group, bool openByDefault)
{
    if (m_docks.contains(dockName)) {
        return;
    }

    m_docks.insert(dockName, Dock { group, openByDefault });

    if (openByDefault) {
        setDockOpen(dockName, true);
    }
}

bool MobileShellView::isDockOpen(const QString& dockName) const
{
    return m_openDocks.contains(dockName);
}

void MobileShellView::setDockOpen(const QString& dockName, bool open)
{
    if (isDockOpen(dockName) == open) {
        return;
    }

    QStringList changed { dockName };

    if (open) {
        // The panels of a group share one sheet; the sheets of exclusive groups are shown one at a time
        const QString group = m_docks.value(dockName).group;
        if (!group.isEmpty()) {
            const bool isExclusive = m_exclusiveGroups.contains(group);
            for (const QString& other : QStringList(m_openDocks)) {
                const QString otherGroup = m_docks.value(other).group;
                if (otherGroup == group || (isExclusive && m_exclusiveGroups.contains(otherGroup))) {
                    m_openDocks.removeAll(other);
                    changed << other;
                }
            }
        }

        m_openDocks << dockName;
    } else {
        m_openDocks.removeAll(dockName);
    }

    notifyAboutDocksChanged(changed);
}

void MobileShellView::toggleDock(const QString& dockName)
{
    setDockOpen(dockName, !isDockOpen(dockName));
}

async::Channel<QStringList> MobileShellView::docksOpenStatusChanged() const
{
    return m_docksOpenStatusChanged;
}

bool MobileShellView::isDockFloating(const QString&) const
{
    return false;
}

void MobileShellView::toggleDockFloating(const QString&)
{
}

dock::DockPageView* MobileShellView::currentPage() const
{
    // Pages are not dock pages here
    return nullptr;
}

QQuickItem& MobileShellView::asItem() const
{
    return const_cast<MobileShellView&>(*this);
}

void MobileShellView::restoreDefaultLayout()
{
    QStringList changed = m_openDocks;
    m_openDocks.clear();

    for (auto it = m_docks.cbegin(); it != m_docks.cend(); ++it) {
        if (it.value().openByDefault) {
            m_openDocks << it.key();
            if (!changed.contains(it.key())) {
                changed << it.key();
            }
        }
    }

    notifyAboutDocksChanged(changed);
}

void MobileShellView::notifyAboutDocksChanged(const QStringList& changed)
{
    emit openDocksChanged();
    m_docksOpenStatusChanged.send(changed);
}
