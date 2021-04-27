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

#include "core/mutex.hpp"
#include "core/thread.hpp"

namespace vc {

int Mutex::set_lock(int blocking)
{
    unsigned irqmask = cpu_irq_disable();
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    if (queue.next == nullptr)
    {
        /* mutex was unlocked */
        queue.next = MUTEX_LOCKED;
        cpu_irq_restore(irqmask);
        return 1;
    }
    else if (blocking)
    {
        Thread *current_thread = (Thread *)sched_active_thread;
        scheduler->set_thread_status(current_thread, THREAD_STATUS_MUTEX_BLOCKED);
        if (queue.next == MUTEX_LOCKED)
        {
            queue.next = current_thread->get_runqueue_entry();
            queue.next->next = nullptr;
        }
        else
        {
            current_thread->add_to_list(static_cast<List *>(&queue));
        }
        cpu_irq_restore(irqmask);
        ThreadScheduler::yield_higher_priority_thread();
        return 1;
    }
    else
    {
        cpu_irq_restore(irqmask);
        return 0;
    }
}

kernel_pid_t Mutex::peek()
{
    unsigned irqmask = cpu_irq_disable();
    if (queue.next == nullptr || queue.next == MUTEX_LOCKED)
    {
        cpu_irq_restore(irqmask);
        return KERNEL_PID_UNDEF;
    }
    List *head = (static_cast<List *>(queue.next));
    Thread *thread = Thread::get_thread_pointer_from_list_member(head);
    return thread->pid;
}

void Mutex::unlock()
{
    unsigned irqmask = cpu_irq_disable();
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    if (queue.next == nullptr)
    {
        /* mutex was unlocked */
        cpu_irq_restore(irqmask);
        return;
    }
    if (queue.next == MUTEX_LOCKED)
    {
        queue.next = nullptr;
        /* mutex was locked but no thread was waiting for it */
        cpu_irq_restore(irqmask);
        return;
    }
    List *next = (static_cast<List *>(&queue))->remove_head();
    Thread *thread = Thread::get_thread_pointer_from_list_member(next);
    scheduler->set_thread_status(thread, THREAD_STATUS_PENDING);

    if (!queue.next)
        queue.next = MUTEX_LOCKED;

    cpu_irq_restore(irqmask);
    scheduler->context_switch(thread->priority);
}

void Mutex::unlock_and_sleep()
{
    unsigned irqmask = cpu_irq_disable();
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    if (queue.next)
    {
        if (queue.next == MUTEX_LOCKED)
        {
            queue.next = nullptr;
        }
        else
        {
            List *next = (static_cast<List *>(&queue))->remove_head();
            Thread *thread = Thread::get_thread_pointer_from_list_member(next);
            scheduler->set_thread_status(thread, THREAD_STATUS_PENDING);

            if (!queue.next)
                queue.next = MUTEX_LOCKED;
        }
    }

    cpu_irq_restore(irqmask);
    scheduler->sleep();
}

} // namespace vc
