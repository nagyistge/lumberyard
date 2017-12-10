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

#include <AzCore/std/containers/unordered_set.h>

#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/Connections/ConnectionFilters/ConnectionFilterBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>

namespace GraphCanvas
{
    enum class ConnectionFilterType : AZ::u32
    {
        Include = 0,
        Exclude,
        Invalid
    };
    
    class SlotTypeFilter
        : public ConnectionFilter
    {
        friend class SlotConnectionFilterComponent;
    public:
        AZ_RTTI(SlotTypeFilter, "{210FB521-041E-4932-BC7F-C91079125F68}", ConnectionFilter);
        AZ_CLASS_ALLOCATOR(SlotTypeFilter, AZ::SystemAllocator, 0);

        SlotTypeFilter()
            : m_filterType(ConnectionFilterType::Invalid)
        {
        }

        SlotTypeFilter(ConnectionFilterType filterType)
            : m_filterType(filterType)
        {
        }
        
        void AddSlotType(SlotType slotType)
        {
            m_slotTypes.insert(slotType);
        }
        
        bool CanConnectWith(const AZ::EntityId& slotId, Connectability& connectability) const override
        {
            SlotType connectingSlotType = SlotTypes::Invalid;
            SlotRequestBus::EventResult(connectingSlotType, slotId, &SlotRequests::GetSlotType);
            AZ_Assert(connectingSlotType != SlotGroups::Invalid, "Slot %s is in an invalid slot group. Connections to it are disabled", slotId.ToString().c_str());
            
            bool canConnect = false;
            
            if (connectingSlotType != SlotTypes::Invalid)
            {            
                bool isInFilter = m_slotTypes.count(connectingSlotType) != 0;
                
                switch (m_filterType)
                {
                case ConnectionFilterType::Include:
                    canConnect = isInFilter;
                    break;
                case ConnectionFilterType::Exclude:
                    canConnect = !isInFilter;
                    break;
                }
                
                if (!canConnect)
                {
                    connectability.status = Connectability::NotConnectable;
                    connectability.details = AZStd::string::format("Slot Type %d not allowed by filter on Slot %s.", connectingSlotType, slotId.ToString().c_str());
                }
            }
            else
            {
                connectability.status = Connectability::NotConnectable;
                connectability.details = "Invalid Slot Type given for comparison.";
            }
            
            return canConnect;
        }
        
    private:
    
        AZStd::unordered_set<SlotType> m_slotTypes;
    
        ConnectionFilterType m_filterType;
    };
    
    class ConnectionTypeFilter
        : public ConnectionFilter
    {
        friend class SlotConnectionFilterComponent;
    public:
        AZ_RTTI(ConnectionTypeFilter, "{57D65203-51AB-47A8-A7D2-248AFF92E058}", ConnectionFilter);
        AZ_CLASS_ALLOCATOR(ConnectionTypeFilter, AZ::SystemAllocator, 0);

        ConnectionTypeFilter()
            : m_filterType(ConnectionFilterType::Invalid)
        {
        }

        ConnectionTypeFilter(ConnectionFilterType filterType)
            : m_filterType(filterType)
        {
        }
        
        void AddConnectionType(ConnectionType connectionType)
        {
            m_connectionTypes.insert(connectionType);
        }
        
        bool CanConnectWith(const AZ::EntityId& slotId, Connectability& connectability) const override
        {
            ConnectionType connectionType = ConnectionType::CT_Invalid;
            SlotRequestBus::EventResult(connectionType, slotId, &SlotRequests::GetConnectionType);
            AZ_Assert(connectionType != ConnectionType::CT_Invalid, "Slot %s is in an invalid slot group. Connections to it are disabled", slotId.ToString().c_str())
            
            bool canConnect = false;
            
            if (connectionType != ConnectionType::CT_Invalid)
            {            
                bool isInFilter = m_connectionTypes.count(connectionType) != 0;
                
                switch (m_filterType)
                {
                case ConnectionFilterType::Include:
                    canConnect = isInFilter;
                    break;
                case ConnectionFilterType::Exclude:
                    canConnect = !isInFilter;
                    break;
                }
                
                if (!canConnect)
                {
                    connectability.status = Connectability::NotConnectable;
                    connectability.details = AZStd::string::format("Slot Type %d not allowed by filter on Slot %s.", connectionType, slotId.ToString().c_str());
                }
            }
            else
            {
                connectability.status = Connectability::NotConnectable;
                connectability.details = "Invalid Slot Type given for comparison.";
            }
            
            return canConnect;
        }
        
    private:
   
        AZStd::unordered_set<ConnectionType> m_connectionTypes;
        ConnectionFilterType m_filterType;
    };
}