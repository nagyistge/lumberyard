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

// include the required headers
#include "EMStudioManager.h"
#include "MainWindow.h"
#include "ToolBarPlugin.h"


namespace EMStudio
{
    // constructor
    ToolBarPlugin::ToolBarPlugin()
        : EMStudioPlugin()
    {
        mBar = nullptr;
    }


    // destructor
    ToolBarPlugin::~ToolBarPlugin()
    {
        if (mBar)
        {
            EMStudio::GetMainWindow()->removeToolBar(mBar);
            delete mBar;
        }
    }


    // check if we have a window that uses this object name
    bool ToolBarPlugin::GetHasWindowWithObjectName(const MCore::String& objectName)
    {
        if (mBar == nullptr)
        {
            return false;
        }

        // check if the object name is equal to the one of the dock widget
        return (objectName.CheckIfIsEqual(FromQtString(mBar->objectName()).AsChar()));
    }


    // create the base interface
    void ToolBarPlugin::CreateBaseInterface(const char* objectName)
    {
        // get the main window
        QMainWindow* mainWindow = (QMainWindow*)GetMainWindow();

        // create the toolbar
        mBar = new QToolBar(GetName());
        mBar->setAllowedAreas(GetAllowedAreas());
        mBar->setFloatable(GetIsFloatable());
        mBar->setMovable(GetIsMovable());
        mBar->setOrientation(GetIsVertical() ? Qt::Vertical : Qt::Horizontal);
        mBar->setToolButtonStyle(GetToolButtonStyle());

        // add the toolbar to the main window
        mainWindow->addToolBar(GetToolBarCreationArea(), mBar);

        // set the object name
        if (objectName == nullptr)
        {
            mBar->setObjectName(GetPluginManager()->GenerateObjectName());
        }
        else
        {
            mBar->setObjectName(objectName);
        }
    }


    // set the interface title
    void ToolBarPlugin::SetInterfaceTitle(const char* name)
    {
        if (mBar)
        {
            mBar->setWindowTitle(name);
        }
    }
}   // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/ToolBarPlugin.moc>