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

#include <AzCore/Debug/Timer.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include "Exporter.h"
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/EventManager.h>
#include <MCore/Source/AttributeSet.h>


//#define EMFX_DETAILED_SAVING_PERFORMANCESTATS

#ifdef EMFX_DETAILED_SAVING_PERFORMANCESTATS
    #define EMFX_DETAILED_SAVING_PERFORMANCESTATS_START(TIMERNAME)      AZ::Debug::Timer TIMERNAME; TIMERNAME.Stamp();
    #define EMFX_DETAILED_SAVING_PERFORMANCESTATS_END(TIMERNAME, TEXT)  const float saveTime = TIMERNAME.GetDeltaTimeInSeconds(); MCore::LogError("Saving %s took %.2f ms.", TEXT, saveTime * 1000.0f);
#else
    #define EMFX_DETAILED_SAVING_PERFORMANCESTATS_START(TIMERNAME)
    #define EMFX_DETAILED_SAVING_PERFORMANCESTATS_END(TIMERNAME, TEXT)
#endif


namespace ExporterLib
{
    // save the actor to a memory file
    void SaveActor(MCore::MemoryFile* file, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        if (actor == nullptr)
        {
            MCore::LogError("SaveActor: Passed actor is not valid.");
            return;
        }

        // clone our actor before saving as we will modify its data
        actor = actor->Clone();

        // update the OBBs for the highest detail level
        EMotionFX::GetEventManager().OnSubProgressText("Calculating OOBs");
        EMotionFX::GetEventManager().OnSubProgressValue(0.0f);

        EMFX_DETAILED_SAVING_PERFORMANCESTATS_START(obbTimer);
        actor->UpdateNodeBindPoseOBBs(0);
        EMFX_DETAILED_SAVING_PERFORMANCESTATS_END(obbTimer, "obbs");

        AZ::Debug::Timer saveTimer;
        saveTimer.Stamp();

        // save header
        SaveActorHeader(file, targetEndianType);

        // save actor info
        MCore::String sourceApplication = actor->GetAttributeSet()->GetStringAttribute("sourceApplication");
        MCore::String originalFileName  = actor->GetAttributeSet()->GetStringAttribute("originalFileName");
        SaveActorFileInfo(file, actor->GetNumLODLevels(), actor->GetMotionExtractionNodeIndex(), sourceApplication.AsChar(), originalFileName.AsChar(), actor->GetName(), /*actor->GetRetargetOffset()*/ 0.0f, actor->GetUnitType(), targetEndianType);

        // save nodes
        EMotionFX::GetEventManager().OnSubProgressText("Saving nodes");
        EMotionFX::GetEventManager().OnSubProgressValue(35.0f);

        EMFX_DETAILED_SAVING_PERFORMANCESTATS_START(nodeTimer);
        SaveNodes(file, actor, targetEndianType);
        EMFX_DETAILED_SAVING_PERFORMANCESTATS_END(nodeTimer, "nodes");

        SaveLimits(file, actor, targetEndianType);
        SaveNodeGroups(file, actor, targetEndianType);
        SaveNodeMotionSources(file, actor, nullptr, targetEndianType);
        SaveAttachmentNodes(file, actor, targetEndianType);

        // save materials
        EMotionFX::GetEventManager().OnSubProgressText("Saving materials");
        EMotionFX::GetEventManager().OnSubProgressValue(45.0f);

        EMFX_DETAILED_SAVING_PERFORMANCESTATS_START(materialTimer);
        SaveMaterials(file, actor, targetEndianType);
        EMFX_DETAILED_SAVING_PERFORMANCESTATS_END(materialTimer, "materials");

        // save meshes
        EMotionFX::GetEventManager().OnSubProgressText("Saving meshes");
        EMotionFX::GetEventManager().OnSubProgressValue(50.0f);

        EMFX_DETAILED_SAVING_PERFORMANCESTATS_START(meshTimer);
        SaveMeshes(file, actor, targetEndianType);
        EMFX_DETAILED_SAVING_PERFORMANCESTATS_END(meshTimer, "meshes");

        // save skins
        EMotionFX::GetEventManager().OnSubProgressText("Saving skins");
        EMotionFX::GetEventManager().OnSubProgressValue(75.0f);

        EMFX_DETAILED_SAVING_PERFORMANCESTATS_START(skinTimer);
        SaveSkins(file, actor, targetEndianType);
        EMFX_DETAILED_SAVING_PERFORMANCESTATS_END(skinTimer, "skins");

        // save morph targets
        EMotionFX::GetEventManager().OnSubProgressText("Saving morph targets");
        EMotionFX::GetEventManager().OnSubProgressValue(90.0f);

        EMFX_DETAILED_SAVING_PERFORMANCESTATS_START(morphTargetTimer);
        SaveMorphTargets(file, actor, targetEndianType);
        EMFX_DETAILED_SAVING_PERFORMANCESTATS_END(morphTargetTimer, "morph targets");

        // get rid of the memory again and unregister the actor
        actor->Destroy();

        const float saveTime = saveTimer.GetDeltaTimeInSeconds() * 1000.0f;
        MCore::LogInfo("Actor saved in %.2f ms.", saveTime);

        // finished sub progress
        EMotionFX::GetEventManager().OnSubProgressText("");
        EMotionFX::GetEventManager().OnSubProgressValue(100.0f);
    }


    // save the actor to disk
    bool SaveActor(AZStd::string& filename, EMotionFX::Actor* actor, MCore::Endian::EEndianType targetEndianType)
    {
        if (filename.empty())
        {
            AZ_Error("EMotionFX", false, "Cannot save actor. Filename is empty.");
            return false;
        }

        MCore::MemoryFile memoryFile;
        memoryFile.Open();
        memoryFile.SetPreAllocSize(262144); // 256kb

        // Save the actor to the memory file.
        SaveActor(&memoryFile, actor, targetEndianType);

        // Make sure the file has the correct extension and write the data from memory to disk.
        AzFramework::StringFunc::Path::ReplaceExtension(filename, GetActorExtension());
        memoryFile.SaveToDiskFile(filename.c_str());
        memoryFile.Close();
        return true;
    }

} // namespace ExporterLib
