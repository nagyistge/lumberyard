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

#include <GraphCanvas/Components/Nodes/NodeBus.h>

#include "Editor/Include/ScriptCanvas/Bus/NodeIdPair.h"

#include "Editor/GraphCanvas/Components/NodeDescriptorComponent.h"

namespace ScriptCanvasEditor
{
    class EBusHandlerEventNodeDescriptorComponent
        : public NodeDescriptorComponent
        , protected GraphCanvas::NodeNotificationBus::Handler
        , public EBusEventNodeDescriptorRequestBus::Handler
    {
    public:
        AZ_COMPONENT(EBusHandlerEventNodeDescriptorComponent, "{F08F673C-0815-4CCA-AB9D-21965E9A14F2}", NodeDescriptorComponent);
        static void Reflect(AZ::ReflectContext* reflectContext);
        
        EBusHandlerEventNodeDescriptorComponent();
        EBusHandlerEventNodeDescriptorComponent(const AZStd::string& busName, const AZStd::string& methodName);
        ~EBusHandlerEventNodeDescriptorComponent() = default;

        void Activate() override;
        void Deactivate() override;

        // EBusEventNodeDescriptorRequestBus
        bool IsWrapped() const override;
        NodeIdPair GetEbusWrapperNodeId() const override;

        AZStd::string GetBusName() const override;
        AZStd::string GetEventName() const override;
        ////

        // NodeNotificationBus::Handler
        void OnAddedToScene(const AZ::EntityId& sceneId) override;

        void OnNodeAboutToSerialize(GraphCanvas::SceneSerialization& sceneSerialization) override;
        void OnNodeWrapped(const AZ::EntityId& wrappingNode) override;
        ////

    private:

        NodeIdPair    m_ebusWrapper;
    
        AZStd::string m_busName;
        AZStd::string m_eventName;
    };
}