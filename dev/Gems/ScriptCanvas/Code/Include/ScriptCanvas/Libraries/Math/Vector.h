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
            const bool k_VectorNodeHasProperties = true;

            class Vector2
                : public NativeDatumNode<Vector2, AZ::Vector2, k_VectorNodeHasProperties>
            {
            public:
                using ParentType = NativeDatumNode<Vector2, AZ::Vector2, k_VectorNodeHasProperties>;
                AZ_COMPONENT(Vector2, "{EB647398-7F56-4727-9C7C-277593DB1F11}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    ParentType::Reflect(reflection);

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Vector2, ParentType>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Vector2>("Vector2", "A 2D vector value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Vector.png")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                                ;
                        }
                    }
                }

                void AddProperties()
                {
                    AddProperty(&AZ::Vector2::GetX, &AZ::Vector2::SetX, "x");
                    AddProperty(&AZ::Vector2::GetY, &AZ::Vector2::SetY, "y");
                }

                void Visit(NodeVisitor& visitor) const override
                {
                    visitor.Visit(*this);
                }
            };

            class Vector3
                : public NativeDatumNode<Vector3, AZ::Vector3, k_VectorNodeHasProperties>
            {
            public:
                using ParentType = NativeDatumNode<Vector3, AZ::Vector3, k_VectorNodeHasProperties>;
                AZ_COMPONENT(Vector3, "{95A12BDE-D4B4-47E8-A917-3E42F678E7FA}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    ParentType::Reflect(reflection);

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Vector3, ParentType>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Vector3>("Vector3", "A 3D vector value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Vector.png")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                                ;
                        }
                    }
                }

                void AddProperties()
                {
                    AddProperty(&AZ::Vector3::GetX, &AZ::Vector3::SetX, "x");
                    AddProperty(&AZ::Vector3::GetY, &AZ::Vector3::SetY, "y");
                    AddProperty(&AZ::Vector3::GetZ, &AZ::Vector3::SetZ, "z");
                }

                void Visit(NodeVisitor& visitor) const override
                {
                    visitor.Visit(*this);
                }
            };

            class Vector4
                : public NativeDatumNode<Vector4, AZ::Vector4, k_VectorNodeHasProperties>
            {
            public:
                using ParentType = NativeDatumNode<Vector4, AZ::Vector4, k_VectorNodeHasProperties>;
                AZ_COMPONENT(Vector4, "{9CAE50A1-C575-4DFC-95C5-FA0A12DABCBD}", ParentType);

                static void Reflect(AZ::ReflectContext* reflection)
                {
                    ParentType::Reflect(reflection);

                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<Vector4, ParentType>()
                            ->Version(0)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            editContext->Class<Vector4>("Vector4", "A 4D vector value")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Vector.png")
                                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                                ;
                        }
                    }
                }

                void AddProperties()
                {
                    AddProperty(&AZ::Vector4::GetX, &AZ::Vector4::SetX, "x");
                    AddProperty(&AZ::Vector4::GetY, &AZ::Vector4::SetY, "y");
                    AddProperty(&AZ::Vector4::GetZ, &AZ::Vector4::SetZ, "z");
                    AddProperty(&AZ::Vector4::GetW, &AZ::Vector4::SetW, "w");
                }

                void Visit(NodeVisitor& visitor) const override
                {
                    visitor.Visit(*this);
                }
            };
        }
    }
}