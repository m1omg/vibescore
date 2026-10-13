/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited and others
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

#include "mocks/controlledviewmock.h"
#include "actions/tests/mocks/actionsdispatchermock.h"
#include "notation/tests/mocks/notationconfigurationmock.h"
#include "notation/tests/mocks/notationinteractionmock.h"
#include "notation/tests/mocks/notationselectionmock.h"
#include "notation/tests/mocks/notationselectionrangemock.h"
#include "playback/tests/mocks/playbackcontrollermock.h"

#include "engraving/dom/chord.h"
#include "engraving/dom/factory.h"
#include "engraving/dom/lyrics.h"
#include "engraving/dom/measure.h"
#include "engraving/dom/segment.h"
#include "engraving/dom/stafftext.h"
#include "engraving/tests/utils/scorerw.h"

#include "notationscene/qml/MuseScore/NotationScene/notationviewinputcontroller.h"

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::InSequence;

using namespace mu;
using namespace mu::notation;
using namespace muse;

using muse::actions::ActionCode;

static const String TEST_SCORE_PATH(u"data/test.mscx");
static constexpr bool FLUSH_SOUND = true;

class NotationViewInputControllerTests : public ::testing::Test
{
public:

    void SetUp() override
    {
        m_interaction = std::make_shared<NiceMock<NotationInteractionMock> >();

        m_selection = std::make_shared<NiceMock<NotationSelectionMock> >();
        ON_CALL(*m_interaction, selection())
        .WillByDefault(Return(m_selection));

        m_selectionRange = std::make_shared<NiceMock<NotationSelectionRangeMock> >();
        ON_CALL(*m_selection, range())
        .WillByDefault(Return(m_selectionRange));

        ON_CALL(m_view, notationInteraction())
        .WillByDefault(Return(m_interaction));

        ON_CALL(m_view, currentScaling())
        .WillByDefault(Return(1));

        m_controller = new NotationViewInputController(&m_view, muse::modularity::globalCtx());

        m_configuration = std::make_shared<NiceMock<NotationConfigurationMock> >();
        m_controller->configuration.set(m_configuration);

        m_playbackController = std::make_shared<NiceMock<playback::PlaybackControllerMock> >();
        m_controller->playbackController.set(m_playbackController);

        setNoWaylandForLinux();
    }

    void TearDown() override
    {
        qDeleteAll(m_events);
        delete m_controller;
    }

    NotationViewInputController* m_controller = nullptr;
    NiceMock<ControlledViewMock> m_view;
    std::shared_ptr<NotationConfigurationMock> m_configuration;
    std::shared_ptr<NotationInteractionMock> m_interaction;
    std::shared_ptr<NotationSelectionMock> m_selection;
    std::shared_ptr<NotationSelectionRangeMock> m_selectionRange;
    std::shared_ptr<playback::PlaybackControllerMock> m_playbackController;
    playback::IPlaybackController::PlayParams m_playParams;

    mutable QList<QInputEvent*> m_events;

    struct HitContextConfig
    {
        ElementType elementType = ElementType::INVALID;
        bool start = true;
    };

    QWheelEvent* make_wheelEvent(QPoint pixelDelta,
                                 QPoint angleDelta,
                                 Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                                 QPointF pos = QPointF(100, 100)) const
    {
        QPointF globalPos = pos;
        Qt::MouseButtons buttons = Qt::NoButton;

        QWheelEvent* ev = new QWheelEvent(pos,  globalPos,  pixelDelta,  angleDelta,
                                          buttons, modifiers, Qt::ScrollPhase::NoScrollPhase, false);

        m_events << ev;

        return ev;
    }

    QMouseEvent* make_mousePressEvent(
        Qt::MouseButton button,
        Qt::KeyboardModifiers modifiers = Qt::NoModifier,
        QPointF pos = QPointF(100, 100)) const
    {
        QMouseEvent* ev = new QMouseEvent(QMouseEvent::Type::MouseButtonPress, pos, pos, button, {}, modifiers);

        m_events << ev;

        return ev;
    }

    notation::EngravingItem* findElement(ElementType type, ChordRest* chord) const
    {
        switch (type) {
        case ElementType::NOTE:
            return engraving::toChord(chord)->notes().front();
        case ElementType::DYNAMIC: {
            for (notation::EngravingItem* element : chord->segment()->annotations()) {
                if (element->isDynamic()) {
                    return element;
                }
            }
            break;
        }
        case ElementType::HAIRPIN: {
            const mu::engraving::SpannerMap& spannerMap = chord->score()->spannerMap();
            auto intervals = spannerMap.findOverlapping(0, chord->score()->endTick().ticks());
            for (const auto& interval : intervals) {
                mu::engraving::Spanner* spanner = interval.value;
                if (spanner->isHairpin()) {
                    return spanner;
                }
            }
            break;
        }
        case ElementType::MEASURE:
            return chord->segment()->measure();
        default:
            break;
        }

        return nullptr;
    }

    INotationInteraction::HitElementContext hitContext(engraving::MasterScore* score, HitContextConfig config) const
    {
        INotationInteraction::HitElementContext context;

        Measure* firstMeasure = score->firstMeasure();
        Segment* segment = config.start ? firstMeasure->segments().firstCRSegment() : firstMeasure->segments().last();
        ChordRest* chord = segment->nextChordRest(0, !config.start);

        context.element = findElement(config.elementType, chord);
        context.staff = context.element->staff();

        return context;
    }

    INotationInteraction::HitElementContext hitMeasureContext(engraving::MasterScore* score, int index) const
    {
        INotationInteraction::HitElementContext context;

        engraving::MeasureBase* measure = score->measure(index);
        context.element = measure;
        context.staff = score->staff(0);

        return context;
    }

