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
#include <precompiled.h>
#include <GraphCanvasModule.h>
#include <GraphCanvas.h>

#include <Components/GeometryComponent.h>
#include <Components/GridComponent.h>
#include <Components/GridVisualComponent.h>
#include <Components/SceneComponent.h>
#include <Components/StylingComponent.h>

#include <Components/ColorPaletteManager/ColorPaletteManagerComponent.h>

#include <Components/Connections/ConnectionComponent.h>
#include <Components/Connections/ConnectionVisualComponent.h>
#include <Components/Connections/DataConnections/DataConnectionComponent.h>
#include <Components/Connections/DataConnections/DataConnectionVisualComponent.h>

#include <Components/Nodes/NodeComponent.h>
#include <Components/Nodes/Comment/CommentNodeLayoutComponent.h>
#include <Components/Nodes/Comment/CommentNodeTextComponent.h>
#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <Components/Nodes/General/GeneralNodeLayoutComponent.h>
#include <Components/Nodes/General/GeneralNodeTitleComponent.h>
#include <Components/Nodes/General/GeneralSlotLayoutComponent.h>
#include <Components/Nodes/Wrapper/WrapperNodeLayoutComponent.h>

#include <Components/Slots/SlotComponent.h>
#include <Components/Slots/SlotConnectionFilterComponent.h>
#include <Components/Slots/Data/DataSlotComponent.h>
#include <Components/Slots/Data/DataSlotLayoutComponent.h>
#include <Components/Slots/Default/DefaultSlotLayoutComponent.h>
#include <Components/Slots/Execution/ExecutionSlotComponent.h>
#include <Components/Slots/Execution/ExecutionSlotLayoutComponent.h>
#include <Components/Slots/Property/PropertySlotComponent.h>
#include <Components/Slots/Property/PropertySlotLayoutComponent.h>

#include <Styling/Style.h>
#include <Styling/PseudoElement.h>

namespace GraphCanvas
{
    //! Create ComponentDescriptors and add them to the list.
    //! The descriptors will be registered at the appropriate time.
    //! The descriptors will be destroyed (and thus unregistered) at the appropriate time.
    GraphCanvasModule::GraphCanvasModule()
    {
        m_descriptors.insert(m_descriptors.end(), {

            // Components
            GraphCanvasSystemComponent::CreateDescriptor(),
            SceneComponent::CreateDescriptor(),

            // Background Grid
            GridComponent::CreateDescriptor(),
            GridVisualComponent::CreateDescriptor(),

            // General
            GeometryComponent::CreateDescriptor(),

            // ColorPalette
            ColorPaletteManagerComponent::CreateDescriptor(),

            // Connections
            ConnectionComponent::CreateDescriptor(),
            ConnectionVisualComponent::CreateDescriptor(),

            // Connections::DataConnections
            DataConnectionComponent::CreateDescriptor(),
            DataConnectionVisualComponent::CreateDescriptor(),

            // Nodes
            NodeComponent::CreateDescriptor(),
            NodeLayoutComponent::CreateDescriptor(),

            // CommentNode
            CommentNodeLayoutComponent::CreateDescriptor(),
            CommentNodeTextComponent::CreateDescriptor(),

            // GeneralNode
            GeneralNodeTitleComponent::CreateDescriptor(),
            GeneralSlotLayoutComponent::CreateDescriptor(),
            GeneralNodeFrameComponent::CreateDescriptor(),
            GeneralNodeLayoutComponent::CreateDescriptor(),

            // Wrapper Node
            WrapperNodeLayoutComponent::CreateDescriptor(),

            // Slots
            SlotComponent::CreateDescriptor(),
            SlotConnectionFilterComponent::CreateDescriptor(),
            DefaultSlotLayoutComponent::CreateDescriptor(),

            // Data Slots
            DataSlotComponent::CreateDescriptor(),
            DataSlotLayoutComponent::CreateDescriptor(),

            // Execution Slots
            ExecutionSlotComponent::CreateDescriptor(),
            ExecutionSlotLayoutComponent::CreateDescriptor(),

            // Property Slots
            PropertySlotComponent::CreateDescriptor(),
            PropertySlotLayoutComponent::CreateDescriptor(),

            // Styling
            StylingComponent::CreateDescriptor(),

            Styling::ComputedStyle::CreateDescriptor(),
            Styling::StyleSheet::CreateDescriptor(),
            Styling::VirtualChildElement::CreateDescriptor()
        });
    }

    //! Request system components on the system entity.
    //! These components' memory is owned by the system entity.
    AZ::ComponentTypeList GraphCanvasModule::GetRequiredSystemComponents() const
    {
        return{
            azrtti_typeid<GraphCanvasSystemComponent>(),
        };
    }
    
}

AZ_DECLARE_MODULE_CLASS(GraphCanvas_875b6fcbdeea44deaae7984ad9bb6cdc, GraphCanvas::GraphCanvasModule)