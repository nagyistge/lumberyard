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

#pragma once

#include "CommandSystemConfig.h"
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <MCore/Source/Endian.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/PlayBackInfo.h>


namespace CommandSystem
{
    // adjust motion command
    MCORE_DEFINECOMMAND_START(CommandAdjustMotion, "Adjust motion", true)
    bool                                mOldDirtyFlag;
    EMotionFX::EMotionExtractionFlags   mOldExtractionFlags;
    MCore::String                       mOldName;
    MCore::String                       mOldMotionExtractionNodeName;

private:
    MCORE_DEFINECOMMAND_END


    // remove motion command
    MCORE_DEFINECOMMAND_START(CommandRemoveMotion, "Remove motion", true)
public:
    uint32          mOldMotionID;
    MCore::String   mOldFileName;
    uint32          mOldIndex;
    bool            mOldWorkspaceDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // scale motion data
    MCORE_DEFINECOMMAND_START(CommandScaleMotionData, "Scale motion data", true)
public:
    MCore::String   mOldUnitType;
    uint32          mMotionID;
    float           mScaleFactor;
    bool            mOldDirtyFlag;
    bool            mUseUnitType;
    MCORE_DEFINECOMMAND_END


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // EMotionFX::Motion* Playback
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    // play motion command
    MCORE_DEFINECOMMAND_START(CommandPlayMotion, "Play motion", true)
    struct UndoObject
    {
        EMotionFX::ActorInstance*   mActorInstance;     /**< The old selected actor on which the motion got started. */
        EMotionFX::MotionInstance*  mMotionInstance;    /**< The old motion instance to be stopped by the undo process. */
    };
    MCore::Array<UndoObject> mOldData;      /**< Array of undo items. Each item means we started a motion on an actor and have to stop it again in the undo process. */
public:
    static void CommandParametersToPlaybackInfo(MCore::Command* command, const MCore::CommandLine& parameters, EMotionFX::PlayBackInfo* outPlaybackInfo);
    static AZStd::string PlayBackInfoToCommandParameters(EMotionFX::PlayBackInfo* playbackInfo);
    MCORE_DEFINECOMMAND_END


    // adjust motion instance command
    MCORE_DEFINECOMMAND_START(CommandAdjustMotionInstance, "Adjust motion instance", true)
    void AdjustMotionInstance(MCore::Command* command, const MCore::CommandLine& parameters, EMotionFX::MotionInstance* motionInstance);
    MCORE_DEFINECOMMAND_END


    // adjust default playback info command
    MCORE_DEFINECOMMAND_START(CommandAdjustDefaultPlayBackInfo, "Adjust default playback info", true)
    EMotionFX::PlayBackInfo mOldPlaybackInfo;
    bool                    mOldDirtyFlag;
    MCORE_DEFINECOMMAND_END


    // stop motion instances command
    MCORE_DEFINECOMMAND(CommandStopMotionInstances, "StopMotionInstances", "Stop motion instances", false)


    // stop all motion instances command
    MCORE_DEFINECOMMAND(CommandStopAllMotionInstances, "StopAllMotionInstances", "Stop all motion instances", false)


    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper Functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    void COMMANDSYSTEM_API LoadMotionsCommand(const AZStd::vector<AZStd::string>& filenames, bool reload = false);
    void COMMANDSYSTEM_API RemoveMotions(const AZStd::vector<EMotionFX::Motion*>& motions, AZStd::vector<EMotionFX::Motion*>* outFailedMotions, MCore::CommandGroup* commandGroup = nullptr, bool forceRemove = false);
    void COMMANDSYSTEM_API ClearMotions(MCore::CommandGroup* commandGroup = nullptr, bool forceRemove = false);
} // namespace CommandSystem