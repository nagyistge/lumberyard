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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYSYSTEM_THREADPROFILER_H
#define CRYINCLUDE_CRYSYSTEM_THREADPROFILER_H
#pragma once

#include "platform.h"
#include "FrameProfiler.h"

#if (defined(USE_FRAME_PROFILER) && defined(WIN32) && !defined(WIN64))
#define THREAD_SAMPLER
#endif

#ifdef THREAD_SAMPLER

enum ETraceThreads
{
    TT_SYSTEM_IDLE_0,
    TT_SYSTEM_IDLE_1,
    TT_MAIN,
    TT_RENDER,
    TT_OTHER,
    TT_TOTAL,   // total = other + render + ... + sys idle 0

    TT_NUM  // must be at the end
};

// Pixels span used to display data.
struct SThreadDisplaySpan
{
    uint16 start;
    uint16 end;
};

typedef std::vector<SThreadDisplaySpan> TTDSpanList;
typedef uint32 TThreadID;
typedef uint32 TProcessID;

enum
{
    INVALID_THREADID = -1
};

struct SSnapshotInfo
{
    SSnapshotInfo(int numProcessors)
        : m_procNumCtxtSwitches(numProcessors) {}

    void Reset()
    {
        for (uint32 i = 0, c = m_procNumCtxtSwitches.size(); i < c; i++)
        {
            m_procNumCtxtSwitches[i] = 0;
        }
    }

    std::vector<int> m_procNumCtxtSwitches; // number of context switches per processor
};

class IThreadSampler
{
public:
    virtual ~IThreadSampler() {}

    virtual void EnumerateThreads(TProcessID processId) = 0;
    virtual int GetNumHWThreads() = 0;  // hardware threads
    virtual int GetNumThreads() = 0;        // software threads
    virtual TThreadID GetThreadId(int idx) = 0; // 0 <= idx < GetNumThreads()
    virtual const char* GetThreadName(TThreadID threadId) = 0;
    virtual float GetExecutionTimeFrame() = 0;  // the time duration of the last tick
    virtual float GetExecutionTime(ETraceThreads tt) = 0;   // the on CPU time for tt

    virtual void Tick() = 0;
    virtual const SSnapshotInfo* GetSnapshot() = 0;

    // Create span list for drawing graph
    // processId, threadId - build list for specified thread.
    // threadId = OTHER_THREADS - build list for threads not belonging to specified process
    // width - width of timeline, in pixels
    // scale - scale of timeline, in 0.5 seconds units, f.e 2 for 1-second timeline. 4 for 2 seconds timeline etc.
    // processorId - index of the hardware processor thread (or core)
    // totalTime - receives total time in milliseconds, used by this thread during one second (does not depend on 'scale' parameter).
    virtual void CreateSpanListForThread(TProcessID processId, TThreadID threadId,
        TTDSpanList& spanList,
        uint32 width, uint32 scale,
        uint32* totalTime, int* processorId, uint32* color) = 0;
};

//////////////////////////////////////////////////////////////////////////
//! the system which does the gathering of stats
class CThreadProfiler
{
public:
    CThreadProfiler();
    ~CThreadProfiler();

    void Start();
    void Stop();
    void Render();
    IThreadSampler* GetThreadSampler() { return m_pSampler; }

private:
    bool m_bRenderEnabled;
    int m_nUsers;
    std::vector<float> m_lastActive;
    int m_nDisplayedThreads;

    IThreadSampler* m_pSampler;
};

#else // THREAD_SAMPLER

class IThreadSampler;

class CThreadProfiler
{
public:
    void Start() {};
    void Stop() {};
    void Render() {};
    IThreadSampler* GetThreadSampler() { return NULL; }
};

#endif

#endif // CRYINCLUDE_CRYSYSTEM_THREADPROFILER_H
