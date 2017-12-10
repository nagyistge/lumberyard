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

#include <ScriptCanvas/Core/NativeDatumNode.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            const bool k_Matrix4x4NodeHasProperties = true;

            class Matrix4x4
                : public NativeDatumNode<Matrix4x4, AZ::Matrix4x4, k_Matrix4x4NodeHasProperties>
            {
            public:
                using ParentType = NativeDatumNode<Matrix4x4, AZ::Matrix4x4, k_Matrix4x4NodeHasProperties>;
                AZ_COMPONENT(Matrix4x4, "{CF059648-8BE5-4CC6-B909-4D3EBD945071}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    ParentType::Reflect(reflection);

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Matrix4x4, ParentType>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Matrix4x4>("Matrix4x4", "A 4x4 matrix value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Matrix4x4.png")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                                ;
                        }
                    }
                }

                void AddProperties()
                {
                    AddProperty(&AZ::Matrix4x4::GetBasisX, static_cast<void(AZ::Matrix4x4::*)(const AZ::Vector4&)>(&AZ::Matrix4x4::SetBasisX), "basisX");
                    AddProperty(&AZ::Matrix4x4::GetBasisY, static_cast<void(AZ::Matrix4x4::*)(const AZ::Vector4&)>(&AZ::Matrix4x4::SetBasisY), "basisY");
                    AddProperty(&AZ::Matrix4x4::GetBasisZ, static_cast<void(AZ::Matrix4x4::*)(const AZ::Vector4&)>(&AZ::Matrix4x4::SetBasisZ), "basisZ");
                    AddProperty(&AZ::Matrix4x4::GetPosition, static_cast<void(AZ::Matrix4x4::*)(const AZ::Vector3&)>(&AZ::Matrix4x4::SetPosition), "position");
                }

                void Visit(NodeVisitor& visitor) const override
                {
                    visitor.Visit(*this);
                }
            };
        }
    }
}