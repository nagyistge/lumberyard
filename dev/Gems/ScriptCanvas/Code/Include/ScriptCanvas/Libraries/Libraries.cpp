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
#include "Libraries.h"

#include <Libraries/Core/Core.h>
#include <Libraries/Logic/Logic.h>
#include <Libraries/Math/Math.h>
#include <Libraries/Comparison/Comparison.h>

namespace ScriptCanvas
{
    static AZ::EnvironmentVariable<NodeRegistry> g_nodeRegistry;

    void InitNodeRegistry()
    {
        g_nodeRegistry = AZ::Environment::CreateVariable<NodeRegistry>(s_nodeRegistryName);

        using namespace Library;
        Core::InitNodeRegistry(*g_nodeRegistry);
        Math::InitNodeRegistry(*g_nodeRegistry);
        Logic::InitNodeRegistry(*g_nodeRegistry);
        Entity::InitNodeRegistry(*g_nodeRegistry);
        Comparison::InitNodeRegistry(*g_nodeRegistry);
        Time::InitNodeRegistry(*g_nodeRegistry);
    }

    void ResetNodeRegistry()
    {
        g_nodeRegistry.Reset();
    }

    AZ::EnvironmentVariable<ScriptCanvas::NodeRegistry> GetNodeRegistry()
    {
        return g_nodeRegistry;
    }

    void ReflectLibraries(AZ::ReflectContext* reflectContext)
    {
        using namespace Library;

        Core::Reflect(reflectContext);
        Math::Reflect(reflectContext);
        Logic::Reflect(reflectContext);
        Entity::Reflect(reflectContext);
        Comparison::Reflect(reflectContext);
        Time::Reflect(reflectContext);
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetLibraryDescriptors()
    {
        using namespace Library;

        AZStd::vector<AZ::ComponentDescriptor*> libraryDescriptors(Core::GetComponentDescriptors());
        
        AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors(Math::GetComponentDescriptors());
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
        
        componentDescriptors = Logic::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
        
        componentDescriptors = Entity::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = Comparison::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = Time::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        return libraryDescriptors;
    }

}