    void setNoWaylandForLinux()
    {
#ifdef Q_OS_LINUX
        //! [GIVEN] No Wayland display
        setenv("WAYLAND_DISPLAY", "OFF", false);
#endif
    }
};

namespace mu::playback {
inline bool operator==(const IPlaybackController::PlayParams& p1, const IPlaybackController::PlayParams& p2)
{
    return p1.duration == p2.duration && p1.flushSound == p2.flushSound;
}
}

/**
 * @brief WheelEvent_ScrollVertical
 * @details Received wheel event, without modifiers
 */
TEST_F(NotationViewInputControllerTests, WheelEvent_ScrollVertical)
{
    //! [THEN] Should be called vertical scroll with value 180
    EXPECT_CALL(m_view, moveCanvas(0, 180))
    .Times(1);

    //! [WHEN] User scrolled mouse wheel
    m_controller->wheelEvent(make_wheelEvent(QPoint(0, 180), QPoint(0, 120)));

    //! [THEN] Should be called vertical scroll with value 80
    EXPECT_CALL(m_view, moveCanvas(0, 80))
    .Times(1);

    //! [WHEN] User scrolled mouse wheel
    m_controller->wheelEvent(make_wheelEvent(QPoint(0, 80), QPoint(0, 120)));

    //! [GIVEN] pixelDelta is Null
    //!  dy = (angleDelta.y() * qMax(2.0, m_view->height() / 10.0)) / QWheelEvent::DefaultDeltasPerStep;

    //! [THEN] Should be called vertical scroll with value 50  (dy = (120 * 500 / 10) / 120 = 50)
    EXPECT_CALL(m_view, moveCanvas(0, 50))
    .Times(1);

    EXPECT_CALL(m_view, height())
    .WillOnce(Return(500));

    //! [WHEN] User scrolled mouse wheel
    m_controller->wheelEvent(make_wheelEvent(QPoint(), QPoint(0, 120)));
}

/**
 * @brief WheelEvent_ScrollHorizontal
 * @details Received wheel event, with key modifier ShiftModifier
 */
TEST_F(NotationViewInputControllerTests, WheelEvent_ScrollHorizontal)
{
    //! [THEN] Should be called horizontal scroll with value 120
    EXPECT_CALL(m_view, moveCanvasHorizontal(120))
    .Times(1);

    //! [WHEN] User scrolled mouse wheel
    m_controller->wheelEvent(make_wheelEvent(QPoint(0, 120), QPoint(), Qt::ShiftModifier));
}

/**
 * @brief WheelEvent_ScrollHorizontal
 * @details Received wheel event, with key modifier ControlModifier
 */
TEST_F(NotationViewInputControllerTests, DISABLED_WheelEvent_Zoom)
{
    //! CASE Received wheel event, with key modifier ControlModifier
    ValCh<int> currentZoom;
    currentZoom.val = 100;

//    ON_CALL(*m_configuration, currentZoom())
//    .WillByDefault(Return(currentZoom));

    //! CHECK Should be called zoomStep with value 110
//    EXPECT_CALL(env.view, setZoom(110, QPoint(100, 100)))
//    .Times(1);

    EXPECT_CALL(m_view, height())
    .WillOnce(Return(500));

    EXPECT_CALL(m_view, toLogical(QPointF(100, 100)))
    .WillOnce(Return(PointF(100, 100)));

    m_controller->wheelEvent(make_wheelEvent(QPoint(), QPoint(0, 120), Qt::ControlModifier, QPointF(100, 100)));
}

/**
 * @brief Mouse_Press_Range_Start_Drag_From_Selected_Element
 * @details User pressed left mouse button on already selected note
 *          The new note should be seeked and played, but no selected again
 */
TEST_F(NotationViewInputControllerTests, Mouse_Press_Range_Start_Drag_From_Selected_Element)
{
    //! [GIVEN] There is a test score
    engraving::MasterScore* score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);

    //! [GIVEN] User selected new note that was already selected
    INotationInteraction::HitElementContext newContext = hitContext(score, { ElementType::NOTE, false /*last note*/ });
    newContext.element->setSelected(true);

    std::vector<EngravingItem*> selectedElements {
        newContext.element
    };

    EXPECT_CALL(*m_interaction, hitElement(_, _))
    .WillOnce(Return(newContext.element));

    EXPECT_CALL(*m_interaction, hitStaff(_))
    .WillOnce(Return(newContext.element->staff()));

    //! [THEN] The new hit element context with new note will be set
    EXPECT_CALL(*m_interaction, setHitElementContext(newContext))
    .Times(1);

    ON_CALL(*m_interaction, hitElementContext())
    .WillByDefault(ReturnRef(newContext));

    //! [GIVEN] There is a range selection
    ON_CALL(*m_selection, isRange())
    .WillByDefault(Return(true));

    ON_CALL(*m_selection, elements())
    .WillByDefault(ReturnRef(selectedElements));

    //! [GIVEN] No note enter mode, no playing
    EXPECT_CALL(m_view, isNoteEnterMode())
    .WillOnce(Return(false));

    ON_CALL(*m_playbackController, isPlaying())
    .WillByDefault(Return(false));

    //! [THEN] seek to the element before playing it
    {
        InSequence seq;

        //! [THEN] We will seek and play selected note, but no select again
        EXPECT_CALL(*m_playbackController, seekElement(newContext.element, FLUSH_SOUND))
        .Times(1);

        std::vector<const EngravingItem*> elements = { newContext.element };
        EXPECT_CALL(*m_playbackController, playElements(elements, m_playParams, false))
        .Times(1);
    }

    std::vector<EngravingItem*> selectElements = { newContext.element };
    EXPECT_CALL(*m_interaction, select(selectElements, _, _))
    .Times(0);

    //! [WHEN] User pressed left mouse button
    m_controller->mousePressEvent(make_mousePressEvent(Qt::LeftButton, Qt::NoModifier, QPointF(100, 100)));
}

