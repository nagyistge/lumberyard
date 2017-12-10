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
#include "PolygonPrismShapeComponent.h"

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace LmbrCentral
{
    void ShapeChangedNotification(AZ::EntityId entityId)
    {
        ShapeComponentNotificationsBus::Event(entityId, &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);
    }

    PolygonPrismCommon::PolygonPrismCommon()
        : m_polygonPrism(AZStd::make_shared<AZ::PolygonPrism>())
    {
    }

    void PolygonPrismCommon::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PolygonPrismCommon>()
                ->Version(1)
                ->Field("PolygonPrism", &PolygonPrismCommon::m_polygonPrism);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<PolygonPrismCommon>("Configuration", "Polygon Prism configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &PolygonPrismCommon::m_polygonPrism, "Polygon Prism", "Data representing the shape in the entity's local coordinate space.")
                    ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void PolygonPrismShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        PolygonPrismCommon::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PolygonPrismShapeComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &PolygonPrismShapeComponent::m_polygonPrismCommon);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PolygonPrismShapeComponentRequestsBus>("PolygonPrismShapeComponentRequestsBus")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                ->Event("GetPolygonPrism", &PolygonPrismShapeComponentRequestsBus::Events::GetPolygonPrism)
                ->Event("SetHeight", &PolygonPrismShapeComponentRequestsBus::Events::SetHeight)
                ->Event("AddVertex", &PolygonPrismShapeComponentRequestsBus::Events::AddVertex)
                ->Event("UpdateVertex", &PolygonPrismShapeComponentRequestsBus::Events::UpdateVertex)
                ->Event("InsertVertex", &PolygonPrismShapeComponentRequestsBus::Events::InsertVertex)
                ->Event("RemoveVertex", &PolygonPrismShapeComponentRequestsBus::Events::RemoveVertex)
                ->Event("SetVertices", &PolygonPrismShapeComponentRequestsBus::Events::SetVertices)
                ->Event("ClearVertices", &PolygonPrismShapeComponentRequestsBus::Events::ClearVertices);
        }
    }

    void PolygonPrismShapeComponent::Activate()
    {
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(m_currentTransform, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        m_intersectionDataCache.SetCacheStatus(PolygonPrismShapeComponent::PolygonPrismIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
        AZ::TransformNotificationBus::Handler::BusConnect(GetEntityId());
        ShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());
        PolygonPrismShapeComponentRequestsBus::Handler::BusConnect(GetEntityId());

        m_polygonPrismCommon.m_polygonPrism->SetCallbacks(
            [this]() { ShapeChangedNotification(GetEntityId()); },
            [this]() { ShapeChangedNotification(GetEntityId()); });
    }

    void PolygonPrismShapeComponent::Deactivate()
    {
        PolygonPrismShapeComponentRequestsBus::Handler::BusDisconnect();
        ShapeComponentRequestsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void PolygonPrismShapeComponent::OnTransformChanged(const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
        m_intersectionDataCache.SetCacheStatus(PolygonPrismIntersectionDataCache::CacheStatus::Obsolete_TransformChange);
        ShapeComponentNotificationsBus::Event(GetEntityId(), &ShapeComponentNotificationsBus::Events::OnShapeChanged, ShapeComponentNotifications::ShapeChangeReasons::TransformChanged);
    }

    AZ::ConstPolygonPrismPtr PolygonPrismShapeComponent::GetPolygonPrism()
    {
        return m_polygonPrismCommon.m_polygonPrism;
    }

    void PolygonPrismShapeComponent::AddVertex(const AZ::Vector2& vertex)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.AddVertex(vertex);
        m_intersectionDataCache.SetCacheStatus(PolygonPrismIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
    }
    
    void PolygonPrismShapeComponent::UpdateVertex(size_t index, const AZ::Vector2& vertex)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.UpdateVertex(index, vertex);
        m_intersectionDataCache.SetCacheStatus(PolygonPrismIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
    }
    
    void PolygonPrismShapeComponent::InsertVertex(size_t index, const AZ::Vector2& vertex)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.InsertVertex(index, vertex);
        m_intersectionDataCache.SetCacheStatus(PolygonPrismIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
    }
    
    void PolygonPrismShapeComponent::RemoveVertex(size_t index)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.RemoveVertex(index);
        m_intersectionDataCache.SetCacheStatus(PolygonPrismIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
    }
    
    void PolygonPrismShapeComponent::SetVertices(const AZStd::vector<AZ::Vector2>& vertices)
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.SetVertices(vertices);
        m_intersectionDataCache.SetCacheStatus(PolygonPrismIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
    }
    
    void PolygonPrismShapeComponent::ClearVertices()
    {
        m_polygonPrismCommon.m_polygonPrism->m_vertexContainer.Clear();
        m_intersectionDataCache.SetCacheStatus(PolygonPrismIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
    }

    void PolygonPrismShapeComponent::SetHeight(float height)
    {
        m_polygonPrismCommon.m_polygonPrism->SetHeight(height);
        m_intersectionDataCache.SetCacheStatus(PolygonPrismIntersectionDataCache::CacheStatus::Obsolete_ShapeChange);
    }

    //////////////////////////////////////////////////////////////////////////

    AZ::Aabb PolygonPrismShapeComponent::GetEncompassingAabb()
    {
        m_intersectionDataCache.UpdateIntersectionParams(m_currentTransform, *m_polygonPrismCommon.m_polygonPrism);
        return m_intersectionDataCache.GetAabb();
    }

    /**
     * Return if the point is inside of the polygon prism volume or not.
     * Use 'Crossings Test' to determine if point lies in or out of the polygon.
     * @param point Position in world space to test against.
     */
    bool PolygonPrismShapeComponent::IsPointInside(const AZ::Vector3& point)
    {
        // initial early aabb rejection test
        // note: will implicitly do height test too
        if (!GetEncompassingAabb().Contains(point))
        {
            return false;
        }

        return AZ::PolygonPrismUtil::IsPointInside(*m_polygonPrismCommon.m_polygonPrism, point, m_currentTransform);
    }

    float PolygonPrismShapeComponent::DistanceSquaredFromPoint(const AZ::Vector3& point)
    {
        return AZ::PolygonPrismUtil::DistanceSquaredFromPoint(*m_polygonPrismCommon.m_polygonPrism, point, m_currentTransform);;
    }

    //////////////////////////////////////////////////////////////////////////
    void PolygonPrismShapeComponent::PolygonPrismIntersectionDataCache::UpdateIntersectionParams(const AZ::Transform& currentTransform, const AZ::PolygonPrism& polygonPrism)
    {
        if (m_cacheStatus > CacheStatus::Current)
        {
            m_aabb = AZ::PolygonPrismUtil::CalculateAabb(polygonPrism, currentTransform);
            SetCacheStatus(CacheStatus::Current);
        }
    }
} // namespace LmbrCentral
