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
#include <gmock/gmock.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QPointingDevice>
#include <QQuickItem>
#include <QThread>
#include <QTouchEvent>

#include "mocks/controlledviewmock.h"
#include "actions/tests/mocks/actionsdispatchermock.h"
#include "notation/tests/mocks/notationconfigurationmock.h"
#include "notation/tests/mocks/notationinteractionmock.h"
#include "notation/tests/mocks/notationnoteinputmock.h"
#include "notation/tests/mocks/notationselectionmock.h"
#include "playback/tests/mocks/playbackcontrollermock.h"

#include "engraving/tests/utils/scorerw.h"

#include "notationscene/qml/MuseScore/NotationScene/notationviewinputcontroller.h"
#include "notationscene/qml/MuseScore/NotationScene/notationviewtouchcontroller.h"

using ::testing::_;
using ::testing::An;
using ::testing::AtLeast;
using ::testing::DoubleEq;
using ::testing::FloatEq;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

using namespace mu;
using namespace mu::notation;
using namespace muse;

using TouchPoints = NotationViewTouchController::TouchPoints;

static const String TEST_SCORE_PATH(u"data/test.mscx");
static constexpr int MOUSE_SELECTION_PROXIMITY = 6;

class TouchModifiersStub : public muse::ui::ITouchModifiers
{
public:
    Qt::KeyboardModifiers modifiers() const override { return m_modifiers; }
    void setModifiers(Qt::KeyboardModifiers modifiers) override { m_modifiers = modifiers; }
    async::Notification modifiersChanged() const override { return async::Notification(); }

private:
    Qt::KeyboardModifiers m_modifiers = Qt::NoModifier;
};

class NotationViewTouchControllerTests : public ::testing::Test
{
public:
    void SetUp() override
    {
        m_interaction = std::make_shared<NiceMock<NotationInteractionMock> >();
        m_selection = std::make_shared<NiceMock<NotationSelectionMock> >();
        m_noteInput = std::make_shared<NiceMock<NotationNoteInputMock> >();

        ON_CALL(*m_interaction, selection()).WillByDefault(Return(m_selection));
        ON_CALL(*m_interaction, noteInput()).WillByDefault(Return(m_noteInput));
        ON_CALL(*m_noteInput, state()).WillByDefault(ReturnRef(m_noteInputState));
        ON_CALL(*m_interaction, hitElementContext()).WillByDefault(ReturnRef(m_hitContext));
        ON_CALL(*m_selection, elements()).WillByDefault(ReturnRef(m_selectedElements));

        ON_CALL(m_view, notationInteraction()).WillByDefault(Return(m_interaction));
        ON_CALL(m_view, asItem()).WillByDefault(Return(&m_item));
        ON_CALL(m_view, currentScaling()).WillByDefault(Return(1));
        ON_CALL(m_view, moveCanvas(_, _)).WillByDefault(Return(true));
        ON_CALL(m_view, toLogical(An<const QPointF&>())).WillByDefault([](const QPointF& pos) {
            return PointF::fromQPointF(pos);
        });

        m_configuration = std::make_shared<NiceMock<NotationConfigurationMock> >();
        ON_CALL(*m_configuration, selectionProximity()).WillByDefault(Return(MOUSE_SELECTION_PROXIMITY));
        ON_CALL(*m_configuration, possibleZoomPercentageList()).WillByDefault(Return(QList<int> { 25, 50, 100, 200, 400 }));
        ON_CALL(*m_configuration, scalingFromZoomPercentage(_)).WillByDefault([](int zoom) {
            return zoom / 100.0;
        });

        m_playbackController = std::make_shared<NiceMock<playback::PlaybackControllerMock> >();

        m_inputController = new NotationViewInputController(&m_view, muse::modularity::globalCtx());
        m_inputController->configuration.set(m_configuration);
        m_inputController->playbackController.set(m_playbackController);
        m_inputController->init(); // no dispatcher yet, so this only loads the zoom limits

        m_dispatcher = std::make_shared<NiceMock<muse::actions::ActionsDispatcherMock> >();
        m_inputController->dispatcher.set(m_dispatcher);

        m_touchController = new NotationViewTouchController(&m_view, m_inputController);
        m_touchModifiers = std::make_shared<TouchModifiersStub>();
        m_touchController->touchModifiers.set(m_touchModifiers);
    }

