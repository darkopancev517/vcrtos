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

#include "core/thread.hpp"
#include "core/code_utils.h"

using namespace vc;

kernel_pid_t thread_create(char *stack,
                           int size,
                           thread_handler_func_t func,
                           const char *name,
                           char priority,
                           void *arg,
                           int flags)
{
    Thread *thread = Thread::init(stack, size, func, name, priority, arg, flags);
    return (thread) ? thread->get_pid() : KERNEL_PID_UNDEF;
}

void thread_scheduler_init()
{
    ThreadScheduler::init();
}

int thread_scheduler_is_initialized()
{
    return ThreadScheduler::is_initialized();
}

int thread_scheduler_requested_context_switch()
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    return scheduler->requested_context_switch();
}

void thread_scheduler_context_switch_request(unsigned state)
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    scheduler->set_context_switch_request(state);
}

void thread_scheduler_run()
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    scheduler->run();
}

void thread_scheduler_set_status(thread_t *thread, thread_status_t status)
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    scheduler->set_thread_status(static_cast<Thread *>(thread), status);
}

void thread_scheduler_switch(uint8_t priority)
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    scheduler->context_switch(priority);
}

void thread_exit()
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    scheduler->exit();
}

void thread_terminate(kernel_pid_t pid)
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    scheduler->terminate(pid);
}

int thread_pid_is_valid(kernel_pid_t pid)
{
    return Thread::is_pid_valid(pid);
}

void thread_yield()
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    scheduler->yield();
}

thread_t *thread_current()
{
    return (thread_t *)sched_active_thread;
}

void thread_sleep()
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    scheduler->sleep();
}

int thread_wakeup(kernel_pid_t pid)
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    return scheduler->wakeup_thread(pid);
}

kernel_pid_t thread_current_pid()
{
    return (kernel_pid_t)sched_active_pid;
}

thread_t *thread_get_from_scheduler(kernel_pid_t pid)
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    return static_cast<thread_t *>(scheduler->get_thread_from_container(pid));
}

uint64_t thread_get_runtime_ticks(kernel_pid_t pid)
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    return scheduler->get_thread_runtime_ticks(pid);
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

uint32_t thread_get_schedules_stat(kernel_pid_t pid)
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    return scheduler->get_thread_schedules_stat(pid);
}

void thread_add_to_list(list_node_t *list, thread_t *thread)
{
    uint16_t my_prio = thread->priority;
    list_node_t *new_node = (list_node_t *)&thread->runqueue_entry;

    while (list->next)
    {
        thread_t *list_entry = container_of((list_node_t *)list->next,
                                            thread_t,
                                            runqueue_entry);

        if (list_entry->priority > my_prio)
            break;

        list = list->next;
    }
    new_node->next = list->next;
    list->next = new_node;
}
