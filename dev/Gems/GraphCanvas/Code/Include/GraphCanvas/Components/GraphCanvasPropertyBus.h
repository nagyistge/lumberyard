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

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/Component.h>

namespace GraphCanvas
{
    // Used by the PropertyGrid to find all Components that have Properties they want to expose to the editor.
    class GraphCanvasPropertyInterface
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual AZ::Component* GetPropertyComponent() = 0;
    };
    
    using GraphCanvasPropertyBus = AZ::EBus<GraphCanvasPropertyInterface>;

    // Stub Component to implement the bus to simplify the usage.
    class GraphCanvasPropertyComponent
        : public AZ::Component
        , public GraphCanvasPropertyBus::Handler
    {
    public:
        AZ_COMPONENT(GraphCanvasPropertyComponent, "{12408A55-4742-45B2-8694-EE1C80430FB4}");
        static void Reflect(AZ::ReflectContext* context) { (void)context; }

        void Init() override {};

        void Activate()
        {
            GraphCanvasPropertyBus::Handler::BusConnect(GetEntityId());
        }

        void Deactivate()
        {
            GraphCanvasPropertyBus::Handler::BusDisconnect();
        }

        // GraphCanvasPropertyBus
        AZ::Component* GetPropertyComponent()
        {
            return this;
        }
    };
}