/**
 * @brief Mouse_Press_On_Selected_Text_Element
 * @details User pressed left mouse button on already selected text element
 *          We should change text cursor position
 */
TEST_F(NotationViewInputControllerTests, Mouse_Press_On_Selected_Text_Element)
{
    //! [GIVEN] There is a test score
    engraving::MasterScore* score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);

    //! [GIVEN] Previous selected dynamic
    INotationInteraction::HitElementContext oldContext = hitContext(score, { ElementType::DYNAMIC });

    //! [GIVEN] User selected new dynamic that was already selected
    INotationInteraction::HitElementContext newContext = oldContext;
    newContext.element->setSelected(true);

    std::vector<EngravingItem*> selectedElements {
        newContext.element
    };

    EXPECT_CALL(*m_interaction, hitElement(_, _))
    .WillOnce(Return(newContext.element));

    EXPECT_CALL(*m_interaction, hitStaff(_))
    .WillOnce(Return(newContext.element->staff()));

    //! [GIVEN] The new hit element context with new dynamic will be set
    EXPECT_CALL(*m_interaction, setHitElementContext(newContext))
    .Times(1);

    //! [GIVEN] There isn't a range selection
    ON_CALL(*m_selection, isRange())
    .WillByDefault(Return(false));

    ON_CALL(*m_selection, elements())
    .WillByDefault(ReturnRef(selectedElements));

    //! [GIVEN] No note enter mode, no playing
    EXPECT_CALL(m_view, isNoteEnterMode())
    .WillOnce(Return(false));

    ON_CALL(*m_playbackController, isPlaying())
    .WillByDefault(Return(false));

    //! [THEN] We will seek and play selected dynamic, but no select again
    EXPECT_CALL(*m_playbackController, seekElement(newContext.element, FLUSH_SOUND))
    .Times(1);

    std::vector<const EngravingItem*> elements = { newContext.element };
    EXPECT_CALL(*m_playbackController, playElements(elements, m_playParams, false))
    .Times(0);

    std::vector<EngravingItem*> selectElements = { newContext.element };
    EXPECT_CALL(*m_interaction, select(selectElements, _, _))
    .Times(0);

    //! [GIVEN] Hit element is text element
    EXPECT_CALL(*m_interaction, isTextSelected())
    .WillOnce(Return(true));
    EXPECT_CALL(*m_interaction, isTextEditingStarted())
    .WillOnce(Return(true));

    //! [THEN] We will change text cursor position
    EXPECT_CALL(*m_interaction, changeTextCursorPosition(_))
    .Times(1);

    //! [WHEN] User pressed left mouse button
    m_controller->mousePressEvent(make_mousePressEvent(Qt::LeftButton, Qt::NoModifier, QPointF(100, 100)));
}

/**
 * @brief Mouse_Press_On_Selected_Non_Text_Element
 * @details User pressed left mouse button on already selected non text element
 *          We shouldn't change text cursor position
 */
TEST_F(NotationViewInputControllerTests, Mouse_Press_On_Selected_Non_Text_Element)
{
    //! [GIVEN] There is a test score
    engraving::MasterScore* score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);

    //! [GIVEN] Previous selected hairpin
    INotationInteraction::HitElementContext oldContext = hitContext(score, { ElementType::HAIRPIN });

    //! [GIVEN] User selected new hairpin that was already selected
    INotationInteraction::HitElementContext newContext = oldContext;
    newContext.element->setSelected(true);

    std::vector<EngravingItem*> selectedElements {
        newContext.element
    };

    EXPECT_CALL(*m_interaction, hitElement(_, _))
    .WillOnce(Return(newContext.element));

    EXPECT_CALL(*m_interaction, hitStaff(_))
    .WillOnce(Return(newContext.element->staff()));

    //! [GIVEN] The new hit element context with new hairpin will be set
    EXPECT_CALL(*m_interaction, setHitElementContext(newContext))
    .Times(1);

    //! [GIVEN] There isn't a range selection
    ON_CALL(*m_selection, isRange())
    .WillByDefault(Return(false));

    ON_CALL(*m_selection, elements())
    .WillByDefault(ReturnRef(selectedElements));

    //! [GIVEN] No note enter mode, no playing
    EXPECT_CALL(m_view, isNoteEnterMode())
    .WillOnce(Return(false));

    ON_CALL(*m_playbackController, isPlaying())
    .WillByDefault(Return(false));

    //! [THEN] We will seek and play selected hairpin, but no select again
    EXPECT_CALL(*m_playbackController, seekElement(newContext.element, FLUSH_SOUND))
    .Times(1);

    std::vector<const EngravingItem*> elements = { newContext.element };
    EXPECT_CALL(*m_playbackController, playElements(elements, m_playParams, false))
    .Times(0);

    std::vector<EngravingItem*> selectElements = { newContext.element };
    EXPECT_CALL(*m_interaction, select(selectElements, _, _))
    .Times(0);

    //! [GIVEN] Hit element isn't text element
    EXPECT_CALL(*m_interaction, isTextSelected())
    .WillOnce(Return(false));

    //! [THEN] No cursor changing, no editing grips and no starting edit element because needStartEditingAfterSelecting is false
    EXPECT_CALL(*m_interaction, changeTextCursorPosition(_))
    .Times(0);
    EXPECT_CALL(*m_interaction, startEditGrip(newContext.element, _))
    .Times(0);
    EXPECT_CALL(*m_interaction, startEditElement(newContext.element))
    .Times(0);

    //! [WHEN] User pressed left mouse button
    m_controller->mousePressEvent(make_mousePressEvent(Qt::LeftButton, Qt::NoModifier, QPointF(100, 100)));
}

