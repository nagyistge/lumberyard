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

#include "MotionSetSelectionWindow.h"
#include "EMStudioManager.h"
#include <QLabel>
#include <QSizePolicy>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>


namespace EMStudio
{
    MotionSetSelectionWindow::MotionSetSelectionWindow(QWidget* parent, bool useSingleSelection, CommandSystem::SelectionList* selectionList)
        : QDialog(parent)
    {
        setWindowTitle("Motion Selection Window");
        resize(850, 500);

        QVBoxLayout* layout = new QVBoxLayout();

        mHierarchyWidget = new MotionSetHierarchyWidget(this, useSingleSelection, selectionList);

        // create the ok and cancel buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mOKButton       = new QPushButton("OK");
        mCancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        layout->addWidget(mHierarchyWidget);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        connect(mOKButton, SIGNAL(clicked()), this, SLOT(accept()));
        connect(mCancelButton, SIGNAL(clicked()), this, SLOT(reject()));
        connect(this, SIGNAL(accepted()), this, SLOT(OnAccept()));
        connect(mHierarchyWidget, &MotionSetHierarchyWidget::SelectionChanged, this, &MotionSetSelectionWindow::OnSelectionChanged);

        // set the selection mode
        mHierarchyWidget->SetSelectionMode(useSingleSelection);
        mUseSingleSelection = useSingleSelection;
    }


    MotionSetSelectionWindow::~MotionSetSelectionWindow()
    {
    }


    void MotionSetSelectionWindow::OnSelectionChanged(AZStd::vector<MotionSetSelectionItem> selection)
    {
        MCORE_UNUSED(selection);
        accept();
    }


    void MotionSetSelectionWindow::OnAccept()
    {
        if (mUseSingleSelection == false)
        {
            mHierarchyWidget->FireSelectionDoneSignal();
        }
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MotionSetSelectionWindow.moc>