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

#include "core/thread.hpp"
#include "core/msg.hpp"

namespace vc {

int Msg::send(kernel_pid_t target_pid, int blocking, unsigned irqmask)
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    Thread *target_thread = scheduler->get_thread_from_container(target_pid);
    sender_pid = sched_active_pid;
    if (target_thread == nullptr || !target_thread->has_msg_queue())
    {
        cpu_irq_restore(irqmask);
        return -1;
    }

    Thread *current_thread = (Thread *)sched_active_thread;
    if (target_thread->status != THREAD_STATUS_RECEIVE_BLOCKED)
    {
        if (target_thread->queued_msg(this))
        {
            cpu_irq_restore(irqmask);
            if (current_thread->status == THREAD_STATUS_REPLY_BLOCKED)
                ThreadScheduler::yield_higher_priority_thread();

            return 1;
        }
        if (!blocking)
        {
            cpu_irq_restore(irqmask);
            return 0;
        }
        current_thread->wait_data = static_cast<void *>(this);
        thread_status_t new_status;
        if (current_thread->status == THREAD_STATUS_REPLY_BLOCKED)
        {
            new_status = THREAD_STATUS_REPLY_BLOCKED;
        }
        else
        {
            new_status = THREAD_STATUS_SEND_BLOCKED;
        }
        scheduler->set_thread_status(current_thread, new_status);
        current_thread->add_to_list(static_cast<List *>(&target_thread->msg_waiters));
        cpu_irq_restore(irqmask);
        ThreadScheduler::yield_higher_priority_thread();
    }
    else
    {
        Msg *target_msg = static_cast<Msg *>(target_thread->wait_data);
        *target_msg = *this;
        scheduler->set_thread_status(target_thread, THREAD_STATUS_PENDING);
        cpu_irq_restore(irqmask);
        ThreadScheduler::yield_higher_priority_thread();
    }
    return 1;
}

int Msg::receive(int blocking)
{
    unsigned irqmask = cpu_irq_disable();

    ThreadScheduler *scheduler = &ThreadScheduler::get();
    Thread *current_thread = (Thread *)sched_active_thread;

    if (current_thread == nullptr || !current_thread->has_msg_queue())
    {
        cpu_irq_restore(irqmask);
        return -1;
    }

    int queue_index = -1;

    if (current_thread->msg_array != nullptr)
        queue_index = (static_cast<Cib *>(&current_thread->msg_queue))->get();

    if (!blocking && (!current_thread->msg_waiters.next && queue_index == -1))
    {
        cpu_irq_restore(irqmask);
        return -1;
    }

    if (queue_index >= 0)
    {
        *this = *static_cast<Msg *>(&current_thread->msg_array[queue_index]);
        cpu_irq_restore(irqmask);
        return 1;
    }
    else
    {
        current_thread->wait_data = static_cast<void *>(this);
    }

    List *next = (static_cast<List *>(&current_thread->msg_waiters))->remove_head();

    if (next == nullptr)
    {
        if (queue_index < 0)
        {
            scheduler->set_thread_status(current_thread, THREAD_STATUS_RECEIVE_BLOCKED);
            cpu_irq_restore(irqmask);
            ThreadScheduler::yield_higher_priority_thread();
        }
        else
        {
            cpu_irq_restore(irqmask);
        }
        return 1;
    }
    else
    {
        Thread *sender_thread = Thread::get_thread_pointer_from_list_member(next);
        Msg *tmp = nullptr;

        if (queue_index >= 0)
        {
            /* We've already got a message from the queue. As there is a
             * waiter, take it's message into the just freed queue space. */
            tmp = static_cast<Msg *>(&current_thread->msg_array[(static_cast<Cib *>(&current_thread->msg_queue))->put()]);
        }

        /* copy msg */
        Msg *sender_msg = static_cast<Msg *>(sender_thread->wait_data);

        if (tmp != nullptr)
        {
            *tmp = *sender_msg;
            *this = *tmp;
        }
        else
        {
            *this = *sender_msg;
        }

        /* remove sender from queue */
        uint8_t sender_priority = KERNEL_THREAD_PRIORITY_IDLE;
        if (sender_thread->get_status() != THREAD_STATUS_REPLY_BLOCKED)
        {
            sender_thread->wait_data = nullptr;
            scheduler->set_thread_status(sender_thread, THREAD_STATUS_PENDING);
            sender_priority = sender_thread->get_priority();
        }

        cpu_irq_restore(irqmask);

        if (sender_priority < KERNEL_THREAD_PRIORITY_IDLE)
            scheduler->context_switch(sender_priority);

        return 1;
    }
}

