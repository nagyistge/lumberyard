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
#ifndef AZSTD_MUTEX_LINIX_H
#define AZSTD_MUTEX_LINUX_H

/**
 * This file is to be included from the mutex.h only. It should NOT be included by the user.
 */

namespace AZStd
{
    //////////////////////////////////////////////////////////////////////////
    // mutex
    inline mutex::mutex()
    {
        pthread_mutex_init(&m_mutex, NULL);
    }
    inline mutex::mutex(const char* name)
    {
        (void)name;
        pthread_mutex_init(&m_mutex, NULL);
    }

    inline mutex::~mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    AZ_FORCE_INLINE void
    mutex::lock()
    {
        pthread_mutex_lock(&m_mutex);
    }
    AZ_FORCE_INLINE bool
    mutex::try_lock()
    {
        return pthread_mutex_trylock(&m_mutex) == 0;
    }
    AZ_FORCE_INLINE void
    mutex::unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }
    AZ_FORCE_INLINE mutex::native_handle_type
    mutex::native_handle()
    {
        return &m_mutex;
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // recursive_mutex
    inline recursive_mutex::recursive_mutex()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    inline recursive_mutex::recursive_mutex(const char* name)
    {
        (void)name;
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    inline recursive_mutex::~recursive_mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    AZ_FORCE_INLINE void
    recursive_mutex::lock()
    {
        pthread_mutex_lock(&m_mutex);
    }
    AZ_FORCE_INLINE bool
    recursive_mutex::try_lock()
    {
        return pthread_mutex_trylock(&m_mutex) == 0;
    }
    AZ_FORCE_INLINE void
    recursive_mutex::unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }
    AZ_FORCE_INLINE recursive_mutex::native_handle_type
    recursive_mutex::native_handle()
    {
        return &m_mutex;
    }
    //////////////////////////////////////////////////////////////////////////
}



#endif // AZSTD_MUTEX_WINDOWS_H
#pragma once