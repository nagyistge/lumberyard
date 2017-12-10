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

#include "Core.h"

#include <AzCore/std/string/string_view.h>
#include <AzCore/EBus/EBus.h>
#include <ScriptCanvas/Data/Data.h>

namespace ScriptCanvas
{
    class Datum;
    class Slot;

    enum class SlotType : AZ::s32;

    class NodeRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ID;

        virtual Slot* GetSlot(const SlotId& slotId) const = 0;

        //! Gets all of the slots on the node.
        //! Name is funky to avoid a mismatch with typing with another function
        //! that returns a better version of this information that cannot be used with
        //! EBuses because of references.
        virtual AZStd::vector<const Slot*> GetAllSlots() const = 0;

        //! Retrieves a slot id that matches the supplied name
        //! There can be multiple slots with the same name on a node
        //! Therefore this should only be used when a slot's name is unique within the node
        virtual SlotId GetSlotId(AZStd::string_view slotName) const = 0;

        //! Retrieves a slot id that matches the supplied name and the supplied slot type
        virtual SlotId GetSlotIdByType(AZStd::string_view slotName, SlotType slotType) const = 0;

        //! Retrieves all slot ids for slots with the specific name
        virtual AZStd::vector<SlotId> GetSlotIds(AZStd::string_view slotName) const = 0;
        
        virtual const AZ::EntityId& GetGraphId() const = 0;

        //! Determines whether the slot on this node with the specified slot id can accept values of the specified type
        virtual bool SlotAcceptsType(const SlotId&, const Data::Type&) const = 0;

        //! Gets the input for the given SlotId
        virtual Data::Type GetSlotDataType(const SlotId& slotId) const = 0; 
        
        virtual bool IsSlotValidStorage(const SlotId& slotId) const = 0;
    };

    using NodeRequestBus = AZ::EBus<NodeRequests>;

    class LogNotifications : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

//        virtual void OnGraphActivate(const AZ::Uuid& editGraphId, const AZ::Uuid& runtimeGraphId) {}
        virtual void OnNodeInputChanged(const AZStd::string& sourceNodeName, const AZStd::string& objectName, const AZStd::string& slotName) {}
        virtual void OnNodeSignalOutput(const AZStd::string& sourceNodeName, const AZStd::string& targetNodeName, const AZStd::string& slotName) {}
        virtual void OnNodeSignalInput(const AZ::Uuid& nodeId, const AZStd::string& name, const AZStd::string& slotName) {}

        virtual void LogMessage(const AZStd::string& log) {}
    };

    using LogNotificationBus = AZ::EBus<LogNotifications>;

    class NodeNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnInputChanged(const SlotId& slotId) = 0;
    };

    using NodeNotificationsBus = AZ::EBus<NodeNotifications>;

    class EditorNodeRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ID;

        //! Get the object from the specified slot.
        virtual const Datum* GetInput(const SlotId& slotId) const = 0;
        virtual Datum* ModInput(const SlotId& slotId) = 0;
        virtual AZ::EntityId GetGraphEntityId() const = 0;
    };

    using EditorNodeRequestBus = AZ::EBus<EditorNodeRequests>;
}