/**
 * @brief Mouse_Press_Range_Start_Play_From_First_Playable_Element
 * @details User selected a range in a note that is located after the previous selected note
 *          The new note should be selected and played, but no seeked
 *          The first note from a range should be seeked
 */
TEST_F(NotationViewInputControllerTests, Mouse_Press_Range_Start_Play_From_First_Playable_Element)
{
    //! [GIVEN] There is a test score
    engraving::MasterScore* score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);

    //! [GIVEN] Previous selected note
    INotationInteraction::HitElementContext oldContext = hitContext(score, { ElementType::NOTE, true /*first note*/ });

    //! [GIVEN] User selected new note that is located after the previous selected note
    INotationInteraction::HitElementContext newContext = hitContext(score, { ElementType::NOTE, false /*last note*/ });

    EXPECT_CALL(*m_interaction, hitElement(_, _))
    .WillOnce(Return(newContext.element));

    EXPECT_CALL(*m_interaction, hitStaff(_))
    .WillOnce(Return(newContext.element->staff()));

    //! [THEN] The new hit element context with new note will be set
    EXPECT_CALL(*m_interaction, setHitElementContext(newContext))
    .Times(1);

    ON_CALL(*m_interaction, hitElementContext())
    .WillByDefault(ReturnRef(newContext));

    //! [GIVEN] No note enter mode, no playing
    EXPECT_CALL(m_view, isNoteEnterMode())
    .WillOnce(Return(false));

    ON_CALL(*m_playbackController, isPlaying())
    .WillByDefault(Return(false));

    //! [THEN] We will select and play selected note, but no seek
    std::vector<EngravingItem*> selectElements = { newContext.element };
    EXPECT_CALL(*m_interaction, select(selectElements, _, _))
    .Times(1)
    .WillOnce([newContext] { newContext.element->setSelected(true); });

    //! [THEN] seek to the element before playing it
    {
        InSequence seq;

        //! [THEN] We will seek first note in the range
        EXPECT_CALL(*m_playbackController, seekElement(oldContext.element, FLUSH_SOUND))
        .Times(1);

        std::vector<const EngravingItem*> playElements = { newContext.element };
        EXPECT_CALL(*m_playbackController, playElements(playElements, m_playParams, false))
        .Times(1);
    }

    EXPECT_CALL(*m_playbackController, seekElement(newContext.element, FLUSH_SOUND))
    .Times(0);

    //! [GIVEN] There is a range selection with two notes
    ON_CALL(*m_selection, isRange())
    .WillByDefault(Return(true));

    selectElements.push_back(oldContext.element);
    EXPECT_CALL(*m_selection, elements())
    .WillOnce(ReturnRef(selectElements));

    //! [WHEN] User pressed left mouse button with ShiftModifier on the new note
    m_controller->mousePressEvent(make_mousePressEvent(Qt::LeftButton, Qt::ShiftModifier, QPointF(100, 100)));
}

/**
 * @brief Mouse_Press_On_Selected_Selected_Range
 * @details User pressed left mouse button on already selected range
 *           The selection shouldn't be changed, but the first measure in the range should be seeked
 */
TEST_F(NotationViewInputControllerTests, Mouse_Press_On_Already_Selected_Range)
{
    //! [GIVEN] There is a test score
    engraving::MasterScore* score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);

    //! [GIVEN] User selects a measure
    INotationInteraction::HitElementContext context = hitMeasureContext(score, 0 /*first measure*/);

    EXPECT_CALL(*m_interaction, hitElement(_, _))
    .WillOnce(Return(context.element));

    EXPECT_CALL(*m_interaction, hitStaff(_))
    .WillOnce(Return(context.staff));

    //! [THEN] The new hit element context with new measure will be set
    EXPECT_CALL(*m_interaction, setHitElementContext(context))
    .Times(1);

    ON_CALL(*m_interaction, hitElementContext())
    .WillByDefault(ReturnRef(context));

    //! [GIVEN] There is a range selection
    ON_CALL(*m_selection, isRange())
    .WillByDefault(Return(true));
    ON_CALL(*m_selectionRange, containsItem(context.element, _))
    .WillByDefault(Return(true));

    std::vector<EngravingItem*> selectElements = { context.element };
    EXPECT_CALL(*m_selection, elements())
    .WillOnce(ReturnRef(selectElements));

    //! [THEN] We should seek measure from the range
    EXPECT_CALL(*m_playbackController, seekElement(context.element, FLUSH_SOUND))
    .Times(1);

    //! [THEN] No selection change
    EXPECT_CALL(*m_interaction, select(_, _, _))
    .Times(0);

    //! [WHEN] User pressed left mouse button
    m_controller->mousePressEvent(make_mousePressEvent(Qt::LeftButton, Qt::NoModifier, QPointF(100, 100)));
}

/**
 * @brief Mouse_Press_Shift_On_Selected_Selected_Range
 * @details User pressed left mouse button with Shift on already selected range
 *          This should result in a call to `select`, to extend/diminish the selection
 */
