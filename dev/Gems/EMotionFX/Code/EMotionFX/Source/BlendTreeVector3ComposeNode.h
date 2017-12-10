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

// include the required headers
#include "EMotionFXConfig.h"
#include "AnimGraphNode.h"


namespace EMotionFX
{
    /**
     *
     */
    class EMFX_API BlendTreeVector3ComposeNode
        : public AnimGraphNode
    {
        MCORE_MEMORYOBJECTCATEGORY(BlendTreeVector3ComposeNode, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_BLENDTREENODES);

    public:
        AZ_RTTI(BlendTreeVector3ComposeNode, "{C78BAE63-C567-4483-A7A1-E68906BE9054}", AnimGraphNode);

        enum
        {
            TYPE_ID = 0x00000128
        };

        //
        enum
        {
            INPUTPORT_X         = 0,
            INPUTPORT_Y         = 1,
            INPUTPORT_Z         = 2,
            OUTPUTPORT_VECTOR   = 0
        };

        enum
        {
            PORTID_INPUT_X          = 0,
            PORTID_INPUT_Y          = 1,
            PORTID_INPUT_Z          = 2,
            PORTID_OUTPUT_VECTOR    = 0
        };

        static BlendTreeVector3ComposeNode* Create(AnimGraph* animGraph);

        void RegisterPorts() override;
        void RegisterAttributes() override;

        uint32 GetVisualColor() const override          { return MCore::RGBA(128, 255, 128); }

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        const char* GetTypeString() const override;
        AnimGraphObject* Clone(AnimGraph* animGraph) override;
        AnimGraphObjectData* CreateObjectData() override;

    private:
        BlendTreeVector3ComposeNode(AnimGraph* animGraph);
        ~BlendTreeVector3ComposeNode();

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
    };
}   // namespace EMotionFX

