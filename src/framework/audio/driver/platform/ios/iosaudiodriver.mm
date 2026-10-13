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
#include "iosaudiodriver.h"

#include <algorithm>
#include <cstring>

#import <AVFoundation/AVFoundation.h>
#include <AudioToolbox/AudioToolbox.h>

#include <QGuiApplication>

#include "translation.h"
#include "log.h"

using namespace muse;
using namespace muse::audio;

struct IosAudioDriver::Data {
    AudioComponentInstance audioUnit = nullptr;
    Callback callback;
    UInt32 channels = 2;

    id interruptionObserver = nil;
    id routeChangeObserver = nil;
    id mediaServicesResetObserver = nil;
};

static OSStatus renderCallback(void* inRefCon, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32,
                               UInt32 inNumberFrames, AudioBufferList* ioData)
{
    auto* data = static_cast<IosAudioDriver::Data*>(inRefCon);
    if (!ioData || ioData->mNumberBuffers == 0) {
        return noErr;
    }

    AudioBuffer& buffer = ioData->mBuffers[0];
    const int len = static_cast<int>(inNumberFrames * data->channels * sizeof(float));

    if (data->callback && buffer.mDataByteSize >= static_cast<UInt32>(len)) {
        data->callback(static_cast<uint8_t*>(buffer.mData), len);
    } else {
        std::memset(buffer.mData, 0, buffer.mDataByteSize);
    }

    return noErr;
}

static void logError(const char* what, OSStatus status)
{
    LOGE() << what << " failed, status: " << static_cast<int>(status);
}

static void logError(const char* what, NSError* error)
{
    LOGE() << what << " failed: " << (error ? error.localizedDescription.UTF8String : "unknown error");
}

IosAudioDriver::IosAudioDriver()
    : m_data(std::make_unique<Data>())
{
}

IosAudioDriver::~IosAudioDriver()
{
    close();
}

void IosAudioDriver::init()
{
    QObject::connect(qGuiApp, &QGuiApplication::applicationStateChanged, &m_appStateReceiver, [this](Qt::ApplicationState state) {
        setSuspended(state == Qt::ApplicationSuspended);
    });
}

void IosAudioDriver::setSuspended(bool suspended)
{
    if (m_suspended == suspended) {
        return;
    }

    m_suspended = suspended;

    if (!m_opened || !m_data->audioUnit) {
        return;
    }

    if (suspended) {
        AudioOutputUnitStop(m_data->audioUnit);
        LOGI() << "Stopped the audio unit";
    } else {
        restartAfterInterruption();
        LOGI() << "Restarted the audio unit";
    }
}

std::string IosAudioDriver::name() const
{
    return "iOS";
}

AudioDeviceID IosAudioDriver::defaultDevice() const
{
    return DEFAULT_DEVICE_ID;
}

bool IosAudioDriver::open(const Spec& spec, Spec* activeSpec)
{
    if (m_data->audioUnit) {
        LOGW() << "Audio unit is already opened";
        return false;
    }

    @autoreleasepool {
        AVAudioSession* session = [AVAudioSession sharedInstance];
        NSError* error = nil;

        // Playback keeps sound on with the silent switch; mixing keeps other apps' audio playing
        if (![session setCategory:AVAudioSessionCategoryPlayback
                             mode:AVAudioSessionModeDefault
                          options:AVAudioSessionCategoryOptionMixWithOthers
                            error:&error]) {
            logError("AVAudioSession setCategory", error);
        }

        const double requestedRate = static_cast<double>(spec.output.sampleRate);
        if (![session setPreferredSampleRate:requestedRate error:&error]) {
            logError("AVAudioSession setPreferredSampleRate", error);
        }

        const double bufferDuration = static_cast<double>(spec.output.samplesPerChannel) / requestedRate;
        if (![session setPreferredIOBufferDuration:bufferDuration error:&error]) {
            logError("AVAudioSession setPreferredIOBufferDuration", error);
        }

        if (![session setActive:YES error:&error]) {
            logError("AVAudioSession setActive", error);
            return false;
        }

        m_activeSpec = spec;
        m_activeSpec.output.sampleRate = static_cast<sample_rate_t>(session.sampleRate);
    }

    AudioComponentDescription description = {};
    description.componentType = kAudioUnitType_Output;
    description.componentSubType = kAudioUnitSubType_RemoteIO;
    description.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent component = AudioComponentFindNext(nullptr, &description);
    if (!component) {
        LOGE() << "RemoteIO audio unit not found";
        return false;
    }

    OSStatus status = AudioComponentInstanceNew(component, &m_data->audioUnit);
    if (status != noErr) {
        logError("AudioComponentInstanceNew", status);
        m_data->audioUnit = nullptr;
        return false;
    }

    auto fail = [this](const char* what, OSStatus s) {
        logError(what, s);
        AudioComponentInstanceDispose(m_data->audioUnit);
        m_data->audioUnit = nullptr;
        return false;
    };

    const UInt32 channels = static_cast<UInt32>(spec.output.audioChannelCount);

    AudioStreamBasicDescription format = {};
    format.mSampleRate = static_cast<Float64>(m_activeSpec.output.sampleRate);
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    format.mChannelsPerFrame = channels;
    format.mBitsPerChannel = 32;
    format.mFramesPerPacket = 1;
    format.mBytesPerFrame = channels * sizeof(float);
    format.mBytesPerPacket = format.mBytesPerFrame;

    status = AudioUnitSetProperty(m_data->audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0,
                                  &format, sizeof(format));
    if (status != noErr) {
        return fail("Set stream format", status);
    }

    const UInt32 maxFrames = 4096;
    AudioUnitSetProperty(m_data->audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Global, 0,
                         &maxFrames, sizeof(maxFrames));

    m_data->callback = spec.callback;
    m_data->channels = channels;

    AURenderCallbackStruct renderCallbackStruct = {};
    renderCallbackStruct.inputProc = renderCallback;
    renderCallbackStruct.inputProcRefCon = m_data.get();
    status = AudioUnitSetProperty(m_data->audioUnit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0,
                                  &renderCallbackStruct, sizeof(renderCallbackStruct));
    if (status != noErr) {
        return fail("Set render callback", status);
    }

    status = AudioUnitInitialize(m_data->audioUnit);
    if (status != noErr) {
        return fail("AudioUnitInitialize", status);
    }

    status = AudioOutputUnitStart(m_data->audioUnit);
    if (status != noErr) {
        AudioUnitUninitialize(m_data->audioUnit);
        return fail("AudioOutputUnitStart", status);
    }

    addSessionObservers();
    m_opened = true;

    LOGI() << "Opened RemoteIO: sample rate " << m_activeSpec.output.sampleRate
           << ", IO buffer " << [AVAudioSession sharedInstance].IOBufferDuration * 1000.0 << " ms"
           << ", block " << spec.output.samplesPerChannel;

    if (activeSpec) {
        *activeSpec = m_activeSpec;
    }

    m_activeSpecChanged.send(m_activeSpec);

    return true;
}

