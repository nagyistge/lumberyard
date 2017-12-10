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

#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Events/ProcessingResult.h>

namespace AZ
{
    namespace rapidxml
    {
        template<class Ch>
        class xml_node;
        template<class Ch>
        class xml_attribute;
        template<class Ch>
        class xml_document;
    }

    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IMaterialData;
            class ISceneNodeGroup;
            class IMeshAdvancedRule;
        }

        namespace Events
        {
            class PreExportEventContext;
        }

        namespace Export
        {
            //! Scene exporting component that exports materials to the cache if needed before any processing happens.
            class MaterialExporterComponent : public SceneCore::ExportingComponent
            {
            public:
                AZ_COMPONENT(MaterialExporterComponent, "{6976CB4F-BA87-4CBF-A49D-0E602BFDC3B2}", SceneCore::ExportingComponent);

                MaterialExporterComponent();
                ~MaterialExporterComponent() override = default;

                static void Reflect(ReflectContext* context);

                /// @cond EXCLUDE_DOCS
                // Prepares for processing and exporting by looking at all the groups and generating materials for them in the cache
                //      if needed and if there isn't already a material in the source folder.
                Events::ProcessingResult ExportMaterials(Events::PreExportEventContext& context) const;
                // Gets the root path that all texture paths have to be relative to, which is usually the game projects root.
                AZStd::string GetTextureRootPath() const;
                /// @endcond

            protected:
#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
                // Workaround for VS2013 - Delete the copy constructor and make it private
                // https://connect.microsoft.com/VisualStudio/feedback/details/800328/std-is-copy-constructible-is-broken
                MaterialExporterComponent(const MaterialExporterComponent&) = delete;
#endif
            };

            class MtlMaterialExporter
            {
            public:
                enum class SaveMaterialResult
                {
                    Success,
                    Skipped,
                    Failure
                };

                virtual ~MtlMaterialExporter() = default;

                //! Save the material references in the given group to the material.
                SCENE_CORE_API SaveMaterialResult SaveMaterialGroup(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& sceneNodeGroup, 
                    const SceneAPI::Containers::Scene& scene, const AZStd::string& textureRootPath);
                //! Add the material references in the given group to previously saved materials.
                SCENE_CORE_API SaveMaterialResult AppendMaterialGroup(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& sceneNodeGroup, 
                    const SceneAPI::Containers::Scene& scene);
                //! Write a previously loaded/constructed material to disk.
                //! @param filePath An absolute path to the target file. Source control action should be done before calling this function.
                SCENE_CORE_API bool WriteToFile(const char* filePath, bool updateWithChanges);

            protected:
                struct MaterialInfo
                {
                    AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IMaterialData> m_materialData { nullptr };
                    bool m_usesVertexColoring { false };
                    bool m_physicalize { false };
                    AZStd::string m_name;
                };

                struct MaterialGroup
                {
                    AZStd::vector<MaterialInfo> m_materials;
                    bool m_removeMaterials;
                    bool m_updateMaterials;
                };

                SCENE_CORE_API SaveMaterialResult BuildMaterialGroup(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& sceneNodeGroup, const SceneAPI::Containers::Scene& scene);
                
                //! Writes the material group to disk.
                //! @param filePath The absolute path to the final destination.
                //! @param materialGroup The material group to be written to disk.
                //! @param updateWithChanges Whether or not to update the material file at filePath. If false, the file will be overwritten regardless of the settings in the material group.
                SCENE_CORE_API bool WriteMaterialFile(const char* filePath, MaterialGroup& materialGroup, bool updateWithChanges) const;

                SCENE_CORE_API bool UsesVertexColoring(const AZ::SceneAPI::DataTypes::ISceneNodeGroup& sceneNodeGroup, const SceneAPI::Containers::Scene& scene,
                    SceneAPI::Containers::SceneGraph::HierarchyStorageConstIterator materialNode) const;

                SCENE_CORE_API const SceneAPI::DataTypes::IMeshAdvancedRule* FindMeshAdvancedRule(const SceneAPI::DataTypes::ISceneNodeGroup* group) const;
                SCENE_CORE_API bool DoesMeshNodeHaveColorStreamChild(const SceneAPI::Containers::Scene& scene,
                    SceneAPI::Containers::SceneGraph::NodeIndex meshNode) const;

                static const AZStd::string m_extension;

                SceneAPI::Containers::SceneGraph::NodeIndex* m_root;
                AZStd::string m_textureRootPath;
                MaterialGroup m_materialGroup;

                bool m_physicalize;
            };
        } // namespace Export
    } // namespace SceneAPI
} // namespace AZ
