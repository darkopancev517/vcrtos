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

#include <vcrtos/thread.h>

#include "core/instance.hpp"
#include "core/thread.hpp"

using namespace vc;

kernel_pid_t thread_create(void *instance,
                           char *stack,
                           int size,
                           char priority,
                           int flags,
                           thread_handler_func_t func,
                           void *arg,
                           const char *name)
{
    Instance &instances = *static_cast<Instance *>(instance);
    Thread *thread = Thread::init(instances, stack, size, priority, flags, func, arg, name);
    return (thread) ? thread->get_pid() : KERNEL_PID_UNDEF;
}

int thread_scheduler_get_context_switch_request(void *instance)
{
    Instance &instances = *static_cast<Instance *>(instance);
    return instances.get<ThreadScheduler>().is_context_switch_requested();
}

void thread_scheduler_set_context_switch_request(void *instance, unsigned state)
{
    Instance &instances = *static_cast<Instance *>(instance);

    if (state)
    {
        instances.get<ThreadScheduler>().enable_context_switch_request();
    }
    else
    {
        instances.get<ThreadScheduler>().disable_context_switch_request();
    }
}

void thread_scheduler_run(void *instance)
{
    Instance &instances = *static_cast<Instance *>(instance);
    instances.get<ThreadScheduler>().run();
}

void thread_scheduler_set_status(void *instance, thread_t *thread, thread_status_t status)
{
    Instance &instances = *static_cast<Instance *>(instance);
    instances.get<ThreadScheduler>().set_thread_status(static_cast<Thread *>(thread), status);
}

void thread_scheduler_switch(void *instance, uint8_t priority)
{
    Instance &instances = *static_cast<Instance *>(instance);
    instances.get<ThreadScheduler>().context_switch(priority);
}

void thread_exit(void *instance)
{
    Instance &instances = *static_cast<Instance *>(instance);
    instances.get<ThreadScheduler>().exit_current_active_thread();
}

int thread_pid_is_valid(kernel_pid_t pid)
{
    return Thread::is_pid_valid(pid);
}

void thread_yield(void *instance)
{
    Instance &instances = *static_cast<Instance *>(instance);
    instances.get<ThreadScheduler>().yield();
}

thread_t *thread_current(void *instance)
{
    Instance &instances = *static_cast<Instance *>(instance);
    Thread *thread = instances.get<ThreadScheduler>().get_current_active_thread();
    return static_cast<thread_t *>(thread);
}

void thread_sleep(void *instance)
{
    Instance &instances = *static_cast<Instance *>(instance);
    instances.get<ThreadScheduler>().sleeping_current_thread();
}

int thread_wakeup(void *instance, kernel_pid_t pid)
{
    Instance &instances = *static_cast<Instance *>(instance);
    return instances.get<ThreadScheduler>().wakeup_thread(pid);
}

kernel_pid_t thread_current_pid(void *instance)
{
    Instance &instances = *static_cast<Instance *>(instance);
    return instances.get<ThreadScheduler>().get_current_active_pid();
}

thread_t *thread_get_from_scheduler(void *instance, kernel_pid_t pid)
{
    Instance &instances = *static_cast<Instance *>(instance);
    Thread *thread = instances.get<ThreadScheduler>().get_thread_from_scheduler(pid);
    return static_cast<thread_t *>(thread);
}

uint64_t thread_get_runtime_ticks(void *instance, kernel_pid_t pid)
{
    Instance &instances = *static_cast<Instance *>(instance);
    return instances.get<ThreadScheduler>().get_thread_runtime_ticks(pid);
}

const char *thread_status_to_string(thread_status_t status)
{
    return ThreadScheduler::thread_status_to_string(status);
}

uintptr_t thread_measure_stack_free(char *stack)
{
    uintptr_t *stackp = (uintptr_t *)stack;

    /* assume that the comparison fails before or after end of stack */
    /* assume that the stack grows "downwards" */
    while (*stackp == (uintptr_t)stackp)
    {
        stackp++;
    }

    uintptr_t space_free = (uintptr_t)stackp - (uintptr_t)stack;

    return space_free;
}

unsigned thread_get_schedules_stat(void *instance, kernel_pid_t pid)
{
    Instance &instances = *static_cast<Instance *>(instance);
    return instances.get<ThreadScheduler>().get_thread_schedules_stat(pid);
}