    void TearDown() override
    {
        delete m_touchController;
        delete m_inputController;
        delete m_score;
    }

    static TouchPoints finger(const QPointF& pos)
    {
        return { { 0, pos } };
    }

    static TouchPoints fingers(const QPointF& pos1, const QPointF& pos2)
    {
        return { { 0, pos1 }, { 1, pos2 } };
    }

    //! Runs the event loop, so that the long press and fling timers fire
    static void wait(int ms)
    {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < ms) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
            QThread::msleep(1);
        }
    }

    void swipeLeftQuickly()
    {
        m_touchController->touchBegin(finger(QPointF(300, 300)));
        for (int i = 1; i <= 5; ++i) {
            wait(8);
            m_touchController->touchUpdate(finger(QPointF(300 - 30 * i, 300)));
        }
        m_touchController->touchEnd(QPointF(150, 300));
    }

    //! The first note of the test score, selected
    EngravingItem* selectedNote()
    {
        m_score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);
        engraving::ChordRest* chordRest = m_score->firstMeasure()->segments().firstCRSegment()->nextChordRest(0);
        EngravingItem* note = engraving::toChord(chordRest)->notes().front();
        note->setSelected(true);
        return note;
    }

    QQuickItem m_item;
    NiceMock<ControlledViewMock> m_view;
    std::shared_ptr<NotationInteractionMock> m_interaction;
    std::shared_ptr<NotationSelectionMock> m_selection;
    std::shared_ptr<NotationNoteInputMock> m_noteInput;
    NoteInputState m_noteInputState;
    std::shared_ptr<NotationConfigurationMock> m_configuration;
    std::shared_ptr<playback::PlaybackControllerMock> m_playbackController;
    std::shared_ptr<muse::actions::ActionsDispatcherMock> m_dispatcher;
    std::shared_ptr<TouchModifiersStub> m_touchModifiers;

    INotationInteraction::HitElementContext m_hitContext;
    std::vector<EngravingItem*> m_selectedElements;
    engraving::MasterScore* m_score = nullptr;

    NotationViewInputController* m_inputController = nullptr;
    NotationViewTouchController* m_touchController = nullptr;
};

/**
 * @brief Drag_On_Empty_Space_Pans_The_Score
 * @details A finger that moves from an empty place moves the score with it, without selecting anything
 */
TEST_F(NotationViewTouchControllerTests, Drag_On_Empty_Space_Pans_The_Score)
{
    //! [GIVEN] The score is zoomed to 200%
    ON_CALL(m_view, currentScaling()).WillByDefault(Return(2));

    //! [THEN] The score follows the finger (in logical units: half of the distance in view pixels)
    {
        ::testing::InSequence seq;
        EXPECT_CALL(m_view, moveCanvas(DoubleEq(15), DoubleEq(20))).WillOnce(Return(true));
        EXPECT_CALL(m_view, moveCanvas(DoubleEq(5), DoubleEq(0))).WillOnce(Return(true));
    }

    //! [THEN] Nothing is selected
    EXPECT_CALL(*m_interaction, select(_, _, _)).Times(0);

    //! [WHEN] The finger moves twice and lifts slowly (so the score is not thrown)
    m_touchController->touchBegin(finger(QPointF(100, 100)));
    m_touchController->touchUpdate(finger(QPointF(130, 140)));
    m_touchController->touchUpdate(finger(QPointF(140, 140)));
    wait(150);
    m_touchController->touchEnd(QPointF(140, 140));
    wait(50);
}

/**
 * @brief Tap_Is_A_Click_With_Touch_Tolerance
 * @details A tap, even with a little finger movement, is a click; elements are hit within a fingertip's width
 */