TEST_F(NotationViewInputControllerTests, Mouse_Press_Shift_On_Already_Selected_Range)
{
    //! [GIVEN] There is a test score
    engraving::MasterScore* score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);

    //! [GIVEN] User selects a measure
    INotationInteraction::HitElementContext context = hitMeasureContext(score, 0 /*first measure*/);

    EXPECT_CALL(*m_interaction, hitElement(_, _))
    .WillOnce(Return(context.element));

    EXPECT_CALL(*m_interaction, hitStaff(_))
    .WillOnce(Return(context.staff));

    //! [THEN] The new hit element context with new measure will be set
    EXPECT_CALL(*m_interaction, setHitElementContext(context))
    .Times(1);

    ON_CALL(*m_interaction, hitElementContext())
    .WillByDefault(ReturnRef(context));

    //! [GIVEN] There is a range selection
    ON_CALL(*m_selection, isRange())
    .WillByDefault(Return(true));
    ON_CALL(*m_selectionRange, containsItem(context.element, _))
    .WillByDefault(Return(true));

    //! [THEN] We should seek measure from the range
    EXPECT_CALL(*m_playbackController, seekElement(context.element, FLUSH_SOUND))
    .Times(1);

    //! [THEN] Selection is extended/diminished
    std::vector<EngravingItem*> selectElements = { context.element };
    EXPECT_CALL(*m_interaction, select(selectElements, _, _))
    .Times(1);

    EXPECT_CALL(*m_selection, elements())
    .WillOnce(ReturnRef(selectElements));

    //! [WHEN] User pressed left mouse button
    m_controller->mousePressEvent(make_mousePressEvent(Qt::LeftButton, Qt::ShiftModifier, QPointF(100, 100)));
}

/**
 * @brief Mouse_Press_On_Already_Selected_Element
 * @details User pressed on already selected note
 *          The selected note should not be selected again, but should be played and seeked
 */
TEST_F(NotationViewInputControllerTests, Mouse_Press_On_Already_Selected_Element)
{
    //! [GIVEN] There is a test score
    engraving::MasterScore* score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);

    //! [GIVEN] Previous selected note
    INotationInteraction::HitElementContext oldContext = hitContext(score, { ElementType::NOTE });
    oldContext.element->setSelected(true);

    //! [GIVEN] User pressed on the previous selected note
    INotationInteraction::HitElementContext newContext = oldContext;

    EXPECT_CALL(*m_interaction, hitElement(_, _))
    .WillOnce(Return(newContext.element));

    EXPECT_CALL(*m_interaction, hitStaff(_))
    .WillOnce(Return(newContext.element->staff()));

    EXPECT_CALL(*m_interaction, setHitElementContext(newContext))
    .Times(1);

    std::vector<EngravingItem*> selectedElements = { oldContext.element };
    ON_CALL(*m_selection, elements())
    .WillByDefault(ReturnRef(selectedElements));

    //! [GIVEN] No note enter mode, no playing
    EXPECT_CALL(m_view, isNoteEnterMode())
    .WillOnce(Return(false));

    ON_CALL(*m_playbackController, isPlaying())
    .WillByDefault(Return(false));

    //! [THEN] We will no select already selected note, but play and seek
    EXPECT_CALL(*m_interaction, select(_, _, _))
    .Times(0);

    //! [THEN] seek to the element before playing it
    {
        InSequence seq;

        EXPECT_CALL(*m_playbackController, seekElement(newContext.element, FLUSH_SOUND))
        .Times(1);

        std::vector<const EngravingItem*> playElements = { newContext.element };
        EXPECT_CALL(*m_playbackController, playElements(playElements, m_playParams, false))
        .Times(1);
    }

    //! [GIVEN] There is no a range selection
    ON_CALL(*m_selection, isRange())
    .WillByDefault(Return(false));

    //! [WHEN] User pressed left mouse button with NoModifier on the new note
    m_controller->mousePressEvent(make_mousePressEvent(Qt::LeftButton, Qt::NoModifier, QPointF(100, 100)));
}

/**
 * @brief Mouse_Press_On_Range
 * @details User pressed selected new measure with ShiftModifier
 *          The new measure should be selected, shouldn't be played, previous measure should be seeked
 */
TEST_F(NotationViewInputControllerTests, Mouse_Press_On_Range)
{
    //! [GIVEN] There is a test score
    engraving::MasterScore* score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);

    //! [GIVEN] Previous selected measure
    INotationInteraction::HitElementContext oldContext = hitMeasureContext(score, 0 /*first measure*/);

    //! [GIVEN] User selected new measure that is located after the previous selected measure
    INotationInteraction::HitElementContext newContext = hitMeasureContext(score, 1 /*second measure*/);
    EXPECT_CALL(*m_interaction, hitElement(_, _))
    .WillOnce(Return(newContext.element));

    EXPECT_CALL(*m_interaction, hitStaff(_))
    .WillOnce(Return(newContext.staff));

    //! [THEN] The new hit element context with new measure will be set
    EXPECT_CALL(*m_interaction, setHitElementContext(newContext))
    .Times(1);

    ON_CALL(*m_interaction, hitElementContext())
    .WillByDefault(ReturnRef(newContext));

    //! [GIVEN] No note enter mode, no playing
    EXPECT_CALL(m_view, isNoteEnterMode())
    .WillOnce(Return(false));

    ON_CALL(*m_playbackController, isPlaying())
    .WillByDefault(Return(false));

    //! [THEN] We will select new measure
    std::vector<EngravingItem*> selectElements = { newContext.element };
    EXPECT_CALL(*m_interaction, select(selectElements, _, _))
    .Times(1)
    .WillOnce([newContext] { newContext.element->setSelected(true); });

    //! [GIVEN] There is a range selection with two measures
    ON_CALL(*m_selection, isRange())
    .WillByDefault(Return(true));

    //! [THEN] No play measure
    std::vector<const EngravingItem*> playElements = { newContext.element };
    EXPECT_CALL(*m_playbackController, playElements(playElements, m_playParams, false))
    .Times(0);

    //! [THEN] Because it's a range selection, we should start playing from first measure in the range
    EXPECT_CALL(*m_playbackController, seekElement(oldContext.element, FLUSH_SOUND))
    .Times(1);

    selectElements.push_back(oldContext.element);
    EXPECT_CALL(*m_selection, elements())
    .WillOnce(ReturnRef(selectElements));

    //! [WHEN] User pressed left mouse button with ShiftModifier on the new note
    m_controller->mousePressEvent(make_mousePressEvent(Qt::LeftButton, Qt::ShiftModifier, QPointF(100, 100)));
}

