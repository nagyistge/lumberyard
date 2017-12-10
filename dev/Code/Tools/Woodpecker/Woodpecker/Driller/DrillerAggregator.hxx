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

#ifndef DRILLER_DRILLERAGGREGATOR_H
#define DRILLER_DRILLERAGGREGATOR_H

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Driller/Stream.h>

#include "qtgui/qcolor.h"
#include "DrillerMainWindowMessages.h"
#include "DrillerDataTypes.h"

namespace AZ
{
    namespace IO
    {
        class SystemFile;
    }
}

namespace Driller
{
    class DrillerEvent;
    class AnnotationsProvider;
    class CSVExportSettings;
    class CustomizeCSVExportWidget;

    class ChannelDataView;
    class ChannelConfigurationWidget;

    /*
    Aggregator is a pure virtual data source that packages its data
    into easily digestible, single frame chunks for external consumption.
    It is a pull, not a push, source.

    Separately, aggregators are responsible for handling their own Options and Drill Down displays.
    */

    class Aggregator
        : public QObject
        , public Driller::DrillerMainWindowMessages::Bus::Handler
        , public Driller::DrillerWorkspaceWindowMessages::Bus::Handler
    {
        Q_OBJECT;
    public:
        typedef AZStd::vector<DrillerEvent*> EventListType;
        typedef AZStd::vector<EventNumberType> FrameToEventIndexType;

        Aggregator(int identity);
        virtual ~Aggregator();
        // MainWindow Bus Commands
        
        virtual AZ::Crc32 GetChannelId() const                              { return 0; }
        virtual AZ::u32 GetDrillerId() const								{ return 0; }
        virtual AZ::Debug::DrillerHandlerParser* GetDrillerDataParser()		{ return nullptr; }
        virtual void EnableCapture(bool enabled)							{ m_isCaptureEnabled = enabled; }
        bool IsCaptureEnabled() const										{ return m_isCaptureEnabled; }
        int GetIdentity() const												{ return m_identity; }

        virtual bool CanExportToCSV() const                                    { return false; }
        virtual CustomizeCSVExportWidget* CreateCSVExportCustomizationWidget() { return nullptr; }

        virtual bool HasConfigurations() const { return false; }
        virtual ChannelConfigurationWidget* CreateConfigurationWidget() { return nullptr; }
        virtual void OnConfigurationChanged() { }

        virtual void AnnotateChannelView(ChannelDataView* dataView) { (void)dataView; }
        virtual void RemoveChannelAnnotation(ChannelDataView* dataView) { (void)dataView; }

        /// reset for another data run
        virtual void Reset();
        virtual bool IsValid();

        /// Make a game frame.
        virtual void AddNewFrame();
        /// Adds new event. We can have many events (or none) for each game frame.
        void AddEvent(DrillerEvent* event)									{ m_events.push_back(event); emit OnDataAddEvent();}
        void FinalizeEvent()												{ emit OnEventFinalized(m_events.back()); }
        EventListType& GetEvents()											{ return m_events; }
        const EventListType& GetEvents() const								{ return m_events; }
        size_t NumOfEventsAtFrame(FrameNumberType frame) const;
        EventNumberType GetCurrentEvent() const										{ return m_currentEvent; }

        EventNumberType GetFirstIndexAtFrame(FrameNumberType frame ) const					{ return m_frameToEventIndex[frame]; }

        size_t GetFrameCount() const { return m_frameToEventIndex.size(); }		

        // ----- annotation functionality ----

        // emit all annotations that match the provider's filter, given the start and end frame:
        virtual void EmitAllAnnotationsForFrameRange( FrameNumberType startFrameInclusive, FrameNumberType endFrameInclusive , AnnotationsProvider* ptrProvider) { (void)startFrameInclusive; (void)endFrameInclusive; (void)ptrProvider; }

        // emit all channels that you are aware of existing within that frame range (You may emit duplicate channels, they will be ignored)
        virtual void EmitAnnotationChannelsForFrameRange( FrameNumberType startFrameInclusive, FrameNumberType endFrameInclusive , AnnotationsProvider* ptrProvider)  { (void)startFrameInclusive; (void)endFrameInclusive; (void)ptrProvider; }        

        QString GetDialogTitle() const;
                
    signals:

        void NormalizedRangeChanged();

        void OnDataCurrentEventChanged();
        void OnDataAddEvent();
        void OnEventFinalized(DrillerEvent* event);

        QString GetInspectionFileName() const;

    public slots:
        // Queries
        virtual bool DataAtFrame(FrameNumberType frame );
        virtual float ValueAtFrame(FrameNumberType frame ) = 0;
        virtual QColor GetColor() const = 0;        
        virtual QString GetChannelName() const = 0;
        virtual QString GetName() const = 0;
        virtual QString GetDescription() const = 0;
        virtual QString GetToolTip() const = 0;
        virtual QString GetDrillDownIcon() { return ":/general/callstack"; }
        virtual AZ::Uuid GetID() const = 0;
        virtual QWidget* DrillDownRequest(FrameNumberType atFrame) = 0;
        virtual void OptionsRequest() = 0;

        void ExportToCSVRequest(const char* filename, CSVExportSettings* exportSettings);

    protected:
		Aggregator(const Aggregator&) = delete;
        virtual void FrameChanged(FrameNumberType frame);
        virtual void EventChanged(EventNumberType eventIndex);

        virtual void ExportColumnDescriptorToCSV(AZ::IO::SystemFile& file, CSVExportSettings* exportSettings);
        virtual void ExportEventToCSV(AZ::IO::SystemFile& file, const DrillerEvent* drillerEvent, CSVExportSettings* exportSettings);
        
        EventNumberType     				m_currentEvent;	///< Current event points last executed event
        EventListType						m_events;
        FrameToEventIndexType				m_frameToEventIndex;
        bool								m_isCaptureEnabled;
        int									m_identity;

    public:
        static void Reflect(AZ::ReflectContext* context);
    };
}

#endif //DRILLER_DRILLERAGGREGATOR_H
