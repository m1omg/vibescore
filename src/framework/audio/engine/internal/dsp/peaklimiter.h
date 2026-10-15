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

#include <cstdint>
#include <vector>

#include "audio/common/audiotypes.h"

namespace muse::audio::dsp {
//! A look-ahead peak limiter for the master output: below the ceiling it does nothing (it only delays the sound by
//! the look-ahead, ~1.3 ms); a louder peak is turned down smoothly, starting before it comes, instead of clipping
//! (many instruments at full volume add up to several times full scale)
class PeakLimiter
{
public:
    void setFormat(sample_rate_t sampleRate, audioch_t channels);
    void reset();

    //! Interleaved samples, in place
    void process(float* buffer, samples_t frames);

private:
    audioch_t m_channels = 0;
    size_t m_lookahead = 0;          // frames
    float m_attackCoeff = 1.f;
    float m_releaseCoeff = 1.f;

    struct Required {
        uint64_t frame = 0;
        float gain = 1.f;
    };

    std::vector<float> m_delay;       // m_lookahead frames of interleaved samples
    std::vector<Required> m_minQueue; // the running minimum of the gains that the last frames need (monotonic queue)
    size_t m_queueHead = 0;
    size_t m_queueSize = 0;
    uint64_t m_pos = 0;               // frames processed since the reset
    float m_gain = 1.f;
};
}