/**
 * @brief Mouse_Press_On_Range_Context_Menu
 * @details The user pressed the right button on the selected measure
 *          The selection shouldn't be changed, context menu should be opened for selected measure
 */
TEST_F(NotationViewInputControllerTests, Mouse_Press_On_Range_Context_Menu)
{
    //! [GIVEN] There is a test score
    engraving::MasterScore* score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);

    //! [GIVEN] User selected a measure
    INotationInteraction::HitElementContext selectMeasureContext = hitMeasureContext(score, 0 /*first measure*/);

    //! [GIVEN] User pressed the right button on the selected measure, same context
    INotationInteraction::HitElementContext contextMenuOnMeasureContext = selectMeasureContext;

    EXPECT_CALL(*m_interaction, hitElement(_, _))
    .WillOnce(Return(selectMeasureContext.element))
    //! right button click
    .WillOnce(Return(contextMenuOnMeasureContext.element));

    EXPECT_CALL(*m_interaction, hitStaff(_))
    .WillOnce(Return(selectMeasureContext.staff))
    //! right button click
    .WillOnce(Return(contextMenuOnMeasureContext.staff));

    //! [THEN] The new hit element context with new measure will be set
    EXPECT_CALL(*m_interaction, setHitElementContext(selectMeasureContext))
    .WillOnce(Return())
    //! right button click
    .WillOnce(Return());

    //! [GIVEN] No note enter mode, no playing
    EXPECT_CALL(m_view, isNoteEnterMode())
    .WillRepeatedly(Return(false));

    EXPECT_CALL(*m_playbackController, isPlaying())
    .WillRepeatedly(Return(false));

    //! [THEN] We will select new measure only one time
    std::vector<EngravingItem*> selectElements = { selectMeasureContext.element };
    EXPECT_CALL(*m_interaction, select(selectElements, _, _))
    .Times(1);

    EXPECT_CALL(*m_selection, isRange())
    .WillRepeatedly(Return(true));

    //! [GIVEN] Element is in selected range at the moment that it is selected for the second time
    EXPECT_CALL(*m_selectionRange, containsItem(contextMenuOnMeasureContext.element, _))
    .WillOnce(Return(true));

    std::vector<const EngravingItem*> playElements = { selectMeasureContext.element };
    EXPECT_CALL(*m_playbackController, playElements(playElements, m_playParams, false))
    .Times(0);

    EXPECT_CALL(*m_playbackController, seekElement(selectMeasureContext.element, FLUSH_SOUND))
    .WillOnce(Return())
    //! right button click
    .WillOnce(Return());

    //! [THEN] The selection shouldn't be changed
    EXPECT_CALL(*m_selection, elements())
    .WillRepeatedly(ReturnRef(selectElements));

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    //! Note: the context menu itself is shown by AbstractNotationPaintView::event
#else
    //! [THEN] Show context menu for measure
    EXPECT_CALL(m_view, showContextMenu(contextMenuOnMeasureContext.element->type(), _))
    .Times(1);
#endif

    //! [WHEN] User pressed left mouse button with ShiftModifier on the new measure
    m_controller->mousePressEvent(make_mousePressEvent(Qt::LeftButton, Qt::ShiftModifier, QPointF(100, 100)));

    //! [WHEN] User pressed right mouse button with NoModifier on the selected measure
    m_controller->mousePressEvent(make_mousePressEvent(Qt::RightButton, Qt::NoModifier, QPointF(100, 100)));
}

/**
 * @brief Mouse_Press_On_Range_Context_Menu_New_Selection
 * @details The user pressed the right button on the new measure
 *          The selection should be changed, context menu should be opened for new measure
 */
TEST_F(NotationViewInputControllerTests, Mouse_Press_On_Range_Context_Menu_New_Selection)
{
    //! [GIVEN] There is a test score
    engraving::MasterScore* score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);

    //! [GIVEN] User selected a measure
    INotationInteraction::HitElementContext selectMeasureContext = hitMeasureContext(score, 0 /*first measure*/);

    //! [GIVEN] User pressed the right button on the selected measure, same context
    INotationInteraction::HitElementContext contextMenuOnMeasureContext = hitMeasureContext(score, 1 /*second measure*/);

    EXPECT_CALL(*m_interaction, hitElement(_, _))
    .WillOnce(Return(selectMeasureContext.element))
    //! right button click
    .WillOnce(Return(contextMenuOnMeasureContext.element));

    EXPECT_CALL(*m_interaction, hitStaff(_))
    .WillOnce(Return(selectMeasureContext.staff))
    //! right button click
    .WillOnce(Return(contextMenuOnMeasureContext.staff));

    EXPECT_CALL(*m_interaction, setHitElementContext(selectMeasureContext))
    .WillOnce(Return());
    EXPECT_CALL(*m_interaction, setHitElementContext(contextMenuOnMeasureContext))
    .WillOnce(Return());

#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
    .WillOnce(ReturnRef(contextMenuOnMeasureContext)) // for context menu