void IosAudioDriver::close()
{
    m_opened = false;

    removeSessionObservers();

    if (m_data->audioUnit) {
        AudioOutputUnitStop(m_data->audioUnit);
        AudioUnitUninitialize(m_data->audioUnit);
        AudioComponentInstanceDispose(m_data->audioUnit);
        m_data->audioUnit = nullptr;
    }

    @autoreleasepool {
        NSError* error = nil;
        [[AVAudioSession sharedInstance] setActive:NO
                                       withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation
                                             error:&error];
    }
}

bool IosAudioDriver::isOpened() const
{
    return m_opened;
}

const IosAudioDriver::Spec& IosAudioDriver::activeSpec() const
{
    return m_activeSpec;
}

async::Channel<IosAudioDriver::Spec> IosAudioDriver::activeSpecChanged() const
{
    return m_activeSpecChanged;
}

std::vector<samples_t> IosAudioDriver::availableOutputDeviceBufferSizes() const
{
    std::vector<samples_t> result;

    for (samples_t n = MAXIMUM_BUFFER_SIZE; n >= MINIMUM_BUFFER_SIZE; n /= 2) {
        result.push_back(n);
    }

    std::sort(result.begin(), result.end());

    return result;
}

std::vector<sample_rate_t> IosAudioDriver::availableOutputDeviceSampleRates() const
{
    // The session may choose another rate depending on the route; the actual rate is reported in activeSpec()
    return {
        44100,
        48000,
    };
}

AudioDeviceList IosAudioDriver::availableOutputDevices() const
{
    // iOS routes output automatically (speaker, headphones, AirPlay, Bluetooth, USB)
    AudioDeviceList devices;
    devices.push_back({ DEFAULT_DEVICE_ID, muse::trc("audio", "System default") });
    return devices;
}

async::Notification IosAudioDriver::availableOutputDevicesChanged() const
{
    return m_availableOutputDevicesChanged;
}

void IosAudioDriver::addSessionObservers()
{
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    AVAudioSession* session = [AVAudioSession sharedInstance];

    m_data->interruptionObserver = [center addObserverForName:AVAudioSessionInterruptionNotification
                                                       object:session
                                                        queue:[NSOperationQueue mainQueue]
                                                   usingBlock:^(NSNotification* note) {
        const NSUInteger type = [note.userInfo[AVAudioSessionInterruptionTypeKey] unsignedIntegerValue];
        if (type == AVAudioSessionInterruptionTypeEnded) {
            restartAfterInterruption();
        }
    }];

    m_data->routeChangeObserver = [center addObserverForName:AVAudioSessionRouteChangeNotification
                                                      object:session
                                                       queue:[NSOperationQueue mainQueue]
                                                  usingBlock:^(NSNotification*) {
        // A new route may run at another sample rate; the controller reopens the driver and adapts the engine
        const auto currentRate = static_cast<sample_rate_t>([AVAudioSession sharedInstance].sampleRate);
        if (m_opened && currentRate != m_activeSpec.output.sampleRate) {
            m_availableOutputDevicesChanged.notify();
        }
    }];

    m_data->mediaServicesResetObserver = [center addObserverForName:AVAudioSessionMediaServicesWereResetNotification
                                                             object:session
                                                              queue:[NSOperationQueue mainQueue]
                                                         usingBlock:^(NSNotification*) {
        // All audio objects are invalid after a media server reset
        if (m_opened) {
            m_availableOutputDevicesChanged.notify();
        }
    }];
}

void IosAudioDriver::removeSessionObservers()
{
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    for (id* observer : { &m_data->interruptionObserver, &m_data->routeChangeObserver, &m_data->mediaServicesResetObserver }) {
        if (*observer) {
            [center removeObserver:*observer];
            *observer = nil;
        }
    }
}

void IosAudioDriver::restartAfterInterruption()
{
    if (!m_opened || !m_data->audioUnit) {
        return;
    }

    @autoreleasepool {
        NSError* error = nil;
        if (![[AVAudioSession sharedInstance] setActive:YES error:&error]) {
            logError("AVAudioSession setActive after interruption", error);
            return;
        }
    }

    OSStatus status = AudioOutputUnitStart(m_data->audioUnit);
    if (status != noErr) {
        logError("AudioOutputUnitStart after interruption", status);
    }
}
