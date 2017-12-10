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

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace GraphCanvas
{
    class Connection;

    class SlotComponent
        : public AZ::Component
        , public SlotRequestBus::Handler
        , public SceneMemberRequestBus::Handler
        , public SceneMemberNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(SlotComponent, "{EACFC8FB-C75B-4ABA-988D-89C964B9A4E4}");
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        static void Reflect(AZ::ReflectContext* context);
        static AZ::Entity* CreateCoreSlotEntity();

        SlotComponent();
        explicit SlotComponent(const SlotType& slotType);
        SlotComponent(const SlotType& slotType, const SlotConfiguration& slotConfiguration);
        ~SlotComponent() override = default;

        // AZ::Component
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(kSlotServiceProviderId);
            provided.push_back(AZ_CRC("GraphCanvas_SceneMemberService", 0xe9759a2d));
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        void Activate() override;
        void Deactivate() override;
        ////

        // SceneMemberRequestBus
        void SetScene(const AZ::EntityId& sceneId) override;
        void ClearScene(const AZ::EntityId& oldSceneId) override;

        AZ::EntityId GetScene() const override;
        ////

        // SceneMemberNotificationBus
        void OnSceneSet(const AZ::EntityId&) override;
        void OnSceneReady() override;
        ////

        // SlotRequestBus
        const AZ::EntityId& GetNode() const override;
        void SetNode(const AZ::EntityId&) override;

        const AZStd::string& GetName() const  override { return m_slotConfiguration.m_name.m_fallback; }
        void SetName(const AZStd::string& name) override;

        TranslationKeyedString GetTranslationKeyedName() const override { return m_slotConfiguration.m_name; }
        void SetTranslationKeyedName(const TranslationKeyedString&) override;

        const AZStd::string& GetTooltip() const override { return m_slotConfiguration.m_tooltip.m_fallback; }
        void SetTooltip(const AZStd::string& tooltip)  override;

        TranslationKeyedString GetTranslationKeyedTooltip() const override { return m_slotConfiguration.m_tooltip; }
        void SetTranslationKeyedTooltip(const TranslationKeyedString&) override;

        void DisplayProposedConnection(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;
        void RemoveProposedConnection(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;

        void AddConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;
        void RemoveConnectionId(const AZ::EntityId& connectionId, const Endpoint& endpoint) override;

        ConnectionType GetConnectionType() const override { return m_slotConfiguration.m_connectionType; }
        SlotGroup GetSlotGroup() const override { return m_slotConfiguration.m_slotGroup; }
        SlotType GetSlotType() const override { return m_slotType; }

        bool CanAcceptConnection(const Endpoint& endpoint);
        AZ::EntityId CreateConnection() const override;
        AZ::EntityId CreateConnectionWithEndpoint(const Endpoint& endpoint) const;

        AZStd::any* GetUserData() override;

        bool HasConnections() const override;

        AZ::EntityId GetLastConnection() const override;
        AZStd::vector<AZ::EntityId> GetConnections() const override;

        void SetConnectionDisplayState(ConnectionDisplayState displayState) override;
        void ClearConnections() override;
        ////

    protected:

        virtual AZ::Entity* ConstructConnectionEntity(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const;

        void FinalizeDisplay();
        virtual void OnFinalizeDisplay();

        //! The Node this Slot belongs to.
        AZ::EntityId m_nodeId;

        SlotType          m_slotType;
        SlotConfiguration m_slotConfiguration;

        //! Keeps track of connections to this slot
        AZStd::vector<AZ::EntityId> m_connections;

        //! Stores custom user data for this slot
        AZStd::any m_userData;
    };
}
