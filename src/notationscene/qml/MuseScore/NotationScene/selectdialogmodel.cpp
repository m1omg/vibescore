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
#include "selectdialogmodel.h"

#include <algorithm>

#include "engraving/dom/measure.h"
#include "engraving/dom/rest.h"
#include "engraving/dom/score.h"
#include "engraving/dom/spanner.h"
#include "engraving/dom/system.h"

using namespace mu::notation;
using namespace mu::engraving;

SelectDialogModel::SelectDialogModel(QObject* parent)
    : QObject(parent), muse::Contextable(muse::iocCtxForQmlObject(this))
{
}

INotationPtr SelectDialogModel::notation() const
{
    return context() ? context()->currentNotation() : nullptr;
}

void SelectDialogModel::load()
{
    const INotationPtr notation = this->notation();
    m_element = notation ? notation->interaction()->contextItem() : nullptr;
    if (!m_element) {
        return;
    }

    // The defaults of the QWidget dialog
    m_sameSubtype = m_element->isHairpinSegment() || m_element->isHarmony();
    m_isSingleSelection = m_element->score()->selection().isSingle();
    m_inSelection = !m_isSingleSelection;

    emit loaded();
    emit changed();
}

QString SelectDialogModel::elementType() const
{
    return m_element ? m_element->translatedTypeUserName().toQString() : QString();
}

QString SelectDialogModel::elementSubtype() const
{
    return m_element ? m_element->translatedSubtypeUserName().toQString() : QString();
}

bool SelectDialogModel::hasSubtype() const
{
    return m_element && m_element->subtype() != -1;
}

bool SelectDialogModel::isRest() const
{
    return m_element && m_element->isRest();
}

bool SelectDialogModel::isSingleSelection() const
{
    return m_isSingleSelection;
}

static const System* elementSystem(const EngravingItem* element)
{
    for (const EngravingItem* item = element; item; item = item->parentItem()) {
        if (item->isSystem()) {
            return toSystem(item);
        }
    }
    return nullptr;
}

FilterElementsOptions SelectDialogModel::elementOptions() const
{
    FilterElementsOptions options;
    options.elementType = m_element->type();
    options.subtype = m_element->subtype();

    if (m_sameStaff) {
        options.staffStart = static_cast<int>(m_element->staffIdx());
        options.staffEnd = static_cast<int>(m_element->staffIdx() + 1);
    } else if (m_inSelection) {
        options.staffStart = static_cast<int>(m_element->score()->selection().staffStart());
        options.staffEnd = static_cast<int>(m_element->score()->selection().staffEnd());
    }

    if (m_sameDuration && m_element->isRest()) {
        options.durationTicks = toRest(m_element)->actualTicks();
    }

    if (m_sameBeat) {
        options.beat = m_element->beat();
    }

    if (m_sameMeasure) {
        const Measure* measure = m_element->findMeasure();
        if (!measure && m_element->isSpannerSegment()) {
            const Spanner* spanner = toSpannerSegment(m_element)->spanner();
            const EngravingItem* start = spanner ? spanner->startElement() : nullptr;
            measure = start ? start->findMeasure() : nullptr;
        }
        options.measure = measure;
    }

    options.voice = m_sameVoice ? static_cast<int>(m_element->voice()) : -1;
    options.bySubtype = m_sameSubtype && hasSubtype();

    if (m_sameSystem) {
        options.system = elementSystem(m_element);
    }

    return options;
}

void SelectDialogModel::apply()
{
    const INotationPtr notation = this->notation();
    if (!notation || !m_element) {
        return;
    }

    const INotationInteractionPtr interaction = notation->interaction();
    std::vector<EngravingItem*> elements = notation->elements()->elements(elementOptions());

    const std::vector<EngravingItem*> selected = interaction->selection()->elements();
    const auto isSelected = [&selected](const EngravingItem* e) {
        return std::find(selected.cbegin(), selected.cend(), e) != selected.cend();
    };

    // "Search in selection": only what is found in the selection, which it replaces
    if (m_inSelection || m_action == SearchInSelection) {
        elements.erase(std::remove_if(elements.begin(), elements.end(), [&isSelected](const EngravingItem* e) {
            return !isSelected(e);
        }), elements.end());
    }

    switch (m_action) {
    case ReplaceSelection:
    case SearchInSelection:
        if (elements.empty()) {
            return;
        }
        interaction->clearSelection();
        interaction->select(elements, SelectType::ADD);
        break;
    case AddToSelection: {
        std::vector<EngravingItem*> all = selected;
        all.insert(all.end(), elements.begin(), elements.end());
        interaction->select(all, SelectType::ADD);
        break;
    }
    case SubtractFromSelection: {
        std::vector<EngravingItem*> remaining = selected;
        remaining.erase(std::remove_if(remaining.begin(), remaining.end(), [&elements](const EngravingItem* e) {
            return std::find(elements.cbegin(), elements.cend(), e) != elements.cend();
        }), remaining.end());
        interaction->clearSelection();
        interaction->select(remaining, SelectType::ADD);
        break;
    }
    }
}
