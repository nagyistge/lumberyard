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
#include "PrimitiveColliderComponent.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/sort.h>
#include <I3DEngine.h>
#include <IPhysics.h>
#include <MathConversion.h>
#include <physinterface.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>
#include <LmbrCentral/Shape/BoxShapeComponentBus.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <LmbrCentral/Shape/CapsuleShapeComponentBus.h>
#include <LmbrCentral/Shape/CylinderShapeComponentBus.h>
#include <LmbrCentral/Shape/CompoundShapeComponentBus.h>

namespace LmbrCentral
{
    extern AZ::Transform GetTransformForColliderGeometry(const IPhysicalEntity& physicalEntity, const AZ::EntityId& colliderEntityId);

    ISurfaceTypeManager* GetSurfaceTypeManager()
    {
        if (gEnv && gEnv->p3DEngine && gEnv->p3DEngine->GetMaterialManager())
        {
            return gEnv->p3DEngine->GetMaterialManager()->GetSurfaceTypeManager();
        }
        return nullptr;
    }

    AZStd::vector<AZStd::string> GetSurfaceTypeNames()
    {
        AZStd::vector<AZStd::string> names;

        if (ISurfaceTypeManager* pSurfaceManager = GetSurfaceTypeManager())
        {
            if (ISurfaceTypeEnumerator* surfaceEnumerator = pSurfaceManager->GetEnumerator())
            {
                for (ISurfaceType* surfaceType = surfaceEnumerator->GetFirst(); surfaceType; surfaceType = surfaceEnumerator->GetNext())
                {
                    // put default material at start of list
                    if (surfaceType->GetId() == 0)
                    {
                        names.insert(names.begin(), surfaceType->GetName());
                    }
                    else
                    {
                        names.emplace_back(surfaceType->GetName());
                    }
                }
            }
        }

        // make empty string the very first entry. It's the default value of PrimitiveColliderConfig::m_surfaceTypeName
        names.insert(names.begin(), "");

        // alphabetize everything after the empty string and the default material.
        if (names.size() >= 2)
        {
            AZStd::sort(names.begin() + 2, names.end());
        }

        return names;
    }

