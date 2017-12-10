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
#include "MotionCommands.h"
#include "CommandManager.h"

#include <MCore/Source/Compare.h>
#include <MCore/Source/FileSystem.h>
#include <MCore/Source/AttributeSet.h>

#include <EMotionFX/Source/MotionSystem.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/SkeletalMotion.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <EMotionFX/Source/WaveletSkeletalMotion.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/ExporterFileProcessor.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>

#include <AzFramework/API/ApplicationAPI.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandPlayMotion
    //--------------------------------------------------------------------------------

    // constructor
    CommandPlayMotion::CommandPlayMotion(MCore::Command* orgCommand)
        : MCore::Command("PlayMotion", orgCommand)
    {
    }


    // destructor
    CommandPlayMotion::~CommandPlayMotion()
    {
    }


    AZStd::string CommandPlayMotion::PlayBackInfoToCommandParameters(EMotionFX::PlayBackInfo* playbackInfo)
    {
        return AZStd::string::format("-blendInTime %f -blendOutTime %f -playSpeed %f -targetWeight %f -eventWeightThreshold %f -maxPlayTime %f -numLoops %i -priorityLevel %i -startNodeIndex %i -blendMode %i -playMode %i -mirrorMotion %s -mix %s -playNow %s -motionExtraction %s -retarget %s -freezeAtLastFrame %s -enableMotionEvents %s -blendOutBeforeEnded %s -canOverwrite %s -deleteOnZeroWeight %s",
            playbackInfo->mBlendInTime,
            playbackInfo->mBlendOutTime,
            playbackInfo->mPlaySpeed,
            playbackInfo->mTargetWeight,
            playbackInfo->mEventWeightThreshold,
            playbackInfo->mMaxPlayTime,
            playbackInfo->mNumLoops,
            playbackInfo->mPriorityLevel,
            playbackInfo->mStartNodeIndex,
            playbackInfo->mBlendMode,
            playbackInfo->mPlayMode,
            playbackInfo->mMirrorMotion ? "true" : "false",
            playbackInfo->mMix ? "true" : "false",
            playbackInfo->mPlayNow ? "true" : "false",
            playbackInfo->mMotionExtractionEnabled ? "true" : "false",
            playbackInfo->mRetarget ? "true" : "false",
            playbackInfo->mFreezeAtLastFrame ? "true" : "false",
            playbackInfo->mEnableMotionEvents ? "true" : "false",
            playbackInfo->mBlendOutBeforeEnded ? "true" : "false",
            playbackInfo->mCanOverwrite ? "true" : "false",
            playbackInfo->mDeleteOnZeroWeight ? "true" : "false");
    }


    // fill the playback info based on the input parameters
    void CommandPlayMotion::CommandParametersToPlaybackInfo(Command* command, const MCore::CommandLine& parameters, EMotionFX::PlayBackInfo* outPlaybackInfo)
    {
        //if (parameters.CheckIfHasParameter( "mirrorPlaneNormal" ))    outPlaybackInfo->mMirrorPlaneNormal     = parameters.GetValueAsVector3( "mirrorPlaneNormal", command );
        if (parameters.CheckIfHasParameter("blendInTime"))
        {
            outPlaybackInfo->mBlendInTime           = parameters.GetValueAsFloat("blendInTime", command);
        }
        if (parameters.CheckIfHasParameter("blendOutTime"))
        {
            outPlaybackInfo->mBlendOutTime          = parameters.GetValueAsFloat("blendOutTime", command);
        }
        if (parameters.CheckIfHasParameter("playSpeed"))
        {
            outPlaybackInfo->mPlaySpeed             = parameters.GetValueAsFloat("playSpeed", command);
        }
        if (parameters.CheckIfHasParameter("targetWeight"))
        {
            outPlaybackInfo->mTargetWeight          = parameters.GetValueAsFloat("targetWeight", command);
        }
        if (parameters.CheckIfHasParameter("eventWeightThreshold"))
        {
            outPlaybackInfo->mEventWeightThreshold  = parameters.GetValueAsFloat("eventWeightThreshold", command);
        }
        if (parameters.CheckIfHasParameter("maxPlayTime"))
        {
            outPlaybackInfo->mMaxPlayTime           = parameters.GetValueAsFloat("maxPlayTime", command);
        }
        //if (parameters.CheckIfHasParameter( "retargetRootOffset" ))   outPlaybackInfo->mRetargetRootOffset    = parameters.GetValueAsFloat( "retargetRootOffset", command );
        if (parameters.CheckIfHasParameter("numLoops"))
        {
            outPlaybackInfo->mNumLoops              = parameters.GetValueAsInt("numLoops", command);
        }
        if (parameters.CheckIfHasParameter("priorityLevel"))
        {
            outPlaybackInfo->mPriorityLevel         = parameters.GetValueAsInt("priorityLevel", command);
        }
        if (parameters.CheckIfHasParameter("startNodeIndex"))
        {
            outPlaybackInfo->mStartNodeIndex        = parameters.GetValueAsInt("startNodeIndex", command);
        }
        //if (parameters.CheckIfHasParameter( "retargetRootIndex" ))    outPlaybackInfo->mRetargetRootIndex     = parameters.GetValueAsInt( "retargetRootIndex", command );
        if (parameters.CheckIfHasParameter("blendMode"))
        {
            outPlaybackInfo->mBlendMode             = (EMotionFX::EMotionBlendMode)parameters.GetValueAsInt("blendMode", command);
        }
        if (parameters.CheckIfHasParameter("playMode"))
        {
            outPlaybackInfo->mPlayMode              = (EMotionFX::EPlayMode)parameters.GetValueAsInt("playMode", command);
        }
        if (parameters.CheckIfHasParameter("mirrorMotion"))
        {
            outPlaybackInfo->mMirrorMotion          = parameters.GetValueAsBool("mirrorMotion", command);
        }
        if (parameters.CheckIfHasParameter("mix"))
        {
            outPlaybackInfo->mMix                   = parameters.GetValueAsBool("mix", command);
        }
        if (parameters.CheckIfHasParameter("playNow"))
        {
            outPlaybackInfo->mPlayNow               = parameters.GetValueAsBool("playNow", command);
        }
        if (parameters.CheckIfHasParameter("motionExtraction"))
        {
            outPlaybackInfo->mMotionExtractionEnabled = parameters.GetValueAsBool("motionExtraction", command);
        }
        if (parameters.CheckIfHasParameter("retarget"))
        {
            outPlaybackInfo->mRetarget              = parameters.GetValueAsBool("retarget", command);
        }
        if (parameters.CheckIfHasParameter("freezeAtLastFrame"))
        {
            outPlaybackInfo->mFreezeAtLastFrame     = parameters.GetValueAsBool("freezeAtLastFrame", command);
        }
        if (parameters.CheckIfHasParameter("enableMotionEvents"))
        {
            outPlaybackInfo->mEnableMotionEvents    = parameters.GetValueAsBool("enableMotionEvents", command);
        }
        if (parameters.CheckIfHasParameter("blendOutBeforeEnded"))
        {
            outPlaybackInfo->mBlendOutBeforeEnded   = parameters.GetValueAsBool("blendOutBeforeEnded", command);
        }
        if (parameters.CheckIfHasParameter("canOverwrite"))
        {
            outPlaybackInfo->mCanOverwrite          = parameters.GetValueAsBool("canOverwrite", command);
        }
        if (parameters.CheckIfHasParameter("deleteOnZeroWeight"))
        {
            outPlaybackInfo->mDeleteOnZeroWeight    = parameters.GetValueAsBool("deleteOnZeroWeight", command);
        }
    }


    // execute
    bool CommandPlayMotion::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // clear our old data so that we start fresh in case of a redo
        mOldData.Clear();

        // check if there is any actor instance selected and if not return false so that the command doesn't get called and doesn't get inside the action history
        const uint32 numSelectedActorInstances = GetCommandManager()->GetCurrentSelection().GetNumSelectedActorInstances();
        if (numSelectedActorInstances == 0)
        {
            return false;
        }

        // get the motion
        MCore::String filename;
        parameters.GetValue("filename", this, &filename);

        AZStd::string azFilename = filename.AsChar();
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, azFilename);
        filename = azFilename.c_str();

        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByFileName(filename.AsChar());
        if (motion == nullptr)
        {
            outResult.Format("Cannot find motion '%s' in motion library.", filename.AsChar());
            return false;
        }

        // fill the playback info based on the parameters
        EMotionFX::PlayBackInfo playbackInfo;
        CommandParametersToPlaybackInfo(this, parameters, &playbackInfo);

        // iterate through all actor instances and start playing all selected motions
        for (uint32 i = 0; i < numSelectedActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            // reset the anim graph instance so that the motion will actually play
            actorInstance->SetAnimGraphInstance(nullptr);

            /*      // check if it is safe to start playing a mirrored motion
                    if (playbackInfo.mMirrorMotion && motion->GetType() == SkeletalMotion::TYPE_ID)
                    {
                        Actor*              actor           = actorInstance->GetActor();
                        SkeletalMotion*     skeletalMotion  = (EMotionFX::SkeletalMotion*)motion;

                        if (skeletalMotion->IsMatchingActorBindPose( actor ) == false)
                            LogError("The bind pose from the skeletal motion does not match the one from the actor. Skinning errors might occur when using motion mirroring. Please read the motion mirroring chapter in the ArtistGuide guide. For more information or contact the support team.");
                    }*/

            // start playing the current motion
            MCORE_ASSERT(actorInstance->GetMotionSystem());
            EMotionFX::MotionInstance* motionInstance = actorInstance->GetMotionSystem()->PlayMotion(motion, &playbackInfo);

            // motion offset
            if (parameters.CheckIfHasParameter("useMotionOffset"))
            {
                if (parameters.CheckIfHasParameter("normalizedMotionOffset"))
                {
                    motionInstance->SetCurrentTimeNormalized(parameters.GetValueAsFloat("normalizedMotionOffset", this));
                    motionInstance->SetPause(true);
                }
                else
                {
                    MCore::LogWarning("Cannot use motion offset. The 'normalizedMotionOffset' parameter is not specified. When using motion offset you need to specify the normalized motion offset value.");
                }
            }

            // store what we did for the undo function
            UndoObject undoObject;
            undoObject.mActorInstance   = actorInstance;
            undoObject.mMotionInstance  = motionInstance;
            mOldData.Add(undoObject);
        }

        // verify if we actually have selected an actor instance
        if (numSelectedActorInstances == 0)
        {
            outResult = "Cannot play motions. No actor instance selected.";
        }

        return true;
    }


    // undo the command
    bool CommandPlayMotion::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        bool result = true;

        // iterate through all undo objects
        const uint32 numUndoObjects = mOldData.GetLength();
        for (uint32 i = 0; i < numUndoObjects; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = mOldData[i].mActorInstance;
            EMotionFX::MotionInstance* motionInstance   = mOldData[i].mMotionInstance;

            // check if the actor instance is valid
            if (EMotionFX::GetActorManager().CheckIfIsActorInstanceRegistered(actorInstance) == false)
            {
                //outResult = "Cannot stop motion instance. Actor instance is not registered in the actor manager.";
                //result = false;
                continue;
            }

            // check if the motion instance is valid
            MCORE_ASSERT(actorInstance->GetMotionSystem());
            if (actorInstance->GetMotionSystem()->CheckIfIsValidMotionInstance(motionInstance))
            {
                // stop the motion instance and remove it directly from the motion system
                motionInstance->Stop(0.0f);
                actorInstance->GetMotionSystem()->RemoveMotionInstance(motionInstance);
            }
            //else
            //{
            //  outResult = "Motion instance to be stopped is not valid.";
            //  return false;
            //}
        }

        return result;
    }


