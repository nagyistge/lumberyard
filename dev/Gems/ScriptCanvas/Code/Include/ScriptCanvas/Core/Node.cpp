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

#include "precompiled.h"
#include "Node.h"

#include "Graph.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/RTTI/AttributeReader.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Core/Contract.h>
#include <ScriptCanvas/Core/Contracts/ConnectionLimitContract.h>
#include <ScriptCanvas/Core/Contracts/DynamicTypeContract.h>
#include <ScriptCanvas/Core/Contracts/StorageRequiredContract.h>
#include <ScriptCanvas/Core/GraphBus.h>

namespace ScriptCanvas
{
    Node::Node()
        : AZ::Component()
    {}

    Node::Node(const Node&)
    {}

    Node& Node::operator=(const Node&)
    {
        return *this;
    }

    Node::~Node()
    {
        NodeRequestBus::Handler::BusDisconnect();
    }
    
    void Node::Init()
    {
        NodeRequestBus::Handler::BusConnect(GetEntityId());
        DatumNotificationBus::Handler::BusConnect(GetEntityId());
        EditorNodeRequestBus::Handler::BusConnect(GetEntityId());

        for (auto& slot : m_slotContainer.m_slots)
        {
            slot.SetNodeId(GetEntityId());
        }

        for (Datum& datum : m_inputData)
        {
            datum.SetNotificationsTarget(GetEntityId());
        }

        ConfigureSlots();
        OnInit();
    }

    void Node::Activate()
    {
        SignalBus::Handler::BusConnect(GetEntityId());

        OnActivate();
    }

    void Node::Deactivate()
    {
        OnDeactivate();

        SignalBus::Handler::BusDisconnect();
    }

    AZStd::string Node::GetSlotName(const SlotId& slotId) const
    {
        if (slotId.IsValid())
        {
            auto slot = GetSlot(slotId);
            if (slot)
            {
                return slot->GetName();
            }
        }
        return "";
    }

