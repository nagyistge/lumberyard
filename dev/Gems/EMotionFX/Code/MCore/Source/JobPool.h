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

// include required headers
#include "StandardHeaders.h"
#include "Array.h"
#include "MultiThreadManager.h"


namespace MCore
{
    // forward declarations
    class Job;


    /**
     *
     *
     */
    class MCORE_API JobPool
    {
        MCORE_MEMORYOBJECTCATEGORY(JobPool, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_JOBSYSTEM)

    public:
        enum EPoolType
        {
            POOLTYPE_STATIC,
            POOLTYPE_DYNAMIC
        };

        JobPool();
        ~JobPool();

        void Init(uint32 numInitialJobs = 1024, EPoolType poolType = POOLTYPE_DYNAMIC, uint32 subPoolSize = 1024);

        // with lock
        Job* RequestNew();
        void Free(Job* job);

        // without lock
        Job* RequestNewWithoutLock();
        void FreeWithoutLock(Job* job);

        // misc
        void LogMemoryStats();  // locks internally
        void Lock();
        void Unlock();

    private:
        class MCORE_API SubPool
        {
            MCORE_MEMORYOBJECTCATEGORY(SubPool, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_JOBSYSTEM)

        public:
            SubPool();
            ~SubPool();

            uint8*      mData;
            uint32      mNumJobs;
        };


        class MCORE_API Pool
        {
            MCORE_MEMORYOBJECTCATEGORY(Pool, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_JOBSYSTEM)

        public:
            Pool();
            ~Pool();

            uint8*                  mData;
            uint32                  mNumJobs;
            uint32                  mNumUsedJobs;
            uint32                  mSubPoolSize;
            MCore::Array<void*>     mFreeList;
            MCore::Array<SubPool*>  mSubPools;
            EPoolType               mPoolType;
        };

        Pool*   mPool;
        Mutex   mLock;
    };
}   // namespace MCore
