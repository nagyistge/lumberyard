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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : implementation file


#include "StdAfx.h"
#include "TVSequenceProps.h"
#include "IMovieSystem.h"
#include "TrackViewUndo.h"
#include "TrackViewSequence.h"
#include "AnimationContext.h"

#include "Objects/BaseObject.h"
#include "Objects/SequenceObject.h"

#include "QtUtilWin.h"
#include <TrackView/ui_TVSequenceProps.h>
#include <QMessageBox>


CTVSequenceProps::CTVSequenceProps(CTrackViewSequence* pSequence, float fps, QWidget* pParent)
    : QDialog(pParent)
    , m_FPS(fps)
    , m_outOfRange(0)
    , m_timeUnit(Seconds)
    , ui(new Ui::CTVSequenceProps)
{
    ui->setupUi(this);
    assert(pSequence);
    m_pSequence = pSequence;
    connect(ui->BTNOK, &QPushButton::clicked, this, &CTVSequenceProps::OnOK);
    connect(ui->CUT_SCENE, &QCheckBox::toggled, this, &CTVSequenceProps::ToggleCutsceneOptions);
    connect(ui->TO_SECONDS, &QRadioButton::toggled, this, &CTVSequenceProps::OnBnClickedToSeconds);
    connect(ui->TO_FRAMES, &QRadioButton::toggled, this, &CTVSequenceProps::OnBnClickedToFrames);

    OnInitDialog();
}

CTVSequenceProps::~CTVSequenceProps()
{
}

