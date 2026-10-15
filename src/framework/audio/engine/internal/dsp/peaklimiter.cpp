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
#include "peaklimiter.h"

#include <algorithm>
#include <cmath>

using namespace muse::audio;
using namespace muse::audio::dsp;

//! Just below full scale
static constexpr float CEILING = 0.98f;
static constexpr double LOOKAHEAD_SECONDS = 0.0013;
static constexpr double RELEASE_SECONDS = 0.15;

void PeakLimiter::setFormat(sample_rate_t sampleRate, audioch_t channels)
{
    const size_t lookahead = std::max<size_t>(1, static_cast<size_t>(LOOKAHEAD_SECONDS * sampleRate));
    if (lookahead == m_lookahead && channels == m_channels) {
        return;
    }

    m_channels = channels;
    m_lookahead = lookahead;
    // The gain gets 99% of the way down during the look-ahead
    m_attackCoeff = 1.f - std::pow(0.01f, 1.f / static_cast<float>(m_lookahead));
    m_releaseCoeff = 1.f - std::exp(-1.f / static_cast<float>(RELEASE_SECONDS * sampleRate));
    m_pos = 1; // so that reset() clears it
    reset();
}

void PeakLimiter::reset()
{
    if (m_pos == 0) {
        return;
    }

    m_delay.assign(m_lookahead * m_channels, 0.f);
    m_minQueue.assign(m_lookahead + 2, Required());
    m_queueHead = 0;
    m_queueSize = 0;
    m_pos = 0;
    m_gain = 1.f;
}

void PeakLimiter::process(float* buffer, samples_t frames)
{
    if (m_channels == 0 || m_lookahead == 0) {
        return;
    }

    const size_t window = m_lookahead + 1;
    const size_t queueCapacity = m_minQueue.size();

    for (samples_t f = 0; f < frames; ++f) {
        float* frame = buffer + f * m_channels;

        // The gain this frame needs
        float peak = 0.f;
        for (audioch_t c = 0; c < m_channels; ++c) {
            peak = std::max(peak, std::fabs(frame[c]));
        }
        const float required = peak > CEILING ? CEILING / peak : 1.f;

        // The lowest gain that the frames from the one that goes out now to this one need (a running minimum)
        while (m_queueSize > 0 && m_minQueue[m_queueHead].frame + window <= m_pos) {
            m_queueHead = (m_queueHead + 1) % queueCapacity;
            --m_queueSize;
        }
        while (m_queueSize > 0 && m_minQueue[(m_queueHead + m_queueSize - 1) % queueCapacity].gain >= required) {
            --m_queueSize;
        }
        m_minQueue[(m_queueHead + m_queueSize) % queueCapacity] = { m_pos, required };
        ++m_queueSize;
        const float target = m_minQueue[m_queueHead].gain;

        m_gain += (target - m_gain) * (target < m_gain ? m_attackCoeff : m_releaseCoeff);

        // Out goes the frame from the look-ahead ago
        float* delayed = m_delay.data() + static_cast<size_t>(m_pos % m_lookahead) * m_channels;
        for (audioch_t c = 0; c < m_channels; ++c) {
            const float out = delayed[c] * m_gain;
            delayed[c] = frame[c];
            frame[c] = std::clamp(out, -1.f, 1.f);
        }

        ++m_pos;
    }
}
