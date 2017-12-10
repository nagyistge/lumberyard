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

#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            void BehaviorComponent::Activate()
            {
            }

            void BehaviorComponent::Deactivate()
            {
            }

            void BehaviorComponent::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<BehaviorComponent>()->Version(1);
                }
            }
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