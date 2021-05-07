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

#include <vcrtos/assert.h>

#include "core/new.hpp"
#include "core/thread.hpp"
#include "core/code_utils.h"

extern "C" {
void *sched_active_thread = nullptr;
int16_t sched_active_pid = KERNEL_PID_UNDEF;
int sched_is_initialized = 0;
}

namespace vc {

DEFINE_ALIGNED_VAR(vcrtosThreadScheduler, sizeof(ThreadScheduler), uint64_t);

ThreadScheduler &ThreadScheduler::init()
{
    ThreadScheduler *sched = &get();
    sched = new (&vcrtosThreadScheduler) ThreadScheduler();
    sched_active_thread = nullptr;
    sched_active_pid = KERNEL_PID_UNDEF;
    sched_is_initialized = 1;
    return *sched;
}

ThreadScheduler &ThreadScheduler::get()
{
    void *sched = &vcrtosThreadScheduler;
    return *static_cast<ThreadScheduler *>(sched);
}

int ThreadScheduler::is_initialized()
{
    return sched_is_initialized;
}

Thread::Thread()
{
    this->stack_pointer = nullptr;
    this->status = THREAD_STATUS_NOT_FOUND;
    this->priority = KERNEL_THREAD_PRIORITY_IDLE;
    this->pid = KERNEL_PID_UNDEF;
#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
    this->flags = 0;
    this->waited_flags = 0;
#endif
    this->runqueue_entry.next = nullptr;
    this->wait_data = nullptr;
    this->msg_waiters.next = nullptr;

    this->msg_queue.read_count = 0;
    this->msg_queue.write_count = 0;
    this->msg_queue.mask = 0;

    this->msg_array = nullptr;
    this->stack_start = nullptr;
    this->name = nullptr;
    this->stack_size = 0;
}

Thread *Thread::init(char *allocated_stack,
                     int size,
                     thread_handler_func_t handler_func,
                     const char *name,
                     unsigned priority,
                     void *arg,
                     int flags)
{
    if (priority >= VCRTOS_CONFIG_THREAD_PRIORITY_LEVELS) return nullptr;

    char *stack = allocated_stack;

    int total_stack_size = size;

    /* aligned the stack on 16/32 bit boundary */
    uintptr_t misalignment = reinterpret_cast<uintptr_t>(stack) % 8;

    if (misalignment)
    {
        misalignment = 8 - misalignment;
        stack += misalignment;
        size -= misalignment;
    }

    /* make room for the Thread */
    size -= sizeof(Thread);

    /* round down the stacksize to multiple of Thread aligments (usually 16/32 bit) */
    size -= size % 8;

    if (size < 0)
    {
        // TODO: warning: stack size is to small
        return nullptr;
    }

    /* allocate thread control block (thread) at the top of our stackspace */
    Thread *thread = new (stack + size) Thread();

    if (flags & THREAD_FLAGS_CREATE_STACKMARKER)
    {
        /* assign each int of the stack the value of it's address, for test purposes */
        uintptr_t *stackmax = reinterpret_cast<uintptr_t *>(stack + size);
        uintptr_t *stackp = reinterpret_cast<uintptr_t *>(stack);

        while (stackp < stackmax)
        {
            *stackp = reinterpret_cast<uintptr_t>(stackp);
            stackp++;
        }
    }
    else
    {
        /* create stack guard */
        *(uintptr_t *)stack = reinterpret_cast<uintptr_t>(stack);
    }

    unsigned irqmask = cpu_irq_disable();

    kernel_pid_t pid = KERNEL_PID_UNDEF;

    ThreadScheduler &scheduler = ThreadScheduler::get();

    for (kernel_pid_t i = KERNEL_PID_FIRST; i <= KERNEL_PID_LAST; ++i)
    {
        if (scheduler.get_thread_from_container(i) == nullptr)
        {
            pid = i;
            break;
        }
    }

    if (pid == KERNEL_PID_UNDEF)
    {
        cpu_irq_restore(irqmask);
        return nullptr;
    }

    scheduler.add_thread(thread, pid);

    thread->pid = pid;
    thread->stack_pointer = thread_arch_stack_init(handler_func, arg, stack, size);
    thread->stack_start = stack;
    thread->stack_size = total_stack_size;
    thread->name = name;
    thread->priority = priority;
    thread->status = THREAD_STATUS_STOPPED;

    scheduler.add_numof_threads();

    if (flags & THREAD_FLAGS_CREATE_SLEEPING)
    {
        scheduler.set_thread_status(thread, THREAD_STATUS_SLEEPING);
    }
    else
    {
        scheduler.set_thread_status(thread, THREAD_STATUS_PENDING);
        if (!(flags & THREAD_FLAGS_CREATE_WOUT_YIELD))
        {
            cpu_irq_restore(irqmask);
            scheduler.context_switch(priority);
            return thread;
        }
    }

    cpu_irq_restore(irqmask);

    return thread;
}

void Thread::add_to_list(List *list)
{
    vcassert(status < THREAD_STATUS_RUNNING);
    uint8_t my_priority = priority;
    List *my_node = static_cast<List *>(get_runqueue_entry());
    while (list->next)
    {
        Thread *thread_on_list = Thread::get_thread_pointer_from_list_member(static_cast<List *>(list->next));
        if (thread_on_list->priority > my_priority)
        {
            break;
        }
        list = static_cast<List *>(list->next);
    }
    my_node->next = list->next;
    list->next = my_node;
}

Thread *Thread::get_thread_pointer_from_list_member(List *list)
{
    list_node_t *node = static_cast<list_node_t *>(list);
    thread_t *thread = container_of(node, thread_t, runqueue_entry);
    return static_cast<Thread *>(thread);
}

void Thread::init_msg_queue(Msg *msg, int num)
{
    msg_array = msg;
    (static_cast<Cib *>(&msg_queue))->init(num);
}

int Thread::has_msg_queue()
{
    return msg_array != nullptr;
}

int Thread::numof_msg_in_queue()
{
    int queued_msgs = -1;
    if (has_msg_queue())
    {
        queued_msgs = (static_cast<Cib *>(&msg_queue))->avail();
    }
    return queued_msgs;
}

int Thread::is_pid_valid(kernel_pid_t pid)
{
    return ((KERNEL_PID_FIRST <= pid) && (pid <= KERNEL_PID_LAST));
}

int Thread::queued_msg(Msg *msg)
{
    int index = (static_cast<Cib *>(&msg_queue))->put();
    if (index < 0)
    {
        return 0;
    }
    Msg *dest = static_cast<Msg *>(&msg_array[index]);
    *dest = *msg;
    return 1;
}

void ThreadScheduler::run()
{
    context_switch_request = 0;
    Thread *current_thread = (Thread *)sched_active_thread;
    Thread *next_thread = get_next_thread_from_runqueue();

    if (current_thread == next_thread)
        return;

    if (current_thread != nullptr)
    {
        if (current_thread->status == THREAD_STATUS_RUNNING)
            current_thread->status = THREAD_STATUS_PENDING;
    }

    scheduler_stats[next_thread->pid].schedules += 1;

    next_thread->status = THREAD_STATUS_RUNNING;
    sched_active_thread = (void *)next_thread;
    sched_active_pid = (int16_t)next_thread->pid;
}

void ThreadScheduler::set_thread_status(Thread *thread, thread_status_t new_status)
{
    uint8_t priority = thread->priority;

    if (new_status >= THREAD_STATUS_RUNNING)
    {
        if (thread->status < THREAD_STATUS_RUNNING)
        {
            list_node_t *thread_runqueue_entry = thread->get_runqueue_entry();
            scheduler_runqueue[priority].right_push(static_cast<Clist *>(thread_runqueue_entry));
            runqueue_bitcache |= 1 << priority;
        }
    }
    else
    {
        if (thread->status >= THREAD_STATUS_RUNNING)
        {
            scheduler_runqueue[priority].left_pop();
            if (scheduler_runqueue[priority].next == nullptr)
                runqueue_bitcache &= ~(1 << priority);
        }
    }

    thread->status = new_status;
}

void ThreadScheduler::context_switch(uint8_t priority_to_switch)
{
    Thread *current_thread = (Thread *)sched_active_thread;
    uint8_t current_priority = current_thread->priority;
    int is_in_runqueue = (current_thread->status >= THREAD_STATUS_RUNNING);
    /* Note: the lowest priority number is the highest priority thread */
    if (!is_in_runqueue || (current_priority > priority_to_switch))
    {
        if (cpu_is_in_isr())
        {
            context_switch_request = 1;
        }
        else
        {
            yield_higher_priority_thread();
        }
    }
}

void ThreadScheduler::yield_higher_priority_thread()
{
    thread_arch_yield_higher();
}

/* Source: http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup */
const uint8_t MultiplyDeBruijnBitPosition[32] =
{
    0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
    31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
};

unsigned ThreadScheduler::bitarithm_lsb(unsigned v)
{
    return MultiplyDeBruijnBitPosition[((uint32_t)((v & -v) * 0x077CB531U)) >> 27];
}

Thread *ThreadScheduler::get_next_thread_from_runqueue()
{
    uint8_t priority = bitarithm_lsb(runqueue_bitcache);
    list_node_t *thread_ptr_in_queue = static_cast<list_node_t *>((scheduler_runqueue[priority].next)->next);
    thread_t *thread = container_of(thread_ptr_in_queue, thread_t, runqueue_entry);
    return static_cast<Thread *>(thread);
}

void ThreadScheduler::sleep()
{
    if (cpu_is_in_isr())
        return;

    unsigned irqmask = cpu_irq_disable();
    set_thread_status((Thread *)sched_active_thread, THREAD_STATUS_SLEEPING);
    cpu_irq_restore(irqmask);
    yield_higher_priority_thread();
}

int ThreadScheduler::wakeup_thread(kernel_pid_t pid)
{
    unsigned irqmask = cpu_irq_disable();
    Thread *thread_to_wakeup = get_thread_from_container(pid);
    if (!thread_to_wakeup)
    {
        //TODO: Warning thread does not exist!
        cpu_irq_restore(irqmask);
        return -1;
    }
    else if (thread_to_wakeup->status == THREAD_STATUS_SLEEPING)
    {
        set_thread_status(thread_to_wakeup, THREAD_STATUS_PENDING);
        cpu_irq_restore(irqmask);
        context_switch(thread_to_wakeup->priority);
        return 1;
    }
    else
    {
        // TODO: Warning thread is not sleeping!
        cpu_irq_restore(irqmask);
        return 0;
    }
}

void ThreadScheduler::yield()
{
    unsigned irqmask = cpu_irq_disable();
    Thread *current_thread = (Thread *)sched_active_thread;

    if (current_thread->status >= THREAD_STATUS_RUNNING)
        scheduler_runqueue[current_thread->priority].left_pop_right_push();

    cpu_irq_restore(irqmask);
    yield_higher_priority_thread();
}

void ThreadScheduler::exit()
{
    (void)cpu_irq_disable();
    threads_container[sched_active_pid] = nullptr;
    numof_threads_in_container -= 1;
    set_thread_status((Thread *)sched_active_thread, THREAD_STATUS_STOPPED);
    sched_active_thread = nullptr;
    // Note: user need to call cpu_switch_context_exit() after this function
}

void ThreadScheduler::terminate(kernel_pid_t pid)
{
    if (sched_active_pid == pid)
    {
        exit();
        return;
    }
    unsigned irqmask = cpu_irq_disable();
    Thread *thread = threads_container[pid];
    threads_container[pid] = nullptr;
    numof_threads_in_container -= 1;
    set_thread_status(thread, THREAD_STATUS_STOPPED);
    cpu_irq_restore(irqmask);
}

const char *ThreadScheduler::thread_status_to_string(thread_status_t status)
{
    const char *retval;

    switch (status)
    {
    case THREAD_STATUS_RUNNING:
        retval = "running";
        break;

    case THREAD_STATUS_PENDING:
        retval = "pending";
        break;

    case THREAD_STATUS_STOPPED:
        retval = "stopped";
        break;

    case THREAD_STATUS_SLEEPING:
        retval = "sleeping";
        break;

    case THREAD_STATUS_MUTEX_BLOCKED:
        retval = "bl mutex";
        break;

    case THREAD_STATUS_RECEIVE_BLOCKED:
        retval = "bl rx";
        break;

    case THREAD_STATUS_SEND_BLOCKED:
        retval = "bl send";
        break;

    case THREAD_STATUS_REPLY_BLOCKED:
        retval = "bl reply";
        break;

    case THREAD_STATUS_FLAG_BLOCKED_ANY:
        retval = "bl flag";
        break;

    case THREAD_STATUS_FLAG_BLOCKED_ALL:
        retval = "bl flags";
        break;

    default:
        retval = "unknown";
        break;
    }

    return retval;
}

uint64_t ThreadScheduler::get_thread_runtime_ticks(kernel_pid_t pid)
{
    vcassert(Thread::is_pid_valid(pid));
    return scheduler_stats[pid].runtime_ticks;
}

uint32_t ThreadScheduler::get_thread_schedules_stat(kernel_pid_t pid)
{
    vcassert(Thread::is_pid_valid(pid));
    return scheduler_stats[pid].schedules;
}

#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
thread_flags_t ThreadScheduler::thread_flags_clear_atomic(Thread *thread, thread_flags_t mask)
{
    unsigned irqmask = cpu_irq_disable();
    mask &= thread->flags;
    thread->flags &= ~mask;
    cpu_irq_restore(irqmask);
    return mask;
}

void ThreadScheduler::thread_flags_wait(thread_flags_t mask, Thread *thread, thread_status_t thread_status, unsigned irqstate)
{
    thread->waited_flags = mask;
    set_thread_status(thread, thread_status);
    cpu_irq_restore(irqstate);
    yield_higher_priority_thread();
}

void ThreadScheduler::thread_flags_wait_any_blocked(thread_flags_t mask)
{
    Thread *current_thread = (Thread *)sched_active_thread;
    unsigned irqmask = cpu_irq_disable();
    if (!(current_thread->flags & mask))
    {
        thread_flags_wait(mask, current_thread, THREAD_STATUS_FLAG_BLOCKED_ANY, irqmask);
    }
    else
    {
        cpu_irq_restore(irqmask);
    }
}

void ThreadScheduler::thread_flags_set(Thread *thread, thread_flags_t mask)
{
    unsigned irqmask = cpu_irq_disable();
    thread->flags |= mask;
    if (thread_flags_wake(thread))
    {
        cpu_irq_restore(irqmask);
        if (!cpu_is_in_isr())
            yield_higher_priority_thread();
    }
    else
    {
        cpu_irq_restore(irqmask);
    }
}

int ThreadScheduler::thread_flags_wake(Thread *thread)
{
    unsigned wakeup;
    thread_flags_t mask = thread->waited_flags;
    switch (thread->status)
    {
    case THREAD_STATUS_FLAG_BLOCKED_ANY:
        wakeup = (thread->flags & mask);
        break;

    case THREAD_STATUS_FLAG_BLOCKED_ALL:
        wakeup = ((thread->flags & mask) == mask);
        break;

    default:
        wakeup = 0;
        break;
    }
    if (wakeup)
    {
        set_thread_status(thread, THREAD_STATUS_PENDING);
        context_switch_request = 1;
    }
    return wakeup;
}

thread_flags_t ThreadScheduler::thread_flags_clear(thread_flags_t mask)
{
    Thread *current_thread = (Thread *)sched_active_thread;
    mask = thread_flags_clear_atomic(current_thread, mask);
    return mask;
}

thread_flags_t ThreadScheduler::thread_flags_wait_any(thread_flags_t mask)
{
    Thread *current_thread = (Thread *)sched_active_thread;
    thread_flags_wait_any_blocked(mask);
    return thread_flags_clear_atomic(current_thread, mask);
}

thread_flags_t ThreadScheduler::thread_flags_wait_all(thread_flags_t mask)
{
    unsigned irqmask = cpu_irq_disable();
    Thread *current_thread = (Thread *)sched_active_thread;
    if (!((current_thread->flags & mask) == mask))
    {
        thread_flags_wait(mask, current_thread, THREAD_STATUS_FLAG_BLOCKED_ALL, irqmask);
    }
    else
    {
        cpu_irq_restore(irqmask);
    }
    return thread_flags_clear_atomic(current_thread, mask);
}

thread_flags_t ThreadScheduler::thread_flags_wait_one(thread_flags_t mask)
{
    thread_flags_wait_any_blocked(mask);
    Thread *current_thread = (Thread *)sched_active_thread;
    thread_flags_t tmp = current_thread->flags & mask;
    /* clear all but least significant bit */
    tmp &= (~tmp + 1);
    return thread_flags_clear_atomic(current_thread, tmp);
}
#endif // #if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE

#if VCRTOS_CONFIG_THREAD_EVENT_ENABLE
void EventQueue::event_post(Event *event, Thread *thread)
{
    vcassert(event && thread != nullptr);
    unsigned irqmask = cpu_irq_disable();
    if (!event->list_node.next)
    {
        (static_cast<Clist *>(&event_list))->right_push(static_cast<Clist *>(&event->list_node));
    }
    cpu_irq_restore(irqmask);
    ThreadScheduler &scheduler = ThreadScheduler::get();
    scheduler.thread_flags_set(thread, THREAD_FLAG_EVENT);
}

void EventQueue::event_cancel(Event *event)
{
    vcassert(event);
    unsigned irqmask = cpu_irq_disable();
    (static_cast<Clist *>(&event_list))->remove(static_cast<Clist *>(&event->list_node));
    event->list_node.next = nullptr;
    cpu_irq_restore(irqmask);
}

Event *EventQueue::event_get()
{
    unsigned irqmask = cpu_irq_disable();
    Event *result = reinterpret_cast<Event *>((static_cast<Clist *>(&event_list))->left_pop());
    cpu_irq_restore(irqmask);

    if (result)
        result->list_node.next = nullptr;

    return result;
}

Event *EventQueue::event_wait()
{
    Event *result = nullptr;
    ThreadScheduler &scheduler = ThreadScheduler::get();
#ifdef UNITTEST
    unsigned irqmask = cpu_irq_disable();
    result = reinterpret_cast<Event *>((static_cast<Clist *>(&event_list))->left_pop());
    cpu_irq_restore(irqmask);
    if (result == nullptr)
        scheduler.thread_flags_wait_any(THREAD_FLAG_EVENT);
#else
    do
    {
        unsigned irqmask = cpu_irq_disable();
        result = reinterpret_cast<Event *>((static_cast<Clist *>(&event_list))->left_pop());
        cpu_irq_restore(irqmask);

        if (result == nullptr)
            scheduler.thread_flags_wait_any(THREAD_FLAG_EVENT);

    } while (result == nullptr);

#endif
    return result;
}

void EventQueue::event_release(Event *event)
{
    /* Note: before releasing the event, make sure it's no longer in the event_queue */
    event->list_node.next = nullptr;
}

int EventQueue::event_pending()
{
    return (static_cast<Clist *>(&event_list))->count();
}

Event *EventQueue::event_peek()
{
    return reinterpret_cast<Event *>((static_cast<Clist *>(&event_list))->left_peek());
}
#endif // #if VCRTOS_CONFIG_THREAD_EVENT_ENABLE

extern "C" void cpu_end_of_isr()
{
    ThreadScheduler &scheduler = ThreadScheduler::get();
    if (scheduler.requested_context_switch())
    {
        ThreadScheduler::yield_higher_priority_thread();
    }
}

} // namespace vc
