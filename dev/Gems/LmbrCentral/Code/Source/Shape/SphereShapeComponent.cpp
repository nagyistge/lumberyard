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
#include "SphereShapeComponent.h"
#include <AzCore/Math/IntersectPoint.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace LmbrCentral
{
    namespace ClassConverters
    {
        static bool DeprecateSphereColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        static bool DeprecateSphereColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void SphereShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: SphereColliderConfiguration -> SphereShapeConfig
            serializeContext->ClassDeprecate(
                "SphereColliderConfiguration",
                "{0319AE62-3355-4C98-873D-3139D0427A53}",
                &ClassConverters::DeprecateSphereColliderConfiguration
            );

            serializeContext->Class<SphereShapeConfig>()
                ->Version(1)
                ->Field("Radius", &SphereShapeConfig::m_radius)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<SphereShapeConfig>("Configuration", "Sphere shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(0, &SphereShapeConfig::m_radius, "Radius", "Radius of sphere")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SphereShapeConfig>()
                ->Constructor()
                ->Constructor<float>()
                ->Property("Radius", BehaviorValueProperty(&SphereShapeConfig::m_radius));
        }
    }

    void SphereShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        SphereShapeConfig::Reflect(context);

        auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            // Deprecate: SphereColliderComponent -> SphereShapeComponent
            serializeContext->ClassDeprecate(
                "SphereColliderComponent",
                "{99F33E4A-4EFB-403C-8918-9171D47A03A4}",
                &ClassConverters::DeprecateSphereColliderComponent
                );

            serializeContext->Class<SphereShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &SphereShapeComponent::m_configuration)
                ;
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Constant("SphereShapeComponentTypeId", BehaviorConstant(SphereShapeComponentTypeId));

            behaviorContext->EBus<SphereShapeComponentRequestsBus>("SphereShapeComponentRequestsBus")
                ->Event("GetSphereConfiguration", &SphereShapeComponentRequestsBus::Events::GetSphereConfiguration)
                ->Event("SetRadius", &SphereShapeComponentRequestsBus::Events::SetRadius)
                ;
        }
    }

    void SphereShapeComponent::Activate()
    {
        SphereShape::Activate(GetEntityId());
    }

    void SphereShapeComponent::Deactivate()
    {
        SphereShape::Deactivate();
    }

    bool SphereShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const SphereShapeConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool SphereShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<SphereShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_configuration;
            return true;
        }
        return false;
    }

    namespace ClassConverters
    {
        static bool DeprecateSphereColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="SphereColliderConfiguration" field="Configuration" version="1" type="{0319AE62-3355-4C98-873D-3139D0427A53}">
            <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>

            New:
            <Class name="SphereShapeConfig" field="Configuration" version="1" type="{4AADFD75-48A7-4F31-8F30-FE4505F09E35}">
            <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            */

            // Cache the Radius
            float oldRadius = 0.f;
            int oldIndex = classElement.FindElement(AZ_CRC("Radius", 0x3b7c6e5a));
            if (oldIndex != -1)
            {
                classElement.GetSubElement(oldIndex).GetData<float>(oldRadius);
            }

            // Convert to SphereShapeConfig
            bool result = classElement.Convert<SphereShapeConfig>(context);
            if (result)
            {
                int newIndex = classElement.AddElement<float>(context, "Radius");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<float>(context, oldRadius);
                }
                return true;
            }
            return false;
        }

        static bool DeprecateSphereColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="SphereColliderComponent" version="1" type="{99F33E4A-4EFB-403C-8918-9171D47A03A4}">
             <Class name="SphereColliderConfiguration" field="Configuration" version="1" type="{0319AE62-3355-4C98-873D-3139D0427A53}">
              <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>

            New:
            <Class name="SphereShapeComponent" version="1" type="{E24CBFF0-2531-4F8D-A8AB-47AF4D54BCD2}">
             <Class name="SphereShapeConfig" field="Configuration" version="1" type="{4AADFD75-48A7-4F31-8F30-FE4505F09E35}">
              <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            SphereShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<SphereShapeConfig>(configuration);
            }

            // Convert to SphereShapeComponent
            bool result = classElement.Convert<SphereShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<SphereShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<SphereShapeConfig>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters

} // namespace LmbrCentral
