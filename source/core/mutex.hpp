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

#ifndef CORE_MUTEX_HPP
#define CORE_MUTEX_HPP

#include <stddef.h>
#include <stdint.h>

#include <vcrtos/config.h>
#include <vcrtos/mutex.h>
#include <vcrtos/thread.h>

#include "core/list.hpp"

namespace vc {

class Mutex : public mutex_t
{
public:
    Mutex(int state = MUTEX_INIT_LOCKED)
    {
        if (state == MUTEX_INIT_LOCKED)
        {
            this->queue.next = MUTEX_LOCKED;
        }
        else
        {
            this->queue.next = nullptr;    
        }
    }

    int try_lock() { return set_lock(0); }
    void lock() { set_lock(1); }
    kernel_pid_t peek();
    void unlock();
    void unlock_and_sleep();

private:
    int set_lock(int blocking);
};

} // namespace vc

#endif /* CORE_MUTEX_HPP */
