/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Input/Events/InputChannelEventListener.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener()
        : m_filter()
        , m_priority(GetPriorityDefault())
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(bool autoConnect)
        : m_filter()
        , m_priority(GetPriorityDefault())
    {
        if (autoConnect)
        {
            Connect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZ::s32 priority)
        : m_filter()
        , m_priority(priority)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZ::s32 priority, bool autoConnect)
        : m_filter()
        , m_priority(priority)
    {
        if (autoConnect)
        {
            Connect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZStd::shared_ptr<InputChannelEventFilter> filter)
        : m_filter(filter)
        , m_priority(GetPriorityDefault())
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZStd::shared_ptr<InputChannelEventFilter> filter,
                                                         AZ::s32 priority)
        : m_filter(filter)
        , m_priority(priority)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputChannelEventListener::InputChannelEventListener(AZStd::shared_ptr<InputChannelEventFilter> filter,
                                                         AZ::s32 priority,
                                                         bool autoConnect)
        : m_filter(filter)
        , m_priority(priority)
    {
        if (autoConnect)
        {
            Connect();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::s32 InputChannelEventListener::GetPriority() const
    {
        return m_priority;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::SetFilter(AZStd::shared_ptr<InputChannelEventFilter> filter)
    {
        m_filter = filter;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::Connect()
    {
        InputChannelEventNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::Disconnect()
    {
        InputChannelEventNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputChannelEventListener::OnInputChannelEvent(const InputChannel& inputChannel,
                                                        bool& o_hasBeenConsumed)
    {
        if (o_hasBeenConsumed)
        {
            return;
        }

        if (m_filter)
        {
            const bool doesPassFilter = m_filter->DoesPassFilter(inputChannel);
            if (!doesPassFilter)
            {
                return;
            }
        }

        o_hasBeenConsumed = OnInputChannelEventFiltered(inputChannel);
    }
} // namespace AzFramework
