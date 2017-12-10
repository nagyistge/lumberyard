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

#include "Commands.h"
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <MCore/Source/AttributeSet.h>
#include <MCore/Source/Endian.h>
#include <MCore/Source/FileSystem.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/ExporterFileProcessor.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MainWindow.h>

#include <SceneAPIExt/Rules/MetaDataRule.h>
#include <SceneAPIExt/Groups/MotionGroup.h>
#include <SceneAPIExt/Groups/ActorGroup.h>


namespace EMStudio
{
    //--------------------------------------------------------------------------------
    // CommandSaveActorAssetInfo
    //--------------------------------------------------------------------------------
    CommandSaveActorAssetInfo::CommandSaveActorAssetInfo(MCore::Command* orgCommand)
        : MCore::Command("SaveActorAssetInfo", orgCommand)
    {
    }


    CommandSaveActorAssetInfo::~CommandSaveActorAssetInfo()
    {
    }


    bool CommandSaveActorAssetInfo::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const uint32 actorID = parameters.GetValueAsInt("actorID", this);

        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult.Format("Actor cannot be saved. Actor with id '%i' does not exist.", actorID);
            return false;
        }

        AZStd::string productFilename = actor->GetFileName();

        // Get the group name from the product filename (assuming the product filename is the group name).
        AZStd::string groupName;
        if (!AzFramework::StringFunc::Path::GetFileName(productFilename.c_str(), groupName))
        {
            outResult.Format("Cannot get product name from asset cache file '%s'.", productFilename.c_str());
            return false;
        }

        // Get the full file path for the asset source file based on the product filename.
        bool fullPathFound = false;
        AZStd::string sourceAssetFilename;
        EBUS_EVENT_RESULT(fullPathFound, AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, productFilename, sourceAssetFilename);

        // Generate meta data command for all changes being made to the actor.
        const AZStd::string metaDataString = CommandSystem::MetaData::GenerateActorMetaData(actor);

        // Save meta data commands to the manifest.
        const bool saveResult = EMotionFX::Pipeline::Rule::MetaDataRule::SaveMetaDataToFile<EMotionFX::Pipeline::Group::ActorGroup>(sourceAssetFilename, groupName, metaDataString);
        if (saveResult)
        {
            actor->SetDirtyFlag(false);
        }

        return saveResult;
    }


    bool CommandSaveActorAssetInfo::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandSaveActorAssetInfo::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("actorID", "The id of the actor to save.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    const char* CommandSaveActorAssetInfo::GetDescription() const
    {
        return "Save the .assetinfo of a actor.";
    }


    //--------------------------------------------------------------------------------
    // CommandSaveMotionAssetInfo
    //--------------------------------------------------------------------------------
    CommandSaveMotionAssetInfo::CommandSaveMotionAssetInfo(MCore::Command* orgCommand)
        : MCore::Command("SaveMotionAssetInfo", orgCommand)
    {
    }


    CommandSaveMotionAssetInfo::~CommandSaveMotionAssetInfo()
    {
    }


    bool CommandSaveMotionAssetInfo::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionId = parameters.GetValueAsInt("motionID", this);
        outResult.Clear();

        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionId);
        if (motion == nullptr)
        {
            outResult.Format("Motion .assetinfo cannot be saved. Motion with id '%i' does not exist.", motionId);
            return false;
        }

        AZStd::string productFilename = motion->GetFileName();

        // Get the group name from the product filename (assuming the product filename is the group name).
        AZStd::string groupName;
        if (!AzFramework::StringFunc::Path::GetFileName(productFilename.c_str(), groupName))
        {
            outResult.Format("Motion .assetinfo cannot be saved. Cannot get product name from asset cache file '%s'.", productFilename.c_str());
            return false;
        }

        // Get the full file path for the asset source file based on the product filename.
        bool fullPathFound = false;
        AZStd::string sourceAssetFilename;
        EBUS_EVENT_RESULT(fullPathFound, AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, productFilename, sourceAssetFilename);

        // Generate meta data command for all changes being made to the motion.
        const AZStd::string metaDataString = CommandSystem::MetaData::GenerateMotionMetaData(motion);

        // Save meta data commands to the manifest.
        const bool saveResult = EMotionFX::Pipeline::Rule::MetaDataRule::SaveMetaDataToFile<EMotionFX::Pipeline::Group::MotionGroup>(sourceAssetFilename, groupName, metaDataString);
        if (saveResult)
        {
            motion->SetDirtyFlag(false);
        }

        return saveResult;
    }


    bool CommandSaveMotionAssetInfo::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandSaveMotionAssetInfo::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("motionID", "The id of the motion to save.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    const char* CommandSaveMotionAssetInfo::GetDescription() const
    {
        return "Save the .assetinfo of a motion.";
    }



    //--------------------------------------------------------------------------------
    // CommandSaveMotionSet
    //--------------------------------------------------------------------------------
    CommandSaveMotionSet::CommandSaveMotionSet(MCore::Command* orgCommand)
        : MCore::Command("SaveMotionSet", orgCommand)
    {
    }


    CommandSaveMotionSet::~CommandSaveMotionSet()
    {
    }


    void CommandSaveMotionSet::RecursiveAddMotionSets(EMotionFX::MotionSet* motionSet, AZStd::vector<EMotionFX::MotionSet*>& motionSets)
    {
        motionSets.push_back(motionSet);

        const uint32 numChildSets = motionSet->GetNumChildSets();
        for (uint32 i = 0; i < numChildSets; ++i)
        {
            EMotionFX::MotionSet* childSet = motionSet->GetChildSet(i);
            RecursiveAddMotionSets(childSet, motionSets);
        }
    }


    void CommandSaveMotionSet::RecursiveSetDirtyFlag(EMotionFX::MotionSet* motionSet, bool dirtyFlag)
    {
        motionSet->SetDirtyFlag(dirtyFlag);

        const uint32 numChildSets = motionSet->GetNumChildSets();
        for (uint32 i = 0; i < numChildSets; ++i)
        {
            EMotionFX::MotionSet* childSet = motionSet->GetChildSet(i);
            RecursiveSetDirtyFlag(childSet, dirtyFlag);
        }
    }


    bool CommandSaveMotionSet::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (motionSet == nullptr)
        {
            outResult.Format("Motion set cannot be saved. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }


        AZStd::string filename;
        parameters.GetValue("filename", this, filename);

        // Avoid saving to asset cache folder.
        if (!GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename))
        {
            outResult.Format("Motion set cannot be saved. Unable to find source asset path for (%s)", filename);
            return false;
        }
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);


        // Recursively collect motion sets and store them in a flat array.
        AZStd::vector<EMotionFX::MotionSet*> motionSets;
        RecursiveAddMotionSets(motionSet, motionSets);


        const bool fileExisted = AZ::IO::FileIOBase::GetInstance()->Exists(filename.c_str());

        // Source Control: Checkout file.
        if (fileExisted)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, filename.c_str(), "Checking out motion set from source control.", [](int& current, int& max) {});
            AZ_Error("EMotionFX", checkoutResult, "Cannot checkout file '%s' from source control.", filename.c_str());
        }


        // Disable logging and save the motion set.
        MCore::LogCallback::ELogLevel oldLogLevels = MCore::GetLogManager().GetLogLevels();
        MCore::GetLogManager().SetLogLevels((MCore::LogCallback::ELogLevel)(MCore::LogCallback::LOGLEVEL_ERROR | MCore::LogCallback::LOGLEVEL_WARNING));
        ExporterLib::Exporter* exporter = ExporterLib::Exporter::Create();
        bool saveResult = MCore::FileSystem::SaveToFileSecured(filename.c_str(), [exporter, filename, motionSets] { return exporter->SaveMotionSet(filename.c_str(), motionSets, MCore::Endian::ENDIAN_LITTLE); }, EMStudio::GetCommandManager());
        if (saveResult == false)
        {
            exporter->Destroy();
            MCore::GetLogManager().SetLogLevels(oldLogLevels);
            return false;
        }
        exporter->Destroy();
        MCore::GetLogManager().SetLogLevels(oldLogLevels);


        // Source Control: Add file in case it did not exist before (when saving it the first time).
        if (saveResult && !fileExisted)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, filename.c_str(), "Adding motion set to source control.", [](int& current, int& max) {});
            AZ_Error("EMotionFX", checkoutResult, "Cannot add file '%s' to source control.", filename.c_str());
        }

        // Set the new filename.
        if (parameters.GetValueAsBool("updateFilename", this))
        {
            motionSet->SetFilename(filename.c_str());
        }

        // Reset all dirty flags as we saved it.
        if (parameters.GetValueAsBool("updateDirtyFlag", this))
        {
            RecursiveSetDirtyFlag(motionSet, false);
        }
        return true;
    }


    bool CommandSaveMotionSet::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandSaveMotionSet::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddRequiredParameter("filename",    "The filename of the motion set file.",             MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("motionSetID", "The id of the motion set to save.",                MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("updateFilename",      "True to update the filename of the motion set.",   MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "true");
        GetSyntax().AddParameter("updateDirtyFlag",     "True to update the dirty flag of the motion set.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "true");
    }


    const char* CommandSaveMotionSet::GetDescription() const
    {
        return "Save the given motion set to disk.";
    }


    //-------------------------------------------------------------------------------------
    // Save the given anim graph
    //-------------------------------------------------------------------------------------
    CommandSaveAnimGraph::CommandSaveAnimGraph(MCore::Command* orgCommand)
        : MCore::Command("SaveAnimGraph", orgCommand)
    {
    }


    CommandSaveAnimGraph::~CommandSaveAnimGraph()
    {
    }


    bool CommandSaveAnimGraph::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // Get the anim graph index inside the anim graph manager and check if it is in range.
        const int32 animGraphIndex = parameters.GetValueAsInt("index", -1);
        if (animGraphIndex == -1 || animGraphIndex >= (int32)EMotionFX::GetAnimGraphManager().GetNumAnimGraphs())
        {
            outResult = "Cannot save anim graph. Anim graph index is not valid.";
            return false;
        }

        AZStd::string filename;
        parameters.GetValue("filename", this, filename);

        // Avoid saving to asset cache folder.
        if (!GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename))
        {
            outResult.Format("Animation graph cannot be saved. Unable to find source asset path for (%s)", filename);
            return false;
        }
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);


        AZStd::string companyName;
        parameters.GetValue("companyName", this, companyName);

        // get the anim graph from the manager
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(animGraphIndex);


        const bool fileExisted = AZ::IO::FileIOBase::GetInstance()->Exists(filename.c_str());

        // Source Control: Checkout file.
        if (fileExisted)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, filename.c_str(), "Checking out anim graph from source control.", [](int& current, int& max) {});
            AZ_Error("EMotionFX", checkoutResult, "Cannot checkout file '%s' from source control.", filename.c_str());
        }


        const MCore::LogCallback::ELogLevel oldLogLevels = MCore::GetLogManager().GetLogLevels();
        MCore::GetLogManager().SetLogLevels((MCore::LogCallback::ELogLevel)(MCore::LogCallback::LOGLEVEL_ERROR | MCore::LogCallback::LOGLEVEL_WARNING));

        ExporterLib::Exporter* exporter = ExporterLib::Exporter::Create();

        bool saveResult = MCore::FileSystem::SaveToFileSecured(filename.c_str(), [exporter, filename, animGraph, companyName] { return exporter->SaveAnimGraph(filename.c_str(), animGraph, MCore::Endian::ENDIAN_LITTLE, companyName.c_str());
        }, EMStudio::GetCommandManager());
        if (saveResult)
        {
            if (parameters.GetValueAsBool("updateFilename", this))
            {
                animGraph->SetFileName(filename.c_str());
            }
            if (parameters.GetValueAsBool("updateDirtyFlag", this))
            {
                animGraph->SetDirtyFlag(false);
            }
        }
        MCore::GetLogManager().SetLogLevels(oldLogLevels);
        exporter->Destroy();


        // Source Control: Add file in case it did not exist before (when saving it the first time).
        if (saveResult && !fileExisted)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, filename.c_str(), "Adding anim graph to source control.", [](int& current, int& max) {});
            AZ_Error("EMotionFX", checkoutResult, "Cannot add file '%s' to source control.", filename.c_str());
        }

        return saveResult;
    }


    bool CommandSaveAnimGraph::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandSaveAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);
        GetSyntax().AddRequiredParameter("filename",    "The filename of the anim graph file.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("index",       "The index inside the anim graph manager of the anim graph to save.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("updateFilename",      "True to update the filename of the anim graph.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("updateDirtyFlag",     "True to update the dirty flag of the anim graph.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("companyName",        "The company name to which this anim graph belongs to.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    const char* CommandSaveAnimGraph::GetDescription() const
    {
        return "This command saves a anim graph to the given file.";
    }


    //--------------------------------------------------------------------------------
    // CommandSaveWorkspace
    //--------------------------------------------------------------------------------
    CommandSaveWorkspace::CommandSaveWorkspace(MCore::Command* orgCommand)
        : MCore::Command("SaveWorkspace", orgCommand)
    {
    }


    CommandSaveWorkspace::~CommandSaveWorkspace()
    {
    }


    bool CommandSaveWorkspace::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(outResult);

        AZStd::string filename;
        parameters.GetValue("filename", this, filename);

        // Avoid saving to asset cache folder.
        if (!GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename))
        {
            outResult.Format("Workspace cannot be saved. Unable to find source asset path for (%s)", filename);
            return false;
        }
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);


        const bool fileExisted = AZ::IO::FileIOBase::GetInstance()->Exists(filename.c_str());

        // Source Control: Checkout file.
        if (fileExisted)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, filename.c_str(), "Checking out workspace from source control.", [](int& current, int& max) {});
            AZ_Error("EMotionFX", checkoutResult, "Cannot checkout file '%s' from source control.", filename.c_str());
        }


        EMStudio::Workspace* workspace = GetManager()->GetWorkspace();
        const bool saveResult = workspace->Save(filename.c_str());
        if (saveResult)
        {
            workspace->SetDirtyFlag(false);
        }


        // Source Control: Add file in case it did not exist before (when saving it the first time).
        if (saveResult && !fileExisted)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, filename.c_str(), "Adding workspace to source control.", [](int& current, int& max) {});
            AZ_Error("EMotionFX", checkoutResult, "Cannot add file '%s' to source control.", filename.c_str());
        }

        return saveResult;
    }


    bool CommandSaveWorkspace::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandSaveWorkspace::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("filename", "The filename of the workspace.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    const char* CommandSaveWorkspace::GetDescription() const
    {
        return "This command save the workspace.";
    }
} // namespace EMStudio