#endif
    ;

    //! [GIVEN] No note enter mode, no playing
    EXPECT_CALL(m_view, isNoteEnterMode())
    .WillRepeatedly(Return(false));

    EXPECT_CALL(*m_playbackController, isPlaying())
    .WillRepeatedly(Return(false));

    EXPECT_CALL(*m_selection, isRange())
    .WillRepeatedly(Return(true));

    //! [THEN] The selection should be changed
    std::vector<EngravingItem*> selectElements = { selectMeasureContext.element };
    EXPECT_CALL(*m_interaction, select(selectElements, _, _))
    .Times(1);

    std::vector<EngravingItem*> contextMenuSelectElements = { contextMenuOnMeasureContext.element };
    EXPECT_CALL(*m_interaction, select(contextMenuSelectElements, _, _))
    .WillOnce(Return());

    EXPECT_CALL(*m_selection, elements())
    .WillOnce(ReturnRef(selectElements))
    .WillOnce(ReturnRef(contextMenuSelectElements));

    //! [GIVEN] New element is not in selected range at the moment that it is selected
    EXPECT_CALL(*m_selectionRange, containsItem(contextMenuOnMeasureContext.element, _))
    .WillOnce(Return(false));

    EXPECT_CALL(*m_playbackController, playElements(_, m_playParams, false))
    .Times(0);

    //! [THEN] We will seek each measures
    EXPECT_CALL(*m_playbackController, seekElement(selectMeasureContext.element, FLUSH_SOUND))
    .WillOnce(Return());

    EXPECT_CALL(*m_playbackController, seekElement(contextMenuOnMeasureContext.element, FLUSH_SOUND))
    .WillOnce(Return());

#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
    //! Note: the context menu itself is shown by AbstractNotationPaintView::event
#else
    //! [THEN] Show context menu for new measure
    EXPECT_CALL(m_view, showContextMenu(contextMenuOnMeasureContext.element->type(), _))
    .Times(1);
#endif

    //! [WHEN] User pressed left mouse button with ShiftModifier on the new measure
    m_controller->mousePressEvent(make_mousePressEvent(Qt::LeftButton, Qt::ShiftModifier, QPointF(100, 100)));

    //! [WHEN] User pressed right mouse button with NoModifier on the selected measure
    m_controller->mousePressEvent(make_mousePressEvent(Qt::RightButton, Qt::NoModifier, QPointF(100, 100)));
}

class NotationViewInputControllerTextInputTests : public NotationViewInputControllerTests
{
public:
    void SetUp() override
    {
        NotationViewInputControllerTests::SetUp();

        m_dispatcher = std::make_shared<NiceMock<muse::actions::ActionsDispatcherMock> >();
        m_controller->dispatcher.set(m_dispatcher);

        m_score = engraving::ScoreRW::readScore(TEST_SCORE_PATH);
        ON_CALL(*m_interaction, isTextEditingStarted()).WillByDefault(Return(true));
    }

    void TearDown() override
    {
        delete m_text;
        delete m_score;
        NotationViewInputControllerTests::TearDown();
    }

    //! A syllable on the first note of the test score, being edited
    void editLyrics(const String& text)
    {
        engraving::Segment* segment = m_score->firstMeasure()->first(engraving::SegmentType::ChordRest);
        engraving::ChordRest* chordRest = engraving::toChordRest(segment->element(0));
        setEditedText(engraving::Factory::createLyrics(chordRest), text);
    }

    //! Staff text on the first note of the test score, being edited
    void editStaffText(const String& text)
    {
        engraving::Segment* segment = m_score->firstMeasure()->first(engraving::SegmentType::ChordRest);
        setEditedText(engraving::Factory::createStaffText(segment), text);
    }

    void setEditedText(engraving::TextBase* text, const String& content)
    {
        m_text = text;
        m_text->setPlainText(content);
        m_score->renderer()->layoutText1(m_text);
        m_text->cursor()->moveCursorToEnd();

        ON_CALL(*m_interaction, editedText()).WillByDefault(Return(m_text));
    }

    //! The software keyboard sends text
    void commit(const QString& text)
    {
        QInputMethodEvent event;
        event.setCommitString(text);
        m_controller->inputMethodEvent(&event);
    }

    std::shared_ptr<muse::actions::ActionsDispatcherMock> m_dispatcher;
    engraving::MasterScore* m_score = nullptr;
    engraving::TextBase* m_text = nullptr;
};

/**
 * @brief Space_From_Software_Keyboard_In_Lyrics_Goes_To_Next_Word
 * @details Software keyboards send a space as text; in lyrics, it moves on as the Space key does
 */
TEST_F(NotationViewInputControllerTextInputTests, Space_From_Software_Keyboard_In_Lyrics_Goes_To_Next_Word)
{
    //! [GIVEN] A syllable is edited
    editLyrics(u"hel");

    //! [THEN] The space is not put into the syllable: the next syllable is edited
    EXPECT_CALL(*m_interaction, editText(_)).Times(0);
    EXPECT_CALL(*m_dispatcher, dispatch(ActionCode("next-word"))).Times(1);

    //! [WHEN] The keyboard sends a space
    commit(" ");
}

/**
 * @brief Space_Shown_Before_It_Is_Sent_Is_Removed_From_The_Syllable
 * @details Qt for Android shows a typed character as composed text first, then sends it: the composed space must
 * not stay in the syllable
 */
TEST_F(NotationViewInputControllerTextInputTests, Space_Shown_Before_It_Is_Sent_Is_Removed_From_The_Syllable)
{
    //! [GIVEN] A syllable is edited, and the keyboard shows a space being composed
    editLyrics(u"hel");

    QInputMethodEvent composing(QString(" "), {});
    m_controller->inputMethodEvent(&composing);

    //! [THEN] The composed space is removed (an empty composition), then the next syllable is edited
    {
        InSequence seq;
        EXPECT_CALL(*m_interaction, editText(_)).WillOnce([](QInputMethodEvent* event) {
            EXPECT_TRUE(event->commitString().isEmpty());
            EXPECT_TRUE(event->preeditString().isEmpty());
        });
        EXPECT_CALL(*m_dispatcher, dispatch(ActionCode("next-word"))).Times(1);
    }

    //! [WHEN] The keyboard sends the space
    commit(" ");
}

