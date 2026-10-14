/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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

#include "../../irpcchannel.h"
#include "global/concurrency/rpcqueue.h"

namespace muse::audio::rpc {
class GeneralRpcChannel : public IRpcChannel
{
public:
    GeneralRpcChannel() = default;
    ~GeneralRpcChannel() override;

    //! With one thread (the web app), the sides of the channel are not threads: the code in this scope is the
    //! engine's side (it runs in the audio driver's callback), the code outside it the main side
    class EngineScope
    {
    public:
        EngineScope();
        ~EngineScope();

    private:
        bool m_wasMainSide = false;
    };

    //! The main side again, inside an EngineScope: the engine lets the app run while it works long (an audio export)
    class MainScope
    {
    public:
        MainScope();
        ~MainScope();

    private:
        bool m_wasMainSide = false;
    };

    void setupOnMain() override;
    void setupOnEngine() override;

    void process() override;

    // IRpcChannel
    // msgs
    void send(const Msg& msg, const Handler& onResponse = nullptr) override;
    void onMethod(Method method, Handler h) override;
    void listenAll(Handler h) override;

    // stream
    void addStream(std::shared_ptr<IRpcStream> s) override;
    void removeStream(StreamId id) override;
    void sendStream(const StreamMsg& msg) override;
    void onStream(StreamId id, StreamHandler h) override;

private:

    struct RpcData {
        // msgs
        Handler listenerAll;
        std::map<Method, Handler> onMethods;
        std::map<CallId, Handler> onResponses;

        // stream
        std::map<StreamId, std::shared_ptr<IRpcStream> > streams;
        std::map<StreamId, StreamHandler> onStreams;
    };

    void receive(RpcData& to, const Msg& m) const;
    void receive(RpcData& to, const StreamMsg& m) const;

    RpcData m_engineRpcData;
    RpcData m_mainRpcData;

    // port1 - main thread
    // port2 - engine rpc thread;
    RpcQueue<Msg> m_msgQueue;
};

//! The engine's side of a GeneralRpcChannel when there is one thread (the web app): everything the engine does
//! through it is done as the engine, also later in asynchronous continuations (e.g. a response sent when a
//! promise resolves), which run outside the audio driver's callback
class EngineSideRpcChannel : public IRpcChannel
{
public:
    explicit EngineSideRpcChannel(std::shared_ptr<IRpcChannel> channel)
        : m_channel(std::move(channel)) {}

    void setupOnMain() override { m_channel->setupOnMain(); }
    void setupOnEngine() override { GeneralRpcChannel::EngineScope scope; m_channel->setupOnEngine(); }
    void process() override { GeneralRpcChannel::EngineScope scope; m_channel->process(); }

    void send(const Msg& msg, const Handler& onResponse = nullptr) override
    {
        GeneralRpcChannel::EngineScope scope;
        m_channel->send(msg, onResponse);
    }

    void onMethod(Method method, Handler h) override { GeneralRpcChannel::EngineScope scope; m_channel->onMethod(method, h); }
    void listenAll(Handler h) override { GeneralRpcChannel::EngineScope scope; m_channel->listenAll(h); }

    void addStream(std::shared_ptr<IRpcStream> s) override { GeneralRpcChannel::EngineScope scope; m_channel->addStream(s); }
    void removeStream(StreamId id) override { GeneralRpcChannel::EngineScope scope; m_channel->removeStream(id); }
    void sendStream(const StreamMsg& msg) override { GeneralRpcChannel::EngineScope scope; m_channel->sendStream(msg); }
    void onStream(StreamId id, StreamHandler h) override { GeneralRpcChannel::EngineScope scope; m_channel->onStream(id, h); }

private:
    std::shared_ptr<IRpcChannel> m_channel;
};
}
