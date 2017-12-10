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
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <QMutex>

namespace AzToolsFramework
{
    using namespace Thumbnailer;

    namespace AssetBrowser
    {
        //! SourceAssetBrowserEntry thumbnail key
        class SourceThumbnailKey
            : public ThumbnailKey
        {
            Q_OBJECT
        public:
            explicit SourceThumbnailKey(const char* fileName);
            const AZStd::string& GetFileName() const;
            const AZStd::string& GetExtension() const;

        protected:
            //! absolute path
            AZStd::string m_fileName;
            //! file extension
            AZStd::string m_extension;
        };

        class SourceThumbnail
            : public Thumbnail
        {
            Q_OBJECT
        public:
            SourceThumbnail(SharedThumbnailKey key, int thumbnailSize);

        protected:
            void LoadThread() override;

        private:
            static QMutex m_mutex;
        };

        namespace
        {
            class SourceKeyHash
            {
            public:
                size_t operator() (const SharedThumbnailKey& val) const
                {
                    auto sourceThumbnailKey = qobject_cast<const SourceThumbnailKey*>(val.data());
                    if (!sourceThumbnailKey)
                    {
                        return 0;
                    }
                    return AZStd::hash<AZStd::string>()(sourceThumbnailKey->GetExtension());
                }
            };

            class SourceKeyEqual
            {
            public:
                bool operator()(const SharedThumbnailKey& val1, const SharedThumbnailKey& val2) const
                {
                    auto sourceThumbnailKey1 = qobject_cast<const SourceThumbnailKey*>(val1.data());
                    auto sourceThumbnailKey2 = qobject_cast<const SourceThumbnailKey*>(val2.data());
                    if (!sourceThumbnailKey1 || !sourceThumbnailKey2)
                    {
                        return false;
                    }
                    //! SourceThumbnailKeys only care about file extension, if multiple different files have same extension, they only need 1 thumbnail
                    return sourceThumbnailKey1->GetExtension() == sourceThumbnailKey2->GetExtension();
                }
            };
        }

        //! SourceAssetBrowserEntry thumbnails
        class SourceThumbnailCache
            : public ThumbnailCache<SourceThumbnail, SourceKeyHash, SourceKeyEqual>
        {
        public:
            SourceThumbnailCache();
            ~SourceThumbnailCache() override;

        protected:
            bool IsSupportedThumbnail(SharedThumbnailKey key) const override;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework