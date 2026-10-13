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
#include "oboeaudiodriver.h"

#include <algorithm>
#include <cstring>

#include <oboe/Oboe.h>

#include "translation.h"
#include "log.h"

using namespace muse;
using namespace muse::audio;

//! Log the underrun counter roughly every 10 s of audio at 48 kHz / 512 frames
static constexpr int64_t XRUN_LOG_INTERVAL_CALLBACKS = 1000;

struct OboeAudioDriver::Stream : public oboe::AudioStreamDataCallback, public oboe::AudioStreamErrorCallback
{
    explicit Stream(OboeAudioDriver* driver)
        : driver(driver) {}

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream* stream, void* audioData, int32_t numFrames) override
    {
        const int len = numFrames * stream->getChannelCount() * static_cast<int>(sizeof(float));

        if (callback) {
            callback(static_cast<uint8_t*>(audioData), len);
        } else {
            std::memset(audioData, 0, static_cast<size_t>(len));
        }

        if (++callbackCount % XRUN_LOG_INTERVAL_CALLBACKS == 0) {
            oboe::ResultWithValue<int32_t> xruns = stream->getXRunCount();
            if (xruns && xruns.value() != lastXRunCount) {
                LOGW() << "Audio underruns: " << xruns.value() << " (+" << (xruns.value() - lastXRunCount) << ")";
                lastXRunCount = xruns.value();
            }
        }

        return oboe::DataCallbackResult::Continue;
    }

    // Called on Oboe's error thread after it has already closed the stream.
    void onErrorAfterClose(oboe::AudioStream*, oboe::Result error) override
    {
        LOGW() << "Oboe stream closed: " << oboe::convertToText(error);
        driver->onStreamDisconnected();
    }

    OboeAudioDriver* driver = nullptr;
    std::shared_ptr<oboe::AudioStream> stream;
    Callback callback;
    int32_t lastXRunCount = 0;
    int64_t callbackCount = 0;
};

OboeAudioDriver::OboeAudioDriver() = default;

OboeAudioDriver::~OboeAudioDriver()
{
    close();
}

void OboeAudioDriver::init()
{
}

std::string OboeAudioDriver::name() const
{
    return "Oboe";
}

AudioDeviceID OboeAudioDriver::defaultDevice() const
{
    return DEFAULT_DEVICE_ID;
}

bool OboeAudioDriver::open(const Spec& spec, Spec* activeSpec)
{
    std::lock_guard lock(m_streamMutex);

    if (m_stream) {
        LOGW() << "Stream is already opened";
        return false;
    }

    auto stream = std::make_unique<Stream>(this);
    stream->callback = spec.callback;

    oboe::AudioStreamBuilder builder;
    builder.setDirection(oboe::Direction::Output)
    ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
    ->setSharingMode(oboe::SharingMode::Exclusive)
    ->setFormat(oboe::AudioFormat::Float)
    ->setFormatConversionAllowed(true)
    ->setChannelCount(static_cast<int32_t>(spec.output.audioChannelCount))
    ->setChannelConversionAllowed(true)
    ->setSampleRate(static_cast<int32_t>(spec.output.sampleRate))
    ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
    ->setFramesPerDataCallback(static_cast<int32_t>(spec.output.samplesPerChannel))
    ->setUsage(oboe::Usage::Media)
    ->setContentType(oboe::ContentType::Music)
    ->setDataCallback(stream.get())
    ->setErrorCallback(stream.get());

    oboe::Result result = builder.openStream(stream->stream);
    if (result != oboe::Result::OK || !stream->stream) {
        LOGE() << "Failed to open Oboe stream: " << oboe::convertToText(result);
        return false;
    }

    oboe::AudioStream* s = stream->stream.get();

    // Two callback blocks of buffering: enough headroom for scheduling hiccups,
    // without the latency of the (often huge) full buffer capacity
    const int32_t burst = std::max<int32_t>(s->getFramesPerBurst(), 1);
    const int32_t block = static_cast<int32_t>(spec.output.samplesPerChannel);
    s->setBufferSizeInFrames(std::min(std::max(2 * block, 2 * burst), s->getBufferCapacityInFrames()));

    result = s->requestStart();
    if (result != oboe::Result::OK) {
        LOGE() << "Failed to start Oboe stream: " << oboe::convertToText(result);
        s->close();
        return false;
    }

    LOGI() << "Opened Oboe stream: api " << (s->getAudioApi() == oboe::AudioApi::AAudio ? "AAudio" : "OpenSL ES")
           << ", sharing " << oboe::convertToText(s->getSharingMode())
           << ", performance " << oboe::convertToText(s->getPerformanceMode())
           << ", sample rate " << s->getSampleRate()
           << ", channels " << s->getChannelCount()
           << ", burst " << burst
           << ", frames per callback " << block
           << ", buffer " << s->getBufferSizeInFrames() << "/" << s->getBufferCapacityInFrames();

    m_stream = std::move(stream);
    m_activeSpec = spec;
    m_opened = true;

    if (activeSpec) {
        *activeSpec = m_activeSpec;
    }

    m_activeSpecChanged.send(m_activeSpec);

    return true;
}

void OboeAudioDriver::close()
{
    m_opened = false;
    closeStream();
}

void OboeAudioDriver::closeStream()
{
    std::unique_ptr<Stream> stream;
    {
        std::lock_guard lock(m_streamMutex);
        stream = std::move(m_stream);
    }

    if (!stream) {
        return;
    }

    // Stopping/closing an already closed (disconnected) stream is harmless
    stream->stream->requestStop();
    stream->stream->close();
}

void OboeAudioDriver::onStreamDisconnected()
{
    // The output route changed (headphones, Bluetooth, USB) or the audio server restarted:
    // the controller will close() and open() the driver again
    if (m_opened) {
        m_availableOutputDevicesChanged.notify();
    }
}

bool OboeAudioDriver::isOpened() const
{
    return m_opened;
}

const OboeAudioDriver::Spec& OboeAudioDriver::activeSpec() const
{
    return m_activeSpec;
}

async::Channel<OboeAudioDriver::Spec> OboeAudioDriver::activeSpecChanged() const
{
    return m_activeSpecChanged;
}

std::vector<samples_t> OboeAudioDriver::availableOutputDeviceBufferSizes() const
{
    std::vector<samples_t> result;

    for (samples_t n = MAXIMUM_BUFFER_SIZE; n >= MINIMUM_BUFFER_SIZE; n /= 2) {
        result.push_back(n);
    }

    std::sort(result.begin(), result.end());

    return result;
}

std::vector<sample_rate_t> OboeAudioDriver::availableOutputDeviceSampleRates() const
{
    // Oboe resamples when the device runs at a different rate
    return {
        44100,
        48000,
        88200,
        96000,
    };
}

AudioDeviceList OboeAudioDriver::availableOutputDevices() const
{
    // Android routes the default stream to the active output (speaker, headphones, Bluetooth, USB)
    AudioDeviceList devices;
    devices.push_back({ DEFAULT_DEVICE_ID, muse::trc("audio", "System default") });
    return devices;
}

async::Notification OboeAudioDriver::availableOutputDevicesChanged() const
{
    return m_availableOutputDevicesChanged;
}
