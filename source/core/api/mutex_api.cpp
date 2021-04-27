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

#include <vcrtos/mutex.h>

#include "core/mutex.hpp"
#include "core/new.hpp"

using namespace vc;

void mutex_init(mutex_t *mutex)
{
    mutex = new (mutex) Mutex();
}

void mutex_init_unlocked(mutex_t *mutex)
{
    mutex = new (mutex) Mutex(MUTEX_INIT_UNLOCKED);
}

void mutex_lock(mutex_t *mutex)
{
    (*static_cast<Mutex *>(mutex)).lock();
}

int mutex_try_lock(mutex_t *mutex)
{
    return (*static_cast<Mutex *>(mutex)).try_lock();
}

void mutex_unlock(mutex_t *mutex)
{
    (*static_cast<Mutex *>(mutex)).unlock();
}

void mutex_unlock_and_sleep(mutex_t *mutex)
{
    (*static_cast<Mutex *>(mutex)).unlock_and_sleep();
}