TEST_F(NotationViewTouchControllerTests, Tap_Is_A_Click_With_Touch_Tolerance)
{
    //! [THEN] The score does not move
    EXPECT_CALL(m_view, moveCanvas(_, _)).Times(0);

    //! [THEN] Elements are looked for within 24 px (half of it on each side) instead of the 6 px used for a mouse
    EXPECT_CALL(*m_interaction, hitElement(PointF(105, 103), FloatEq(12))).Times(1);

    //! [WHEN] The finger moves less than the tap slop
    m_touchController->touchBegin(finger(QPointF(100, 100)));
    m_touchController->touchUpdate(finger(QPointF(105, 103)));
    m_touchController->touchEnd(QPointF(105, 103));
}

/**
 * @brief Mouse_Click_Keeps_Mouse_Tolerance
 * @details Mouse events are not affected by the touch tolerance
 */
TEST_F(NotationViewTouchControllerTests, Mouse_Click_Keeps_Mouse_Tolerance)
{
    //! [GIVEN] There was a tap before
    m_touchController->touchBegin(finger(QPointF(100, 100)));
    m_touchController->touchEnd(QPointF(100, 100));

    //! [THEN] A mouse click uses the configured proximity
    EXPECT_CALL(*m_interaction, hitElement(_, FloatEq(MOUSE_SELECTION_PROXIMITY * 0.5))).Times(1);

    //! [WHEN] The mouse is clicked
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(200, 200), QPointF(200, 200), QPointF(200, 200),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    m_inputController->mousePressEvent(&event);
}

/**
 * @brief Long_Press_Opens_Context_Menu
 */
TEST_F(NotationViewTouchControllerTests, Long_Press_Opens_Context_Menu)
{
    //! [THEN] The context menu opens where the finger is
    EXPECT_CALL(m_view, showContextMenu(_, QPointF(100, 100))).Times(1);

    //! [WHEN] The finger rests longer than the long press interval
    m_touchController->touchBegin(finger(QPointF(100, 100)));
    wait(NotationViewTouchController::LONG_PRESS_INTERVAL_MS + 150);
    m_touchController->touchEnd(QPointF(100, 100));
}

/**
 * @brief Long_Press_In_Note_Input_Previews_Note
 * @details In note input mode, a long press shows the note under the finger, follows it, and puts it on release
 */
TEST_F(NotationViewTouchControllerTests, Long_Press_In_Note_Input_Previews_Note)
{
    //! [GIVEN] Note input mode
    ON_CALL(m_view, isNoteEnterMode()).WillByDefault(Return(true));
    ON_CALL(*m_noteInput, isNoteInputMode()).WillByDefault(Return(true));

    //! [THEN] No context menu
    EXPECT_CALL(m_view, showContextMenu(_, _)).Times(0);

    //! [THEN] The preview appears under the resting finger
    EXPECT_CALL(*m_interaction, showShadowNoteForPosition(PointF(100, 100))).Times(1);

    //! [WHEN] The finger rests
    m_touchController->touchBegin(finger(QPointF(100, 100)));
    wait(NotationViewTouchController::LONG_PRESS_INTERVAL_MS + 150);
    ::testing::Mock::VerifyAndClearExpectations(m_interaction.get());

    //! [THEN] The preview follows the finger
    EXPECT_CALL(*m_interaction, showShadowNoteForPosition(PointF(100, 130))).Times(1);

    //! [WHEN] The finger moves
    m_touchController->touchUpdate(finger(QPointF(100, 130)));
    ::testing::Mock::VerifyAndClearExpectations(m_interaction.get());

    //! [THEN] The note is put where the finger is lifted
    EXPECT_CALL(*m_dispatcher, dispatch(muse::actions::ActionCode("put-note"), _)).Times(1);

    //! [WHEN] The finger lifts
    m_touchController->touchEnd(QPointF(100, 130));
}

/**
 * @brief Tap_In_Note_Input_Puts_Note
 */
