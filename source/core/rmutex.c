/*
 * Copyright (c) 2020, Vertexcom Technologies, Inc.
 * All rights reserved.
 *
 * NOTICE: All information contained herein is, and remains
 * the property of Vertexcom Technologies, Inc. and its suppliers,
 * if any. The intellectual and technical concepts contained
 * herein are proprietary to Vertexcom Technologies, Inc.
 * and may be covered by U.S. and Foreign Patents, patents in process,
 * and protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Vertexcom Technologies, Inc.
 *
 * Authors: Darko Pancev <darko.pancev@vertexcom.com>
 */

#include <vcrtos/rmutex.h>
#include <vcrtos/thread.h>
#include <vcrtos/assert.h>

static int _lock(rmutex_t *rmutex, int trylock)
{
    kernel_pid_t owner;
    if (mutex_try_lock(&rmutex->mutex) == 0)
    {
        owner = atomic_load_explicit(&rmutex->owner, memory_order_relaxed);
        if (owner != thread_current_pid())
        {
            if (trylock)
            {
                return 0;
            }
            else
            {
                mutex_lock(&rmutex->mutex);
            }
        }
    }
    atomic_store_explicit(&rmutex->owner, thread_current_pid(), memory_order_relaxed);
    rmutex->refcount++;
    return 1;
}

void rmutex_init(rmutex_t *rmutex)
{
    mutex_init(&rmutex->mutex);
    rmutex->refcount = 0;
    rmutex->owner = ATOMIC_VAR_INIT(KERNEL_PID_UNDEF);
}

void rmutex_lock(rmutex_t *rmutex)
{
    _lock(rmutex, 0);
}

int rmutex_trylock(rmutex_t *rmutex)
{
    return _lock(rmutex, 1);
}

void rmutex_unlock(rmutex_t *rmutex)
{
    vcassert(atomic_load_explicit(&rmutex->owner, memory_order_relaxed) == thread_current_pid());
    vcassert(rmutex->refcount > 0);
    rmutex->refcount--;
    if (rmutex->refcount == 0)
    {
        atomic_store_explicit(&rmutex->owner, KERNEL_PID_UNDEF, memory_order_relaxed);
        mutex_unlock(&rmutex->mutex);
    }
}
