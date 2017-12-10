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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace CloudGemFramework
{
    /// Class for generating multipart form data capable of sending files via HTTP POST.
    /// The current implementation writes the entire contents of the file to an output buffer in a single operation,
    /// i.e. there is no streaming for large files.
    /// A stream-based solution would require better bridging of types between CGF and AWS.
    class MultipartFormData
    {
    public:
        struct ComposeResult
        {
            AZStd::string m_content;        // Use for the request body
            AZStd::string m_contentLength;  // Use for the 'Content-Length' HTTP header field
            AZStd::string m_contentType;    // Use for the 'Content-Type' HTTP header field
        };

    public:
        AZ_CLASS_ALLOCATOR(MultipartFormData, AZ::SystemAllocator, 0);

        /// Add a field/value pair to the form
        void AddField(AZStd::string name, AZStd::string value);

        /// Add a file's contents to the form
        void AddFile(AZStd::string fieldName, AZStd::string fileName, const char* path);
        void AddFileBytes(AZStd::string fieldName, AZStd::string fileName, const void* bytes, size_t length);

        /// Set a custom boundary delimeter to use in the form. This is optional; a random one will be generated normally.
        void SetCustomBoundary(AZStd::string boundary);

        /// Compose the form's contents and returns those contents along with metadata.
        ComposeResult ComposeForm();

    private:
        struct Field
        {
            AZStd::string m_fieldName;
            AZStd::string m_value;
        };

        struct FileField
        {
            AZStd::string m_fieldName;
            AZStd::string m_fileName;
            AZStd::vector<char> m_fileData;
        };

        using Fields = AZStd::vector<Field>;
        using FileFields = AZStd::vector<FileField>;

    private:
        void Prepare();
        size_t EstimateBodySize() const;

    private:
        AZStd::string m_boundary;
        AZStd::string m_separator;
        AZStd::string m_composedBody;
        Fields m_fields;
        FileFields m_fileFields;
    };

} // namespace CloudGemFramework
