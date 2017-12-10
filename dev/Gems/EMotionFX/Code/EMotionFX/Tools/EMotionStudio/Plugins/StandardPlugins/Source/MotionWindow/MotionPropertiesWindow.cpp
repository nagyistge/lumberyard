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

// include required headers
#include "MotionPropertiesWindow.h"
#include "MotionWindowPlugin.h"
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <MysticQt/Source/ButtonGroup.h>
#include <EMotionFX/CommandSystem/Source/MotionCommands.h>
#include <MCore/Source/Compare.h>


namespace EMStudio
{
    // constructor
    MotionPropertiesWindow::MotionPropertiesWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin)
        : QWidget(parent)
    {
        mMotionWindowPlugin = motionWindowPlugin;

        mButtonLoopForever          = nullptr;
        mButtonMirror               = nullptr;

        mPlaySpeedResetButton       = nullptr;
        mPlaySpeedSlider            = nullptr;
        mPlaySpeedLabel             = nullptr;
    }


    // destructor
    MotionPropertiesWindow::~MotionPropertiesWindow()
    {
    }


    // init after the parent dock window has been created
    void MotionPropertiesWindow::Init()
    {
        // motion properties
        QVBoxLayout* motionPropertiesLayout = new QVBoxLayout(this);
        motionPropertiesLayout->setMargin(0);

        // button group
        MysticQt::ButtonGroup* buttonGroup = new MysticQt::ButtonGroup(this, 1, 2);

        mButtonLoopForever          = buttonGroup->GetButton(0, 0);
        mButtonMirror               = buttonGroup->GetButton(0, 1);

        mButtonLoopForever->setText("Loop Forever");
        mButtonMirror->setText("Mirror");

        connect(mButtonLoopForever, SIGNAL(clicked()), this, SLOT(UpdateMotions()));
        connect(mButtonMirror, SIGNAL(clicked()), this, SLOT(UpdateMotions()));

        motionPropertiesLayout->addWidget(buttonGroup);

        // play mode radio button group
        MysticQt::ButtonGroup* playModeButtonGroup = new MysticQt::ButtonGroup(this, 1, 2, MysticQt::ButtonGroup::MODE_RADIOBUTTONS);
        mButtonPlayForward  = playModeButtonGroup->GetButton(0, 0);
        mButtonPlayBackward = playModeButtonGroup->GetButton(0, 1);
        mButtonPlayForward->setText("Forward");
        mButtonPlayBackward->setText("Backward");
        connect(mButtonPlayForward, SIGNAL(clicked()), this, SLOT(UpdateMotions()));
        connect(mButtonPlayBackward, SIGNAL(clicked()), this, SLOT(UpdateMotions()));
        motionPropertiesLayout->addWidget(playModeButtonGroup);

        // sliders
        QGridLayout* slidersLayout = new QGridLayout();
        slidersLayout->setMargin(0);
        slidersLayout->setSpacing(1);

        // play speed slider
        slidersLayout->addWidget(new QLabel("Play Speed"), 2, 0);

        mPlaySpeedSlider = new MysticQt::Slider(Qt::Horizontal);
        mPlaySpeedSlider->setRange(10, 5000);
        slidersLayout->addWidget(mPlaySpeedSlider, 2, 1);

        mPlaySpeedLabel = new QLabel("0.0");
        slidersLayout->addWidget(mPlaySpeedLabel, 2, 2);

        mPlaySpeedResetButton = new QPushButton("R");
        mPlaySpeedResetButton->setMaximumHeight(18);
        slidersLayout->addWidget(mPlaySpeedResetButton, 2, 3);

        connect(mPlaySpeedSlider, SIGNAL(valueChanged(int)), this, SLOT(PlaySpeedSliderChanged(int)));
        connect(mPlaySpeedSlider, SIGNAL(sliderReleased()), this, SLOT(UpdateMotions()));
        connect(mPlaySpeedResetButton, SIGNAL(clicked()), this, SLOT(ResetPlaySpeed()));

        motionPropertiesLayout->addLayout(slidersLayout);

        // reset the values to the defaults
        ResetPlaySpeed();
    }


    void MotionPropertiesWindow::UpdateMotions()
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();

        // create our command group
        MCore::CommandGroup commandGroup("Adjust default motion instances");

        // get the number of selected motions and iterate through them
        const uint32 numMotions = selection.GetNumSelectedMotions();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            MotionWindowPlugin::MotionTableEntry* entry = mMotionWindowPlugin->FindMotionEntryByID(selection.GetMotion(i)->GetID());
            if (entry == nullptr)
            {
                MCore::LogError("Cannot find motion table entry for the given motion.");
                continue;
            }

            EMotionFX::Motion*          motion          = entry->mMotion;
            EMotionFX::PlayBackInfo*    playbackInfo    = motion->GetDefaultPlayBackInfo();

            MCore::String commandParameters;

            if (MCore::Compare<float>::CheckIfIsClose(playbackInfo->mPlaySpeed, GetPlaySpeed(), 0.001f) == false)
            {
                commandParameters += MCore::String().Format("-playSpeed %f ", GetPlaySpeed());
            }

            // Either loop forever or freeze at the last frame after playing the animation once.
            if (mButtonLoopForever->isChecked())
            {
                commandParameters += MCore::String().Format("-numLoops %i ", EMFX_LOOPFOREVER);
            }
            else
            {
                commandParameters += "-numLoops 1 -freezeAtLastFrame true ";
            }

            if (playbackInfo->mMirrorMotion != mButtonMirror->isChecked())
            {
                commandParameters += MCore::String().Format("-mirrorMotion %s", mButtonMirror->isChecked() ? "true" : "false");
            }

            // playmode
            const bool playForward = playbackInfo->mPlayMode == EMotionFX::PLAYMODE_FORWARD;
            int32 playMode;
            if (mButtonPlayForward->isChecked())
            {
                playMode = (int32)EMotionFX::PLAYMODE_FORWARD;
            }
            else
            {
                playMode = (int32)EMotionFX::PLAYMODE_BACKWARD;
            }

            if (playForward != mButtonPlayForward->isChecked())
            {
                commandParameters += MCore::String().Format("-playMode %i ", playMode);
            }

            // in case the command parameters are empty it means nothing changed, so we can skip this command
            if (commandParameters.GetIsEmpty() == false)
            {
                commandGroup.AddCommandString(MCore::String().Format("AdjustDefaultPlayBackInfo -filename \"%s\" %s", motion->GetFileName(), commandParameters.AsChar()).AsChar());
            }
        }

        // execute the group command
        MCore::String outResult;
        if (CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.AsChar());
        }
    }


    void MotionPropertiesWindow::UpdateInterface()
    {
        const CommandSystem::SelectionList& selection = CommandSystem::GetCommandManager()->GetCurrentSelection();
        //MCore::Array<EMotionFX::MotionInstance*>& motionInstances = mMotionWindowPlugin->GetSelectedMotionInstances();

        // check if there actually is any motion selected
        const uint32 numSelectedMotions = selection.GetNumSelectedMotions();
        const bool isEnabled = (numSelectedMotions != 0);

        mButtonLoopForever->setEnabled(isEnabled);
        mPlaySpeedSlider->setEnabled(isEnabled);
        mPlaySpeedResetButton->setEnabled(isEnabled);
        mButtonPlayForward->setEnabled(isEnabled);
        mButtonPlayBackward->setEnabled(isEnabled);
        mButtonMirror->setEnabled(isEnabled);

        if (isEnabled == false)
        {
            return;
        }

        // get the number of selected motions and iterate through them
        const uint32 numMotions = selection.GetNumSelectedMotions();
        for (uint32 i = 0; i < numMotions; ++i)
        {
            MotionWindowPlugin::MotionTableEntry* entry = mMotionWindowPlugin->FindMotionEntryByID(selection.GetMotion(i)->GetID());
            if (entry == nullptr)
            {
                MCore::LogError("Cannot find motion table entry for the given motion.");
                continue;
            }

            EMotionFX::Motion*          motion              = entry->mMotion;
            EMotionFX::PlayBackInfo*    defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();

            if (defaultPlayBackInfo == nullptr)
            {
                motion->CreateDefaultPlayBackInfo();
                defaultPlayBackInfo = motion->GetDefaultPlayBackInfo();
            }

            // set button group
            mButtonMirror->setChecked(defaultPlayBackInfo->mMirrorMotion);

            // set playmode radio button group
            const bool playForward = defaultPlayBackInfo->mPlayMode == EMotionFX::PLAYMODE_FORWARD;
            mButtonPlayForward->setChecked(playForward);
            mButtonPlayBackward->setChecked(!playForward);

            bool loopForever = false;
            if (defaultPlayBackInfo->mNumLoops == EMFX_LOOPFOREVER)
            {
                loopForever = true;
            }
            mButtonLoopForever->setChecked(loopForever);

            // set slider values
            SetPlaySpeed(defaultPlayBackInfo->mPlaySpeed);
        }
    }


    void MotionPropertiesWindow::PlaySpeedSliderChanged(int value)
    {
        MCORE_UNUSED(value);
        const float playSpeed = GetPlaySpeed();

        const AZStd::vector<EMotionFX::MotionInstance*>& motionInstances = mMotionWindowPlugin->GetSelectedMotionInstances();
        const size_t numMotionInstances = motionInstances.size();
        for (size_t i = 0; i < numMotionInstances; ++i)
        {
            EMotionFX::MotionInstance* motionInstance = motionInstances[i];
            motionInstance->SetPlaySpeed(playSpeed);
        }

        mPlaySpeedLabel->setText(AZStd::string::format("%.2f", playSpeed).c_str());
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionWindow/MotionPropertiesWindow.moc>