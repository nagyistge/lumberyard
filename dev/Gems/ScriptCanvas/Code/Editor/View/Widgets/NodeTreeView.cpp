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
#include "NodeTreeView.h"

#include <Editor/Model/NodePalette/NodePaletteSortFilterProxyModel.h>
#include <Editor/View/Widgets/NodePalette/NodePaletteTreeItem.h>

#include <QMouseEvent>

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        NodeTreeView::NodeTreeView(QWidget* parent /*= nullptr*/)
            : AzToolsFramework::QTreeViewWithStateSaving(parent)
        {
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            setDragEnabled(true);
            setHeaderHidden(true);
            setAutoScroll(true);
            setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
            setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
            setDragDropMode(QAbstractItemView::DragDropMode::DragOnly);
            setMouseTracking(true);

            QObject::connect(this,
                &AzToolsFramework::QTreeViewWithStateSaving::entered,
                [this](const QModelIndex &modelIndex)
                {
                    UpdatePointer(modelIndex, false);
                });
        }

        void NodeTreeView::resizeEvent(QResizeEvent* event)
        {
            resizeColumnToContents(0);
            QTreeView::resizeEvent(event);
        }

        void NodeTreeView::mousePressEvent(QMouseEvent* ev)
        {
            UpdatePointer(indexAt(ev->pos()), true);

            AzToolsFramework::QTreeViewWithStateSaving::mousePressEvent(ev);
        }

        void NodeTreeView::mouseMoveEvent(QMouseEvent* ev)
        {
            UpdatePointer(indexAt(ev->pos()), false);

            AzToolsFramework::QTreeViewWithStateSaving::mouseMoveEvent(ev);
        }

        void NodeTreeView::mouseReleaseEvent(QMouseEvent* ev)
        {
            UpdatePointer(indexAt(ev->pos()), false);

            AzToolsFramework::QTreeViewWithStateSaving::mouseReleaseEvent(ev);
        }

        void NodeTreeView::UpdatePointer(const QModelIndex &modelIndex, bool isMousePressed)
        {
            if (!modelIndex.isValid())
            {
                setCursor(Qt::ArrowCursor);
                return;
            }

            // IMPORTANT: mapToSource() is NECESSARY. Otherwise the internalPointer
            // in the index is relative to the proxy model, NOT the source model.
            QModelIndex sourceIndex = qobject_cast<const Model::NodePaletteSortFilterProxyModel*>(modelIndex.model())->mapToSource(modelIndex);
            NodePaletteTreeItem* treeItem = static_cast<NodePaletteTreeItem*>(sourceIndex.internalPointer());
            if(treeItem)
            {
                if(treeItem->Flags(QModelIndex()) & Qt::ItemIsDragEnabled && isMousePressed)
                {
                    setCursor(Qt::ClosedHandCursor);
                }
                else
                {
                    setCursor(Qt::ArrowCursor);
                }
            }
        }

        #include <Editor/View/Widgets/NodeTreeView.moc>
    }
}
