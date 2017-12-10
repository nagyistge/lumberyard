/*
* All or portions of this file Copyright(c) Amazon.com, Inc.or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*/

#pragma once

#include "StdAfx.h"

#include <CloudCanvasCommonSystemComponent.h>

#include <IGem.h>

namespace CloudCanvasCommon
{
    class CloudCanvasCommonModule
        : public CryHooksModule
    {
    public:
        AZ_RTTI(CloudCanvasCommonModule, "{72F7EB98-205C-42D4-8195-94BBC350C333}", CryHooksModule);

        CloudCanvasCommonModule();
        virtual ~CloudCanvasCommonModule() override = default;
        /**
         * Add required SystemComponents to the SystemEntity.
         */
        virtual AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}

