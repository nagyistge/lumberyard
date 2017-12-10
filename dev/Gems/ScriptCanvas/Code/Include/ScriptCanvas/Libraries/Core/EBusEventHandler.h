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

#include <ScriptCanvas/CodeGen/CodeGen.h>

#include <Include/ScriptCanvas/Libraries/Core/EBusEventHandler.generated.h>

#include <AzCore/std/parallel/mutex.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Graph.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ
{
    class BehaviorEBus;
    class BehaviorEBusHandler;
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            struct EBusEventEntry
            {
                AZ_TYPE_INFO(EBusEventEntry, "{92A20C1B-A54A-4583-97DB-A894377ACE21}");

                bool IsExpectingResult() const 
                {
                    return m_resultSlotId.IsValid();
                }

                AZStd::string m_eventName;
                SlotId m_eventSlotId;
                SlotId m_resultSlotId;
                AZStd::vector<SlotId> m_parameterSlotIds;
                int m_numExpectedArguments = {};
                bool m_resultEvaluated = {};

                static void Reflect(AZ::ReflectContext* context);
            };

            class EBusEventHandler 
                : public Node
            {
            public:

                static const char* c_busIdName;
                static const char* c_busIdTooltip;

                using Events = AZStd::vector<EBusEventEntry>;
                using EventMap = AZStd::unordered_map<AZ::Crc32, EBusEventEntry>;

                ScriptCanvas_Node(EBusEventHandler,
                    ScriptCanvas_Node::Name("Event Handler", "Allows you to handle a event.")
                    ScriptCanvas_Node::Uuid("{33E12915-EFCA-4AA7-A188-D694DAD58980}")
                    ScriptCanvas_Node::Icon("Editor/Icons/ScriptCanvas/Bus.png")
                    ScriptCanvas_Node::EventHandler("SerializeContextEventHandlerDefault<EBusEventHandler>")
                    ScriptCanvas_Node::Version(2, EBusEventHandlerVersionConverter)
                    ScriptCanvas_Node::GraphEntryPoint(true)
                    ScriptCanvas_Node::EditAttributes(AZ::Script::Attributes::ExcludeFrom(AZ::Script::Attributes::ExcludeFlags::All))
                );

                EBusEventHandler() {}
                
                ~EBusEventHandler() override;

                void OnInit() override;
                
                void OnActivate() override;
                void OnDeactivate() override;

                void InitializeBus(const AZStd::string& ebusName);

                void InitializeEvent(int eventIndex);
                
                bool CreateHandler(AZStd::string_view ebusName);

                void Connect();
                
                void Disconnect();


                const EBusEventEntry* FindEvent(const AZStd::string& name) const;
                AZ_INLINE const char* GetEBusName() const { return m_ebusName.c_str(); }
                AZ_INLINE const EventMap& GetEvents() const { return m_eventMap; }
                AZStd::vector<SlotId> GetNonEventSlotIds() const;
                bool IsEventSlotId(const SlotId& slotId) const;

                bool IsEventConnected(const EBusEventEntry& entry) const;
                bool IsValid() const;
                
                AZ_INLINE bool IsIDRequired() const { return m_ebus ? !m_ebus->m_idParam.m_typeId.IsNull() : false; }

                void OnWriteEnd();

                AZStd::string GetDebugName() const override
                {
                    return AZStd::string::format("%s Handler", GetEBusName());
                }

                void Visit(NodeVisitor& visitor) const override { visitor.Visit(*this); }

            protected:
                AZ_INLINE bool IsConfigured() const { return !m_eventMap.empty(); }
                
                void OnEvent(const char* eventName, const int eventIndex, AZ::BehaviorValueParameter* result, const int numParameters, AZ::BehaviorValueParameter* parameters);

                void OnInputSignal(const SlotId&) override;

            private:
                EBusEventHandler(const EBusEventHandler&) = delete;
                static void OnEventGenericHook(void* userData, const char* eventName, int eventIndex, AZ::BehaviorValueParameter* result, int numParameters, AZ::BehaviorValueParameter* parameters);


                // Inputs
                ScriptCanvas_In(ScriptCanvas_In::Name("Connect", "Connect this event handler to the specified entity."));
                ScriptCanvas_In(ScriptCanvas_In::Name("Disconnect", "Disconnect this event handler."));

                // Outputs
                ScriptCanvas_Out(ScriptCanvas_Out::Name("OnConnected", "Signalled when a connection has taken place."));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("OnDisconnected", "Signalled when this event handler is disconnected."));
                ScriptCanvas_Out(ScriptCanvas_Out::Name("OnFailure", "Signalled when it is not possible to connect this handler."));
                                
                Property(EventMap, m_eventMap);
                Property(AZStd::string, m_ebusName);

                AZ::BehaviorEBusHandler* m_handler = nullptr;
                AZ::BehaviorEBus* m_ebus = nullptr;
                AZStd::recursive_mutex m_mutex; // post-serialization
            };
        } // namespace Core
    } // namespace Nodes
} // namespace ScriptCanvas