    void PrimitiveColliderConfig::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PrimitiveColliderConfig>()
                ->Version(1)
                ->Field("SurfaceTypeName", &PrimitiveColliderConfig::m_surfaceTypeName)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PrimitiveColliderConfig>("Primitive Collider Configuration", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &PrimitiveColliderConfig::m_surfaceTypeName,
                        "Surface Type", "The collider will use this surface type in the physics simulation.")
                        ->Attribute(AZ::Edit::Attributes::StringList, &GetSurfaceTypeNames)
                ;
            }
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<PrimitiveColliderConfig>()
                ->Property("SurfaceTypeName", BehaviorValueProperty(&PrimitiveColliderConfig::m_surfaceTypeName))
                ;
        }
    }

    void PrimitiveColliderComponent::Reflect(AZ::ReflectContext* context)
    {
        PrimitiveColliderConfig::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<PrimitiveColliderComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &PrimitiveColliderComponent::m_configuration)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<PrimitiveColliderComponent>(
                    "Primitive Collider", "The Primitive Collider component specifies that the collider geometry is provided by a primitive Shape component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Physics")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/Components/PrimitiveCollider.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Editor/Icons/Components/Viewport/PrimitiveCollider.png")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game", 0x232b318c))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://docs.aws.amazon.com/lumberyard/latest/userguide/component-physics-primitive-collider.html")
                    ->DataElement(0, &PrimitiveColliderComponent::m_configuration, "Configuration", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
        
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("PrimitiveColliderComponentTypeId", BehaviorConstant(PrimitiveColliderComponentTypeId));
        }
    }

    //! Create IGeometry. Returns IGeometry wrapped in a _smart_ptr.
    //! If unsuccessful _smart_ptr is empty.
    static _smart_ptr<IGeometry> CreatePrimitiveGeometry(int primitiveType, const primitives::primitive& primitive)
    {
        IGeomManager& geometryManager = *gEnv->pPhysicalWorld->GetGeomManager();
        _smart_ptr<IGeometry> geometry = geometryManager.CreatePrimitive(primitiveType, &primitive);

        // IGeomManager::CreatePrimitive() returns a raw pointer with a refcount of 1.
        // Putting the IGeometry* in a _smart_ptr increments the refcount to 2.
        // Therefore, we decrement the refcount back to 1 so the thing we return
        // will behave like people expect.
        if (geometry)
        {
            geometry->Release();
        }

        return geometry;
    }

    int PrimitiveColliderComponent::AddEntityShapeToPhysicalEntity(IPhysicalEntity& physicalEntity, int nextPartId, const AZ::EntityId& entityId)
    {
        AZ::Crc32 shapeType;
        EBUS_EVENT_ID_RESULT(shapeType, entityId, ShapeComponentRequestsBus, GetShapeType);

        if (shapeType == AZ::Crc32("Sphere"))
        {
            SphereShapeConfig config;
            EBUS_EVENT_ID_RESULT(config, entityId, SphereShapeComponentRequestsBus, GetSphereConfiguration);
            primitives::sphere sphere;
            sphere.center.Set(0.f, 0.f, 0.f);
            sphere.r = config.GetRadius();
            return AddPrimitiveFromEntityToPhysicalEntity(entityId, physicalEntity, nextPartId, primitives::sphere::type, sphere);
        }
        else if (shapeType == AZ::Crc32("Box"))
        {
            BoxShapeConfig config;
            EBUS_EVENT_ID_RESULT(config, entityId, BoxShapeComponentRequestsBus, GetBoxConfiguration);
            primitives::box box;
            box.Basis.SetIdentity();
            box.bOriented = 0;
            box.center.Set(0.f, 0.f, 0.f);

            // box.m_size[i] is "half length" of side
            // config.m_dimensions[i] is "total length" of side
            box.size = AZVec3ToLYVec3(config.GetDimensions() * 0.5f);
            return AddPrimitiveFromEntityToPhysicalEntity(entityId, physicalEntity, nextPartId, primitives::box::type, box);
        }
        else if (shapeType == AZ::Crc32("Cylinder"))
        {
            CylinderShapeConfig config;
            EBUS_EVENT_ID_RESULT(config, entityId, CylinderShapeComponentRequestsBus, GetCylinderConfiguration);
            primitives::cylinder cylinder;
            cylinder.center.Set(0.f, 0.f, 0.f);
            cylinder.axis.Set(0.f, 0.f, 1.f);
            cylinder.r = config.GetRadius();
            // cylinder.hh is "half height"
            // config.m_height is "total height"
            cylinder.hh = 0.5f * config.GetHeight();
            return AddPrimitiveFromEntityToPhysicalEntity(entityId, physicalEntity, nextPartId, primitives::cylinder::type, cylinder);
        }
        else if (shapeType == AZ::Crc32("Capsule"))
        {
            CapsuleShapeConfig config;
            EBUS_EVENT_ID_RESULT(config, entityId, CapsuleShapeComponentRequestsBus, GetCapsuleConfiguration);
            primitives::capsule capsule;
            capsule.center.Set(0.f, 0.f, 0.f);
            capsule.axis.Set(0.f, 0.f, 1.f);
            capsule.r = config.GetRadius();
            // config.m_height specifies "total height" of capsule.
            // capsule.hh is the "half height of the straight section of a capsule".
            // config.hh == (2 * capsule.hh) + (2 * capsule.r)
            capsule.hh = AZStd::max(0.f, (0.5f * config.GetHeight()) - config.GetRadius());
            return AddPrimitiveFromEntityToPhysicalEntity(entityId, physicalEntity, nextPartId, primitives::capsule::type, capsule);
        }
        else if (shapeType == AZ::Crc32("Compound"))
        {
            CompoundShapeConfiguration config;
            EBUS_EVENT_ID_RESULT(config, entityId, CompoundShapeComponentRequestsBus, GetCompoundShapeConfiguration);

            // Connect to EntityBus of child shapes.
            // If the child is active, OnEntityActivated will fire immediately
            // and we'll add the child's shape to physicalEntity.
            // If the child activates later, we won't try to add its shape,
            // we'll just send out a notification that the collider has changed.
            m_recipientOfNewlyActivatedShapes = &physicalEntity;
            m_recipientOfNewlyActivatedShapesNextPartId = nextPartId;
            m_recipientOfNewlyActivatedShapesFinalPartId = NoPartsAdded;
            for (const AZ::EntityId& childEntityId : config.GetChildEntities())
            {
                AZ::EntityBus::MultiHandler::BusConnect(childEntityId);
            }
            m_recipientOfNewlyActivatedShapes = nullptr;
            return m_recipientOfNewlyActivatedShapesFinalPartId;
        }

        return NoPartsAdded;
    }

    void PrimitiveColliderComponent::OnEntityActivated(const AZ::EntityId& entityId)
    {
        // A compound shape's child entity has activated.
        // If m_recipientOfNewlyActivatedShapes is set, add colliders to it.
        // Otherwise, just send out a notification that the collider has changed.
        if (m_recipientOfNewlyActivatedShapes)
        {
            int finalPartId = AddEntityShapeToPhysicalEntity(*m_recipientOfNewlyActivatedShapes, m_recipientOfNewlyActivatedShapesNextPartId, entityId);
            if (finalPartId != NoPartsAdded)
            {
                m_recipientOfNewlyActivatedShapesFinalPartId = finalPartId;
                m_recipientOfNewlyActivatedShapesNextPartId = finalPartId + 1;
            }
        }
        else
        {
            // Check that child does in fact have shape on it before announcing shape change.
            AZ::Crc32 shapeType;
            EBUS_EVENT_ID_RESULT(shapeType, entityId, ShapeComponentRequestsBus, GetShapeType);
            if (shapeType != AZ::Crc32())
            {
                EBUS_EVENT_ID(GetEntityId(), ColliderComponentEventBus, OnColliderChanged);
            }
        }
        AZ::EntityBus::MultiHandler::BusDisconnect(entityId);
    }

    int PrimitiveColliderComponent::AddColliderToPhysicalEntity(IPhysicalEntity& physicalEntity, int nextPartId)
    {
        return AddEntityShapeToPhysicalEntity(physicalEntity, nextPartId, GetEntityId());
    }

    int PrimitiveColliderComponent::AddPrimitiveFromEntityToPhysicalEntity(
        const AZ::EntityId& entityId,
        IPhysicalEntity& physicalEntity,
        int nextPartId,
        int primitiveType,
        const primitives::primitive& primitive)
    {
        _smart_ptr<IGeometry> geometry = CreatePrimitiveGeometry(primitiveType, primitive);
        if (!geometry)
        {
            return NoPartsAdded;
        }

        phys_geometry* physGeometry = gEnv->pPhysicalWorld->GetGeomManager()->RegisterGeometry(geometry);
        if (!physGeometry)
        {
            return NoPartsAdded;
        }

        // configure pe_geomparams
        pe_geomparams geometryParameters;
        geometryParameters.flags = geom_collides | geom_floats | geom_colltype_ray;

        // set surface type
        if (!m_configuration.m_surfaceTypeName.empty())
        {
            if (ISurfaceType* surfaceType = GetSurfaceTypeManager()->GetSurfaceTypeByName(m_configuration.m_surfaceTypeName.c_str()))
            {
                geometryParameters.surface_idx = surfaceType->GetId();
            }
        }

        // set full local transform matrix
        // some (but not all!) meshes support non-uniform scaling
        AZ::Transform geometryAzTransform = GetTransformForColliderGeometry(physicalEntity, GetEntityId());
        Matrix34 geometryTransform = AZTransformToLYTransform(geometryAzTransform);
        geometryParameters.pMtx3x4 = &geometryTransform;

        // add geometry
        int finalPartId = physicalEntity.AddGeometry(physGeometry, &geometryParameters, nextPartId);

        // release our phys_geometry reference (IPhysicalEntity should have
        // a reference now)
        gEnv->pPhysicalWorld->GetGeomManager()->UnregisterGeometry(physGeometry);

        return finalPartId;
    }

    void PrimitiveColliderComponent::Activate()
    {
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        ColliderComponentRequestBus::Handler::BusConnect(GetEntityId());
    }

    void PrimitiveColliderComponent::Deactivate()
    {
        ColliderComponentRequestBus::Handler::BusDisconnect();
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
    }

    bool PrimitiveColliderComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (auto config = azrtti_cast<const PrimitiveColliderConfig*>(baseConfig))
        {
            m_configuration = *config;
            return true;
        }
        return false;
    }

    bool PrimitiveColliderComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<PrimitiveColliderConfig*>(outBaseConfig))
        {
            *outConfig = m_configuration;
            return true;
        }
        return false;
    }

    void PrimitiveColliderComponent::OnShapeChanged(ShapeComponentNotifications::ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged)
        {
            AZ_Warning("[Primitive Collider Component]", false, "Primitive collider component does not currently support dynamic changes to collider shape...Entity %s [%llu]..."
                       , m_entity->GetName().c_str(), m_entity->GetId());
        }
    }
} // namespace LmbrCentral
