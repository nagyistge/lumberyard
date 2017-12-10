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
#pragma once
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include "FlyCameraInputBus.h"

namespace LYGame
{
    class FlyCameraInputComponent
        : public AZ::Component
        , public AZ::TickBus::Handler
        , public AzFramework::InputChannelEventListener
        , public FlyCameraInputBus::Handler
    {
    public:
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void Reflect(AZ::ReflectContext* reflection);

        AZ_COMPONENT(FlyCameraInputComponent, "{EB588B1E-AC2E-44AA-A1E6-E5960942E950}");
        virtual ~FlyCameraInputComponent();

        // AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AZ::TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        // AzFramework::InputChannelEventListener
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

        // FlyCameraInputInterface
        void SetIsEnabled(bool isEnabled) override;
        bool GetIsEnabled() override;

    private:
        void OnMouseEvent(const AzFramework::InputChannel& inputChannel);
        void OnKeyboardEvent(const AzFramework::InputChannel& inputChannel);
        void OnGamepadEvent(const AzFramework::InputChannel& inputChannel);
        void OnTouchEvent(const AzFramework::InputChannel& inputChannel, const Vec2& screenPosition);
        void OnVirtualLeftThumbstickEvent(const AzFramework::InputChannel& inputChannel, const Vec2& screenPosition);
        void OnVirtualRightThumbstickEvent(const AzFramework::InputChannel& inputChannel, const Vec2& screenPosition);

    private:
        static const AZ::Crc32 UnknownInputChannelId;

        // Editable Properties
        float m_moveSpeed = 20.0f;
        float m_rotationSpeed = 5.0f;

        float m_mouseSensitivity = 0.025f;
        float m_virtualThumbstickRadiusAsPercentageOfScreenWidth = 0.1f;

        bool m_InvertRotationInputAxisX = false;
        bool m_InvertRotationInputAxisY = false;

        bool m_isEnabled = true;

        // Run-time Properties
        Vec2 m_movement = ZERO;
        Vec2 m_rotation = ZERO;

        Vec2 m_leftDownPosition = ZERO;
        AZ::Crc32 m_leftFingerId = UnknownInputChannelId;

        Vec2 m_rightDownPosition = ZERO;
        AZ::Crc32 m_rightFingerId = UnknownInputChannelId;

        int m_thumbstickTextureId = 0;
    };
} // LYGame
