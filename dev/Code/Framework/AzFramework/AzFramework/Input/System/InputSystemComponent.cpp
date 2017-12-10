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

#include <AzFramework/Input/System/InputSystemComponent.h>

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Motion/InputDeviceMotion.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::vector<AZStd::string> GetAllMotionChannelNames()
    {
        AZStd::vector<AZStd::string> allMotionChannelNames;
        for (AZ::u32 i = 0; i < InputDeviceMotion::Acceleration::All.size(); ++i)
        {
            const InputChannelId& channelId = InputDeviceMotion::Acceleration::All[i];
            allMotionChannelNames.push_back(channelId.GetName());
        }
        for (AZ::u32 i = 0; i < InputDeviceMotion::RotationRate::All.size(); ++i)
        {
            const InputChannelId& channelId = InputDeviceMotion::RotationRate::All[i];
            allMotionChannelNames.push_back(channelId.GetName());
        }
        for (AZ::u32 i = 0; i < InputDeviceMotion::MagneticField::All.size(); ++i)
        {
            const InputChannelId& channelId = InputDeviceMotion::MagneticField::All[i];
            allMotionChannelNames.push_back(channelId.GetName());
        }
        for (AZ::u32 i = 0; i < InputDeviceMotion::Orientation::All.size(); ++i)
        {
            const InputChannelId& channelId = InputDeviceMotion::Orientation::All[i];
            allMotionChannelNames.push_back(channelId.GetName());
        }
        return allMotionChannelNames;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<InputSystemComponent, AZ::Component>()
                ->Version(1)
                ->Field("GamepadsEnabled", &InputSystemComponent::m_gamepadsEnabled)
                ->Field("KeyboardEnabled", &InputSystemComponent::m_keyboardEnabled)
                ->Field("MotionEnabled", &InputSystemComponent::m_motionEnabled)
                ->Field("MouseEnabled", &InputSystemComponent::m_mouseEnabled)
                ->Field("TouchEnabled", &InputSystemComponent::m_touchEnabled)
                ->Field("VirtualKeyboardEnabled", &InputSystemComponent::m_virtualKeyboardEnabled)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<InputSystemComponent>(
                    "Input System", "Controls which core input devices are made available")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, &InputSystemComponent::m_gamepadsEnabled, "Gamepads", "The number of game-pads enabled.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0)
                        ->Attribute(AZ::Edit::Attributes::Max, 4)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &InputSystemComponent::m_keyboardEnabled, "Keyboard", "Is keyboard input enabled?")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &InputSystemComponent::m_motionEnabled, "Motion", "Is motion input enabled?")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &InputSystemComponent::m_mouseEnabled, "Mouse", "Is mouse input enabled?")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &InputSystemComponent::m_touchEnabled, "Touch", "Is touch enabled?")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &InputSystemComponent::m_virtualKeyboardEnabled, "Virtual Keyboard", "Is the virtual keyboard enabled?")
                ;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("InputSystemService", 0x5438d51a));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("InputSystemService", 0x5438d51a));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputSystemComponent::InputSystemComponent()
        : m_gamepads()
        , m_keyboard()
        , m_motion()
        , m_mouse()
        , m_touch()
        , m_virtualKeyboard()
        , m_gamepadsEnabled(4)
        , m_keyboardEnabled(true)
        , m_motionEnabled(true)
        , m_mouseEnabled(true)
        , m_touchEnabled(true)
        , m_virtualKeyboardEnabled(true)
        , m_currentlyUpdatingInputDevices(false)
        , m_recreateInputDevicesAfterUpdate(false)
        , m_pimpl()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputSystemComponent::~InputSystemComponent()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::Activate()
    {
        // Create the platform specific implementation
        m_pimpl.reset(InputSystemComponent::Implementation::Create(*this));

        // Create all enabled input devices
        CreateEnabledInputDevices();

        InputSystemRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
        InputSystemRequestBus::Handler::BusDisconnect();

        // Destroy all enabled input devices
        DestroyEnabledInputDevices();

        // Destroy the platform specific implementation
        m_pimpl.reset(nullptr);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::OnTick(float /*deltaTime*/, AZ::ScriptTimePoint /*scriptTimePoint*/)
    {
        // At some point we should remove InputSystemRequests::TickInput and use the OnTick function
        // instead, but for now we must update input independently to maintain existing frame order.
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::TickInput()
    {
        if (m_pimpl)
        {
            m_pimpl->PreTickInputDevices();
        }

        m_currentlyUpdatingInputDevices = true;
        InputDeviceRequestBus::Broadcast(&InputDeviceRequests::TickInputDevice);
        m_currentlyUpdatingInputDevices = false;

        if (m_recreateInputDevicesAfterUpdate)
        {
            CreateEnabledInputDevices();
            m_recreateInputDevicesAfterUpdate = false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::RecreateEnabledInputDevices()
    {
        if (m_currentlyUpdatingInputDevices)
        {
            // Delay the request until we've finished updating to protect against getting called in
            // response to an input event, in which case calling CreateEnabledInputDevices here will
            // cause a crash (when the stack unwinds back up to the device which dispatced the event
            // but was then destroyed). An unlikely (but possible) scenario we must protect against.
            m_recreateInputDevicesAfterUpdate = true;
        }
        else
        {
            CreateEnabledInputDevices();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::CreateEnabledInputDevices()
    {
        DestroyEnabledInputDevices();

        m_gamepads.resize(m_gamepadsEnabled);
        for (AZ::u32 i = 0; i < m_gamepadsEnabled; ++i)
        {
            m_gamepads[i].reset(aznew InputDeviceGamepad(i));
        }

        m_keyboard.reset(m_keyboardEnabled ? aznew InputDeviceKeyboard() : nullptr);
        m_motion.reset(m_motionEnabled ? aznew InputDeviceMotion() : nullptr);
        m_mouse.reset(m_mouseEnabled ? aznew InputDeviceMouse() : nullptr);
        m_touch.reset(m_touchEnabled ? aznew InputDeviceTouch() : nullptr);
        m_virtualKeyboard.reset(m_virtualKeyboardEnabled ? aznew InputDeviceVirtualKeyboard() : nullptr);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputSystemComponent::DestroyEnabledInputDevices()
    {
        m_virtualKeyboard.reset(nullptr);
        m_touch.reset(nullptr);
        m_mouse.reset(nullptr);
        m_motion.reset(nullptr);
        m_keyboard.reset(nullptr);
        m_gamepads.clear();
    }
} // namespace AzFramework
