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

#ifndef DRILLER_TRACE_MESSAGE_DATAAGGREGATOR_H
#define DRILLER_TRACE_MESSAGE_DATAAGGREGATOR_H

#include "Woodpecker/Driller/DrillerAggregator.hxx"
#include "Woodpecker/Driller/DrillerAggregatorOptions.hxx"

#include "TraceMessageDataParser.h"

namespace Driller
{
    class TraceMessageDataAggregatorSavedState;

    /**
     * Trace message driller data aggregator.
     */
    class TraceMessageDataAggregator : public Aggregator
    {
        Q_OBJECT;
		TraceMessageDataAggregator(const TraceMessageDataAggregator&) = delete;
    public:
        AZ_RTTI(TraceMessageDataAggregator, "{CA33E0B0-6E16-4D8C-B3D0-C833AC8574C6}");
        AZ_CLASS_ALLOCATOR(TraceMessageDataAggregator,AZ::SystemAllocator,0);

        TraceMessageDataAggregator(int identity = 0);

        static AZ::u32 DrillerId()											{ return TraceMessageHandlerParser::GetDrillerId(); }
        virtual AZ::u32 GetDrillerId() const								{ return DrillerId(); }

        static const char* ChannelName() { return "Logging"; }
        AZ::Crc32 GetChannelId() const override { return AZ::Crc32(ChannelName()); }

        virtual AZ::Debug::DrillerHandlerParser* GetDrillerDataParser() 	{ return &m_parser; }

        virtual void ApplySettingsFromWorkspace(WorkspaceSettingsProvider*);
        virtual void ActivateWorkspaceSettings(WorkspaceSettingsProvider*);
        virtual void SaveSettingsToWorkspace(WorkspaceSettingsProvider*);

    //////////////////////////////////////////////////////////////////////////
    // Aggregator

        // emit all annotations that match the provider's filter, given the start and end frame:
        virtual void EmitAllAnnotationsForFrameRange( int startFrameInclusive, int endFrameInclusive , AnnotationsProvider* ptrProvider);

        // emit all channels that you are aware of existing within that frame range (You may emit duplicate channels, they will be ignored)
        virtual void EmitAnnotationChannelsForFrameRange( int startFrameInclusive, int endFrameInclusive , AnnotationsProvider* ptrProvider);

    public slots:
        float ValueAtFrame( FrameNumberType frame ) override;
        QColor GetColor() const override;        
        QString GetName() const override;
        QString GetChannelName() const override;
        QString GetDescription() const override;
        QString GetToolTip() const override;
        AZ::Uuid GetID() const override;
        QWidget* DrillDownRequest(FrameNumberType frame) override;
        void OptionsRequest() override;
        virtual void OnDataViewDestroyed(QObject*);

    public:
        TraceMessageHandlerParser			m_parser;			///< Parser for this aggregator

        QObject*                                                    m_dataView;                
        AZStd::intrusive_ptr<TraceMessageDataAggregatorSavedState>	m_persistentState;

    public:
        static void Reflect(AZ::ReflectContext* context);
    };

}

#endif