TEST_F(NotationViewTouchControllerTests, Tap_In_Note_Input_Puts_Note)
{
    //! [GIVEN] Note input mode
    ON_CALL(m_view, isNoteEnterMode()).WillByDefault(Return(true));
    ON_CALL(*m_noteInput, isNoteInputMode()).WillByDefault(Return(true));

    //! [THEN] A note is put, nothing is removed
    EXPECT_CALL(*m_dispatcher, dispatch(muse::actions::ActionCode("put-note"), _)).Times(1);
    EXPECT_CALL(*m_dispatcher, dispatch(muse::actions::ActionCode("remove-note"), _)).Times(0);

    //! [WHEN] The staff is tapped
    m_touchController->touchBegin(finger(QPointF(100, 100)));
    m_touchController->touchEnd(QPointF(100, 100));
}

/**
 * @brief Two_Fingers_Zoom_Around_Their_Center
 */
TEST_F(NotationViewTouchControllerTests, Two_Fingers_Zoom_Around_Their_Center)
{
    //! [THEN] The zoom doubles around the point between the fingers
    EXPECT_CALL(m_view, setScaling(DoubleEq(2.0), PointF(150, 100), _)).Times(1);

    //! [THEN] The center did not move, so neither does the score; nothing is selected
    EXPECT_CALL(m_view, moveCanvas(_, _)).Times(0);
    EXPECT_CALL(*m_interaction, select(_, _, _)).Times(0);

    //! [WHEN] Two fingers spread from 100 to 200 px apart
    m_touchController->touchBegin(finger(QPointF(100, 100)));
    m_touchController->touchUpdate(fingers(QPointF(100, 100), QPointF(200, 100)));
    m_touchController->touchUpdate(fingers(QPointF(50, 100), QPointF(250, 100)));
    m_touchController->touchEnd(QPointF(250, 100));
}

/**
 * @brief Two_Fingers_Pan
 */
TEST_F(NotationViewTouchControllerTests, Two_Fingers_Pan)
{
    //! [THEN] The score moves with the fingers, without zooming
    EXPECT_CALL(m_view, moveCanvas(DoubleEq(0), DoubleEq(40))).WillOnce(Return(true));
    EXPECT_CALL(m_view, setScaling(_, _, _)).Times(0);

    //! [WHEN] Two fingers move down together
    m_touchController->touchBegin(fingers(QPointF(100, 100), QPointF(200, 100)));
    m_touchController->touchUpdate(fingers(QPointF(100, 140), QPointF(200, 140)));
    m_touchController->touchEnd(QPointF(200, 140));
}

/**
 * @brief Quick_Swipe_Flings_The_Score
 * @details After a quick swipe, the score keeps moving in the same direction for a while
 */
TEST_F(NotationViewTouchControllerTests, Quick_Swipe_Flings_The_Score)
{
    //! [WHEN] The finger swipes to the left quickly
    swipeLeftQuickly();

    //! [THEN] The score keeps moving to the left after the finger lifted
    int steps = 0;
    EXPECT_CALL(m_view, moveCanvas(_, _)).WillRepeatedly([&steps](qreal dx, qreal dy) {
        EXPECT_LT(dx, 0);
        EXPECT_DOUBLE_EQ(dy, 0);
        ++steps;
        return true;
    });

    wait(150);
    EXPECT_GE(steps, 3);

    //! [WHEN] A finger touches the score again
    //! [THEN] It stops
    m_touchController->touchBegin(finger(QPointF(100, 100)));
    const int stepsWhenStopped = steps;
    wait(100);
    EXPECT_EQ(steps, stepsWhenStopped);
}

/**
 * @brief Fling_Stops_At_The_End_Of_The_Score
 */
TEST_F(NotationViewTouchControllerTests, Fling_Stops_At_The_End_Of_The_Score)
{
    swipeLeftQuickly();

    //! [THEN] Once the score cannot move further, the fling stops
    EXPECT_CALL(m_view, moveCanvas(_, _)).WillOnce(Return(false));

    wait(150);
}

/**
 * @brief Stylus_Is_Not_Handled
 * @details Stylus input keeps the default handling (mouse events, with hover), only fingers are handled
 */
