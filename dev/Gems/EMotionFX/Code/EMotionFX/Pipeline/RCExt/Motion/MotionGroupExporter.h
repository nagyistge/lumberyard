#pragma once

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

#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        struct MotionGroupExportContext;

        class MotionGroupExporter
            : public AZ::SceneAPI::SceneCore::ExportingComponent
        {
        public:
            AZ_COMPONENT(MotionGroupExporter, "{46AE1F54-6C71-405B-B63F-7BDCEAE8EB9B}", AZ::SceneAPI::SceneCore::ExportingComponent);

            explicit MotionGroupExporter();
            ~MotionGroupExporter() override = default;

            static void Reflect(AZ::ReflectContext* context);

            AZ::SceneAPI::Events::ProcessingResult ProcessContext(MotionGroupExportContext& context) const;

        protected:
            static const AZStd::string  s_fileExtension;

        private:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
            // Workaround for VS2013 - Delete the copy constructor and make it private
            // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
            MotionGroupExporter(const MotionGroupExporter&) = delete;
#endif
        };
    }
}