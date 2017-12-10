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

#include "StdAfx.h"

#include "MappingsComponent.h"

#include <AzCore/base.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <CloudCanvas/CloudCanvasIdentityBus.h>

#include <CloudGemFramework/AwsApiJob.h>
#include <CloudGemFramework/AwsApiJobConfig.h>

#include <ICryPak.h>
#include <ICmdLine.h>
#include <ISystem.h>

#include <platform.h>

namespace CloudGemFramework
{
    const char* CloudCanvasMappingsComponent::SERVICE_NAME = "CloudCanvasCommonMappingsService";

    void CloudCanvasMappingsComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<CloudCanvasMappingsComponent, AZ::Component>()
                ->Version(0)
                ->SerializerForEmptyClass();
        }
    }

    void CloudCanvasMappingsComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC(SERVICE_NAME));
    }

    void CloudCanvasMappingsComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC(SERVICE_NAME));
    }

    void CloudCanvasMappingsComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void CloudCanvasMappingsComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void CloudCanvasMappingsComponent::Init()
    {

    }

    void CloudCanvasMappingsComponent::Activate()
    {
        CloudCanvasMappingsBus::Handler::BusConnect();
        CloudCanvasUserPoolMappingsBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
    }

    void CloudCanvasMappingsComponent::Deactivate()
    {
        CrySystemEventBus::Handler::BusDisconnect();
        CloudCanvasUserPoolMappingsBus::Handler::BusDisconnect();
        CloudCanvasMappingsBus::Handler::BusDisconnect();
    }

    void CloudCanvasMappingsComponent::OnCrySystemInitialized(ISystem& system, const SSystemInitParams&)
    {
        InitializeGameMappings();
    }

    void CloudCanvasMappingsComponent::OnCrySystemShutdown(ISystem&)
    {

    }

    void CloudCanvasMappingsComponent::ClearData()
    {
        AZStd::lock_guard<AZStd::mutex> dataLock(m_dataMutex);
        m_mappingData.clear();
    }

    MappingData CloudCanvasMappingsComponent::GetAllMappings() 
    {
        return m_mappingData;
    }

    AZStd::string CloudCanvasMappingsComponent::GetLogicalToPhysicalResourceMapping(const AZStd::string& logicalResourceName)
    {
        AZStd::lock_guard<AZStd::mutex> dataLock(m_dataMutex);
        auto mappingInfo = m_mappingData.find(logicalResourceName);
        if (mappingInfo != m_mappingData.end())
        {
            return mappingInfo->second->GetPhysicalName();
        }
        return{};
    }

    void CloudCanvasMappingsComponent::SetLogicalMapping(AZStd::string resourceType, AZStd::string logicalName, AZStd::string physicalName)
    {
        HandleMappingType(resourceType, logicalName, physicalName);

        AZStd::lock_guard<AZStd::mutex> dataLock(m_dataMutex);
        m_mappingData[logicalName] = AZStd::make_shared<MappingInfo>(AZStd::move(physicalName), AZStd::move(resourceType));
    }

    void CloudCanvasMappingsComponent::HandleMappingType(const AZStd::string& resourceType, const AZStd::string& logicalName, const AZStd::string& physicalName)
    {
        if (resourceType == "Configuration")
        {
            if (logicalName == "region")
            {
                AwsApiJob::Config* defaultClientSettings{ nullptr };
                EBUS_EVENT_RESULT(defaultClientSettings, CloudGemFrameworkRequestBus, GetDefaultConfig);

                if (defaultClientSettings)
                {
                    defaultClientSettings->region = physicalName.c_str();
                }
            }
        }
    }

    AZStd::vector<AZStd::string> CloudCanvasMappingsComponent::GetMappingsOfType(const AZStd::string& resourceType)
    {
        AZStd::lock_guard<AZStd::mutex> dataLock(m_dataMutex);

        AZStd::vector<AZStd::string> returnVec;
        for (auto thisData : m_mappingData)
        {
            if (thisData.second->GetResourceType() == resourceType)
            {
                returnVec.push_back(thisData.first);
            }
        }
        return returnVec;
    }

    bool CloudCanvasMappingsComponent::LoadLogicalMappingsFromFile(const AZStd::string& mappingsFileName)
    {
        ClearData();

        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
        {
            AZ_Error("CloudCanvas", false, "Can't load mappings - no FileIOBase Instance");
            return false;
        }

        AZ::IO::HandleType mappingsFile;
        if (!fileIO->Open(mappingsFileName.c_str(), AZ::IO::OpenMode::ModeRead, mappingsFile))
        {
            AZ_TracePrintf("Failed to open mappings file '%s'", mappingsFileName.c_str());
            return false;
        }

        AZ::u64 fileSize{ 0 };
        fileIO->Size(mappingsFile, fileSize);
        if(!fileSize)
        {
            AZ_Warning("CloudCanvas", false, "AWS Logical Mappings file '%s' is empty", mappingsFileName.c_str());
            fileIO->Close(mappingsFile);
            return false;
        }
        AZStd::vector<char> fileData;
        fileData.resize(fileSize);

        fileIO->Read(mappingsFile, &fileData[0], fileSize);
        fileData.push_back('\0');

        Aws::String fileDataStr(&fileData[0]);
        Aws::Utils::Json::JsonValue jsonValue(fileDataStr);

        fileIO->Close(mappingsFile);

        return LoadLogicalMappingsFromJson(jsonValue);
    }

    static const char* kLogicalMappingsName = "LogicalMappings";
    static const char* kResourceTypeFieldName = "ResourceType";
    static const char* kPhysicalNameFieldName = "PhysicalResourceId";
    static const char* kProtectedFieldName = "Protected";

    static const char* kUserPoolClientCollectionName = "UserPoolClients";
    static const char* kUserPoolClientIdFieldName = "ClientId";
    static const char* kUserPoolClientSecretFieldName = "ClientSecret";

    static const char* baseMappingsFolder = "Config/";
    static const char* baseMappingsPattern = ".awsLogicalMappings.json";

    static const char* resourceMapOverride = "cc_override_resource_map";

    bool CloudCanvasMappingsComponent::LoadLogicalMappingsFromJson(const Aws::Utils::Json::JsonValue& mappingsJsonData)
    {

        if (!mappingsJsonData.WasParseSuccessful())
        {
            AZ_Warning("CloudCanvas",false, "Could not parse logical mappings json");
            return false;
        }

        m_isProtectedMapping = mappingsJsonData.GetBool(kProtectedFieldName);

        Aws::Utils::Json::JsonValue logicalMappingsObject = mappingsJsonData.GetObject(kLogicalMappingsName);
        Aws::Map<Aws::String, Aws::Utils::Json::JsonValue> mappingObjects = logicalMappingsObject.GetAllObjects();

        for (const std::pair<Aws::String, Aws::Utils::Json::JsonValue>& mapping : mappingObjects)
        {
            const Aws::String& logicalName = mapping.first;

            const Aws::String& resourceType = mapping.second.GetString(kResourceTypeFieldName);
            const Aws::String& physicalName = mapping.second.GetString(kPhysicalNameFieldName);

            SetLogicalMapping(resourceType.c_str(), logicalName.c_str(), physicalName.c_str());

            HandleCustomResourceMapping(logicalName, resourceType, mapping);
        }

        return true;
    }

    void CloudCanvasMappingsComponent::HandleCustomResourceMapping(const Aws::String& logicalName, const Aws::String& resourceType, const std::pair<Aws::String, Aws::Utils::Json::JsonValue>& mapping)
    {
        if (resourceType == "Custom::CognitoUserPool")
        {
            Aws::Utils::Json::JsonValue clientAppsObject = mapping.second.GetObject(kUserPoolClientCollectionName);
            Aws::Map<Aws::String, Aws::Utils::Json::JsonValue> clientApps = clientAppsObject.GetAllObjects();
            for (const std::pair<Aws::String, Aws::Utils::Json::JsonValue>& currApp : clientApps)
            {
                const Aws::String& clientName = currApp.first;
                const Aws::String& clientId = currApp.second.GetString(kUserPoolClientIdFieldName);
                const Aws::String& clientSecret = currApp.second.GetString(kUserPoolClientSecretFieldName);
                SetLogicalUserPoolClientMapping(
                    logicalName.c_str(),
                    clientName.c_str(),
                    clientId.c_str(),
                    clientSecret.c_str());
            }
        }
    }

    void CloudCanvasMappingsComponent::SetLogicalUserPoolClientMapping(const AZStd::string& logicalName, const AZStd::string& clientName, AZStd::string clientId, AZStd::string clientSecret)
    {
        AZStd::lock_guard<AZStd::mutex> dataLock(m_userPoolDataMutex);
        m_userPoolMappingData[logicalName][clientName] = AZStd::make_shared<UserPoolClientInfo>(clientId, clientSecret);
    }

    AZStd::shared_ptr<UserPoolClientInfo> CloudCanvasMappingsComponent::GetUserPoolClientInfo(const AZStd::string& logicalName, const AZStd::string& clientName)
    {
        AZStd::lock_guard<AZStd::mutex> dataLock(m_userPoolDataMutex);
        auto clientSettings = m_userPoolMappingData.find(logicalName);

        if (clientSettings == m_userPoolMappingData.end())
        {
            return{};
        }

        auto clientInfo = clientSettings->second.find(clientName);
        if (clientInfo == clientSettings->second.end())
        {
            return{};
        }

        return clientInfo->second;
    }

    void CloudCanvasMappingsComponent::InitializeGameMappings()
    {
        if (!GetISystem() || !GetISystem()->GetGlobalEnvironment())
        {
            return;
        }

        if (!GetISystem()->GetGlobalEnvironment()->IsEditor())
        {
            AZStd::string mappingPath;

            ICmdLine* cmdLine = gEnv->pSystem->GetICmdLine();
            if (cmdLine)
            {
                const ICmdLineArg* command = cmdLine->FindArg(eCLAT_Pre, resourceMapOverride);
                if (command)
                {
                    mappingPath = command->GetValue();
                }
            }

#if defined(WIN64) || defined(MAC) || defined(IOS)
            if (mappingPath.empty())
            {
                const char* value = std::getenv(resourceMapOverride);
                if (value)
                {
                    mappingPath = value;
                }
            }
#endif

            if (mappingPath.empty())
            {
                mappingPath = GetLogicalMappingsPath();
            }

            if (LoadLogicalMappingsFromFile(mappingPath))
            {
                bool shouldApplyMapping = true;

#if defined (WIN32) && defined (_DEBUG)
                // Dialog boxes seem to be only available on windows for now :(
                static const char* PROTECTED_MAPPING_MSG_TITLE = "AWS Mapping Is Protected";
                static const char* PROTECTED_MAPPING_MSG_TEXT = "Warning: The AWS resource mapping file is marked as protected and shouldn't be used for normal development work. Are you sure you want to continue?";
                if (IsProtectedMapping())
                {
                    
                    static const AZ::u64 cMB_YESNO = 0x00000004L;
                    static const AZ::u64 cMB_ICONEXCLAMATION = 0x00000030L;
                    static const AZ::u64 cIDYES = 6;

                    shouldApplyMapping = CryMessageBox(PROTECTED_MAPPING_MSG_TEXT, PROTECTED_MAPPING_MSG_TITLE, cMB_ICONEXCLAMATION | cMB_YESNO) == cIDYES ? true : false;
                    SetIgnoreProtection(shouldApplyMapping);
                }
#endif
                if (shouldApplyMapping)
                {
                    EBUS_EVENT(CloudGemFramework::CloudCanvasPlayerIdentityBus, ApplyConfiguration);
                }
            }
        }
    }

    AZStd::string CloudCanvasMappingsComponent::GetLogicalMappingsPath() const
    {
        
        // enumerate files
        auto cryPak = gEnv->pCryPak;
        _finddata_t findData;

        const char* role = gEnv->IsDedicated() ? "*.server" : "*.player";
        AZStd::string path = AZStd::string(baseMappingsFolder) + role + baseMappingsPattern;
        intptr_t findHandle = cryPak->FindFirst(path.c_str(), &findData);

        AZ_TracePrintf("Loading Game Mappings (IsDedicated=>%s) from path '%s'", gEnv->IsDedicated() ? "True" : "False", path.c_str());

        AZStd::vector<AZStd::string> mappingFiles;
        if (findHandle != -1)
        {
            do
            {
                mappingFiles.push_back(findData.name);
            } while (cryPak->FindNext(findHandle, &findData) != -1);
            cryPak->FindClose(findHandle);
        }

        // if only one mapping file, provide that name
        if (mappingFiles.size() == 1)
        {
            return baseMappingsFolder + mappingFiles[0];
        }
        else if (mappingFiles.size() > 1)
        {
            AZ_Warning("Cloud Canvas", false, "Multiple Cloud Canvas mapping files found. Please use the %s commands line parameter to select a mapping file.", resourceMapOverride);
            return{};
        }
        else
        {
            AZ_Warning("Cloud Canvas", false, "No Cloud Canvas mapping file found");
            return{};
        }
    }
}

