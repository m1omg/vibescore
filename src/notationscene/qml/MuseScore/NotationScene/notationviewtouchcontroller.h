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

#include <vector>

#include <QElapsedTimer>
#include <QEvent>
#include <QObject>
#include <QPointF>
#include <QTimer>

class QTouchEvent;

namespace mu::notation {
class IControlledView;
class NotationViewInputController;

//! Turns touch screen input on the score into the existing mouse interactions,
//! plus gestures that only make sense with fingers:
//! - tap: click (select; put a note in note input mode)
//! - double tap: double click (edit text, select a chord, ...)
//! - drag that starts on a selected element or a grip: drags it; any other drag pans the score, with momentum
//! - long press: context menu; in note input mode, a preview of the note under the finger, placed on release
//! - two fingers: zoom and pan
//! Stylus and mouse input are not handled here, so they keep their precise desktop behavior.
class NotationViewTouchController : public QObject
{
    Q_OBJECT

public:
    static constexpr int LONG_PRESS_INTERVAL_MS = 450;
    static constexpr qreal TAP_SLOP = 10; // view pixels (dp)

    struct TouchPoint {
        int id = 0;
        QPointF pos; // in view coordinates
    };
    using TouchPoints = std::vector<TouchPoint>;

    NotationViewTouchController(IControlledView* view, NotationViewInputController* inputController, QObject* parent = nullptr);

    //! On by default on mobile; MUSE_UI_TOUCH=0|1 overrides it (e.g. to try it on a desktop touch screen)
    static bool isEnabled();

    //! Returns false for events this controller does not handle (e.g. from a stylus)
    bool handleTouchEvent(QTouchEvent* event);

    //! The fingers currently on the screen
    void touchBegin(const TouchPoints& points);
    void touchUpdate(const TouchPoints& points);
    void touchEnd(const QPointF& lastPos);

    //! The touch sequence was taken away (e.g. by a popup)
    void cancel();

signals:
    void focusRequested();

private:
    enum class State {
        Idle,
        Pending,     // finger down, not yet moved or held
        Panning,
        Dragging,    // moving a selected element or a grip
        LongPressed, // context menu shown
        NotePreview, // note input: the shadow note follows the finger
        Pinching
    };

    void onTap(const QPointF& pos);
    void onLongPressTimeout();

    void pan(const QPointF& delta);
    void beginPinch(const QPointF& p1, const QPointF& p2);
    void updatePinch(const QPointF& p1, const QPointF& p2);

    void startFling();
    void onFlingStep();
    void stopFling();

    void sendMouse(QEvent::Type type, const QPointF& pos, Qt::MouseButton button, Qt::MouseButtons buttons);
    void sendHover(const QPointF& pos, const QPointF& oldPos);
    void sendHoverLeave(const QPointF& pos);

    IControlledView* m_view = nullptr;
    NotationViewInputController* m_input = nullptr;

    State m_state = State::Idle;
    int m_primaryPointId = -1;
    QPointF m_startPos;
    QPointF m_lastPos;
    QTimer m_longPressTimer;

    bool m_hasLastTap = false;
    QPointF m_lastTapPos;
    QElapsedTimer m_lastTapTimer;

    qreal m_lastPinchDistance = 0;
    QPointF m_lastPinchCenter;

    QPointF m_velocity; // view pixels per millisecond
    QElapsedTimer m_moveTimer;
    QTimer m_flingTimer;
    QElapsedTimer m_flingClock;
};
}
