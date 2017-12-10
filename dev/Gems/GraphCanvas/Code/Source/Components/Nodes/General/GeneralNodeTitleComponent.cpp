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

#include <QGraphicsItem>
#include <QFont>
#include <QPainter>

#include <AzCore/Serialization/EditContext.h>

#include <Components/Nodes/General/GeneralNodeTitleComponent.h>

#include <Components/ColorPaletteManager/ColorPaletteManagerBus.h>
#include <Components/Nodes/General/GeneralNodeFrameComponent.h>
#include <Components/Nodes/NodeBus.h>
#include <GraphCanvas/tools.h>
#include <Styling/StyleHelper.h>

namespace GraphCanvas
{
    //////////////////////////////
    // GeneralNodeTitleComponent
    //////////////////////////////
	
    void GeneralNodeTitleComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<GeneralNodeTitleComponent>()
                ->Version(2)
                ->Field("Title", &GeneralNodeTitleComponent::m_title)
                ->Field("SubTitle", &GeneralNodeTitleComponent::m_subTitle)
                ;
        }
    }

    GeneralNodeTitleComponent::GeneralNodeTitleComponent()
    {
    }

    void GeneralNodeTitleComponent::Init()
    {
        m_generalNodeTitleWidget = aznew GeneralNodeTitleGraphicsWidget(GetEntityId());
    }

    void GeneralNodeTitleComponent::Activate()
    {
        NodeTitleRequestBus::Handler::BusConnect(GetEntityId());

        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetTitle(m_title);
            m_generalNodeTitleWidget->SetSubTitle(m_subTitle);

            m_generalNodeTitleWidget->UpdateLayout();

            m_generalNodeTitleWidget->Activate();
        }
    }

    void GeneralNodeTitleComponent::Deactivate()
    {
        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->Deactivate();
        }

        NodeTitleRequestBus::Handler::BusDisconnect();
    }

    void GeneralNodeTitleComponent::SetTitle(const AZStd::string& title)
    {
        m_title.m_fallback = title;

        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetTitle(title);
        }
    }

    void GeneralNodeTitleComponent::SetTranslationKeyedTitle(const TranslationKeyedString& title)
    {
        m_title = title;

        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetTitle(title);
        }
    }

    AZStd::string GeneralNodeTitleComponent::GetTitle() const
    {
        return m_title.GetDisplayString();
    }

    void GeneralNodeTitleComponent::SetSubTitle(const AZStd::string& subtitle)
    {
        m_subTitle.m_fallback = subtitle;

        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetSubTitle(subtitle);
        }
    }

    void GeneralNodeTitleComponent::SetTranslationKeyedSubTitle(const TranslationKeyedString& subtitle)
    {
        m_subTitle = subtitle;

        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetSubTitle(subtitle);
        }
    }

    AZStd::string GeneralNodeTitleComponent::GetSubTitle() const
    {
        return m_subTitle.GetDisplayString();
    }

    QGraphicsWidget* GeneralNodeTitleComponent::GetGraphicsWidget()
    {
        return m_generalNodeTitleWidget;
    }

    void GeneralNodeTitleComponent::SetPaletteOverride(const AZStd::string& paletteOverride)
    {
        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetPaletteOverride(paletteOverride);
        }
    }

    void GeneralNodeTitleComponent::SetDataPaletteOverride(const AZ::Uuid& uuid)
    {
        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->SetPaletteOverride(uuid);
        }
    }

    void GeneralNodeTitleComponent::ClearPaletteOverride()
    {
        if (m_generalNodeTitleWidget)
        {
            m_generalNodeTitleWidget->ClearPaletteOverride();
        }
    }

    ///////////////////////////////////
    // GeneralNodeTitleGraphicsWidget
    ///////////////////////////////////
    GeneralNodeTitleGraphicsWidget::GeneralNodeTitleGraphicsWidget(const AZ::EntityId& entityId)
        : m_entityId(entityId)
        , m_paletteOverride(nullptr)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setGraphicsItem(this);
        setAcceptHoverEvents(false);
        setFlag(ItemIsMovable, false);

        m_titleWidget = aznew GraphCanvasLabel(this);
        m_subTitleWidget = aznew GraphCanvasLabel(this);
        m_linearLayout = new QGraphicsLinearLayout(Qt::Vertical);
        m_linearLayout->setSpacing(0);
        setLayout(m_linearLayout);
        setData(GraphicsItemName, QStringLiteral("Title/%1").arg(static_cast<AZ::u64>(GetEntityId()), 16, 16, QChar('0')));
    }

    void GeneralNodeTitleGraphicsWidget::Activate()
    {
        SceneMemberNotificationBus::Handler::BusConnect(GetEntityId());
        NodeNotificationBus::Handler::BusConnect(GetEntityId());

        AZ::EntityId scene;
        SceneMemberRequestBus::EventResult(scene, GetEntityId(), &SceneMemberRequests::GetScene);
        if (scene.IsValid())
        {
            SceneNotificationBus::Handler::BusConnect(scene);
            UpdateStyles();
        }
    }

    void GeneralNodeTitleGraphicsWidget::Deactivate()
    {
        SceneMemberNotificationBus::Handler::BusDisconnect();
        NodeNotificationBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusDisconnect();
    }

    void GeneralNodeTitleGraphicsWidget::SetTitle(const TranslationKeyedString& title)
    {
        if (m_titleWidget)
        {
            m_titleWidget->SetLabel(title);
            UpdateLayout();
        }
    }

    void GeneralNodeTitleGraphicsWidget::SetSubTitle(const TranslationKeyedString& subtitle)
    {
        if (m_subTitleWidget)
        {
            m_subTitleWidget->SetLabel(subtitle);
            UpdateLayout();
        }
    }

    void GeneralNodeTitleGraphicsWidget::SetPaletteOverride(const AZStd::string& paletteOverride)
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);
        
        ColorPaletteManagerRequestBus::EventResult(m_paletteOverride, sceneId, &ColorPaletteManagerRequests::FindColorPalette, paletteOverride);
        update();
    }

    void GeneralNodeTitleGraphicsWidget::SetPaletteOverride(const AZ::Uuid& uuid)
    {
        AZ::EntityId sceneId;
        SceneMemberRequestBus::EventResult(sceneId, GetEntityId(), &SceneMemberRequests::GetScene);

        ColorPaletteManagerRequestBus::EventResult(m_paletteOverride, sceneId, &ColorPaletteManagerRequests::FindDataColorPalette, uuid);
        update();
    }

    void GeneralNodeTitleGraphicsWidget::ClearPaletteOverride()
    {
        m_paletteOverride = nullptr;
        update();
    }

    void GeneralNodeTitleGraphicsWidget::UpdateLayout()
    {
        while (m_linearLayout->count() != 0)
        {
            m_linearLayout->removeAt(0);
        }

        if (!m_titleWidget->GetLabel().empty())
        {
            m_linearLayout->addItem(m_titleWidget);
        }

        if (!m_subTitleWidget->GetLabel().empty())
        {
            m_linearLayout->addItem(m_subTitleWidget);
        }

        adjustSize();
        RefreshDisplay();
        NodeTitleNotificationsBus::Event(GetEntityId(), &NodeTitleNotifications::OnTitleChanged);
    }

    void GeneralNodeTitleGraphicsWidget::UpdateStyles()
    {
        m_styleHelper.SetStyle(GetEntityId(), Styling::Elements::Title);
        m_titleWidget->SetStyle(GetEntityId(), Styling::Elements::MainTitle);
        m_subTitleWidget->SetStyle(GetEntityId(), Styling::Elements::SubTitle);
    }

    void GeneralNodeTitleGraphicsWidget::RefreshDisplay()
    {
        updateGeometry();
        update();
    }

    void GeneralNodeTitleGraphicsWidget::OnStyleSheetChanged()
    {
        UpdateStyles();
        RefreshDisplay();
    }

    void GeneralNodeTitleGraphicsWidget::OnSceneSet(const AZ::EntityId& scene)
    {
        SceneNotificationBus::Handler::BusConnect(scene);
        UpdateStyles();
        RefreshDisplay();
    }

    void GeneralNodeTitleGraphicsWidget::OnSceneCleared(const AZ::EntityId& scene)
    {
        SceneNotificationBus::Handler::BusDisconnect();
    }

    void GeneralNodeTitleGraphicsWidget::OnTooltipChanged(const AZStd::string& tooltip)
    {
        setToolTip(Tools::qStringFromUtf8(tooltip));
    }

    void GeneralNodeTitleGraphicsWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
    {
        // Background
        {
            QRectF bounds = boundingRect();

            qreal cornerRadius = 0.0;

            NodeUIRequestBus::EventResult(cornerRadius, GetEntityId(), &NodeUIRequests::GetCornerRadius);

            // Ensure the bounds are large enough to draw the full radius
            // Even in our smaller section
            if (bounds.height() < 2.0 * cornerRadius)
            {
                bounds.setHeight(2.0 * cornerRadius);
            }

            QBrush brush = m_styleHelper.GetBrush(Styling::Attribute::BackgroundColor);

            if (m_paletteOverride)
            {
                brush.setColor(m_paletteOverride->GetColor(Styling::Attribute::BackgroundColor));
            }

            QPainterPath path;
            path.setFillRule(Qt::WindingFill);

            // -1.0 because the rounding is a little bit short(for some reason), so I subtract one and let it overshoot a smidge.
            path.addRoundedRect(bounds, cornerRadius - 1.0, cornerRadius - 1.0);

            // We only want corners on the top half. So we need to draw a rectangle over the bottom bits to square it out.
            QPointF bottomTopLeft(bounds.bottomLeft());
            bottomTopLeft.setY(bottomTopLeft.y() - cornerRadius - 1.0);
            path.addRect(QRectF(bottomTopLeft, bounds.bottomRight()));

            painter->fillPath(path, brush);

            QLinearGradient gradient(bounds.bottomLeft(), bounds.topLeft());
            gradient.setColorAt(0, QColor(0, 0, 0, 102));
            gradient.setColorAt(1, QColor(0, 0, 0, 51));
            painter->fillPath(path, gradient);
        }

        QGraphicsWidget::paint(painter, option, widget);
    }
}