    void Node::Reflect(AZ::ReflectContext* context)
    {
        Slot::Reflect(context);
        SlotContainer::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Node, AZ::Component>()
                ->Version(5)
                ->Field("UniqueGraphID", &Node::m_graphId)
                ->Field("Slots", &Node::m_slotContainer)
                ->Field("m_inputData", &Node::m_inputData)
                ->Field("m_inputIndexBySlotIndex", &Node::m_inputIndexBySlotIndex)
                ->Field("m_outputTypes", &Node::m_nonDatumTypes)
                ->Field("m_outputTypeIndexBySlotIndex", &Node::m_nonDatumTypeIndexBySlotIndex)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Node>("Node", "Node")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Node::m_inputData, "Input", "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    void Node::SignalInput(const SlotId& slotId)
    {
        SCRIPT_CANVAS_IF_NOT_IN_INFINITE_LOOP((*this))
        {
            DebugTrace("Node::SignalInput: %s : %s\n", GetDebugName().data(), GetSlotName(slotId).c_str());
            LogNotificationBus::Event(GetGraphId(), &LogNotifications::OnNodeSignalInput, slotId.m_id, GetNodeName(), GetSlotName(slotId));
            OnInputSignal(slotId);
        }
        SCRIPTCANVAS_HANDLE_ERROR((*this));
    }

    void Node::SignalOutput(const SlotId& slotId)
    {
        SCRIPTCANVAS_RETURN_IF_ERROR_STATE((*this));
        
        if (slotId.IsValid())
        {
            auto slotIdIt = m_slotContainer.m_slotIdSlotMap.find(slotId);
            if (slotIdIt != m_slotContainer.m_slotIdSlotMap.end())
            {
                AZStd::vector<Endpoint> connectedEndpoints;
                GraphRequestBus::EventResult(connectedEndpoints, m_graphId, &GraphRequests::GetConnectedEndpoints, Endpoint{ GetEntityId(), slotId });
                for (const Endpoint& endpoint : connectedEndpoints)
                {
                    DebugTrace("SignalOutput: Node %s Target Slot: %s\n", GetEntityId().ToString().data(), slotId.m_id.ToString<AZStd::string>().data());
                    SignalBus::Event(endpoint.GetNodeId(), &SignalInterface::SignalInput, endpoint.GetSlotId());

                    Slot* slot = nullptr;
                    AZ::Entity* nodeEntity = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, endpoint.GetNodeId());

                    if (auto* node = nodeEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity) : nullptr)
                    {
                        slot = node->GetSlot(endpoint.GetSlotId());
                        LogNotificationBus::Event(GetGraphId(), &LogNotifications::OnNodeSignalOutput, GetNodeName(), node->GetNodeName(), slot ? slot->GetName() : endpoint.GetSlotId().m_id.ToString<AZStd::string>().data());
                    }
                    else
                    {
                        // TODO: Log an error I can catch
                        LogNotificationBus::Event(GetGraphId(), &LogNotifications::OnNodeSignalOutput, GetNodeName(), "", slotId.m_id.ToString<AZStd::string>().data());
                    }
                }
            }
            else
            {
                AZ_Warning("Script Canvas", false, "Node does not have the output slot that was signaled. Node: %s Slot: %s", RTTI_GetTypeName(), slotId.m_id.ToString<AZStd::string>().data());
            }
        }
    }

    bool Node::SlotAcceptsType(const SlotId& slotID, const Data::Type& type) const 
    {
        const Slot* slot(nullptr);
        int slotIndex(-1);
        if (GetValidSlotIndex(slotID, slot, slotIndex))
        {
            if (slot->GetType() == SlotType::DataIn)
            {
                if (const Datum* datum = GetInput(slotID))
                {
                    return datum && (Data::IS_A(type, datum->GetType()) || datum->IsConvertibleFrom(type));
                }
                
                const Data::Type* inputType = GetNonDatumType(slotID);
                return inputType && (Data::IS_A(*inputType, type) || inputType->IsConvertibleFrom(type));
            }
            else
            {
                AZ_Assert(slot->GetType() == SlotType::DataOut, "unsupported slot type");
                const Data::Type* outputType = GetNonDatumType(slotID);
                return outputType && (Data::IS_A(*outputType, type) || outputType->IsConvertibleTo(type));
            }
        }

        AZ_Error("ScriptCanvas", false, "SlotID not found in node");
        return false;
    }

    Data::Type Node::GetSlotDataType(const SlotId& slotId) const
    {
        const Slot* slot(nullptr);
        int slotIndex(-1);
        if (GetValidSlotIndex(slotId, slot, slotIndex))
        {
            if (slot->GetType() == SlotType::DataIn)
            {
                const Datum* datum = GetInput(slotId);

                if (datum)
                {
                    return datum->GetType();
                }
            }
            else if (slot->GetType() == SlotType::DataOut)
            {
                const Data::Type* outputType = GetNonDatumType(slotId);

                if (outputType)
                {
                    return (*outputType);
                }
            }
        }

        return Data::Type::Invalid();
    }

    bool Node::IsSlotValidStorage(const SlotId& slotId) const
    {
        const Slot* slot(nullptr);
        int slotIndex(-1);
        if (GetValidSlotIndex(slotId, slot, slotIndex))
        {
            if (slot->GetType() == SlotType::DataIn)
            {
                const Datum* datum = GetInput(slotId);
                return datum && datum->IsStorage();
            }
        }

        return false;
    }

    bool Node::DynamicSlotAcceptsType(const SlotId& slotID, const Data::Type& type, DynamicTypeArity arity, const Slot& outputSlot, const AZStd::vector<Slot*>& inputSlots) const
    {
        if (!type.IsValid())
        {
            // this could be handled, technically, but might be more confusing than anything else)
            return false;
        }

        auto iter = AZStd::find_if(inputSlots.begin(), inputSlots.end(), [&slotID](Slot* slot) { return slot->GetId() == slotID; });

        if (iter != inputSlots.end())
        {
            for (Slot* slot : inputSlots)
            {
                if (!DynamicSlotInputAcceptsType(slotID, type, arity, *slot))
                {
                    return false;
                }
            }
        }
        else if (slotID == outputSlot.GetId())
        {
            for (Slot* inputSlot : inputSlots)
            {
                auto inputs = GetConnectedNodes(*inputSlot);

                for (const auto& input : inputs)
                {
                    if (!input.first->GetSlotDataType(input.second).IS_A(type))
                    {
                        // the new output doesn't match the previous inputs
                        return false;
                    }
                }
            }
        }

        const AZStd::vector<AZStd::pair<const Node*, const SlotId>> outputs = GetConnectedNodes(outputSlot);

        // check new input/output against previously existing output types
        for (const auto& output : outputs)
        {
            if (!type.IS_A(output.first->GetSlotDataType(output.second)))
            {
                return false;
            }
        }

        return true;
    }

    bool Node::DynamicSlotInputAcceptsType(const SlotId& slotID, const Data::Type& type, DynamicTypeArity arity, const Slot& inputSlot) const
    {
        const AZStd::vector<AZStd::pair<const Node*, const SlotId>> inputs = GetConnectedNodes(inputSlot);

        if (arity == DynamicTypeArity::Single && !inputs.empty())
        {
            // this input can only be connected to one source
            return false;
        }

        for (const auto& iter : inputs)
        {
            const auto& previousInputType = iter.first->GetSlotDataType(iter.second);
            if (!(previousInputType.IS_A(type) || type.IS_A(previousInputType)))
            {
                // no acceptable tyep relationship
                return false;
            }
        }

        return true;
    }

    SlotId Node::GetSlotId(AZStd::string_view slotName) const
    {
        auto slotNamePair = m_slotContainer.m_slotNameSlotMap.find(slotName);
        return slotNamePair != m_slotContainer.m_slotNameSlotMap.end() ? m_slotContainer.m_slots[slotNamePair->second].GetId() : SlotId{};
    }

    AZStd::vector<const Slot*> Node::GetSlotsByType(SlotType slotType) const
    {
        AZStd::vector<const Slot*> slots;

        for (const auto& slot : m_slotContainer.m_slots)
        {
            if (slot.GetType() == slotType)
            {
                slots.emplace_back(&slot);
            }
        }

        return slots;
    }

    SlotId Node::GetSlotIdByType(AZStd::string_view slotName, SlotType slotType) const
    {
        auto slotNameRange = m_slotContainer.m_slotNameSlotMap.equal_range(slotName);
        auto nameSlotIt = AZStd::find_if(slotNameRange.first, slotNameRange.second, [this, slotType](const AZStd::pair<AZStd::string, int>& nameSlotPair)
        {
            return slotType == m_slotContainer.m_slots[nameSlotPair.second].GetType();
        });
        return nameSlotIt != slotNameRange.second ? m_slotContainer.m_slots[nameSlotIt->second].GetId() : SlotId{};
    }

    AZStd::vector<SlotId> Node::GetSlotIds(AZStd::string_view slotName) const
    {
        auto nameSlotRange = m_slotContainer.m_slotNameSlotMap.equal_range(slotName);
        AZStd::vector<SlotId> result;
        for (auto nameSlotIt = nameSlotRange.first; nameSlotIt != nameSlotRange.second; ++nameSlotIt)
        {
            result.push_back(m_slotContainer.m_slots[nameSlotIt->second].GetId());
        }
        return result;
    }

    Slot* Node::GetSlot(const SlotId& slotId) const
    {        
        if (slotId.IsValid())
        {
            const Slot* slot = nullptr;
            int ignoredIndex = false;

            if (GetValidSlotIndex(slotId, slot, ignoredIndex))
            {
                return const_cast<Slot*>(slot);
            }

            AZ_Warning("Script Canvas", false, "Node %s does not have the specified slot: %s. Supported slots are:\n", GetEntity()->GetName().data(), slotId.m_id.ToString<AZStd::string>().c_str());
//             for (auto& currentSlot : m_slotContainer.m_slots)
//             {
//                 AZ_TracePrintf("Script Canvas", "%s\n", currentSlot.GetName().data());
//             }
        }

        return {};
    }    

    AZStd::vector<const Slot*> Node::GetAllSlots() const
    {
        const AZStd::vector<Slot>& slots = GetSlots();

        AZStd::vector<const Slot*> retVal;
        retVal.reserve(slots.size());

        for (const auto& slot : GetSlots())
        {
            retVal.push_back(&slot);
        }

        return retVal;
    }

    bool Node::SlotExists(AZStd::string_view name, SlotType type) const
    {
        SlotId unused;
        return SlotExists(name, type, unused);
    }

    bool Node::SlotExists(AZStd::string_view name, SlotType type, SlotId& out) const
    {
        out = GetSlotIdByType(name, type);
        return out.IsValid();
    }

    SlotId Node::AddSlot(const SlotConfiguration& slotConfiguration)
    {
        if (slotConfiguration.m_name.empty())
        {
            AZ_Warning("Script Canvas", false, "attempting to add a slot with no name");
            return SlotId{};
        }

        SlotId prexisiting;
        if (slotConfiguration.m_addUniqueSlotByNameAndType && SlotExists(slotConfiguration.m_name, slotConfiguration.m_type, prexisiting))
        {
            return prexisiting;
        }

        int slotIndex = azlossy_caster(m_slotContainer.m_slots.size());
        m_slotContainer.m_slots.emplace_back(slotConfiguration.m_name, slotConfiguration.m_toolTip, slotConfiguration.m_type, slotIndex, slotConfiguration.m_contractDescs);
        auto& newSlot = m_slotContainer.m_slots.back();
        m_slotContainer.m_slotIdSlotMap.emplace(newSlot.GetId(), slotIndex);
        m_slotContainer.m_slotNameSlotMap.emplace(newSlot.GetName(), slotIndex);

        if (GetEntity())
        {
            newSlot.SetNodeId(GetEntityId());
        }

        return newSlot.GetId();
    }

    SlotId Node::AddSlot(AZStd::string_view name, AZStd::string_view toolTip, SlotType type, const AZStd::vector<ContractDescriptor>& contractDescs, bool addUniqueSlotByNameAndType)
    {
        return AddSlot({ name, toolTip, type, contractDescs, addUniqueSlotByNameAndType });  
    }

    void Node::SetDatumLabel(const SlotId& slotID, AZStd::string_view name)
    {
        // Slot ID to Slot index.
        const Slot* unused(nullptr);
        int slotIndex(-1);
        if(GetValidSlotIndex(slotID, unused, slotIndex))
        {
            // Slot index to Datum index.
            int inputDatumIndex(-1);
            if(GetValidInputDataIndex(slotIndex, inputDatumIndex))
            {
                m_inputData[inputDatumIndex].SetLabel(name);
            }
        }
    }

    void Node::ResolveSelfEntityReferences(const AZ::EntityId& graphOwnerId)
    {
        // remap graph unique id to graph entity id
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> uniqueIdMap;
        uniqueIdMap.emplace(ScriptCanvas::SelfReferenceId, graphOwnerId);
        for (Datum& datum : m_inputData)
        {
            AZ::IdUtils::Remapper<AZ::EntityId>::RemapIds(&datum, [&uniqueIdMap](AZ::EntityId sourceId, bool, const AZ::IdUtils::Remapper<AZ::EntityId>::IdGenerator&) -> AZ::EntityId
            {
                auto findIt = uniqueIdMap.find(sourceId);
                return findIt != uniqueIdMap.end() ? findIt->second : sourceId;
            }, serializeContext, false);
        }
    }

    SlotId Node::AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, Datum::eOriginality originality, bool addUniqueSlotByNameAndType)
    {
        return AddInputDatumSlot(name, toolTip, AZStd::move(type), nullptr, originality, addUniqueSlotByNameAndType);
    }

    SlotId Node::AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const Data::Type& type, const void* source, Datum::eOriginality originality, bool addUniqueSlotByNameAndType)
    {
        SlotId slotIDOut;
        AZStd::vector<ContractDescriptor> contracts;

        auto func = []() { return aznew TypeContract(); };
        ContractDescriptor descriptor{ AZStd::move(func) };
        contracts.emplace_back(descriptor);

        int slotIndex;
        if (AddSlotInternal({ name, toolTip, SlotType::DataIn, contracts, addUniqueSlotByNameAndType }, slotIDOut, slotIndex))
        {
            const int inputIndex = azlossy_cast<int>(m_inputData.size());
            m_inputData.emplace_back(type, originality, source, AZ::Uuid::CreateNull());

            Datum& datum = m_inputData[inputIndex];
            datum.SetNotificationsTarget(GetEntityId());
            datum.SetLabel(name);

            m_inputIndexBySlotIndex.insert(AZStd::make_pair(slotIndex, inputIndex));
        }

        return slotIDOut;
    }

    SlotId Node::AddInputDatumSlot(AZStd::string_view name, AZStd::string_view toolTip, const AZ::BehaviorParameter& typeDesc, Datum::eOriginality originality, bool addUniqueSlotByNameAndType)
    {
        SlotId slotIDOut;
        AZStd::vector<ContractDescriptor> contracts;
        contracts.push_back({ []() { return aznew TypeContract(); } });

        int slotIndex;
        if (AddSlotInternal({name, toolTip, SlotType::DataIn, contracts, addUniqueSlotByNameAndType }, slotIDOut, slotIndex))
        {
            const int inputIndex = azlossy_cast<int>(m_inputData.size());
            m_inputData.emplace_back(typeDesc, originality, nullptr);

            Datum& datum = m_inputData.back();
            datum.SetNotificationsTarget(GetEntityId());
            datum.SetLabel(name);

            m_inputIndexBySlotIndex.insert(AZStd::make_pair(slotIndex, inputIndex));
        }

        return slotIDOut;
    }

    SlotId Node::AddInputDatumDynamicTypedSlot(AZStd::string_view name, AZStd::string_view toolTip, bool addUniqueSlotByNameAndType)
    {
        SlotId slotIDOut;
        AZStd::vector<ContractDescriptor> contracts;
        auto func = []() { return aznew DynamicTypeContract(); };
        ContractDescriptor descriptor{ AZStd::move(func) };
        contracts.emplace_back(descriptor);

        int slotIndex;
        if (AddSlotInternal({ name, toolTip, SlotType::DataIn, contracts, addUniqueSlotByNameAndType }, slotIDOut, slotIndex))
        {
            const int inputIndex = azlossy_cast<int>(m_inputData.size());
            m_inputData.emplace_back(Datum::CreateUntypedStorage());

            Datum& datum = m_inputData[inputIndex];
            datum.SetNotificationsTarget(GetEntityId());
            datum.SetLabel(name);

            m_inputIndexBySlotIndex.insert(AZStd::make_pair(slotIndex, inputIndex));
        }

        return slotIDOut;
    }

    SlotId Node::AddInputDatumUntypedSlot(AZStd::string_view name, const AZStd::vector<ContractDescriptor>* contractsIn, AZStd::string_view toolTip, bool addUniqueSlotByNameAndType)
    {
        SlotId slotIDOut;
        AZStd::vector<ContractDescriptor> contracts = contractsIn ? *contractsIn : AZStd::vector<ContractDescriptor>{};
        int slotIndex;
        if (AddSlotInternal({ name, toolTip, SlotType::DataIn, contracts, addUniqueSlotByNameAndType }, slotIDOut, slotIndex))
        {
            const int inputIndex = azlossy_cast<int>(m_inputData.size());
            m_inputData.emplace_back(Datum::CreateUntypedStorage());
            m_inputData[inputIndex].SetLabel(name.data());
            m_inputIndexBySlotIndex.insert(AZStd::make_pair(slotIndex, inputIndex));
        }

        return slotIDOut;
    }

    SlotId Node::AddInputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, Data::Type&& type, InputTypeContract contractType, bool addUniqueSlotByNameAndType)
    {
        SlotId slotIDOut;
        AZStd::vector<ContractDescriptor> contracts;
        if (contractType == InputTypeContract::CustomType)
        {
            auto func = [type]() { return aznew TypeContract{ type }; };
            ContractDescriptor descriptor{ AZStd::move(func) };
            contracts.emplace_back(descriptor);
        }
        else if (contractType == InputTypeContract::DatumType)
        {
            auto func = []() { return aznew TypeContract{}; };
            ContractDescriptor descriptor{ AZStd::move(func) };
            contracts.emplace_back(descriptor);
        }

        int slotIndex;
        if (AddSlotInternal({ name, toolTip, SlotType::DataIn, contracts, addUniqueSlotByNameAndType }, slotIDOut, slotIndex))
        {
            AddNonDatumType(AZStd::move(type), slotIndex);
        }

        return slotIDOut;
    }

    SlotId Node::AddInputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, const AZ::BehaviorParameter& typeDesc, InputTypeContract contractType, bool addUniqueSlotByNameAndType)
    {
        return AddInputTypeSlot(name, toolTip, AZ::BehaviorContextHelper::IsStringParameter(typeDesc) ? Data::Type::String() : Data::FromBehaviorContextTypeChecked(typeDesc.m_typeId), contractType, addUniqueSlotByNameAndType);
    }

    SlotId Node::AddOutputTypeSlot(AZStd::string_view name, AZStd::string_view toolTip, Data::Type&& type, OutputStorage outputStorage, bool addUniqueSlotByNameAndType)
    {
        SlotId slotIDOut;
        AZStd::vector<ContractDescriptor> contracts;
        /*
        \todo reevaluate a time to use this. It allows a lot of optimization on at edit time, at the cost of some convenience

        if (outputStorage == OutputStorage::Required && !Data::IsValueType(type))
        {
            auto func = []() { return aznew StorageRequiredContract(); };
            ContractDescriptor descriptor{ AZStd::move(func) };
            contracts.emplace_back(descriptor);
        }
        */

        int slotIndex;
        if (AddSlotInternal({ name, toolTip, SlotType::DataOut, contracts, addUniqueSlotByNameAndType }, slotIDOut, slotIndex))
        {
            AddNonDatumType(AZStd::move(type), slotIndex);
        }

        return slotIDOut;
    }

    void Node::AddNonDatumType(Data::Type&& type, int slotIndex)
    {
        const int outputTypeIndex = azlossy_cast<int>(m_nonDatumTypes.size());
        m_nonDatumTypes.emplace_back(AZStd::move(type));
        m_nonDatumTypeIndexBySlotIndex.insert(AZStd::make_pair(slotIndex, outputTypeIndex));

    }

    bool Node::AddSlotInternal(const SlotConfiguration& slotConfiguration, SlotId& slotIDOut, int& indexOut)
    {
        if (slotConfiguration.m_addUniqueSlotByNameAndType && SlotExists(slotConfiguration.m_name, slotConfiguration.m_type, slotIDOut))
        {
            SetDatumLabel(slotIDOut, slotConfiguration.m_name);
            return false;
        }

        slotIDOut = AddSlot(slotConfiguration);
        const Slot* unused(nullptr);
        return GetValidSlotIndex(slotIDOut, unused, indexOut);
    }

    AZStd::vector<Endpoint> Node::GetEndpointsByType(SlotType slotType) const
    {
        AZStd::vector<Endpoint> endpoints;
        for (auto& slot : m_slotContainer.m_slots)
        {
            if (slot.GetType() == slotType)
            {
                AZStd::vector<Endpoint> connectedEndpoints;
                GraphRequestBus::EventResult(connectedEndpoints, m_graphId, &GraphRequests::GetConnectedEndpoints, Endpoint{ GetEntityId(), slot.GetId() });
                endpoints.insert(endpoints.end(), connectedEndpoints.begin(), connectedEndpoints.end());
            }
        }

        return endpoints;
    }

    void Node::SetGraphId(const AZ::EntityId& id)
    {
        m_graphId = id;
    }

    Graph* Node::GetGraph() const
    {
        Graph* graph{};
        GraphRequestBus::EventResult(graph, m_graphId, &GraphRequests::GetGraph);
        return graph;
    }

    const Node* Node::FindNodeConst(const AZ::EntityId& nodeID)
    {
        return FindNode(nodeID);
    }

    Node* Node::FindNode(const AZ::EntityId& nodeID)
    {
        AZ::Entity* nodeEntity{};
        AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, nodeID);
        return nodeEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity) : nullptr;
    }

    NodePtrConstList Node::GetConnectedNodesByType(SlotType slotType) const
    {
        NodePtrConstList connectedNodes;

        for (const auto& endpoint : GetEndpointsByType(slotType))
        {
            AZ::Entity* nodeEntity{};
            AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, endpoint.GetNodeId());
            auto* node = nodeEntity ? AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity) : nullptr;
            if (node)
            {
                connectedNodes.emplace_back(node);
            }
        }

        return connectedNodes;
    }

    const Node* Node::GetNextExecutableNode() const
    {
        auto connectedNodes = GetConnectedNodesByType(SlotType::ExecutionOut);
        return connectedNodes.empty() ? nullptr : connectedNodes[0];
    }

    void Node::OnDatumChanged(const Datum* datum)
    {
        SlotId slotId;
        for (auto mapPair : m_inputIndexBySlotIndex)
        {
            if (&m_inputData[mapPair.second] == datum)
            {
                GetValidInputSlotId(mapPair.second, slotId);
                break;
            }
        }

        if (slotId.IsValid())
        {
            NodeNotificationsBus::Event(GetEntityId(), &NodeNotifications::OnInputChanged, slotId);
        }
    }

    const Datum* Node::GetInput(int index) const
    {
        return index >= 0 && index < m_inputData.size() ? &m_inputData[index] : nullptr;
    }

    const Datum* Node::GetInput(const SlotId& slotID) const
    {
        const Slot* unused(nullptr);
        int slotIndex(-1);
        return GetValidSlotIndex(slotID, unused, slotIndex)
            ? GetInputBySlotIndex(slotIndex)
            : nullptr;
    }

    AZ::EntityId Node::GetGraphEntityId() const
    {
        auto graph = GetGraph();
        return graph ? graph->GetEntityId() : AZ::EntityId();
    }


    const Datum* Node::GetInput(const Node& node, int index)
    {
        return node.GetInput(index);
    }

    const Datum* Node::GetInput(const Node& node, const SlotId slotID)
    {
        return node.GetInput(slotID);
    }

    const Datum* Node::GetInputBySlotIndex(int slotIndex) const
    {
        int inputDatumIndex(-1);
        return GetValidInputDataIndex(slotIndex, inputDatumIndex)
            ? &m_inputData[inputDatumIndex]
            : nullptr;
    }

    const Data::Type* Node::GetNonDatumType(int index) const
    {
        return index >= 0 && index < m_nonDatumTypes.size() ? &m_nonDatumTypes[index] : nullptr;
    }

    const Data::Type* Node::GetNonDatumType(const SlotId& slotID) const
    {
        const Slot* unused(nullptr);
        int slotIndex(-1);
        return GetValidSlotIndex(slotID, unused, slotIndex)
            ? GetNonDatumTypeBySlotIndex(slotIndex)
            : nullptr;
    }

    const Data::Type* Node::GetNonDatumTypeBySlotIndex(int slotIndex) const
    {
        int outputIndex(-1);
        return GetValidNonDatumTypeIndex(slotIndex, outputIndex)
            ? &m_nonDatumTypes[outputIndex]
            : nullptr;
    }

    bool Node::GetValidNonDatumTypeIndex(const int slotIndex, int& outputDatumIndexOut) const
    {
        auto iter = m_nonDatumTypeIndexBySlotIndex.find(slotIndex);
        if (iter != m_nonDatumTypeIndexBySlotIndex.end())
        {
            outputDatumIndexOut = iter->second;
            return true;
        }

        return false;
    }

    Datum* Node::ModInput(Node& node, int index)
    {
        return node.ModInput(index);
    }

    Datum* Node::ModInput(Node& node, const SlotId slotID)
    {
        return node.ModInput(slotID);
    }

    Datum* Node::ModInput(int index)
    {
        return const_cast<Datum*>(GetInput(index));
    }

    Datum* Node::ModInput(const SlotId& slotID)
    {
        return const_cast<Datum*>(GetInput(slotID));
    }

    bool Node::GetValidSlotIndex(const SlotId& slotID, const Slot*& slotOut, int& slotIndexOut) const
    {
        auto iter = m_slotContainer.m_slotIdSlotMap.find(slotID);
        if (iter != m_slotContainer.m_slotIdSlotMap.end())
        {
            slotOut = &m_slotContainer.m_slots[iter->second];
            slotIndexOut = iter->second;
            return true;
        }

        return false;
    }

    bool Node::GetValidInputSlotId(int inputIndex, SlotId& slotId) const
    {
        int slotIndex(-1);

        for (auto& mapPair : m_inputIndexBySlotIndex)
        {
            if (mapPair.second == inputIndex)
            {
                slotIndex = mapPair.first;
                break;
            }
        }

        for (auto& mapPair : m_slotContainer.m_slotIdSlotMap)
        {
            if (mapPair.second == slotIndex)
            {
                slotId = mapPair.first;
                break;
            }
        }

        return slotId.IsValid();
    }

    bool Node::GetValidInputDataIndex(const int slotIndex, int& inputDatumIndexOut) const
    {
        auto iter = m_inputIndexBySlotIndex.find(slotIndex);
        if (iter != m_inputIndexBySlotIndex.end())
        {
            inputDatumIndexOut = iter->second;
            return true;
        }

        return false;
    }

    bool Node::IsConnected(const Slot& slot) const
    {
        AZStd::vector<Endpoint> connectedEndpoints;
        GraphRequestBus::EventResult(connectedEndpoints, m_graphId, &GraphRequests::GetConnectedEndpoints, Endpoint{ GetEntityId(), slot.GetId() });
        return !connectedEndpoints.empty();
    }

    AZStd::vector<AZStd::pair<const Node*, const SlotId>> Node::GetConnectedNodes(const Slot& slot) const
    {
        AZStd::vector<AZStd::pair<const Node*, const SlotId>> connectedNodes;
        AZStd::vector<Endpoint> connectedEndpoints;
        GraphRequestBus::EventResult(connectedEndpoints, m_graphId, &GraphRequests::GetConnectedEndpoints, Endpoint{ GetEntityId(), slot.GetId() });
        Graph* graph = GetGraph();

        for (const Endpoint& endpoint : connectedEndpoints)
        {
            connectedNodes.emplace_back(AZStd::make_pair(graph->GetNode(endpoint.GetNodeId()), endpoint.GetSlotId()));
        }
        return connectedNodes;
    }

    AZStd::vector<AZStd::pair<Node*, const SlotId>> Node::ModConnectedNodes(const Slot& slot) const
    {
        AZStd::vector<AZStd::pair<Node*, const SlotId>> connectedNodes;
        AZStd::vector<Endpoint> connectedEndpoints;
        GraphRequestBus::EventResult(connectedEndpoints, m_graphId, &GraphRequests::GetConnectedEndpoints, Endpoint{ GetEntityId(), slot.GetId() });
        Graph* graph = GetGraph();

        for (const Endpoint& endpoint : connectedEndpoints)
        {
            connectedNodes.emplace_back(AZStd::make_pair(graph->GetNode(endpoint.GetNodeId()), endpoint.GetSlotId()));
        }
        return connectedNodes;
    }

    void Node::OnInputChanged(Node& node, const Datum& input, const SlotId& slotID)
    {
        node.OnInputChanged(input, slotID);
        LogNotificationBus::Event(node.GetGraphId(), &LogNotifications::OnNodeInputChanged, node.GetNodeName(), input.ToString(), node.GetSlot(slotID)->GetName());
    }

    void Node::PushOutput(const Datum& output, const Slot& slot) const
    {
        ForEachConnectedNode
            ( slot
            , [&output](Node& node, const SlotId& slotID)
            {
                node.SetInput(output, slotID);
            });

    }

    void Node::SetInput(const Datum& newInput, const SlotId& id)
    {
        const Slot* slot(nullptr);
        int slotIndex(-1);
        if (GetValidSlotIndex(id, slot, slotIndex))
        {
            int inputDatumIndex(-1);
            if (GetValidInputDataIndex(slotIndex, inputDatumIndex))
            {
                Datum& input = m_inputData[inputDatumIndex];
                WriteInput(input, newInput);
                OnInputChanged(input, id);
            }
        }
    }

    void Node::SetInput(Node& node, const SlotId& id, const Datum& input)
    {
        node.SetInput(input, id);
    }

    void Node::WriteInput(Datum& destination, const Datum& source)
    {
        destination = source;
    }
}