int Msg::send(kernel_pid_t target_pid)
{
    if (cpu_is_in_isr())
        return send_in_isr(target_pid);

    if (sched_active_pid == target_pid)
        return send_to_self_queue();

    return send(target_pid, 1 /* blocking */, cpu_irq_disable());
}

int Msg::try_send(kernel_pid_t target_pid)
{
    if (cpu_is_in_isr())
        return send_in_isr(target_pid);

    if (sched_active_pid == target_pid)
        return send_to_self_queue();

    return send(target_pid, 0 /* non-blocking */, cpu_irq_disable());
}

int Msg::send_to_self_queue(void)
{
    unsigned irqmask = cpu_irq_disable();
    Thread *current_thread = (Thread *)sched_active_thread;
    if (current_thread == nullptr || !current_thread->has_msg_queue())
    {
        cpu_irq_restore(irqmask);
        return -1;
    }
    sender_pid = current_thread->get_pid();
    int result = current_thread->queued_msg(this);
    cpu_irq_restore(irqmask);
    return result;
}

int Msg::send_in_isr(kernel_pid_t target_pid)
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    Thread *target_thread = scheduler->get_thread_from_container(target_pid);

    if (target_thread == nullptr || !target_thread->has_msg_queue())
        return -1;

    sender_pid = KERNEL_PID_ISR;

    if (target_thread->status == THREAD_STATUS_RECEIVE_BLOCKED)
    {
        Msg *target_msg = static_cast<Msg *>(target_thread->wait_data);
        *target_msg = *this;
        scheduler->set_thread_status(target_thread, THREAD_STATUS_PENDING);
        scheduler->request_context_switch();
        return 1;
    }
    else
    {
        return target_thread->queued_msg(this);
    }
}

int Msg::send_receive(Msg *reply_msg, kernel_pid_t target_pid)
{
    unsigned irqmask = cpu_irq_disable();

    ThreadScheduler *scheduler = &ThreadScheduler::get();
    Thread *current_thread = (Thread *)sched_active_thread;

    if (current_thread->pid == target_pid || !current_thread->has_msg_queue())
        return -1;

    scheduler->set_thread_status(current_thread, THREAD_STATUS_REPLY_BLOCKED);
    current_thread->wait_data = static_cast<void *>(reply_msg);

    /* we re-use (abuse) reply for sending, because wait_data might be
     * overwritten if the target is not in RECEIVE_BLOCKED */

    *reply_msg = *this;

    /* Send() blocks until reply received */
    return reply_msg->send(target_pid, 1 /* blocking */, irqmask);
}

int Msg::reply(Msg *reply_msg)
{
    unsigned irqmask = cpu_irq_disable();

    ThreadScheduler *scheduler = &ThreadScheduler::get();
    Thread *target_thread = scheduler->get_thread_from_container(sender_pid);

    if (target_thread == nullptr ||
        !target_thread->has_msg_queue() ||
        target_thread->status != THREAD_STATUS_REPLY_BLOCKED)
    {
        cpu_irq_restore(irqmask);
        return -1;
    }

    Msg *target_msg = static_cast<Msg *>(target_thread->wait_data);
    *target_msg = *reply_msg;
    scheduler->set_thread_status(target_thread, THREAD_STATUS_PENDING);
    cpu_irq_restore(irqmask);
    scheduler->context_switch(target_thread->priority);
    return 1;
}

int Msg::reply_in_isr(Msg *reply_msg)
{
    ThreadScheduler *scheduler = &ThreadScheduler::get();
    Thread *target_thread = scheduler->get_thread_from_container(sender_pid);

    if (target_thread == nullptr ||
        !target_thread->has_msg_queue() ||
        target_thread->status != THREAD_STATUS_REPLY_BLOCKED)
    {
        return -1;
    }

    Msg *target_msg = static_cast<Msg *>(target_thread->wait_data);
    *target_msg = *reply_msg;
    scheduler->set_thread_status(target_thread, THREAD_STATUS_PENDING);
    scheduler->request_context_switch();
    return 1;
}

} // namespace vc