// CTVSequenceProps message handlers
BOOL CTVSequenceProps::OnInitDialog()
{
    QString name = QtUtil::ToQString(m_pSequence->GetName());
    ui->NAME->setText(name);
    int seqFlags = m_pSequence->GetFlags();

    ui->ALWAYS_PLAY->setChecked((seqFlags & IAnimSequence::eSeqFlags_PlayOnReset));
    ui->CUT_SCENE->setChecked((seqFlags & IAnimSequence::eSeqFlags_CutScene));
    ui->DISABLEPLAYER->setChecked((seqFlags & IAnimSequence::eSeqFlags_NoPlayer));
    ui->DISABLESOUNDS->setChecked((seqFlags & IAnimSequence::eSeqFlags_NoGameSounds));
    ui->NOSEEK->setChecked((seqFlags & IAnimSequence::eSeqFlags_NoSeek));
    ui->NOABORT->setChecked((seqFlags & IAnimSequence::eSeqFlags_NoAbort));
    ui->EARLYMOVIEUPDATE->setChecked((seqFlags & IAnimSequence::eSeqFlags_EarlyMovieUpdate));

    ToggleCutsceneOptions(ui->CUT_SCENE->isChecked());

    ui->MOVE_SCALE_KEYS->setChecked(BST_UNCHECKED);

    ui->START_TIME->setRange(0.0, (1e+5));
    ui->END_TIME->setRange(0.0, (1e+5));

    Range timeRange = m_pSequence->GetTimeRange();
    float invFPS = 1.0f / m_FPS;

    m_timeUnit = Seconds;
    ui->START_TIME->setValue(timeRange.start);
    ui->START_TIME->setSingleStep(invFPS);
    ui->END_TIME->setValue(timeRange.end);
    ui->END_TIME->setSingleStep(invFPS);


    m_outOfRange = 0;
    if (m_pSequence->GetFlags() & IAnimSequence::eSeqFlags_OutOfRangeConstant)
    {
        m_outOfRange = 1;
        ui->ORT_CONSTANT->setChecked(true);
    }
    else if (m_pSequence->GetFlags() & IAnimSequence::eSeqFlags_OutOfRangeLoop)
    {
        m_outOfRange = 2;
        ui->ORT_LOOP->setChecked(true);
    }
    else
    {
        ui->ORT_ONCE->setChecked(true);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CTVSequenceProps::MoveScaleKeys()
{
    // Move/Rescale the sequence to a new time range.
    Range timeRangeOld = m_pSequence->GetTimeRange();
    Range timeRangeNew;
    timeRangeNew.start = ui->START_TIME->value();
    timeRangeNew.end = ui->END_TIME->value();

    if (!(timeRangeNew == timeRangeOld))
    {
        m_pSequence->AdjustKeysToTimeRange(timeRangeNew);
    }
}

void CTVSequenceProps::OnOK()
{
    QString name = ui->NAME->text();
    if (name.isEmpty())
    {
        QMessageBox::warning(this, "Sequence Properties", "A sequence name cannot be empty!");
        return;
    }
    else if (name.contains('/'))
    {
        QMessageBox::warning(this, "Sequence Properties", "A sequence name cannot contain a '/' character!");
        return;
    }

    CUndo undo("Change TrackView Sequence Settings");
    CUndo::Record(new CUndoSequenceSettings(m_pSequence));

    if (ui->MOVE_SCALE_KEYS->isChecked())
    {
        MoveScaleKeys();
    }

    Range timeRange;
    timeRange.start = ui->START_TIME->value();
    timeRange.end = ui->END_TIME->value();

    if (m_timeUnit == Frames)
    {
        float invFPS = 1.0f / m_FPS;
        timeRange.start = ui->START_TIME->value() * invFPS;
        timeRange.end = ui->END_TIME->value() * invFPS;
    }

    m_pSequence->SetTimeRange(timeRange);

    CAnimationContext* ac = GetIEditor()->GetAnimation();
    if (ac)
    {
        ac->UpdateTimeRange();
    }

    QString seqName = m_pSequence->GetName();
    if (name != seqName)
    {
        // Rename sequence.
        const CTrackViewSequenceManager* sequenceManager = GetIEditor()->GetSequenceManager();

        sequenceManager->RenameNode(m_pSequence, name.toLatin1().data());
    }

    int seqFlags = m_pSequence->GetFlags();
    seqFlags &= ~(IAnimSequence::eSeqFlags_OutOfRangeConstant | IAnimSequence::eSeqFlags_OutOfRangeLoop);

    if (ui->ALWAYS_PLAY->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_PlayOnReset;
    }
    else
    {
        seqFlags &= ~IAnimSequence::eSeqFlags_PlayOnReset;
    }

    if (ui->CUT_SCENE->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_CutScene;
    }
    else
    {
        seqFlags &= ~IAnimSequence::eSeqFlags_CutScene;
    }

    if (ui->DISABLEPLAYER->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_NoPlayer;
    }
    else
    {
        seqFlags &= (~IAnimSequence::eSeqFlags_NoPlayer);
    }

    if (ui->ORT_CONSTANT->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_OutOfRangeConstant;
    }
    else if (ui->ORT_LOOP->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_OutOfRangeLoop;
    }

    if (ui->DISABLESOUNDS->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_NoGameSounds;
    }
    else
    {
        seqFlags &= (~IAnimSequence::eSeqFlags_NoGameSounds);
    }

    if (ui->NOSEEK->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_NoSeek;
    }
    else
    {
        seqFlags &= (~IAnimSequence::eSeqFlags_NoSeek);
    }

    if (ui->NOABORT->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_NoAbort;
    }
    else
    {
        seqFlags &= (~IAnimSequence::eSeqFlags_NoAbort);
    }

    if (ui->EARLYMOVIEUPDATE->isChecked())
    {
        seqFlags |= IAnimSequence::eSeqFlags_EarlyMovieUpdate;
    }
    else
    {
        seqFlags &= (~IAnimSequence::eSeqFlags_EarlyMovieUpdate);
    }

    m_pSequence->SetFlags((IAnimSequence::EAnimSequenceFlags)seqFlags);
    accept();
}

void CTVSequenceProps::ToggleCutsceneOptions(bool bActivated)
{
    if (bActivated == FALSE)
    {
        ui->NOABORT->setChecked(false);
        ui->DISABLEPLAYER->setChecked(false);
        ui->DISABLESOUNDS->setChecked(false);
    }

    ui->NOABORT->setEnabled(bActivated);
    ui->DISABLEPLAYER->setEnabled(bActivated);
    ui->DISABLESOUNDS->setEnabled(bActivated);
}

void CTVSequenceProps::OnBnClickedToFrames(bool v)
{
    if (!v)
    {
        return;
    }

    ui->START_TIME->setSingleStep(1.0f);
    ui->END_TIME->setSingleStep(1.0f);

    ui->START_TIME->setValue(std::round(ui->START_TIME->value() * static_cast<double>(m_FPS)));
    ui->END_TIME->setValue(std::round(ui->END_TIME->value() * static_cast<double>(m_FPS)));

    m_timeUnit = Frames;
}


void CTVSequenceProps::OnBnClickedToSeconds(bool v)
{
    if (!v)
    {
        return;
    }

    float fInvFPS = 1.0f / m_FPS;

    ui->START_TIME->setSingleStep(fInvFPS);
    ui->END_TIME->setSingleStep(fInvFPS);

    ui->START_TIME->setValue(ui->START_TIME->value() * fInvFPS);
    ui->END_TIME->setValue(ui->END_TIME->value() * fInvFPS);

    m_timeUnit = Seconds;
}

#include <TrackView/TVSequenceProps.moc>