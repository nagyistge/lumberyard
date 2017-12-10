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
#include "StdAfx.h"
#include "LyShineExamplesSerialize.h"

#include <LyShine/UiSerializeHelpers.h>

#include <LyShineExamples/UiCustomImageBus.h>

#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// NAMESPACE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////

namespace LyShineExamplesSerialize
{

    //////////////////////////////////////////////////////////////////////////
    void UVCoordsScriptConstructor(UiCustomImageInterface::UVRect* thisPtr, AZ::ScriptDataContext& dc)
    {
        int numArgs = dc.GetNumArguments();

        const int noArgsGiven = 0;
        const int allArgsGiven = 4;

        switch (numArgs)
        {
        case noArgsGiven:
        {
            *thisPtr = UiCustomImageInterface::UVRect();
        }
        break;

        case allArgsGiven:
        {
            if (dc.IsNumber(0) && dc.IsNumber(1) && dc.IsNumber(2) && dc.IsNumber(3))
            {
                float left = 0;
                float top = 0;
                float right = 0;
                float bottom = 0;
                dc.ReadArg(0, left);
                dc.ReadArg(1, top);
                dc.ReadArg(2, right);
                dc.ReadArg(3, bottom);
                *thisPtr = UiCustomImageInterface::UVRect(left, top, right, bottom);
            }
            else
            {
                dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "When providing 4 arguments to UVCoords(), all must be numbers!");
            }
        }
        break;

        default:
        {
            dc.GetScriptContext()->Error(AZ::ScriptContext::ErrorType::Error, true, "UVCoords() accepts only 0 or 4 arguments, not %d!", numArgs);
        }
        break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    void ReflectTypes(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);

        // Serialize the UVs struct
        {
            if (serializeContext)
            {
                serializeContext->Class<UiCustomImageInterface::UVRect>()->
                    Field("left", &UiCustomImageInterface::UVRect::m_left)->
                    Field("top", &UiCustomImageInterface::UVRect::m_top)->
                    Field("right", &UiCustomImageInterface::UVRect::m_right)->
                    Field("bottom", &UiCustomImageInterface::UVRect::m_bottom);

                AZ::EditContext* ec = serializeContext->GetEditContext();
                if (ec)
                {
                    auto editInfo = ec->Class<UiCustomImageInterface::UVRect>(0, "");
                    editInfo->DataElement(0, &UiCustomImageInterface::UVRect::m_left, "Left", "The lower X UV coordinate.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiCustomImageInterface::UVRect::m_top, "Top", "The higher Y UV coordinate.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiCustomImageInterface::UVRect::m_right, "Right", "The higher X UV coordinate.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                    editInfo->DataElement(0, &UiCustomImageInterface::UVRect::m_bottom, "Bottom", "The lower Y UV coordinate.")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::UISliceFlags::PushableEvenIfInvisible);
                }
            }

            if (behaviorContext)
            {
                behaviorContext->Class<UiCustomImageInterface::UVRect>("UVCoords")
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Constructor<>()
                    ->Constructor<float, float, float, float>()
                    ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::Preview)
                    ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                    ->Attribute(AZ::Script::Attributes::ConstructorOverride, &UVCoordsScriptConstructor)
                    ->Property("left", BehaviorValueProperty(&UiCustomImageInterface::UVRect::m_left))
                    ->Property("top", BehaviorValueProperty(&UiCustomImageInterface::UVRect::m_top))
                    ->Property("right", BehaviorValueProperty(&UiCustomImageInterface::UVRect::m_right))
                    ->Property("bottom", BehaviorValueProperty(&UiCustomImageInterface::UVRect::m_bottom));
            }
        }
    }
}
