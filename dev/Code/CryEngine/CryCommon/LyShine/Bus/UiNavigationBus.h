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

#include <AzCore/Component/ComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiNavigationInterface
    : public AZ::ComponentBus
{
public: // types

    enum class NavigationMode
    {
        Automatic,
        Custom,
        None
    };

public: // member functions

    virtual ~UiNavigationInterface() {}

    //! Get the navigation mode
    virtual NavigationMode GetNavigationMode() = 0;

    //! Set the navigation mode
    virtual void SetNavigationMode(NavigationMode navigationMode) = 0;

    //! Get the entity to receive focus when up is pressed
    virtual AZ::EntityId GetOnUpEntity() = 0;

    //! Set the entity to receive focus when up is pressed
    virtual void SetOnUpEntity(AZ::EntityId entityId) = 0;

    //! Get the entity to receive focus when down is pressed
    virtual AZ::EntityId GetOnDownEntity() = 0;

    //! Set the entity to receive focus when down is pressed
    virtual void SetOnDownEntity(AZ::EntityId entityId) = 0;

    //! Get the entity to receive focus when left is pressed
    virtual AZ::EntityId GetOnLeftEntity() = 0;

    //! Set the entity to receive focus when left is pressed
    virtual void SetOnLeftEntity(AZ::EntityId entityId) = 0;

    //! Get the entity to receive focus when right is pressed
    virtual AZ::EntityId GetOnRightEntity() = 0;

    //! Set the entity to receive focus when right is pressed
    virtual void SetOnRightEntity(AZ::EntityId entityId) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiNavigationInterface> UiNavigationBus;