#define SYNTAX_MOTIONCOMMANDS                                                                                                                                                                                                                                                                                                         \
    GetSyntax().ReserveParameters(30);                                                                                                                                                                                                                                                                                                \
    GetSyntax().AddRequiredParameter("filename", "The filename of the motion file to play.", MCore::CommandSyntax::PARAMTYPE_STRING);                                                                                                                                                                                                 \
    /*GetSyntax().AddParameter( "mirrorPlaneNormal", "The motion mirror plane normal, which is (1,0,0) on default. This setting is only used when mMirrorMotion is set to true.", MCore::CommandSyntax::PARAMTYPE_VECTOR3, "(1, 0, 0)" );*/                                                                                           \
    GetSyntax().AddParameter("blendInTime", "The time, in seconds, which it will take to fully have blended to the target weight.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.3");                                                                                                                                                    \
    GetSyntax().AddParameter("blendOutTime", "The time, in seconds, which it takes to smoothly fadeout the motion, after it has been stopped playing.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.3");                                                                                                                                \
    GetSyntax().AddParameter("playSpeed", "The playback speed factor. A value of 1 stands for the original speed, while for example 2 means twice the original speed.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");                                                                                                                \
    GetSyntax().AddParameter("targetWeight", "The target weight, where 1 means fully active, and 0 means not active at all.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "1.0");                                                                                                                                                          \
    GetSyntax().AddParameter("eventWeightThreshold", "The motion event weight threshold. If the motion instance weight is lower than this value, no motion events will be executed for this motion instance.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.0");                                                                         \
    GetSyntax().AddParameter("maxPlayTime", "The maximum play time, in seconds. Set to zero or a negative value to disable it.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.0");                                                                                                                                                       \
    GetSyntax().AddParameter("retargetRootOffset", "The retarget root offset. Can be used to prevent actors from floating in the air or going through the ground. Read the manual for more information.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.0");                                                                              \
    GetSyntax().AddParameter("numLoops", "The number of times you want to play this motion. A value of EMFX_LOOPFOREVER (4294967296) means it will loop forever.", MCore::CommandSyntax::PARAMTYPE_INT, "4294967296");   /* 4294967296 == EMFX_LOOPFOREVER */                                                                         \
    GetSyntax().AddParameter("priorityLevel", "The priority level, the higher this value, the higher priority it has on overwriting other motions.", MCore::CommandSyntax::PARAMTYPE_INT, "0");                                                                                                                                       \
    GetSyntax().AddParameter("startNodeIndex", "The node to start the motion from, using MCORE_INVALIDINDEX32 (4294967296) to effect the whole body, or use for example the upper arm node to only play the motion on the arm.", MCore::CommandSyntax::PARAMTYPE_INT, "4294967296");   /* 4294967296 == MCORE_INVALIDINDEX32 */       \
    GetSyntax().AddParameter("retargetRootIndex", "The retargeting root node index.", MCore::CommandSyntax::PARAMTYPE_INT, "0");                                                                                                                                                                                                      \
    GetSyntax().AddParameter("blendMode", "The motion blend mode. Please read the MotionInstance::SetBlendMode(...) method for more information.", MCore::CommandSyntax::PARAMTYPE_INT, "0");   /* 4294967296 == MCORE_INVALIDINDEX32 */                                                                                              \
    GetSyntax().AddParameter("playMode", "The motion playback mode. This means forward or backward playback.", MCore::CommandSyntax::PARAMTYPE_INT, "0");                                                                                                                                                                             \
    GetSyntax().AddParameter("mirrorMotion", "Is motion mirroring enabled or not? When set to true, the mMirrorPlaneNormal is used as mirroring axis.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "No");                                                                                                                               \
    GetSyntax().AddParameter("mix", "Set to true if you want this motion to mix or not.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "No");                                                                                                                                                                                             \
    GetSyntax().AddParameter("playNow", "Set to true if you want to start playing the motion right away. If set to false it will be scheduled for later by inserting it into the motion queue.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");                                                                                     \
    GetSyntax().AddParameter("motionExtraction", "Set to true when you want to use motion extraction.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");                                                                                                                                                                               \
    GetSyntax().AddParameter("retarget", "Set to true if you want to enable motion retargeting. Read the manual for more information.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "No");                                                                                                                                               \
    GetSyntax().AddParameter("freezeAtLastFrame", "Set to true if you like the motion to freeze at the last frame, for example in case of a death motion.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "No");                                                                                                                           \
    GetSyntax().AddParameter("enableMotionEvents", "Set to true to enable motion events, or false to disable processing of motion events for this motion instance.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");                                                                                                                 \
    GetSyntax().AddParameter("blendOutBeforeEnded", "Set to true if you want the motion to be stopped so that it exactly faded out when the motion/loop fully finished. If set to false it will fade out after the loop has completed (and starts repeating). The default is true.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes"); \
    GetSyntax().AddParameter("canOverwrite", "Set to true if you want this motion to be able to delete other underlaying motion instances when this motion instance reaches a weight of 1.0.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");                                                                                       \
    GetSyntax().AddParameter("deleteOnZeroWeight", "Set to true if you wish to delete this motion instance once it reaches a weight of 0.0.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "Yes");                                                                                                                                        \
    GetSyntax().AddParameter("normalizedMotionOffset", "The normalized motion offset time to be used when the useMotionOffset flag is enabled. 0.0 means motion offset is disabled while 1.0 means the motion starts at the end of the motion.", MCore::CommandSyntax::PARAMTYPE_FLOAT, "0.0");                                       \
    GetSyntax().AddParameter("useMotionOffset", "Set to true if you wish to use the motion offset. This will start the motion from the given normalized motion offset value instead of from time=0.0. The motion instance will get paused afterwards.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "No");

    // init the syntax of the command
    void CommandPlayMotion::InitSyntax()
    {
        SYNTAX_MOTIONCOMMANDS
    }


    // get the description
    const char* CommandPlayMotion::GetDescription() const
    {
        return "This command can be used to start playing the given motion on the selected actor instances.";
    }


    //--------------------------------------------------------------------------------
    // CommandAdjustMotionInstance
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustMotionInstance::CommandAdjustMotionInstance(MCore::Command* orgCommand)
        : MCore::Command("AdjustMotionInstance", orgCommand)
    {
    }


    // destructor
    CommandAdjustMotionInstance::~CommandAdjustMotionInstance()
    {
    }


    void CommandAdjustMotionInstance::AdjustMotionInstance(Command* command, const MCore::CommandLine& parameters, EMotionFX::MotionInstance* motionInstance)
    {
        //if (parameters.CheckIfHasParameter( "mirrorPlaneNormal" ))    motionInstance->SetMirrorPlaneNormal        = parameters.GetValueAsVector3( "mirrorPlaneNormal", command );
        if (parameters.CheckIfHasParameter("playSpeed"))
        {
            motionInstance->SetPlaySpeed(parameters.GetValueAsFloat("playSpeed", command));
        }
        if (parameters.CheckIfHasParameter("eventWeightThreshold"))
        {
            motionInstance->SetEventWeightThreshold(parameters.GetValueAsFloat("eventWeightThreshold", command));
        }
        if (parameters.CheckIfHasParameter("maxPlayTime"))
        {
            motionInstance->SetMaxPlayTime(parameters.GetValueAsFloat("maxPlayTime", command));
        }
        //if (parameters.CheckIfHasParameter( "retargetRootOffset" ))   motionInstance->SetRetargetRootOffset( parameters.GetValueAsFloat( "retargetRootOffset", command ) );
        if (parameters.CheckIfHasParameter("numLoops"))
        {
            motionInstance->SetNumCurrentLoops(parameters.GetValueAsInt("numLoops", command));
        }
        if (parameters.CheckIfHasParameter("priorityLevel"))
        {
            motionInstance->SetPriorityLevel(parameters.GetValueAsInt("priorityLevel", command));
        }
        //  if (parameters.CheckIfHasParameter( "retargetRootIndex" ))      motionInstance->SetRetargetRootIndex( parameters.GetValueAsInt( "retargetRootIndex", command ) );
        if (parameters.CheckIfHasParameter("blendMode"))
        {
            motionInstance->SetBlendMode((EMotionFX::EMotionBlendMode)parameters.GetValueAsInt("blendMode", command));
        }
        if (parameters.CheckIfHasParameter("playMode"))
        {
            motionInstance->SetPlayMode((EMotionFX::EPlayMode)parameters.GetValueAsInt("playMode", command));
        }
        if (parameters.CheckIfHasParameter("mirrorMotion"))
        {
            motionInstance->SetMirrorMotion(parameters.GetValueAsBool("mirrorMotion", command));
        }
        if (parameters.CheckIfHasParameter("mix"))
        {
            motionInstance->SetMixMode(parameters.GetValueAsBool("mix", command));
        }
        if (parameters.CheckIfHasParameter("motionExtraction"))
        {
            motionInstance->SetMotionExtractionEnabled(parameters.GetValueAsBool("motionExtraction", command));
        }
        if (parameters.CheckIfHasParameter("retarget"))
        {
            motionInstance->SetRetargetingEnabled(parameters.GetValueAsBool("retarget", command));
        }
        if (parameters.CheckIfHasParameter("freezeAtLastFrame"))
        {
            motionInstance->SetFreezeAtLastFrame(parameters.GetValueAsBool("freezeAtLastFrame", command));
        }
        if (parameters.CheckIfHasParameter("enableMotionEvents"))
        {
            motionInstance->SetMotionEventsEnabled(parameters.GetValueAsBool("enableMotionEvents", command));
        }
        if (parameters.CheckIfHasParameter("blendOutBeforeEnded"))
        {
            motionInstance->SetBlendOutBeforeEnded(parameters.GetValueAsBool("blendOutBeforeEnded", command));
        }
        if (parameters.CheckIfHasParameter("canOverwrite"))
        {
            motionInstance->SetCanOverwrite(parameters.GetValueAsBool("canOverwrite", command));
        }
        if (parameters.CheckIfHasParameter("deleteOnZeroWeight"))
        {
            motionInstance->SetDeleteOnZeroWeight(parameters.GetValueAsBool("deleteOnZeroWeight", command));
        }
    }


    // execute
    bool CommandAdjustMotionInstance::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(outResult);

        // iterate through the motion instances and modify them
        const uint32 numSelectedMotionInstances = GetCommandManager()->GetCurrentSelection().GetNumSelectedMotionInstances();
        for (uint32 i = 0; i < numSelectedMotionInstances; ++i)
        {
            // get the current selected motion instance and adjust it based on the parameters
            EMotionFX::MotionInstance* selectedMotionInstance = GetCommandManager()->GetCurrentSelection().GetMotionInstance(i);
            AdjustMotionInstance(this, parameters, selectedMotionInstance);
        }

        return true;
    }


    // undo the command
    bool CommandAdjustMotionInstance::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // TODO: not sure yet how to undo this on the best way
        return true;
    }


    // init the syntax of the command
    void CommandAdjustMotionInstance::InitSyntax()
    {
        SYNTAX_MOTIONCOMMANDS
    }


    // get the description
    const char* CommandAdjustMotionInstance::GetDescription() const
    {
        return "This command can be used to adjust the selected motion instances.";
    }


    //--------------------------------------------------------------------------------
    // CommandAdjustDefaultPlayBackInfo
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustDefaultPlayBackInfo::CommandAdjustDefaultPlayBackInfo(MCore::Command* orgCommand)
        : MCore::Command("AdjustDefaultPlayBackInfo", orgCommand)
    {
    }


    // destructor
    CommandAdjustDefaultPlayBackInfo::~CommandAdjustDefaultPlayBackInfo()
    {
    }


    // execute
    bool CommandAdjustDefaultPlayBackInfo::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the motion
        MCore::String filename;
        parameters.GetValue("filename", this, &filename);

        AZStd::string azFilename = filename.AsChar();
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, azFilename);
        filename = azFilename.c_str();

        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByFileName(filename.AsChar());
        if (motion == nullptr)
        {
            outResult.Format("Cannot find motion '%s' in motion library.", filename.AsChar());
            return false;
        }

        // get the default playback info from the motion
        EMotionFX::PlayBackInfo* defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();
        if (defaultPlayBackInfo == nullptr)
        {
            outResult.Format("Motion '%s' does not have a default playback info. Cannot adjust default playback info.", filename.AsChar());
            return false;
        }

        // copy the current playback info to the undo data
        mOldPlaybackInfo = *defaultPlayBackInfo;

        // adjust the playback info based on the parameters
        CommandPlayMotion::CommandParametersToPlaybackInfo(this, parameters, defaultPlayBackInfo);

        // save the current dirty flag and tell the motion that something got changed
        mOldDirtyFlag = motion->GetDirtyFlag();
        return true;
    }


    // undo the command
    bool CommandAdjustDefaultPlayBackInfo::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the motion
        MCore::String filename;
        parameters.GetValue("filename", this, &filename);
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByFileName(filename.AsChar());
        if (motion == nullptr)
        {
            outResult = MCore::String().Format("Cannot find motion '%s' in motion library.", filename.AsChar());
            return false;
        }

        // get the default playback info from the motion
        EMotionFX::PlayBackInfo* defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();
        if (defaultPlayBackInfo == nullptr)
        {
            outResult.Format("Motion '%s' does not have a default playback info. Cannot adjust default playback info.", filename.AsChar());
            return false;
        }

        // copy the saved playback info to the actual one
        *defaultPlayBackInfo = mOldPlaybackInfo;

        // set the dirty flag back to the old value
        motion->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    // init the syntax of the command
    void CommandAdjustDefaultPlayBackInfo::InitSyntax()
    {
        SYNTAX_MOTIONCOMMANDS
    }


    // get the description
    const char* CommandAdjustDefaultPlayBackInfo::GetDescription() const
    {
        return "This command can be used to adjust the default playback info of the given motion.";
    }


    //--------------------------------------------------------------------------------
    // CommandStopMotionInstances
    //--------------------------------------------------------------------------------

    // execute
    bool CommandStopMotionInstances::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // clear our old data so that we start fresh in case of a redo
        //mOldData.Clear();

        // get the number of selected actor instances
        const uint32 numSelectedActorInstances = GetCommandManager()->GetCurrentSelection().GetNumSelectedActorInstances();

        // check if there is any actor instance selected and if not return false so that the command doesn't get called and doesn't get inside the action history
        if (numSelectedActorInstances == 0)
        {
            return false;
        }

        // get the motion
        MCore::String filename;
        parameters.GetValue("filename", this, &filename);

        AZStd::string azFilename = filename.AsChar();
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, azFilename);
        filename = azFilename.c_str();

        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByFileName(filename.AsChar());
        if (motion == nullptr)
        {
            outResult.Format("Cannot find motion '%s' in motion library.", filename.AsChar());
            return false;
        }

        // iterate through all actor instances and stop all selected motion instances
        for (uint32 i = 0; i < numSelectedActorInstances; ++i)
        {
            // get the actor instance and the corresponding motion system
            EMotionFX::ActorInstance*   actorInstance   = GetCommandManager()->GetCurrentSelection().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            EMotionFX::MotionSystem*    motionSystem    = actorInstance->GetMotionSystem();

            // stop simulating the anim graph instance
            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (animGraphInstance)
            {
                animGraphInstance->Stop();
            }

            // get the number of motion instances and iterate through them
            const uint32 numMotionInstances = motionSystem->GetNumMotionInstances();
            for (uint32 j = 0; j < numMotionInstances; ++j)
            {
                EMotionFX::MotionInstance* motionInstance = motionSystem->GetMotionInstance(j);

                // stop the motion instance in case its from the given motion
                if (motion == motionInstance->GetMotion())
                {
                    motionInstance->Stop();
                }
            }
        }

        return true;
    }


    // undo the command
    bool CommandStopMotionInstances::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandStopMotionInstances::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("filename", "The filename of the motion file to stop all motion instances for.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    // get the description
    const char* CommandStopMotionInstances::GetDescription() const
    {
        return "Stop all motion instances for the currently selected motions on all selected actor instances.";
    }


    //--------------------------------------------------------------------------------
    // CommandStopAllMotionInstances
    //--------------------------------------------------------------------------------

    // execute
    bool CommandStopAllMotionInstances::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // clear our old data so that we start fresh in case of a redo
        //mOldData.Clear();

        // iterate through all actor instances and stop all selected motion instances
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            // get the actor instance and the corresponding motion system
            EMotionFX::ActorInstance*   actorInstance   = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            EMotionFX::MotionSystem*    motionSystem    = actorInstance->GetMotionSystem();

            // stop simulating the anim graph instance
            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (animGraphInstance)
            {
                animGraphInstance->Stop();
            }

            // get the number of motion instances and iterate through them
            const uint32 numMotionInstances = motionSystem->GetNumMotionInstances();
            for (uint32 j = 0; j < numMotionInstances; ++j)
            {
                // get the motion instance and stop it
                EMotionFX::MotionInstance* motionInstance = motionSystem->GetMotionInstance(j);
                motionInstance->Stop(0.0f);
            }

            // directly remove the motion instances
            actorInstance->UpdateTransformations(0.0f, true);
        }

        return true;
    }


    // undo the command
    bool CommandStopAllMotionInstances::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    // init the syntax of the command
    void CommandStopAllMotionInstances::InitSyntax()
    {
    }


    // get the description
    const char* CommandStopAllMotionInstances::GetDescription() const
    {
        return "Stop all currently playing motion instances on all selected actor instances.";
    }


    //--------------------------------------------------------------------------------
    // CommandAdjustMotion
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustMotion::CommandAdjustMotion(MCore::Command* orgCommand)
        : MCore::Command("AdjustMotion", orgCommand)
    {
    }


    // destructor
    CommandAdjustMotion::~CommandAdjustMotion()
    {
    }

    
    // execute
    bool CommandAdjustMotion::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion with the given id exists
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (motion == nullptr)
        {
            outResult.Format("Cannot adjust motion. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // adjust the dirty flag
        if (parameters.CheckIfHasParameter("dirtyFlag"))
        {
            mOldDirtyFlag = motion->GetDirtyFlag();
            const bool dirtyFlag = parameters.GetValueAsBool("dirtyFlag", this);
            motion->SetDirtyFlag(dirtyFlag);
        }

        // adjust the name
        if (parameters.CheckIfHasParameter("name"))
        {
            mOldName = motion->GetName();
            MCore::String name;
            parameters.GetValue("name", this, &name);
            motion->SetName(name.AsChar());

            mOldDirtyFlag = motion->GetDirtyFlag();
            motion->SetDirtyFlag(true);
        }


        // Adjust the motion extraction flags.
        if (parameters.CheckIfHasParameter("motionExtractionFlags"))
        {
            mOldExtractionFlags = motion->GetMotionExtractionFlags();
            const uint32 flags = parameters.GetValueAsInt("motionExtractionFlags", this);
            motion->SetMotionExtractionFlags( static_cast<EMotionFX::EMotionExtractionFlags>(flags) );
            mOldDirtyFlag = motion->GetDirtyFlag();
            motion->SetDirtyFlag(true);
        }

        mOldDirtyFlag = motion->GetDirtyFlag();
        motion->SetDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandAdjustMotion::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        const int32 motionID = parameters.GetValueAsInt("motionID", this);

        // check if the motion with the given id exists
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
        if (!motion)
        {
            outResult.Format("Cannot adjust motion. Motion with id='%i' does not exist.", motionID);
            return false;
        }

        // adjust the dirty flag
        if (parameters.CheckIfHasParameter("dirtyFlag"))
        {
            motion->SetDirtyFlag(mOldDirtyFlag);
        }

        // adjust the name
        if (parameters.CheckIfHasParameter("name"))
        {
            motion->SetName(mOldName.AsChar());
            motion->SetDirtyFlag(mOldDirtyFlag);
        }

        if (parameters.CheckIfHasParameter("motionExtractionFlags"))
        {
            motion->SetMotionExtractionFlags(mOldExtractionFlags);
            motion->SetDirtyFlag(mOldDirtyFlag);
        }

        return true;
    }


    // init the syntax of the command
    void CommandAdjustMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(6);
        GetSyntax().AddRequiredParameter("motionID",         "The id of the motion to adjust.",                                                     MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("dirtyFlag",                "The dirty flag indicates whether the user has made changes to the motion or not.",    MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
        GetSyntax().AddParameter("name",                     "The name of the motion.",                                                             MCore::CommandSyntax::PARAMTYPE_STRING, "Unknown Motion");
        GetSyntax().AddParameter("motionExtractionFlags",    "The motion extraction flags value.",                                                  MCore::CommandSyntax::PARAMTYPE_INT, "0");
    }


    // get the description
    const char* CommandAdjustMotion::GetDescription() const
    {
        return "This command can be used to adjust the given motion.";
    }

    //--------------------------------------------------------------------------------
    // CommandRemoveMotion
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveMotion::CommandRemoveMotion(MCore::Command* orgCommand)
        : MCore::Command("RemoveMotion", orgCommand)
    {
        mOldMotionID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandRemoveMotion::~CommandRemoveMotion()
    {
    }


    // execute
    bool CommandRemoveMotion::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        // get the filename
        MCore::String filename;
        parameters.GetValue("filename", "", &filename);

        AZStd::string azFilename = filename.AsChar();
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, azFilename);
        filename = azFilename.c_str();

        // find the corresponding motion
        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByFileName(filename.AsChar());
        if (motion == nullptr)
        {
            outResult.Format("Cannot remove motion. Motion with filename '%s' is not part of the motion manager.", filename.AsChar());
            return false;
        }

        if (motion->GetIsOwnedByRuntime())
        {
            outResult.Format("Cannot remove motion. Motion with filename '%s' is being used by the engine runtime.", filename.AsChar());
            return false;
        }

        // make sure the motion is not part of any motion set
        const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
        for (uint32 i = 0; i < numMotionSets; ++i)
        {
            // get the current motion set and check if the motion we want to remove is used by it
            EMotionFX::MotionSet*               motionSet   = EMotionFX::GetMotionManager().GetMotionSet(i);
            EMotionFX::MotionSet::MotionEntry*  motionEntry = motionSet->FindMotionEntry(motion);
            if (motionEntry)
            {
                outResult.Format("Cannot remove motion '%s'. Motion set named '%s' is using the motion.", motion->GetFileName(), motionSet->GetName());
                return false;
            }
        }

        // remove the motion from the selection and the motion library
        MCore::String commandString;
        commandString.Format("Unselect -motionName \"%s\"", motion->GetFileName());
        GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult);

        // store the previously used id and remove the motion
        mOldIndex           = EMotionFX::GetMotionManager().FindMotionIndex(motion);
        mOldMotionID        = motion->GetID();
        mOldFileName        = motion->GetFileName();

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        EMotionFX::GetMotionManager().RemoveMotionByID(motion->GetID());
        return true;
    }


    // undo the command
    bool CommandRemoveMotion::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        // execute the group command
        MCore::String commandString;
        commandString.Format("ImportMotion -filename \"%s\" -motionID %i", mOldFileName.AsChar(), mOldMotionID);
        bool result = GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandRemoveMotion::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("filename", "The filename of the motion file to remove.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    // get the description
    const char* CommandRemoveMotion::GetDescription() const
    {
        return "This command can be used to remove the given motion from the motion library.";
    }


    //--------------------------------------------------------------------------------
    // CommandScaleMotionData
    //--------------------------------------------------------------------------------

    // constructor
    CommandScaleMotionData::CommandScaleMotionData(MCore::Command* orgCommand)
        : MCore::Command("ScaleMotionData", orgCommand)
    {
        mMotionID       = MCORE_INVALIDINDEX32;
        mScaleFactor    = 1.0f;
        mOldDirtyFlag   = false;
    }


    // destructor
    CommandScaleMotionData::~CommandScaleMotionData()
    {
    }


    // execute
    bool CommandScaleMotionData::Execute(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        EMotionFX::Motion* motion;
        if (parameters.CheckIfHasParameter("id"))
        {
            uint32 motionID = parameters.GetValueAsInt("id", MCORE_INVALIDINDEX32);

            motion = EMotionFX::GetMotionManager().FindMotionByID(motionID);
            if (motion == nullptr)
            {
                outResult.Format("Cannot get the motion, with ID %d.", motionID);
                return false;
            }
        }
        else
        {
            // check if there is any actor selected at all
            SelectionList& selection = GetCommandManager()->GetCurrentSelection();
            if (selection.GetNumSelectedActors() == 0)
            {
                outResult = "No motion has been selected, please select one first.";
                return false;
            }

            // get the first selected motion
            motion = selection.GetMotion(0);
        }

        if (parameters.CheckIfHasParameter("unitType") == false && parameters.CheckIfHasParameter("scaleFactor") == false)
        {
            outResult = "You have to either specify -unitType or -scaleFactor.";
            return false;
        }

        mMotionID = motion->GetID();
        mScaleFactor = parameters.GetValueAsFloat("scaleFactor", 1.0f);

        MCore::String targetUnitTypeString;
        parameters.GetValue("unitType", this, &targetUnitTypeString);
        mUseUnitType = parameters.CheckIfHasParameter("unitType");

        MCore::Distance::EUnitType targetUnitType;
        bool stringConvertSuccess = MCore::Distance::StringToUnitType(targetUnitTypeString, &targetUnitType);
        if (mUseUnitType && stringConvertSuccess == false)
        {
            outResult.Format("The passed unitType '%s' is not a valid unit type.", targetUnitTypeString.AsChar());
            return false;
        }
        mOldUnitType = MCore::Distance::UnitTypeToString(motion->GetUnitType());

        mOldDirtyFlag = motion->GetDirtyFlag();
        motion->SetDirtyFlag(true);

        // perform the scaling
        if (mUseUnitType == false)
        {
            motion->Scale(mScaleFactor);
        }
        else
        {
            motion->ScaleToUnitType(targetUnitType);
        }

        return true;
    }


    // undo the command
    bool CommandScaleMotionData::Undo(const MCore::CommandLine& parameters, MCore::String& outResult)
    {
        MCORE_UNUSED(parameters);

        if (mUseUnitType == false)
        {
            MCore::String commandString;
            commandString.Format("ScaleMotionData -id %d -scaleFactor %.8f", mMotionID, 1.0f / mScaleFactor);
            GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult);
        }
        else
        {
            MCore::String commandString;
            commandString.Format("ScaleMotionData -id %d -unitType \"%s\"", mMotionID, mOldUnitType.AsChar());
            GetCommandManager()->ExecuteCommandInsideCommand(commandString.AsChar(), outResult);
        }

        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(mMotionID);
        if (motion)
        {
            motion->SetDirtyFlag(mOldDirtyFlag);
        }

        return true;
    }


    // init the syntax of the command
    void CommandScaleMotionData::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        GetSyntax().AddParameter("id",                      "The identification number of the motion we want to scale.",                    MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("scaleFactor",             "The scale factor, for example 10.0 to make the motion pose 10x as large.",     MCore::CommandSyntax::PARAMTYPE_FLOAT,      "1.0");
        GetSyntax().AddParameter("unitType",                "The unit type to convert to, for example 'meters'.",                           MCore::CommandSyntax::PARAMTYPE_STRING,     "meters");
        GetSyntax().AddParameter("skipInterfaceUpdate",     ".",                                                                            MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "false");
    }


    // get the description
    const char* CommandScaleMotionData::GetDescription() const
    {
        return "This command can be used to scale all internal motion data. This means positional keyframe data will be modified as well as stored pose and bind pose data.";
    }



    //--------------------------------------------------------------------------------
    // Helper Functions
    //--------------------------------------------------------------------------------

    void LoadMotionsCommand(const AZStd::vector<AZStd::string>& filenames, bool reload)
    {
        if (filenames.empty())
        {
            return;
        }

        // get the number of motions to load
        const size_t numFileNames = filenames.size();

        // construct the command name
        MCore::String valueString;
        valueString.Format("%s %d motion%s", (reload) ? "Reload" : "Load", numFileNames, (numFileNames > 1) ? "s" : "");

        // create our command group
        MCore::CommandGroup commandGroup(valueString.AsChar(), (uint32)numFileNames * 2);

        // iterate over all filenames and load the motions
        AZStd::string command;
        for (size_t i = 0; i < numFileNames; ++i)
        {
            // in case we want to reload the same motion remove the old one first
            if (reload)
            {
                valueString.Format("RemoveMotion -filename \"%s\"", filenames[i].c_str());
                commandGroup.AddCommandString(valueString.AsChar());
            }

            command = AZStd::string::format("ImportMotion -filename \"%s\"", filenames[i].c_str());
            commandGroup.AddCommandString(command);
        }

        // execute the group command
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, valueString) == false)
        {
            if (valueString.GetIsEmpty() == false)
            {
                MCore::LogError(valueString.AsChar());
            }
        }
    }


    void ClearMotions(MCore::CommandGroup* commandGroup, bool forceRemove)
    {
        // iterate through the motions and put them into some array
        const uint32 numMotions = EMotionFX::GetMotionManager().GetNumMotions();
        AZStd::vector<EMotionFX::Motion*> motionsToRemove;
        motionsToRemove.reserve(numMotions);

        for (uint32 i=0; i<numMotions; ++i)
        {
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }

            motionsToRemove.push_back(motion);
        }

        // construct the command group and remove the selected motions
        AZStd::vector<EMotionFX::Motion*> failedRemoveMotions;
        RemoveMotions(motionsToRemove, &failedRemoveMotions, commandGroup, forceRemove);
    }


    void RemoveMotions(const AZStd::vector<EMotionFX::Motion*>& motions, AZStd::vector<EMotionFX::Motion*>* outFailedMotions, MCore::CommandGroup* commandGroup, bool forceRemove)
    {
        outFailedMotions->clear();

        if (motions.empty())
        {
            return;
        }

        const size_t numMotions = motions.size();

        // Set the command group name.
        AZStd::string commandGroupName;
        if (numMotions == 1)
        {
            commandGroupName = "Remove 1 motion";
        }
        else
        {
            commandGroupName = AZStd::string::format("Remove %d motions", numMotions);
        }

        // Create the internal command group which is used in case the parameter command group is not specified.
        MCore::CommandGroup internalCommandGroup(commandGroupName);

        // Iterate through all motions and remove them.
        AZStd::string commandString;
        for (uint32 i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion* motion = motions[i];

            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }

            // Is the motion part of a motion set?
            bool isUsed = false;
            const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
            for (uint32 j = 0; j < numMotionSets; ++j)
            {
                EMotionFX::MotionSet*               motionSet   = EMotionFX::GetMotionManager().GetMotionSet(j);
                EMotionFX::MotionSet::MotionEntry*  motionEntry = motionSet->FindMotionEntry(motion);

                if (motionEntry)
                {
                    outFailedMotions->push_back(motionEntry->GetMotion());
                    isUsed = true;
                    break;
                }
            }

            if (!isUsed || forceRemove)
            {
                // Construct the command and add it to the corresponding group
                commandString = AZStd::string::format("RemoveMotion -filename \"%s\"", motion->GetFileName());

                if (!commandGroup)
                {
                    internalCommandGroup.AddCommandString(commandString);
                }
                else
                {
                    commandGroup->AddCommandString(commandString);
                }
            }
        }

        if (!commandGroup)
        {
            // Execute the internal command group in case the command group parameter is not specified.
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }
} // namespace CommandSystem
