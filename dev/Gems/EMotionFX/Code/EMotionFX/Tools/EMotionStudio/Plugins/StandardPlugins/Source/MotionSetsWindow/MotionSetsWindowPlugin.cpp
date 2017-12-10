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

#include "MotionSetsWindowPlugin.h"
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QHeaderView>
#include <QApplication>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QContextMenuEvent>
#include <QTableWidget>
#include <MCore/Source/DiskTextFile.h>
#include "../../../../EMStudioSDK/Source/EMStudioCore.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include "../../../../EMStudioSDK/Source/SaveChangedFilesManager.h"
#include <MysticQt/Source/ButtonGroup.h>
#include "../../../../EMStudioSDK/Source/FileManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/IDGenerator.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../MotionWindow/MotionListWindow.h"

#include <AzCore/Debug/Trace.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/ApplicationAPI.h>


namespace EMStudio
{
    class SaveDirtyMotionSetFilesCallback
        : public SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyMotionSetFilesCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS)

    public:
        SaveDirtyMotionSetFilesCallback(MotionSetsWindowPlugin* plugin)
            : SaveDirtyFilesCallback()  { mPlugin = plugin; }
        ~SaveDirtyMotionSetFilesCallback()                                                      {}

        enum
        {
            TYPE_ID = 0x00000002
        };
        uint32 GetType() const override                                         { return TYPE_ID; }
        uint32 GetPriority() const override                                     { return 2; }
        bool GetIsPostProcessed() const override                                { return false; }

        void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) override
        {
            const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
            for (uint32 i = 0; i < numMotionSets; ++i)
            {
                EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);

                if (motionSet->GetIsOwnedByRuntime())
                {
                    continue;
                }

                // only save root motion sets
                if (motionSet->GetParentSet())
                {
                    continue;
                }

                // return in case we found a dirty file
                if (motionSet->GetDirtyFlag())
                {
                    // add the filename to the dirty filenames array
                    outFileNames->push_back(motionSet->GetFilename());

                    // add the link to the actual object
                    ObjectPointer objPointer;
                    objPointer.mMotionSet = motionSet;
                    outObjects->push_back(objPointer);
                }
            }
        }

        int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) override
        {
            MCORE_UNUSED(filenamesToSave);

            const size_t numObjects = objects.size();
            for (size_t i = 0; i < numObjects; ++i)
            {
                // get the current object pointer and skip directly if the type check fails
                ObjectPointer objPointer = objects[i];
                if (objPointer.mMotionSet == nullptr)
                {
                    continue;
                }

                EMotionFX::MotionSet* motionSet = objPointer.mMotionSet;
                if (mPlugin->SaveDirtyMotionSet(motionSet, commandGroup, false) == DirtyFileManager::CANCELED)
                {
                    return DirtyFileManager::CANCELED;
                }
            }

            return DirtyFileManager::FINISHED;
        }

        const char* GetExtension() const override       { return "motionset"; }
        const char* GetFileType() const override        { return "motion set"; }

    private:
        MotionSetsWindowPlugin* mPlugin;
    };


    class MotionSetsWindowPluginEventHandler
        : public EMotionFX::EventHandler
    {
    public:
        void OnDeleteMotionSet(EMotionFX::MotionSet* motionSet) override
        {
            if (motionSet == nullptr)
            {
                return;
            }

            OutlinerManager* manager = GetOutlinerManager();
            if (manager == nullptr)
            {
                return;
            }

            OutlinerCategory* category = manager->FindCategoryByName("Motion Sets");
            if (category == nullptr)
            {
                return;
            }

            category->RemoveItem(motionSet->GetID());
        }
    };


    class MotionSetsOutlinerCategoryCallback
        : public OutlinerCategoryCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(MotionSetsOutlinerCategoryCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS)

    public:
        MotionSetsOutlinerCategoryCallback(MotionSetsWindowPlugin* plugin)
        {
            mPlugin = plugin;
        }

        ~MotionSetsOutlinerCategoryCallback()
        {
        }

        QString BuildNameItem(OutlinerCategoryItem* item) const override
        {
            EMotionFX::MotionSet* motionSet = static_cast<EMotionFX::MotionSet*>(item->mUserData);
            return motionSet->GetName();
        }

        QString BuildToolTipItem(OutlinerCategoryItem* item) const override
        {
            EMotionFX::MotionSet* motionSet = static_cast<EMotionFX::MotionSet*>(item->mUserData);
            const MCore::String relativeFileName = MCore::String(motionSet->GetFilename()).ExtractPathRelativeTo(EMotionFX::GetEMotionFX().GetMediaRootFolder());
            QString toolTip = "<table border=\"0\">";
            toolTip += "<tr><td><p style='white-space:pre'><b>Name: </b></p></td>";
            toolTip += QString("<td><p style='color:rgb(115, 115, 115); white-space:pre'>%1</p></td></tr>").arg(motionSet->GetNameString().empty() ? "&#60;no name&#62;" : motionSet->GetName());
            toolTip += "<tr><td><p style='white-space:pre'><b>FileName: </b></p></td>";
            toolTip += QString("<td><p style='color:rgb(115, 115, 115); white-space:pre'>%1</p></td></tr>").arg(relativeFileName.GetIsEmpty() ? "&#60;not saved yet&#62;" : relativeFileName.AsChar());
            toolTip += "<tr><td><p style='white-space:pre'><b>Num Motions: </b></p></td>";
            toolTip += QString("<td><p style='color:rgb(115, 115, 115); white-space:pre'>%1</p></td></tr>").arg(motionSet->GetNumMotionEntries());
            toolTip += "<tr><td><p style='white-space:pre'><b>Num Child Sets: </b></p></td>";
            toolTip += QString("<td><p style='color:rgb(115, 115, 115); white-space:pre'>%1</p></td></tr>").arg(motionSet->GetNumChildSets());
            toolTip += "</table>";
            return toolTip;
        }

        QIcon GetIcon(OutlinerCategoryItem* item) const override
        {
            MCORE_UNUSED(item);
            return MysticQt::GetMysticQt()->FindIcon("Images/OutlinerPlugin/MotionSetsCategory.png");
        }

        void OnRemoveItems(QWidget* parent, const AZStd::vector<OutlinerCategoryItem*>& items, MCore::CommandGroup* commandGroup) override
        {
            // ask to remove motions
            bool removeMotions;
            if (QMessageBox::question(parent, "Remove Motions From Project?", "Remove the motions from the project entirely? This would also remove them from the motion list. Pressing no will remove them from the motion set but keep them inside the motion list inside the motions window.", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
            {
                removeMotions = true;
            }
            else
            {
                removeMotions = false;
            }

            // clear the failed remove array
            mFailedRemoveMotions.clear();

            // remove each item
            const size_t numItems = items.size();
            for (size_t i = 0; i < numItems; ++i)
            {
                // get the current motion set and only process the root sets
                EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(items[i]->mID);

                if (motionSet->GetIsOwnedByRuntime())
                {
                    continue;
                }

                // in case we modified the motion set ask if the user wants to save changes it before removing it
                mPlugin->SaveDirtyMotionSet(motionSet, nullptr, true, false);

                // recursively increase motions reference count
                RecursiveIncreaseMotionsReferenceCount(motionSet);

                // recursively remove motion sets (post order traversing)
                CommandSystem::RecursivelyRemoveMotionSets(motionSet, *commandGroup);

                // Recursively remove motions from motion sets.
                if (removeMotions)
                {
                    MotionSetManagementWindow::RecursiveRemoveMotionsFromSet(motionSet, *commandGroup, mFailedRemoveMotions);
                }
            }
        }

        void OnPostRemoveItems(QWidget* parent) override
        {
            if (!mFailedRemoveMotions.empty())
            {
                MotionListRemoveMotionsFailedWindow removeMotionsFailedWindow(parent, mFailedRemoveMotions);
                removeMotionsFailedWindow.exec();
            }
        }

        void OnLoadItem(QWidget* parent) override
        {
            AZStd::string filename = GetMainWindow()->GetFileManager()->LoadMotionSetFileDialog(parent);
            mPlugin->LoadMotionSet(filename);
        }

    private:
        void RecursiveIncreaseMotionsReferenceCount(EMotionFX::MotionSet* motionSet)
        {
            // Increase the reference counter if needed for each motion.
            const EMotionFX::MotionSet::EntryMap& motionEntries = motionSet->GetMotionEntries();
            for (const auto& item : motionEntries)
            {
                const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;

                // Check the reference counter if only one reference registered.
                // Two is needed because the remove motion command has to be called to have the undo/redo possible
                // without it the motion list is also not updated because the remove motion callback is not called.
                // Also avoid the issue to remove from set but not the motion list, to keep it in the motion list.
                if (motionEntry->GetMotion() && motionEntry->GetMotion()->GetReferenceCount() == 1)
                {
                    motionEntry->GetMotion()->IncreaseReferenceCount();
                }
            }

            // Get the number of child sets and recursively call.
            const uint32 numChildSets = motionSet->GetNumChildSets();
            for (uint32 i = 0; i < numChildSets; ++i)
            {
                EMotionFX::MotionSet* childSet = motionSet->GetChildSet(i);
                RecursiveIncreaseMotionsReferenceCount(childSet);
            }
        }

    private:
        MotionSetsWindowPlugin*             mPlugin;
        AZStd::vector<EMotionFX::Motion*>   mFailedRemoveMotions;
    };


    // constructor
    MotionSetsWindowPlugin::MotionSetsWindowPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        mDialogStack                    = nullptr;
        mSelectedSet                    = nullptr;
        mCreateMotionSetCallback        = nullptr;
        mRemoveMotionSetCallback        = nullptr;
        mSaveMotionSetCallback          = nullptr;
        mAdjustMotionSetCallback        = nullptr;
        mMotionSetAddMotionCallback     = nullptr;
        mMotionSetRemoveMotionCallback  = nullptr;
        mMotionSetAdjustMotionCallback  = nullptr;
        mLoadMotionSetCallback          = nullptr;
        //mStringIDWindow               = nullptr;
        mMotionSetManagementWindow      = nullptr;
        mMotionSetWindow                = nullptr;
        mDirtyFilesCallback             = nullptr;
        mOutlinerCategoryCallback       = nullptr;
    }


    // destructor
    MotionSetsWindowPlugin::~MotionSetsWindowPlugin()
    {
        GetCommandManager()->RemoveCommandCallback(mCreateMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSaveMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustMotionSetCallback, false);
        GetCommandManager()->RemoveCommandCallback(mMotionSetAddMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mMotionSetRemoveMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mMotionSetAdjustMotionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mLoadMotionSetCallback, false);

        delete mCreateMotionSetCallback;
        delete mRemoveMotionSetCallback;
        delete mSaveMotionSetCallback;
        delete mAdjustMotionSetCallback;
        delete mMotionSetAddMotionCallback;
        delete mMotionSetRemoveMotionCallback;
        delete mMotionSetAdjustMotionCallback;
        delete mLoadMotionSetCallback;

        GetMainWindow()->GetDirtyFileManager()->RemoveCallback(mDirtyFilesCallback, false);
        delete mDirtyFilesCallback;

        EMotionFX::GetEventManager().RemoveEventHandler(mEventHandler, true);

        // unregister the outliner category
        GetOutlinerManager()->UnregisterCategory("Motion Sets");
        delete mOutlinerCategoryCallback;
    }


    // clone the log window
    EMStudioPlugin* MotionSetsWindowPlugin::Clone()
    {
        MotionSetsWindowPlugin* newPlugin = new MotionSetsWindowPlugin();
        return newPlugin;
    }


    // init after the parent dock window has been created
    bool MotionSetsWindowPlugin::Init()
    {
        //LogDebug("Initializing motion sets window.");

        mCreateMotionSetCallback        = new CommandCreateMotionSetCallback(false);
        mRemoveMotionSetCallback        = new CommandRemoveMotionSetCallback(false);
        mSaveMotionSetCallback          = new CommandSaveMotionSetCallback(false);
        mAdjustMotionSetCallback        = new CommandAdjustMotionSetCallback(false);
        mMotionSetAddMotionCallback     = new CommandMotionSetAddMotionCallback(false);
        mMotionSetRemoveMotionCallback  = new CommandMotionSetRemoveMotionCallback(false, true);
        mMotionSetAdjustMotionCallback  = new CommandMotionSetAdjustMotionCallback(false);
        mLoadMotionSetCallback          = new CommandLoadMotionSetCallback(false);

        GetCommandManager()->RegisterCommandCallback("CreateMotionSet", mCreateMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveMotionSet", mRemoveMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("SaveMotionSet", mSaveMotionSetCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustMotionSet", mAdjustMotionSetCallback);

        GetCommandManager()->RegisterCommandCallback("MotionSetAddMotion", mMotionSetAddMotionCallback);
        GetCommandManager()->RegisterCommandCallback("MotionSetRemoveMotion", mMotionSetRemoveMotionCallback);
        GetCommandManager()->RegisterCommandCallback("MotionSetAdjustMotion", mMotionSetAdjustMotionCallback);
        GetCommandManager()->RegisterCommandCallback("LoadMotionSet", mLoadMotionSetCallback);

        // create the dialog stack
        assert(mDialogStack == nullptr);
        mDialogStack = new MysticQt::DialogStack(mDock);
        mDock->SetContents(mDialogStack);

        // connect the window activation signal to refresh if reactivated
        connect(mDock, SIGNAL(visibilityChanged(bool)), this, SLOT(WindowReInit(bool)));

        // create the set management window
        mMotionSetManagementWindow = new MotionSetManagementWindow(this, mDialogStack);
        mMotionSetManagementWindow->Init();
        mDialogStack->Add(mMotionSetManagementWindow, "Motion Set Management", false, true, true, false);

        // create the motion set properties window
        mMotionSetWindow = new MotionSetWindow(this, mDialogStack);
        mMotionSetWindow->Init();
        mDialogStack->Add(mMotionSetWindow, "Motion Set", false, true);

        ReInit();
        SetSelectedSet(nullptr);

        // initialize the dirty files callback
        mDirtyFilesCallback = new SaveDirtyMotionSetFilesCallback(this);
        GetMainWindow()->GetDirtyFileManager()->AddCallback(mDirtyFilesCallback);

        // register the outliner category
        mOutlinerCategoryCallback = new MotionSetsOutlinerCategoryCallback(this);
        OutlinerCategory* outlinerCategory = GetOutlinerManager()->RegisterCategory("Motion Sets", mOutlinerCategoryCallback);

        // add each item in the outliner category
        const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);

            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }

            OutlinerCategoryItem* outlinerCategoryItem = new OutlinerCategoryItem();
            outlinerCategoryItem->mID = motionSet->GetID();
            outlinerCategoryItem->mUserData = motionSet;
            outlinerCategory->AddItem(outlinerCategoryItem);
        }

        // add the event handler
        mEventHandler = new MotionSetsWindowPluginEventHandler();
        EMotionFX::GetEventManager().AddEventHandler(mEventHandler);

        return true;
    }


    EMotionFX::MotionSet* MotionSetsWindowPlugin::GetSelectedSet() const
    {
        if (EMotionFX::GetMotionManager().FindMotionSetIndex(mSelectedSet) == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        return mSelectedSet;
    }


    void MotionSetsWindowPlugin::ReInit()
    {
        // Validate existence of selected motion set and reset selection in case selection is invalid.
        if (EMotionFX::GetMotionManager().FindMotionSetIndex(mSelectedSet) == MCORE_INVALIDINDEX32)
        {
            mSelectedSet = nullptr;
        }

        SetSelectedSet(mSelectedSet);
        mMotionSetManagementWindow->ReInit();
        mMotionSetWindow->ReInit();
    }


    int MotionSetsWindowPlugin::SaveDirtyMotionSet(EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton)
    {
        // only save root motion sets
        if (motionSet->GetParentSet())
        {
            return DirtyFileManager::NOFILESTOSAVE;
        }

        // only process changed files
        if (motionSet->GetDirtyFlag() == false)
        {
            return DirtyFileManager::NOFILESTOSAVE;
        }

        if (askBeforeSaving)
        {
            EMStudio::GetApp()->setOverrideCursor(QCursor(Qt::ArrowCursor));

            QMessageBox msgBox(GetMainWindow());
            AZStd::string text;
            const AZStd::string& filename = motionSet->GetFilenameString();
            AZStd::string extension;
            AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false);

            if (!filename.empty() && !extension.empty())
            {
                text = AZStd::string::format("Save changes to '%s'?", motionSet->GetFilename());
            }
            else if (!motionSet->GetNameString().empty())
            {
                text = AZStd::string::format("Save changes to the motion set named '%s'?", motionSet->GetName());
            }
            else
            {
                text = "Save changes to untitled motion set?";
            }

            msgBox.setText(text.c_str());
            msgBox.setWindowTitle("Save Changes");

            if (showCancelButton)
            {
                msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            }
            else
            {
                msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard);
            }

            msgBox.setDefaultButton(QMessageBox::Save);
            msgBox.setIcon(QMessageBox::Question);


            int messageBoxResult = msgBox.exec();
            switch (messageBoxResult)
            {
            case QMessageBox::Save:
            {
                GetMainWindow()->GetFileManager()->SaveMotionSet(GetManagementWindow(), motionSet, commandGroup);
                break;
            }
            case QMessageBox::Discard:
            {
                EMStudio::GetApp()->restoreOverrideCursor();
                return DirtyFileManager::FINISHED;
            }
            case QMessageBox::Cancel:
            {
                EMStudio::GetApp()->restoreOverrideCursor();
                return DirtyFileManager::CANCELED;
            }
            }
        }
        else
        {
            // save without asking first
            GetMainWindow()->GetFileManager()->SaveMotionSet(GetManagementWindow(), motionSet, commandGroup);
        }

        return DirtyFileManager::FINISHED;
    }


    void MotionSetsWindowPlugin::SetSelectedSet(EMotionFX::MotionSet* motionSet)
    {
        mSelectedSet = motionSet;

        if (motionSet)
        {
            mMotionSetManagementWindow->SelectItemsByName(motionSet->GetName());
        }
        mMotionSetManagementWindow->ReInit();
        mMotionSetManagementWindow->UpdateInterface();
        mMotionSetWindow->ReInit();
        mMotionSetWindow->UpdateInterface();
    }


    // reinit the window when it gets activated
    void MotionSetsWindowPlugin::WindowReInit(bool visible)
    {
        if (visible)
        {
            ReInit();
        }
    }


    void MotionSetsWindowPlugin::LoadMotionSet(AZStd::string filename)
    {
        if (filename.empty())
        {
            return;
        }

        // Auto-relocate to asset source folder.
        if (!GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename))
        {
            const AZStd::string errorString = AZStd::string::format("Unable to find MotionSet -filename \"%s\"", filename.c_str());
            AZ_Error("EMotionFX", false, errorString.c_str());
            return;
        }

        const AZStd::string command = AZStd::string::format("LoadMotionSet -filename \"%s\"", filename.c_str());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    //-----------------------------------------------------------------------------------------
    // Command callbacks
    //-----------------------------------------------------------------------------------------
    bool ReInitMotionSetsPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (motionSetsPlugin->GetDockWidget()->visibleRegion().isEmpty() == false)
        {
            motionSetsPlugin->ReInit();
        }

        return true;
    }


    bool UpdateMotionSetsPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (motionSetsPlugin->GetDockWidget()->visibleRegion().isEmpty() == false)
        {
            motionSetsPlugin->SetSelectedSet(motionSetsPlugin->GetSelectedSet());
        }

        return true;
    }


    bool MotionSetsWindowPlugin::GetMotionSetCommandInfo(MCore::Command* command, const MCore::CommandLine& parameters, EMotionFX::MotionSet** outMotionSet, MotionSetsWindowPlugin** outPlugin)
    {
        // Get the motion set id and find the corresponding motion set.
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", command);
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            return false;
        }

        // Get the plugin and add the motion entry to the user interface.
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }
        MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;

        *outMotionSet = motionSet;
        *outPlugin = motionSetsPlugin;
        return true;
    }



    bool MotionSetsWindowPlugin::CommandCreateMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        CommandSystem::CommandCreateMotionSet* createMotionSetCommand = static_cast<CommandSystem::CommandCreateMotionSet*>(command);
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(createMotionSetCommand->mPreviouslyUsedID);
        OutlinerCategoryItem* outlinerCategoryItem = new OutlinerCategoryItem();
        outlinerCategoryItem->mID = motionSet->GetID();
        outlinerCategoryItem->mUserData = motionSet;
        GetOutlinerManager()->FindCategoryByName("Motion Sets")->AddItem(outlinerCategoryItem);
        return ReInitMotionSetsPlugin();
    }


    bool MotionSetsWindowPlugin::CommandCreateMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                   { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionSetsPlugin(); }


    bool MotionSetsWindowPlugin::CommandRemoveMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        CommandSystem::CommandRemoveMotionSet* removeMotionSetCommand = static_cast<CommandSystem::CommandRemoveMotionSet*>(command);
        GetOutlinerManager()->FindCategoryByName("Motion Sets")->RemoveItem(removeMotionSetCommand->mPreviouslyUsedID);
        return ReInitMotionSetsPlugin();
    }


    bool MotionSetsWindowPlugin::CommandRemoveMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                   { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionSetsPlugin(); }


    bool MotionSetsWindowPlugin::CommandSaveMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        GetOutlinerManager()->FireItemModifiedEvent();
        return true;
    }


    bool MotionSetsWindowPlugin::CommandSaveMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return true;
    }


    bool MotionSetsWindowPlugin::CommandAdjustMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);

        GetOutlinerManager()->FireItemModifiedEvent();

        if (commandLine.CheckIfHasParameter("newName"))
        {
            // add it to the interfaces
            EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
            if (plugin == nullptr)
            {
                return false;
            }

            MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;
            motionSetsPlugin->GetManagementWindow()->ReInit();
        }

        return true;
    }


    bool MotionSetsWindowPlugin::CommandAdjustMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);

        GetOutlinerManager()->FireItemModifiedEvent();

        if (commandLine.CheckIfHasParameter("newName"))
        {
            // add it to the interfaces
            EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
            if (plugin == nullptr)
            {
                return false;
            }

            MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;
            motionSetsPlugin->GetManagementWindow()->ReInit();
        }

        return true;
    }


    bool MotionSetsWindowPlugin::CommandMotionSetAddMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        EMotionFX::MotionSet* motionSet = nullptr;
        MotionSetsWindowPlugin* plugin = nullptr;
        if (!MotionSetsWindowPlugin::GetMotionSetCommandInfo(command, commandLine, &motionSet, &plugin))
        {
            return false;
        }

        // Get the motion id.
        AZStd::string motionId;
        commandLine.GetValue("idString", command, motionId);

        // Find the corresponding motion entry.
        EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryByStringID(motionId.c_str());
        if (!motionEntry)
        {
            return false;
        }

        return plugin->GetMotionSetWindow()->AddMotion(motionSet, motionEntry);
    }


    bool MotionSetsWindowPlugin::CommandMotionSetAddMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        // Calls the MotionSetRemoveMotion command internally, so the callback from that is already called.
        return true;
    }


    bool MotionSetsWindowPlugin::CommandMotionSetRemoveMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        EMotionFX::MotionSet* motionSet = nullptr;
        MotionSetsWindowPlugin* plugin = nullptr;
        if (!MotionSetsWindowPlugin::GetMotionSetCommandInfo(command, commandLine, &motionSet, &plugin))
        {
            return false;
        }

        // Get the motion id.
        AZStd::string motionId;
        commandLine.GetValue("idString", command, motionId);

        // Find the corresponding motion entry.
        EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryByStringID(motionId.c_str());
        if (!motionEntry)
        {
            return false;
        }

        return plugin->GetMotionSetWindow()->RemoveMotion(motionSet, motionEntry);
    }


    bool MotionSetsWindowPlugin::CommandMotionSetRemoveMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        // Calls the MotionSetAddMotion command internally, so the callback from that is already called.
        return true;
    }


    bool CommandMotionSetAdjustMotionCallbackHelper(MCore::Command* command, const MCore::CommandLine& commandLine, const char* newMotionId, const char* oldMotionId)
    {
        EMotionFX::MotionSet* motionSet = nullptr;
        MotionSetsWindowPlugin* plugin = nullptr;
        if (!MotionSetsWindowPlugin::GetMotionSetCommandInfo(command, commandLine, &motionSet, &plugin))
        {
            return false;
        }

        // Find the corresponding motion entry.
        EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryByStringID(newMotionId);
        if (!motionEntry)
        {
            return false;
        }

        return plugin->GetMotionSetWindow()->UpdateMotion(motionSet, motionEntry, oldMotionId);
    }


    bool MotionSetsWindowPlugin::CommandMotionSetAdjustMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        // Get the previous motion id.
        AZStd::string newMotionId;
        AZStd::string oldMotionId;
        if (commandLine.CheckIfHasParameter("newIDString"))
        {
            commandLine.GetValue("newIDString", command, newMotionId);
            commandLine.GetValue("idString", command, oldMotionId);
        }
        else
        {
            commandLine.GetValue("idString", command, newMotionId);
            commandLine.GetValue("idString", command, oldMotionId);
        }

        return CommandMotionSetAdjustMotionCallbackHelper(command, commandLine, newMotionId.c_str(), oldMotionId.c_str());
    }


    bool MotionSetsWindowPlugin::CommandMotionSetAdjustMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        // Calls the MotionSetAdjustMotion command internally, so the callback from that is already called.
        return true;
    }


    bool MotionSetsWindowPlugin::CommandLoadMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);

        AZStd::string filename;
        commandLine.GetValue("filename", command, filename);
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByFileName(filename.c_str());
        if (motionSet == nullptr)
        {
            AZ_Error("Animation", false, "Cannot find motion set.");
            return false;
        }

        OutlinerCategoryItem* outlinerCategoryItem = new OutlinerCategoryItem();
        outlinerCategoryItem->mID = motionSet->GetID();
        outlinerCategoryItem->mUserData = motionSet;
        GetOutlinerManager()->FindCategoryByName("Motion Sets")->AddItem(outlinerCategoryItem);

        const EMotionFX::MotionSet::EntryMap& motionEntries = motionSet->GetMotionEntries();
        for (const auto& item : motionEntries)
        {
            const EMotionFX::MotionSet::MotionEntry* motionEntry = item.second;
            EMotionFX::Motion* motion = motionEntry->GetMotion();
            if (!motion)
            {
                continue;
            }

            OutlinerCategoryItem* motionOutlinerCategoryItem = new OutlinerCategoryItem();
            motionOutlinerCategoryItem->mID = motion->GetID();
            motionOutlinerCategoryItem->mUserData = motion;
            GetOutlinerManager()->FindCategoryByName("Motions")->AddItem(motionOutlinerCategoryItem);
        }

        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;

        // select the first motion set
        if (EMotionFX::GetMotionManager().GetNumMotionSets() > 0)
        {
            const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();

            for (uint32 i = 0; i < numMotionSets; ++i)
            {
                EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(0);

                if (motionSet->GetIsOwnedByRuntime())
                {
                    continue;
                }

                motionSetsPlugin->SetSelectedSet(motionSet);
                break;
            }
        }

        return ReInitMotionSetsPlugin();
    }


    bool MotionSetsWindowPlugin::CommandLoadMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return ReInitMotionSetsPlugin();
    }


    int MotionSetsWindowPlugin::OnSaveDirtyMotionSets()
    {
        return GetMainWindow()->GetDirtyFileManager()->SaveDirtyFiles(SaveDirtyMotionSetFilesCallback::TYPE_ID);
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.moc>