TEST_F(NotationViewTouchControllerTests, Stylus_Is_Not_Handled)
{
    const QList<QEventPoint> points { QEventPoint(0, QEventPoint::Pressed, QPointF(10, 10), QPointF(10, 10)) };

    QPointingDevice stylus("test stylus", 1001, QInputDevice::DeviceType::Stylus, QPointingDevice::PointerType::Pen,
                           QInputDevice::Capability::Position, 1, 2);
    QTouchEvent stylusEvent(QEvent::TouchBegin, &stylus, Qt::NoModifier, points);
    EXPECT_FALSE(m_touchController->handleTouchEvent(&stylusEvent));

    QPointingDevice screen("test touch screen", 1002, QInputDevice::DeviceType::TouchScreen,
                           QPointingDevice::PointerType::Finger, QInputDevice::Capability::Position, 10, 0);
    QTouchEvent touchEvent(QEvent::TouchBegin, &screen, Qt::NoModifier, points);
    EXPECT_TRUE(m_touchController->handleTouchEvent(&touchEvent));
}

/**
 * @brief Drag_From_Selected_Note_Moves_It
 * @details A drag that starts on a selected element moves that element, not the score
 */
TEST_F(NotationViewTouchControllerTests, Drag_From_Selected_Note_Moves_It)
{
    //! [GIVEN] A selected note under the finger
    EngravingItem* note = selectedNote();
    m_hitContext.element = note;
    m_hitContext.staff = note->staff();
    m_selectedElements = { note };

    ON_CALL(*m_interaction, hitElement(_, _)).WillByDefault(Return(note));
    ON_CALL(*m_interaction, isDragStarted()).WillByDefault(Return(true));

    //! [THEN] The note is dragged from where the finger went down to where it is
    EXPECT_CALL(*m_interaction, drag(PointF(100, 100), PointF(100, 60), _)).Times(1);

    //! [THEN] The score does not move
    EXPECT_CALL(m_view, moveCanvas(_, _)).Times(0);

    //! [WHEN] The finger moves up
    m_touchController->touchBegin(finger(QPointF(100, 100)));
    m_touchController->touchUpdate(finger(QPointF(100, 60)));
    m_touchController->touchEnd(QPointF(100, 60));
}

/**
 * @brief Tap_With_Shift_On_Selects_A_Range
 * @details With the Shift toggle on, a tap is a Shift+click
 */
TEST_F(NotationViewTouchControllerTests, Tap_With_Shift_On_Selects_A_Range)
{
    //! [GIVEN] A note (not selected) under the finger
    EngravingItem* note = selectedNote();
    note->setSelected(false);
    m_hitContext.element = note;
    m_hitContext.staff = note->staff();

    ON_CALL(*m_interaction, hitElement(_, _)).WillByDefault(Return(note));

    //! [GIVEN] The Shift toggle is on
    m_touchModifiers->setModifiers(Qt::ShiftModifier);

    //! [THEN] The selection is extended to the note as a range
    EXPECT_CALL(*m_interaction, select(std::vector<EngravingItem*> { note }, SelectType::RANGE, _)).Times(1);

    //! [WHEN] The note is tapped
    m_touchController->touchBegin(finger(QPointF(100, 100)));
    m_touchController->touchEnd(QPointF(100, 100));
}

/**
 * @brief Drag_With_Shift_On_Selects_With_A_Lasso
 * @details With the Shift toggle on, a drag on an empty place draws a selection rectangle instead of panning
 */
TEST_F(NotationViewTouchControllerTests, Drag_With_Shift_On_Selects_With_A_Lasso)
{
    //! [GIVEN] The Shift toggle is on
    m_touchModifiers->setModifiers(Qt::ShiftModifier);

    //! [THEN] A lasso goes from where the finger went down to where it is
    EXPECT_CALL(*m_interaction, startDrag(_, _, _)).Times(1);
    EXPECT_CALL(*m_interaction, drag(PointF(100, 100), PointF(160, 140), DragMode::BothXY)).Times(1);

    //! [THEN] The score does not move
    EXPECT_CALL(m_view, moveCanvas(_, _)).Times(0);

    //! [WHEN] The finger moves over an empty place
    m_touchController->touchBegin(finger(QPointF(100, 100)));
    m_touchController->touchUpdate(finger(QPointF(160, 140)));
    m_touchController->touchEnd(QPointF(160, 140));
}