/**
 * @brief Hyphen_After_Text_In_Lyrics_Ends_The_Syllable
 * @details When a keyboard sends a hyphen together with the rest of the syllable, the text is put in first
 */
TEST_F(NotationViewInputControllerTextInputTests, Hyphen_After_Text_In_Lyrics_Ends_The_Syllable)
{
    //! [GIVEN] A syllable is edited
    editLyrics(u"hel");

    //! [THEN] "lo" is put into the syllable, then the next syllable is edited (with a hyphen between them)
    {
        InSequence seq;
        EXPECT_CALL(*m_interaction, editText(_)).WillOnce([](QInputMethodEvent* event) {
            EXPECT_EQ(event->commitString(), QString("lo"));
        });
        EXPECT_CALL(*m_dispatcher, dispatch(ActionCode("next-syllable"))).Times(1);
    }

    //! [WHEN] The keyboard sends "lo-"
    commit("lo-");
}

/**
 * @brief Space_In_Staff_Text_Is_Text
 * @details Other text keeps the spaces typed on a software keyboard
 */
TEST_F(NotationViewInputControllerTextInputTests, Space_In_Staff_Text_Is_Text)
{
    //! [GIVEN] Staff text is edited
    editStaffText(u"dolce");

    //! [THEN] The space is put into the text, and nothing else happens
    EXPECT_CALL(*m_interaction, editText(_)).WillOnce([](QInputMethodEvent* event) {
        EXPECT_EQ(event->commitString(), QString(" "));
    });
    EXPECT_CALL(*m_dispatcher, dispatch(::testing::An<const ActionCode&>())).Times(0);

    //! [WHEN] The keyboard sends a space
    commit(" ");
}

/**
 * @brief Input_Method_Sees_The_Text_Around_The_Cursor
 * @details Software keyboards need the text before the cursor for suggestions, corrections and deleting
 */
TEST_F(NotationViewInputControllerTextInputTests, Input_Method_Sees_The_Text_Around_The_Cursor)
{
    //! [GIVEN] Staff text is edited, with the cursor after "dol"
    editStaffText(u"dolce");
    m_text->cursor()->setColumn(3);
    m_text->cursor()->setSelectColumn(3);

    //! [THEN] The input method gets the text of the line and the cursor position in it
    EXPECT_TRUE(m_controller->canHandleInputMethodQuery(Qt::ImSurroundingText));
    EXPECT_EQ(m_controller->inputMethodQuery(Qt::ImSurroundingText).toString(), QString("dolce"));
    EXPECT_EQ(m_controller->inputMethodQuery(Qt::ImCursorPosition).toInt(), 3);
    EXPECT_EQ(m_controller->inputMethodQuery(Qt::ImAnchorPosition).toInt(), 3);
}

/**
 * @brief Input_Method_Does_Not_See_The_Text_It_Composes
 * @details The text being composed (preedit) is shown in the text, but input methods see it as separate from it
 */
TEST_F(NotationViewInputControllerTextInputTests, Input_Method_Does_Not_See_The_Text_It_Composes)
{
    //! [GIVEN] Staff text is edited, and shows "wal" being composed at its end
    editStaffText(u"dolce wal");

    QInputMethodEvent event(QString("wal"), {});
    m_controller->inputMethodEvent(&event);

    //! [THEN] The input method sees the text and the cursor without "wal"
    EXPECT_EQ(m_controller->inputMethodQuery(Qt::ImSurroundingText).toString(), QString("dolce "));
    EXPECT_EQ(m_controller->inputMethodQuery(Qt::ImCursorPosition).toInt(), 6);
    EXPECT_EQ(m_controller->inputMethodQuery(Qt::ImAnchorPosition).toInt(), 6);

    //! [WHEN] The composition is committed
    commit("wal");

    //! [THEN] All of the text is text
    EXPECT_EQ(m_controller->inputMethodQuery(Qt::ImSurroundingText).toString(), QString("dolce wal"));
    EXPECT_EQ(m_controller->inputMethodQuery(Qt::ImCursorPosition).toInt(), 9);
}

/**
 * @brief Musical_Text_Gets_No_Corrections
 * @details Keyboards should not correct or capitalize syllables, chord symbols and the like; other text is language
 */
TEST_F(NotationViewInputControllerTextInputTests, Musical_Text_Gets_No_Corrections)
{
    editLyrics(u"hel");
    EXPECT_EQ(m_controller->inputMethodQuery(Qt::ImHints).toInt(), int(Qt::ImhNoPredictiveText | Qt::ImhNoAutoUppercase));

    delete m_text;
    m_text = nullptr;
    editStaffText(u"dolce");
    EXPECT_EQ(m_controller->inputMethodQuery(Qt::ImHints).toInt(), int(Qt::ImhNone));
}

/**
 * @brief Line_Positions_Count_Surrogate_Pairs
 * @details Text columns count a character outside the BMP as one; input methods count UTF-16 units
 */
TEST_F(NotationViewInputControllerTextInputTests, Line_Positions_Count_Surrogate_Pairs)
{
    const QString line = QString("a") + QString::fromUcs4(U"\U0001F3B5") + QString("b");
    EXPECT_EQ(NotationViewInputController::lineColumnToTextPosition(line, 0), 0);
    EXPECT_EQ(NotationViewInputController::lineColumnToTextPosition(line, 1), 1);
    EXPECT_EQ(NotationViewInputController::lineColumnToTextPosition(line, 2), 3);
    EXPECT_EQ(NotationViewInputController::lineColumnToTextPosition(line, 3), 4);
}
