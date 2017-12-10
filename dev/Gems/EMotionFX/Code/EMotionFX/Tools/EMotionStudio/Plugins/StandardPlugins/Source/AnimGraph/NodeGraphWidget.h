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

#ifndef __EMSTUDIO_NODEGRAPHWIDGET_H
#define __EMSTUDIO_NODEGRAPHWIDGET_H

//
#include <MCore/Source/StandardHeaders.h>
#include <MCore/Source/Array.h>
#include <AzCore/Debug/Timer.h>
#include <MCore/Source/UnicodeString.h>
#include "../StandardPluginsConfig.h"
#include <QOpenGLWidget>
#include <QPoint>
#include <QPainter>
#include <QOpenGLFunctions>


#define NODEGRAPHWIDGET_USE_OPENGL


namespace EMStudio
{
    // forward declarations
    class NodeGraph;
    class GraphNode;
    class GraphWidgetCallback;
    class NodePort;
    class NodeConnection;
    class AnimGraphPlugin;


    /**
     *
     *
     */
    class NodeGraphWidget
        : public QOpenGLWidget
        , public QOpenGLFunctions
    {
        Q_OBJECT
                 MCORE_MEMORYOBJECTCATEGORY(NodeGraphWidget, EMFX_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS_ANIMGRAPH);

    public:
        NodeGraphWidget(AnimGraphPlugin* plugin, NodeGraph* activeGraph = nullptr, QWidget* parent = nullptr);
        virtual ~NodeGraphWidget();

        void SetActiveGraph(NodeGraph* graph);
        NodeGraph* GetActiveGraph();

        void SetCallback(GraphWidgetCallback* callback);
        MCORE_INLINE GraphWidgetCallback* GetCallback()                 { return mCallback; }
        MCORE_INLINE const QPoint& GetMousePos() const                  { return mMousePos; }
        MCORE_INLINE void SetMousePos(const QPoint& pos)                { mMousePos = pos; }
        MCORE_INLINE void SetShowFPS(bool showFPS)                      { mShowFPS = showFPS; }

        uint32 CalcNumSelectedNodes() const;

        QPoint LocalToGlobal(const QPoint& inPoint);
        QPoint GlobalToLocal(const QPoint& inPoint);
        QPoint SnapLocalToGrid(const QPoint& inPoint, uint32 cellSize = 10);

        void CalcSelectRect(QRect& outRect);

        virtual bool PreparePainting() { return true; }

        virtual bool CheckIfIsCreateConnectionValid(uint32 portNr, GraphNode* portNode, NodePort* port, bool isInputPort);
        virtual bool CreateConnectionMustBeCurved() { return true; }
        virtual bool CreateConnectionShowsHelpers() { return true; }

        // callbacks
        virtual void OnDrawOverlay(QPainter& painter)                                   { MCORE_UNUSED(painter); }
        virtual void OnMoveStart()                                                      {}
        virtual void OnMoveNode(GraphNode* node, int32 x, int32 y)                      { MCORE_UNUSED(node); MCORE_UNUSED(x); MCORE_UNUSED(y); }
        virtual void OnMoveEnd()                                                        {}
        virtual void OnSelectionChanged()                                               {}
        virtual void OnCreateConnection(uint32 sourcePortNr, GraphNode* sourceNode, bool sourceIsInputPort, uint32 targetPortNr, GraphNode* targetNode, bool targetIsInputPort, const QPoint& startOffset, const QPoint& endOffset);
        virtual void OnNodeCollapsed(GraphNode* node, bool isCollapsed)                 { MCORE_UNUSED(node); MCORE_UNUSED(isCollapsed); }
        virtual void OnShiftClickedNode(GraphNode* node)                                { MCORE_UNUSED(node); }
        virtual void OnVisualizeToggle(GraphNode* node, bool visualizeEnabled)          { MCORE_UNUSED(node); MCORE_UNUSED(visualizeEnabled); }
        virtual void OnEnabledToggle(GraphNode* node, bool enabled)                     { MCORE_UNUSED(node); MCORE_UNUSED(enabled); }
        virtual void OnSetupVisualizeOptions(GraphNode* node)                           { MCORE_UNUSED(node); }
        virtual void ReplaceTransition(NodeConnection* connection, QPoint startOffset, QPoint endOffset, GraphNode* sourceNode, GraphNode* targetNode) { MCORE_UNUSED(connection); MCORE_UNUSED(startOffset); MCORE_UNUSED(endOffset); MCORE_UNUSED(sourceNode); MCORE_UNUSED(targetNode); }

    protected:
        //virtual void paintEvent(QPaintEvent* event);
        virtual void mouseMoveEvent(QMouseEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
        virtual void mouseDoubleClickEvent(QMouseEvent* event);
        virtual void mouseReleaseEvent(QMouseEvent* event);
        virtual void wheelEvent(QWheelEvent* event);
        virtual void resizeEvent(QResizeEvent* event);
        virtual void keyPressEvent(QKeyEvent* event); // TODO: check if it is really needed to have them virutal as the events get propagated
        virtual void keyReleaseEvent(QKeyEvent* event);
        virtual void focusInEvent(QFocusEvent* event);
        virtual void focusOutEvent(QFocusEvent* event);

        virtual void initializeGL() override;
        virtual void paintGL() override;
        virtual void resizeGL(int w, int h) override;

        GraphNode* UpdateMouseCursor(const QPoint& localMousePos, const QPoint& globalMousePos);

    protected:
        AnimGraphPlugin*            mPlugin;
        bool                        mShowFPS;
        QPoint                      mMousePos;
        QPoint                      mMouseLastPos;
        QPoint                      mMouseLastPressPos;
        QPoint                      mSelectStart;
        QPoint                      mSelectEnd;
        int                         mPrevWidth;
        int                         mPrevHeight;
        int                         mCurWidth;
        int                         mCurHeight;
        GraphNode*                  mMoveNode;  // the node we're moving
        NodeGraph*                  mActiveGraph;
        GraphWidgetCallback*        mCallback;
        QFont                       mFont;
        QFontMetrics*               mFontMetrics;
        AZ::Debug::Timer            mRenderTimer;
        MCore::String               mTempString;
        MCore::String               mFullActorName;
        MCore::String               mActorName;
        bool                        mAllowContextMenu;
        bool                        mLeftMousePressed;
        bool                        mMiddleMousePressed;
        bool                        mRightMousePressed;
        bool                        mPanning;
        bool                        mRectSelecting;
        bool                        mShiftPressed;
        bool                        mControlPressed;
        bool                        mAltPressed;
    };
}   // namespace EMStudio

#endif
