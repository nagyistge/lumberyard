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

#include "EBusEventHandler.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Utils.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            const char* EBusEventHandler::c_busIdName = "Source";
            const char* EBusEventHandler::c_busIdTooltip = "ID used to connect on a specific Event address";

            static bool EBusEventHandlerVersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
            {
                if (rootElement.GetVersion() <= 2)
                {
                    // Renamed "BusId" to "Source"

                    auto slotContainerElements = AZ::Utils::FindDescendantElements(serializeContext, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("BaseClass1", 0xd4925735), AZ_CRC("Slots", 0xc87435d0)});
                    if (!slotContainerElements.empty())
                    {
                        // This contains the pair elements stored in the SlotNameMap(Name -> Index)
                        auto slotNameToIndexElements = AZ::Utils::FindDescendantElements(serializeContext, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slotNameSlotMap", 0x69040afb), AZ_CRC("element", 0x41405e39)});

                        // This contains the Slot class elements stored in Node class
                        auto slotElements = AZ::Utils::FindDescendantElements(serializeContext, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slots", 0x84838ab4), AZ_CRC("element", 0x41405e39)});
                        
                        AZStd::string slotName;
                        for (auto slotNameToIndexElement : slotNameToIndexElements)
                        {
                            int index = -1;
                            if (slotNameToIndexElement->GetChildData(AZ_CRC("value1", 0xa2756c5a), slotName) && slotName == "BusId" && slotNameToIndexElement->GetChildData(AZ_CRC("value2", 0x3b7c3de0), index))
                            {
                                SlotType slotType;
                                if (slotElements.size() > static_cast<size_t>(index) && slotElements[index]->GetChildData(AZ_CRC("type", 0x8cde5729), slotType) && slotType == SlotType::DataIn)
                                {
                                    AZ::SerializeContext::DataElementNode& slotElement = *slotElements[index];

                                    slotName = "Source";
                                    slotElement.RemoveElementByName(AZ_CRC("slotName", 0x817c3511));
                                    if (slotElement.AddElementWithData(serializeContext, "slotName", slotName) == -1)
                                    {
                                        AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<EBusEventHandler>::Name());
                                        return false;
                                    }

                                    if (slotNameToIndexElement->AddElementWithData(serializeContext, "value1", slotName) == -1)
                                    {
                                        AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<EBusEventHandler>::Name());
                                        return false;
                                    }
                                }
                            }
                        }
                    }
                }

                if (rootElement.GetVersion() <= 1)
                {
                    // Changed:
                    //  using Events = AZStd::vector<EBusEventEntry>;
                    //  to:
                    //  using EventMap = AZStd::unordered_map<AZ::Crc32, EBusEventEntry>;

                    auto ebusEventEntryElements = AZ::Utils::FindDescendantElements(serializeContext, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("m_events", 0x191405b4), AZ_CRC("element", 0x41405e39)});

                    EBusEventHandler::EventMap eventMap;
                    for (AZ::SerializeContext::DataElementNode* ebusEventEntryElement : ebusEventEntryElements)
                    {
                        EBusEventEntry eventEntry;
                        if (!ebusEventEntryElement->GetDataHierarchy(serializeContext, eventEntry))
                        {
                            return false;
                        }
                        AZ::Crc32 key = AZ::Crc32(eventEntry.m_eventName.c_str());
                        AZ_Assert(eventMap.find(key) == eventMap.end(), "Duplicated event found while converting EBusEventHandler from version 1 to 2.");
                        eventMap[key] = eventEntry;
                    }

                    rootElement.RemoveElementByName(AZ_CRC("m_events", 0x191405b4));
                    if (rootElement.AddElementWithData(serializeContext, "m_eventMap", eventMap) == -1)
                    {
                        return false;
                    }

                    return true;
                }
                
                if (rootElement.GetVersion() == 0)
                {
                    auto slotContainerElements = AZ::Utils::FindDescendantElements(serializeContext, rootElement, AZStd::vector<AZ::Crc32>{AZ_CRC("BaseClass1", 0xd4925735), AZ_CRC("Slots", 0xc87435d0)});
                    if (!slotContainerElements.empty())
                    {
                        // This contains the pair elements stored in the SlotNameMap(Name -> Index)
                        auto slotNameToIndexElements = AZ::Utils::FindDescendantElements(serializeContext, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slotNameSlotMap", 0x69040afb), AZ_CRC("element", 0x41405e39)});
                        // This contains the Slot class elements stored in Node class
                        auto slotElements = AZ::Utils::FindDescendantElements(serializeContext, *slotContainerElements.front(), AZStd::vector<AZ::Crc32>{AZ_CRC("m_slots", 0x84838ab4), AZ_CRC("element", 0x41405e39)});

                        for (auto slotNameToIndexElement : slotNameToIndexElements)
                        {
                            AZStd::string slotName;
                            int index = -1;
                            if (slotNameToIndexElement->GetChildData(AZ_CRC("value1", 0xa2756c5a), slotName) && slotName == "EntityId" && slotNameToIndexElement->GetChildData(AZ_CRC("value2", 0x3b7c3de0), index))
                            {
                                SlotType slotType;
                                if (slotElements.size() > static_cast<size_t>(index) && slotElements[index]->GetChildData(AZ_CRC("type", 0x8cde5729), slotType) && slotType == SlotType::DataIn)
                                {
                                    AZ::SerializeContext::DataElementNode& slotElement = *slotElements[index];

                                    slotName = "BusId";
                                    slotElement.RemoveElementByName(AZ_CRC("slotName", 0x817c3511));
                                    if (slotElement.AddElementWithData(serializeContext, "slotName", slotName) == -1)
                                    {
                                        AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<EBusEventHandler>::Name());
                                        return false;
                                    }

                                    if (slotNameToIndexElement->AddElementWithData(serializeContext, "value1", slotName) == -1)
                                    {
                                        AZ_Assert(false, "Version Converter failed. Graph contained %s node is in an invalid state", AZ::AzTypeInfo<EBusEventHandler>::Name());
                                        return false;
                                    }
                                }
                            }
                        }
                    }
                }

                return true;
            }

            EBusEventHandler::~EBusEventHandler()
            {
                if (m_ebus)
                {
                    m_ebus->m_destroyHandler->Invoke(m_handler);
                }
            }

            void EBusEventHandler::OnInit()
            {
                if (m_ebus && m_handler)
                {
                    for (int eventIndex(0); eventIndex < m_handler->GetEvents().size(); ++eventIndex)
                    {
                        InitializeEvent(eventIndex);
                    }
                }
            }

            void EBusEventHandler::OnActivate()
            {
                if (m_ebus && m_handler)
                {
                    Connect();
                }
            }

            void EBusEventHandler::OnDeactivate()
            {
                Disconnect();
            }

            void EBusEventHandler::Connect()
            {
                AZ_Assert(m_handler, "null handler");

                AZ::EntityId connectToEntityId;
                AZ::BehaviorValueParameter busIdParameter;
                busIdParameter.Set(m_ebus->m_idParam);
                const AZ::Uuid busIdType = m_ebus->m_idParam.m_typeId;

                const Datum* busIdDatum = IsIDRequired() ? GetInput(GetSlotId(c_busIdName)) : nullptr;
                if (busIdDatum && !busIdDatum->Empty())
                {
                    if (busIdDatum->IS_A(Data::FromAZType(busIdType)) || busIdDatum->IsConvertibleTo(Data::FromAZType(busIdType)))
                    {
                        auto busIdOutcome = busIdDatum->ToBehaviorValueParameter(m_ebus->m_idParam);
                        if (busIdOutcome.IsSuccess())
                        {
                            busIdParameter = busIdOutcome.TakeValue();
                        }
                    }

                    if (busIdType == azrtti_typeid<AZ::EntityId>())
                    {
                        if (auto busEntityId = busIdDatum->GetAs<AZ::EntityId>())
                        {
                            if (!busEntityId->IsValid() || *busEntityId == ScriptCanvas::SelfReferenceId)
                            {
                                const Graph* graph = GetGraph();
                                connectToEntityId = graph->GetEntity()->GetId();
                                busIdParameter.m_value = &connectToEntityId;
                            }
                        }
                    }
                    
                }
                if (!IsIDRequired() || busIdParameter.GetValueAddress())
                {
                    // Ensure we disconnect if this bus is already connected, this could happen if a different bus Id is provided
                    // and this node is connected through the Connect slot.
                    m_handler->Disconnect();

                    AZ_VerifyError("Script Canvas", m_handler->Connect(&busIdParameter),
                        "Unable to connect to EBus with BusIdType %s. The BusIdType of the EBus (%s) does not match the BusIdType stored in the Datum",
                        busIdType.ToString<AZStd::string>().data(), m_ebusName.data());
                }
            }

            void EBusEventHandler::Disconnect()
            {
                AZ_Assert(m_handler, "null handler");
                m_handler->Disconnect();
            }

            const AZ::BehaviorEBusHandler::BusForwarderEvent* GetEventHandlerFromName(int& eventIndexOut, AZ::BehaviorEBusHandler& handler, const AZStd::string& eventName)
            {
                const AZ::BehaviorEBusHandler::EventArray& events(handler.GetEvents());
                int eventIndex(0);

                for (auto& event : events)
                {
                    if (!(eventName.compare(event.m_name)))
                    {
                        eventIndexOut = eventIndex;
                        return &event;
                    }

                    ++eventIndex;
                }

                return nullptr;
            }

            AZStd::vector<SlotId> EBusEventHandler::GetNonEventSlotIds() const
            {
                AZStd::vector<SlotId> nonEventSlotIds;

                for (const auto& slot : m_slotContainer.m_slots)
                {
                    const SlotId slotId = slot.GetId();

                    if (!IsEventSlotId(slotId))
                    {
                        nonEventSlotIds.push_back(slotId);
                    }
                }

                return nonEventSlotIds;
            }

            bool EBusEventHandler::IsEventSlotId(const SlotId& slotId) const
            {
                for (const auto& eventItem : m_eventMap)
                {
                    const auto& event = eventItem.second;
                    if (slotId == event.m_eventSlotId
                        || slotId == event.m_resultSlotId
                        || event.m_parameterSlotIds.end() != AZStd::find_if(event.m_parameterSlotIds.begin(), event.m_parameterSlotIds.end(), [&slotId](const SlotId& candidate) { return slotId == candidate; }))
                    {
                        return true;
                    }
                }

                return false;
            }

            const EBusEventEntry* EBusEventHandler::FindEvent(const AZStd::string& name) const
            {
                AZ::Crc32 key = AZ::Crc32(name.c_str());
                auto iter = m_eventMap.find(key);
                return (iter != m_eventMap.end()) ? &iter->second : nullptr;
            }

            bool EBusEventHandler::CreateHandler(AZStd::string_view ebusName)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);

                if (m_handler)
                {
                    AZ_Assert(false, "Handler %s is already initialized", ebusName.data());
                    return true;
                }

                AZ::BehaviorContext* behaviorContext = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
                if (!behaviorContext)
                {
                    AZ_Error("Script Canvas", false, "Can't create the ebus handler without a behavior context!");
                    return false;
                }

                const auto& ebusIterator = behaviorContext->m_ebuses.find(ebusName);
                if (ebusIterator == behaviorContext->m_ebuses.end())
                {
                    AZ_Error("Script Canvas", false, "No ebus by name of %s in the behavior context!", ebusName.data());
                    return false;
                }

                m_ebus = ebusIterator->second;
                AZ_Assert(m_ebus, "ebus == nullptr in %s", ebusName.data());

                if (!m_ebus->m_createHandler)
                {
                    AZ_Error("Script Canvas", false, "The ebus %s has no create handler!", ebusName.data());
                }

                if (!m_ebus->m_destroyHandler)
                {
                    AZ_Error("Script Canvas", false, "The ebus %s has no destroy handler!", ebusName.data());
                }

                AZ_Verify(m_ebus->m_createHandler->InvokeResult(m_handler), "Ebus handler creation failed %s", ebusName.data());
                AZ_Assert(m_handler, "Ebus create handler failed %s", ebusName.data());

                return true;
            }

            void EBusEventHandler::InitializeBus(const AZStd::string& ebusName)
            {
                if (!CreateHandler(ebusName))
                {
                    return;
                }

                const bool wasConfigured = IsConfigured();

                if (!wasConfigured)
                {
                    if (IsIDRequired())
                    {
                        const auto busToolTip = AZStd::string::format("%s (Type: %s)", c_busIdTooltip, m_ebus->m_idParam.m_name);
                        const AZ::TypeId& busId = m_ebus->m_idParam.m_typeId;
                        if (busId == azrtti_typeid<AZ::EntityId>())
                        {
                            AddInputDatumSlot(c_busIdName, busToolTip, Datum::eOriginality::Copy, ScriptCanvas::SelfReferenceId);
                        }
                        else
                        {
                            Data::Type busIdType(AZ::BehaviorContextHelper::IsStringParameter(m_ebus->m_idParam) ? Data::Type::String() : Data::FromAZType(busId));
                            AddInputDatumSlot(c_busIdName, busToolTip, busIdType, Datum::eOriginality::Copy);
                        }
                    }
                }

                m_ebusName = m_ebus->m_name;

                const AZ::BehaviorEBusHandler::EventArray& events = m_handler->GetEvents();
                for (int eventIndex(0); eventIndex < events.size(); ++eventIndex)
                {
                    InitializeEvent(eventIndex);
                }
            }

            void EBusEventHandler::InitializeEvent(int eventIndex)
            {
                if (!m_handler)
                {
                    AZ_Error("Script Canvas", false, "BehaviorEBusHandler is nullptr. Cannot initialize event");
                    return;
                }

                const AZ::BehaviorEBusHandler::EventArray& events = m_handler->GetEvents();
                if (eventIndex >= events.size())
                {
                    AZ_Error("Script Canvas", false, "Event index %d is out of range. Total number of events: %zu", eventIndex, events.size());
                    return;
                }

                const AZ::BehaviorEBusHandler::BusForwarderEvent& event = events[eventIndex];
                AZ_Assert(!event.m_parameters.empty(), "No parameters in event!");
                m_handler->InstallGenericHook(events[eventIndex].m_name, &OnEventGenericHook, this);

                if (m_eventMap.find(AZ::Crc32(event.m_name)) != m_eventMap.end())
                {
                    // Event is already associated with the EBusEventHandler
                    return;
                }

                const size_t sentinel(event.m_parameters.size());

                EBusEventEntry ebusEventEntry;
                ebusEventEntry.m_numExpectedArguments = static_cast<int>(sentinel - AZ::eBehaviorBusForwarderEventIndices::ParameterFirst);

                if (event.HasResult())
                {
                    const AZ::BehaviorParameter& argument(event.m_parameters[AZ::eBehaviorBusForwarderEventIndices::Result]);
                    Data::Type inputType(AZ::BehaviorContextHelper::IsStringParameter(argument) ? Data::Type::String() : Data::FromBehaviorContextType(argument.m_typeId));
                    const char* argumentTypeName = Data::GetName(inputType);
                    const AZStd::string argName(AZStd::string::format("Result: %s", argumentTypeName));
                    ebusEventEntry.m_resultSlotId = AddInputDatumSlot(argName.c_str(), "", AZStd::move(inputType), Datum::eOriginality::Copy, false);
                }

                for (size_t parameterIndex(AZ::eBehaviorBusForwarderEventIndices::ParameterFirst)
                    ; parameterIndex < sentinel
                    ; ++parameterIndex)
                {
                    const AZ::BehaviorParameter& parameter(event.m_parameters[parameterIndex]);
                    Data::Type outputType(AZ::BehaviorContextHelper::IsStringParameter(parameter) ? Data::Type::String() : Data::FromBehaviorContextType(parameter.m_typeId));
                    // multiple outs will need out value names
                    AZStd::string argName = event.m_metadataParameters[parameterIndex].m_name;
                    if (argName.empty())
                    {
                        argName = AZStd::string::format("%s", Data::GetName(outputType));
                    }
                    const AZStd::string& argToolTip = event.m_metadataParameters[parameterIndex].m_toolTip;
                    ebusEventEntry.m_parameterSlotIds.push_back(AddOutputTypeSlot(argName, argToolTip, AZStd::move(outputType), OutputStorage::Required, false));
                }

                const AZStd::string eventID(AZStd::string::format("Handle:%s", event.m_name));
                // \todo this should be removed... the handling should all be considered "inside" the execution of this node
                ebusEventEntry.m_eventSlotId = AddSlot(eventID.c_str(), "", SlotType::ExecutionOut);
                AZ_Assert(ebusEventEntry.m_eventSlotId.IsValid(), "the event execution out slot must be valid");
                ebusEventEntry.m_eventName = event.m_name;

                m_eventMap[AZ::Crc32(event.m_name)] = ebusEventEntry;
            }

            bool EBusEventHandler::IsEventConnected(const EBusEventEntry& entry) const
            {
                return Node::IsConnected(*GetSlot(entry.m_eventSlotId))
                    || (entry.m_resultSlotId.IsValid() && Node::IsConnected(*GetSlot(entry.m_resultSlotId)))
                    || AZStd::any_of(entry.m_parameterSlotIds.begin(), entry.m_parameterSlotIds.end(), [this](const SlotId& id) { return this->Node::IsConnected(*GetSlot(id)); });
            }

            bool EBusEventHandler::IsValid() const
            {
                return !m_eventMap.empty();
            }

            void EBusEventHandler::OnEvent(const char* eventName, const int eventIndex, AZ::BehaviorValueParameter* result, const int numParameters, AZ::BehaviorValueParameter* parameters)
            {
                SCRIPTCANVAS_RETURN_IF_ERROR_STATE((*this));

                auto iter = m_eventMap.find(AZ::Crc32(eventName));
                if (iter == m_eventMap.end())
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Invalid event index in Ebus handler");
                    return;
                }

                EBusEventEntry& ebusEventEntry = iter->second;

                if (!IsEventConnected(ebusEventEntry))
                {
                    // this is fine, it's optional to implement the calls
                    return;
                }

                ebusEventEntry.m_resultEvaluated = !ebusEventEntry.IsExpectingResult();

                AZ_Assert(ebusEventEntry.m_eventName.compare(eventName) == 0, "Wrong event handled by this EBusEventHandler! received %s, expected %s", eventName, ebusEventEntry.m_eventName.c_str());
                AZ_Assert(numParameters == ebusEventEntry.m_numExpectedArguments, "Wrong number of events passed into EBusEventHandler %s", eventName);

                // route my parameters to the connect nodes input
                AZ_Assert(ebusEventEntry.m_parameterSlotIds.size() == numParameters, "Node %s-%s Num parameters = %d, but num output slots = %d", m_ebusName.c_str(), ebusEventEntry.m_eventName.c_str(), numParameters, ebusEventEntry.m_parameterSlotIds.size());

                for (int parameterIndex(0); parameterIndex < numParameters; ++parameterIndex)
                {
                    const Slot* slot = GetSlot(ebusEventEntry.m_parameterSlotIds[parameterIndex]);
                    const auto& value = *(parameters + parameterIndex);
                    const Datum input = Datum::CreateFromBehaviorContextValue(value);

                    auto callable = [this, &input](Node& node, const SlotId& slotID)
                    {
                        SetInput(node, slotID, input);
                    };

                    ForEachConnectedNode(*slot, AZStd::move(callable));
                }

                // now, this should pass execution off to the nodes that will push their output into this result input
                SignalOutput(ebusEventEntry.m_eventSlotId);

                // route executed nodes output to my input, and my input to the result
                if (ebusEventEntry.IsExpectingResult())
                {
                    if (result)
                    {
                        if (const Datum* resultInput = GetInput(ebusEventEntry.m_resultSlotId))
                        {
                            ebusEventEntry.m_resultEvaluated = resultInput->ToBehaviorContext(*result);
                            AZ_Warning("Script Canvas", ebusEventEntry.m_resultEvaluated, "Script Canvas failed to write a value back to the caller!");
                        }
                        else
                        {
                            AZ_Warning("Script Canvas", false, "Script Canvas handler expecting a result, but had no ability to return it");
                        }
                    }
                    else
                    {
                        AZ_Warning("Script Canvas", false, "Script Canvas handler is expecting a result, but was called without expecting one!");
                    }
                }
                else
                {
                    AZ_Warning("Script Canvas", !result, "Script Canvas handler is not expecting a result, but was called expecting one!");
                }

                // route executed nodes output to my input, and my input to the result
                AZ_Warning("Script Canvas", (result != nullptr) == ebusEventEntry.IsExpectingResult(), "Node %s-%s mismatch between expecting a result and getting one!", m_ebusName.c_str(), ebusEventEntry.m_eventName.c_str());
                AZ_Warning("Script Canvas", ebusEventEntry.m_resultEvaluated, "Node %s-%s result not evaluated properly!", m_ebusName.c_str(), ebusEventEntry.m_eventName.c_str());
            }

            void EBusEventHandler::OnInputSignal(const SlotId& slotId)
            {
                SlotId connectSlot = EBusEventHandlerProperty::GetConnectSlotId(this);

                if (connectSlot == slotId)
                {
                    const Datum* busIdDatum = GetInput(GetSlotId(c_busIdName));
                    if (IsIDRequired() && (!busIdDatum || busIdDatum->Empty()))
                    {
                        SlotId failureSlot = EBusEventHandlerProperty::GetOnFailureSlotId(this);
                        SignalOutput(failureSlot);
                        SCRIPTCANVAS_REPORT_ERROR((*this), "In order to connect this node, a valid BusId must be provided.");
                        return;
                    }
                    else
                    {
                        Connect();
                        SlotId onConnectSlotId = EBusEventHandlerProperty::GetOnConnectedSlotId(this);
                        SignalOutput(onConnectSlotId);
                        return;
                    }
                }

                SlotId disconnectSlot = EBusEventHandlerProperty::GetDisconnectSlotId(this);
                if (disconnectSlot == slotId)
                {
                    Disconnect();

                    SlotId onDisconnectSlotId = EBusEventHandlerProperty::GetOnDisconnectedSlotId(this);
                    SignalOutput(onDisconnectSlotId);
                    return;
                }

            }

            void EBusEventHandler::OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters)
            {
                EBusEventHandler* handler(reinterpret_cast<EBusEventHandler*>(userData));
                handler->OnEvent(eventName, eventIndex, result, numParameters, parameters);
            }

            void EBusEventHandler::OnWriteEnd()
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_mutex);
                if (!m_ebus)
                {
                    CreateHandler(m_ebusName);
                }
            }

            void EBusEventEntry::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
                if (serialize)
                {
                    serialize->Class<EBusEventEntry>()
                        ->Version(0)
                        ->Field("m_eventName", &EBusEventEntry::m_eventName)
                        ->Field("m_eventSlotId", &EBusEventEntry::m_eventSlotId)
                        ->Field("m_resultSlotId", &EBusEventEntry::m_resultSlotId)
                        ->Field("m_parameterSlotIds", &EBusEventEntry::m_parameterSlotIds)
                        ->Field("m_numExpectedArguments", &EBusEventEntry::m_numExpectedArguments)
                        ->Field("m_resultEvaluated", &EBusEventEntry::m_resultEvaluated)
                        ;
                }
            }

        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Core/EBusEventHandler.generated.cpp>