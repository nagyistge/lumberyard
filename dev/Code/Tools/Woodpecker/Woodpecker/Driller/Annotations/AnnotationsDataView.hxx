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

#ifndef ANNOTATIONS_DATA_VIEW
#define ANNOTATIONS_DATA_VIEW

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QtWidgets/QWidget>



namespace Driller
{
    class AnnotationsProvider;
    class Annotation;
    class AnnotationHeaderView;
    /** Annotations Data View just shows the annotations that are available in a horizontal strip with indicators for easy clickability.
    */

    class AnnotationsDataView : public QWidget
    {
        Q_OBJECT;
    public:
        AZ_CLASS_ALLOCATOR(AnnotationsDataView, AZ::SystemAllocator, 0);
        AnnotationsDataView(QWidget* parent = nullptr);		
        virtual ~AnnotationsDataView();

        void RegisterAnnotationHeaderView(AnnotationHeaderView* header, AnnotationsProvider* annotations);
    
        int PositionToFrame( const QPoint &pt );

        float GetBarWidth();

        virtual void paintEvent( QPaintEvent *event );
        virtual void mouseMoveEvent( QMouseEvent *event );
        virtual void mousePressEvent( QMouseEvent *event );
        virtual void mouseReleaseEvent( QMouseEvent *event );

signals:
        void InformOfMouseOverAnnotation(const Annotation& annotation);
        void InformOfClickAnnotation(const Annotation& annotation);
    private:
        typedef AZStd::unordered_map<AZ::s64, QPainterPath> EventIndexToClickablePath;
        EventIndexToClickablePath m_ClickableAreas;
        AZStd::unordered_set<AZ::s64> m_eventsToHighlight;

        AnnotationsProvider *m_ptrAnnotations;
        AnnotationHeaderView *m_ptrHeaderView;
    };
}

#endif // ANNOTATIONS_DATA_VIEW
