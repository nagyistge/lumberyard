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
#include "StdAfx.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <ISystem.h>

#include "Cinematics/Movie.h"

#include "MaestroSystemComponent.h"

namespace Maestro
{
    void MaestroSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaestroSystemComponent, AZ::Component>()
                ->Version(0)
                ->SerializerForEmptyClass();

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<MaestroSystemComponent>("Maestro", "Provides the Lumberyard Cinematics Service")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        // ->Attribute(AZ::Edit::Attributes::Category, "") Set a category
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void MaestroSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("MaestroService"));
    }

    void MaestroSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("MaestroService"));
    }

    void MaestroSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void MaestroSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void MaestroSystemComponent::Init()
    {
    }

    void MaestroSystemComponent::Activate()
    {
        MaestroRequestBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
    }

    void MaestroSystemComponent::Deactivate()
    {
        MaestroRequestBus::Handler::BusDisconnect();
        CrySystemEventBus::Handler::BusDisconnect();
    }

    //////////////////////////////////////////////////////////////////////////   
    void CSystemEventListener_Movie::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam)
    {
        switch (event)
        {
            case ESYSTEM_EVENT_LEVEL_POST_UNLOAD:
            {
                STLALLOCATOR_CLEANUP;
                CLightAnimWrapper::ReconstructCache();
                break;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void MaestroSystemComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams& startupParams)
    {
        if (!startupParams.bSkipMovie && !startupParams.bShaderCacheGen)
        {
            // OnCrySystemInitialized should only ever be called once, and we should be the only one initializing gEnv->pMovieSystem
            AZ_Assert(!m_movieSystemEventListener && gEnv && !gEnv->pMovieSystem, "MaestroSystemComponent::OnCrySystemInitialized - movie system was alread initialized.");

            if (!m_movieSystemEventListener)
            {
                m_movieSystemEventListener.reset(new CSystemEventListener_Movie);
            }
            system.GetISystemEventDispatcher()->RegisterListener(m_movieSystemEventListener.get());

            // Create the movie System
            m_movieSystem.reset(new CMovieSystem(&system));
            gEnv->pMovieSystem = m_movieSystem.get();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    void MaestroSystemComponent::OnCrySystemShutdown(ISystem& system)
    {
        // Remove the system movie listener and clean up allocations
        if (m_movieSystemEventListener)
        {
            if (gEnv && gEnv->pSystem && gEnv->pSystem->GetISystemEventDispatcher())
            {
                gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(m_movieSystemEventListener.get());
            }
            // delete m_movieSystemEventListener
            m_movieSystemEventListener.reset();
        }

        if (gEnv && gEnv->pMovieSystem)
        {
            gEnv->pMovieSystem = nullptr;
            // delete m_movieSystem
            m_movieSystem.reset();
        }
    }
}
