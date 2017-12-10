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
#include "MCoreSystem.h"

// some c++11 concurrency features
#include <functional>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>


namespace MCore
{
    // forward declarations
    class JobList;


    class MCORE_API Mutex
    {
        friend class ConditionVariable;
    public:
        MCORE_INLINE Mutex()                {}
        MCORE_INLINE ~Mutex()               {}

        MCORE_INLINE void Lock()            { mMutex.lock(); }
        MCORE_INLINE void Unlock()          { mMutex.unlock(); }
        MCORE_INLINE bool TryLock()         { return mMutex.try_lock(); }

    private:
        std::mutex  mMutex;
    };


    class MCORE_API MutexRecursive
    {
    public:
        MCORE_INLINE MutexRecursive()       {}
        MCORE_INLINE ~MutexRecursive()      {}

        MCORE_INLINE void Lock()            { mMutex.lock(); }
        MCORE_INLINE void Unlock()          { mMutex.unlock(); }
        MCORE_INLINE bool TryLock()         { return mMutex.try_lock(); }

    private:
        std::recursive_mutex    mMutex;
    };


    class MCORE_API ConditionVariable
    {
    public:
        MCORE_INLINE ConditionVariable()    {}
        MCORE_INLINE ~ConditionVariable()   {}

        MCORE_INLINE void Wait(Mutex& mtx, const std::function<bool()>& predicate)              { std::unique_lock<std::mutex> lock(mtx.mMutex); mVariable.wait(lock, predicate); }
        MCORE_INLINE void WaitWithTimeout(Mutex& mtx, uint32 microseconds, const std::function<bool()>& predicate)  { std::unique_lock<std::mutex> lock(mtx.mMutex); mVariable.wait_for(lock, std::chrono::microseconds(microseconds), predicate); }
        MCORE_INLINE void NotifyOne()                                                           { mVariable.notify_one(); }
        MCORE_INLINE void NotifyAll()                                                           { mVariable.notify_all(); }

    private:
        std::condition_variable mVariable;
    };


    class MCORE_API AtomicInt32
    {
    public:
        MCORE_INLINE AtomicInt32()                  { SetValue(0); }
        MCORE_INLINE ~AtomicInt32()                 {}

        MCORE_INLINE void SetValue(int32 value)     { mAtomic.store(value); }
        MCORE_INLINE int32 GetValue() const         { int32 value = mAtomic.load(); return value; }

        MCORE_INLINE int32 Increment()              { return mAtomic++; }
        MCORE_INLINE int32 Decrement()              { return mAtomic--; }

    private:
        std::atomic<int32>  mAtomic;
    };



    class MCORE_API AtomicUInt32
    {
    public:
        MCORE_INLINE AtomicUInt32()                 { SetValue(0); }
        MCORE_INLINE ~AtomicUInt32()                {}

        MCORE_INLINE void SetValue(uint32 value)    { mAtomic.store(value); }
        MCORE_INLINE uint32 GetValue() const        { uint32 value = mAtomic.load(); return value; }

        MCORE_INLINE uint32 Increment()             { return mAtomic++; }
        MCORE_INLINE uint32 Decrement()             { return mAtomic--; }

    private:
        std::atomic<uint32> mAtomic;
    };


    class MCORE_API Thread
    {
    public:
        Thread() {}
        Thread(const std::function<void()>& threadFunction)         { Init(threadFunction); }
        ~Thread() {}

        void Init(const std::function<void()>& threadFunction)      { mThread = std::thread(threadFunction); }
        void Join()         { mThread.join(); }

    private:
        std::thread mThread;
    };


    class MCORE_API LockGuard
    {
    public:
        MCORE_INLINE LockGuard(Mutex& mutex)        { mMutex = &mutex; mutex.Lock(); }
        MCORE_INLINE ~LockGuard()                   { mMutex->Unlock(); }

    private:
        Mutex*  mMutex;
    };


    class MCORE_API LockGuardRecursive
    {
    public:
        MCORE_INLINE LockGuardRecursive(MutexRecursive& mutex)  { mMutex = &mutex; mutex.Lock(); }
        MCORE_INLINE ~LockGuardRecursive()                      { mMutex->Unlock(); }

    private:
        MutexRecursive* mMutex;
    };


    class MCORE_API ConditionEvent
    {
    public:
        ConditionEvent()            { mConditionValue = false; }
        ~ConditionEvent()           { }

        void Reset()                { mConditionValue = false; }
        void Wait()
        {
            mCV.Wait(mMutex, [this] { return mConditionValue;
                });
        }
        void WaitWithTimeout(uint32 microseconds)
        {
            mCV.WaitWithTimeout(mMutex, microseconds, [this] { return mConditionValue;
                });
        }
        void NotifyAll()            { mConditionValue = true; mCV.NotifyAll(); }
        void NotifyOne()            { mConditionValue = true; mCV.NotifyOne(); }

    private:
        Mutex               mMutex;
        ConditionVariable   mCV;
        bool                mConditionValue;
    };


    // the main job list execute function, call this one to execute work
    MCORE_API void ExecuteJobList(JobList* jobList, bool addSyncPointAfterList = true, bool waitForJobListToFinish = true); // the main function you should call

    // implementations (do not call these directly, use ExecuteJobList instead)
    MCORE_API void JobListExecuteSerial(JobList* jobList, bool addSyncPointAfterList, bool waitForJobListToFinish);         // serial execution (no multi-threading)
    MCORE_API void JobListExecuteMCoreJobSystem(JobList* jobList, bool addSyncPointAfterList, bool waitForJobListToFinish); // parallel execution using the MCore job system
#ifdef MCORE_OPENMP_ENABLED
    MCORE_API void JobListExecuteOpenMP(JobList* jobList, bool addSyncPointAfterList, bool waitForJobListToFinish);         // parallel execution using OpenMP
#endif
}   // namespace MCore
