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

#include "stdafx.h"
#include "TargetContextButton.hxx"
#include <Woodpecker/LUA/TargetContextButton.moc>
#include <Woodpecker/LUA/LUAEditorContextMessages.h>
#include <Woodpecker/LUA/LUAEditorDebuggerMessages.h>
#include <Woodpecker/LUA/LUATargetContextTrackerMessages.h>

namespace LUA
{
    TargetContextButton::TargetContextButton(QWidget* pParent)
        : QPushButton(pParent)
    {
        LUAEditor::Context_ControlManagement::Handler::BusConnect();

        AZStd::string context("Default");
        EBUS_EVENT(LUAEditor::LUATargetContextRequestMessages::Bus, SetCurrentTargetContext, context);
        this->setText("Context: Default");

        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(sizePolicy().hasHeightForWidth());
        setSizePolicy(sizePolicy1);
        setMinimumSize(QSize(128, 24));

        this->setToolTip(tr("Click to change context"));
        connect(this, SIGNAL(clicked()), this, SLOT(DoPopup()));
    }

    TargetContextButton::~TargetContextButton()
    {
        LUAEditor::Context_ControlManagement::Handler::BusDisconnect();
    }

    void TargetContextButton::DoPopup()
    {
        AzFramework::TargetContainer targets;

        EBUS_EVENT(AzFramework::TargetManager::Bus, EnumTargetInfos, targets);

        QMenu menu;

        AZStd::vector<AZStd::string> contexts;
        EBUS_EVENT_RESULT(contexts, LUAEditor::LUATargetContextRequestMessages::Bus, RequestTargetContexts);

        for (AZStd::vector<AZStd::string>::const_iterator it = contexts.begin(); it != contexts.end(); ++it)
        {
            QAction* targetAction = new QAction((*it).c_str(), this);
            targetAction->setProperty("context", (*it).c_str());
            menu.addAction(targetAction);
        }

        QAction* resultAction = menu.exec(QCursor::pos());

        if (resultAction)
        {
            AZStd::string context = resultAction->property("context").toString().toUtf8().data();
            this->setText("Context: None"); // prepare for failure
            EBUS_EVENT(LUAEditor::LUATargetContextRequestMessages::Bus, SetCurrentTargetContext, context);
        }
    }

    void TargetContextButton::OnTargetContextPrepared(AZStd::string& contextName)
    {
        QString qstr = QString("Context: %1").arg(contextName.c_str());
        this->setText(qstr); // plan for success
    }

    TargetContextButtonAction::TargetContextButtonAction(QObject* pParent)
        : QWidgetAction(pParent)
    {
    }

    QWidget* TargetContextButtonAction::createWidget(QWidget* pParent)
    {
        return aznew  TargetContextButton(pParent);
    }
}
