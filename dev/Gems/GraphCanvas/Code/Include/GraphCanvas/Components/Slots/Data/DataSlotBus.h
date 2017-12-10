/*
* All or Portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
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

#include <qcolor.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>

namespace GraphCanvas
{
    class DataSlotConnectionPin;

    enum class DataSlotType
    {
        Unknown,

        // These are options that can be used on most DataSlots
        Value,
        Reference,

        // This is a special value that is used as a 'source' variable.
        // Rather then a set to a variable.
        Variable
    };

    class DataSlotRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual bool AssignVariable(const AZ::EntityId& variableSourceId) = 0;
        virtual AZ::EntityId GetVariableId() const = 0;

        virtual bool ConvertToReference() = 0;
        virtual bool CanConvertToReference() const = 0;

        virtual bool ConvertToValue() = 0;
        virtual bool CanConvertToValue() const = 0;

        virtual DataSlotType GetDataSlotType() const = 0;

        virtual const AZ::Uuid& GetDataTypeId() const = 0;
        virtual QColor GetDataColor() const = 0;
    };

    using DataSlotRequestBus = AZ::EBus<DataSlotRequests>;
    
    class DataSlotNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void OnVariableAssigned(const AZ::EntityId& variableId) { };
        virtual void OnDataSlotTypeChanged(const DataSlotType& dataSlotType) { };
    };
    
    using DataSlotNotificationBus = AZ::EBus<DataSlotNotifications>;

    class DataSlotLayoutRequests
        : public AZ::EBusTraits
    {
    public:
        // BusId here is the specific slot that we want to make requests to.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        
        virtual const DataSlotConnectionPin* GetConnectionPin() const = 0;

        virtual void UpdateDisplay() = 0;
    };

    using DataSlotLayoutRequestBus = AZ::EBus<DataSlotLayoutRequests>;

    //! Actions that occur from a DataSlot that require some interaction with the underlying data model.
    class DataSlotActionRequests : public AZ::EBusTraits
    {
    public:
        // The id here is the SceneId that the slot belongs to.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        using BusIdType = AZ::EntityId;

        //! Get the ScriptCanvas type name for the given AZ type
        virtual AZStd::string GetTypeString(const AZ::Uuid& typeId) = 0;
    };

    using DataSlotActionRequestBus = AZ::EBus<DataSlotActionRequests>;
}