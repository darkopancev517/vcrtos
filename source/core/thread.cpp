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

#include "core/instance.hpp"
#include "core/thread.hpp"
#include "core/code_utils.h"

#if VCRTOS_CONFIG_ZTIMER_ENABLE
#include <vcrtos/ztimer.h>
#endif

namespace vc {

Thread *Thread::init(Instance &instances, char *stack, int size, unsigned priority, int flags,
                     thread_handler_func_t handler_func, void *arg, const char *name)
{
    if (priority >= VCRTOS_CONFIG_THREAD_PRIORITY_LEVELS) return NULL;

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
    }

    /* allocate thread control block (tcb) at the top of our stackspace */
    Thread *tcb = (Thread *)(stack + size);

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

    unsigned state = cpu_irq_disable();

    /* initialize instances for this thread */
#if VCRTOS_CONFIG_MULTIPLE_INSTANCE_ENABLE
    tcb->instance = static_cast<void *>(&instances);
#else
    (void)instances;
#endif

    kernel_pid_t pid = KERNEL_PID_UNDEF;

    for (kernel_pid_t i = KERNEL_PID_FIRST; i <= KERNEL_PID_LAST; ++i)
    {
        if (tcb->get<ThreadScheduler>().get_thread_from_scheduler(i) == NULL)
        {
            pid = i;
            break;
        }
    }

    if (pid == KERNEL_PID_UNDEF)
    {
        cpu_irq_restore(state);
        return NULL;
    }

    tcb->get<ThreadScheduler>().set_thread_scheduler(tcb, pid);

    tcb->set_pid(pid);

    tcb->stack_init(handler_func, arg, stack, size);

    tcb->set_stack_start(stack);

    tcb->set_stack_size(total_stack_size);

    tcb->set_name(name);

    tcb->set_priority(priority);

    tcb->set_status(THREAD_STATUS_STOPPED);

    tcb->init_runqueue_entry();

    tcb->init_msg();

    tcb->get<ThreadScheduler>().increment_numof_threads_in_scheduler();

#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
    tcb->init_flags();
#endif

    if (flags & THREAD_FLAGS_CREATE_SLEEPING)
    {
        tcb->get<ThreadScheduler>().set_thread_status(tcb, THREAD_STATUS_SLEEPING);
    }
    else
    {
        tcb->get<ThreadScheduler>().set_thread_status(tcb, THREAD_STATUS_PENDING);

        if (!(flags & THREAD_FLAGS_CREATE_WOUT_YIELD))
        {
            cpu_irq_restore(state);

            tcb->get<ThreadScheduler>().context_switch(priority);

            return tcb;
        }
    }

    cpu_irq_restore(state);

    return tcb;
}

void Thread::stack_init(thread_handler_func_t func, void *arg, void *ptr, int size)
{
    set_stack_pointer(thread_arch_stack_init(func, arg, ptr, size));
}

void Thread::add_to_list(List *list)
{
    vcassert(get_status() < THREAD_STATUS_RUNNING);

    uint8_t my_priority = get_priority();

    List *my_node = static_cast<List *>(get_runqueue_entry());

    while (list->next)
    {
        Thread *thread_on_list = Thread::get_thread_pointer_from_list_member(static_cast<List *>(list->next));

        if (thread_on_list->get_priority() > my_priority)
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

int Thread::has_msg_queue(void)
{
    return msg_array != NULL;
}

int Thread::get_numof_msg_in_queue(void)
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

void Thread::init_msg(void)
{
    wait_data = NULL;
    msg_waiters.next = NULL;
    (static_cast<Cib *>(&msg_queue))->init(0);
    msg_array = NULL;
}

#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
void Thread::init_flags(void)
{
    flags = 0;
    waited_flags = 0;
}
#endif

void ThreadScheduler::run(void)
{
    disable_context_switch_request();

    Thread *current_thread = get_current_active_thread();

    Thread *next_thread = get_next_thread_from_runqueue();

    if (current_thread == next_thread) return;

#if VCRTOS_CONFIG_ZTIMER_ENABLE
    uint32_t time_now = ztimer_now(ZTIMER_USEC);
#else
    uint32_t time_now = 0;
#endif

    if (current_thread != NULL)
    {
        if (current_thread->get_status() == THREAD_STATUS_RUNNING)
        {
            current_thread->set_status(THREAD_STATUS_PENDING);
        }

        scheduler_stat_t *active_stat = &scheduler_stats[current_thread->get_pid()];

        if (active_stat->last_start)
        {
            active_stat->runtime_ticks += time_now - active_stat->last_start;
        }
    }

    scheduler_stat_t *next_stat = &scheduler_stats[next_thread->get_pid()];

    next_stat->last_start = time_now;
    next_stat->schedules++;

    next_thread->set_status(THREAD_STATUS_RUNNING);
    set_current_active_thread(next_thread);
    set_current_active_pid(next_thread->get_pid());
}

void ThreadScheduler::set_thread_status(Thread *thread, thread_status_t status)
{
    uint8_t priority = thread->get_priority();

    if (status >= THREAD_STATUS_RUNNING)
    {
        if (thread->get_status() < THREAD_STATUS_RUNNING)
        {
            list_node_t *thread_runqueue_entry = thread->get_runqueue_entry();

            scheduler_runqueue[priority].right_push(static_cast<Clist *>(thread_runqueue_entry));

            set_runqueue_bitcache(priority);
        }
    }
    else
    {
        if (thread->get_status() >= THREAD_STATUS_RUNNING)
        {
            scheduler_runqueue[priority].left_pop();

            if (scheduler_runqueue[priority].next == NULL)
            {
                reset_runqueue_bitcache(priority);
            }
        }
    }

    thread->set_status(status);
}

void ThreadScheduler::context_switch(uint8_t priority_to_switch)
{
    Thread *current_thread = get_current_active_thread();

    uint8_t current_priority = current_thread->get_priority();

    int is_in_runqueue = (current_thread->get_status() >= THREAD_STATUS_RUNNING);

    /* Note: the lowest priority number is the highest priority thread */

    if (!is_in_runqueue || (current_priority > priority_to_switch))
    {
        if (cpu_is_in_isr())
        {
            enable_context_switch_request();
        }
        else
        {
            yield_higher_priority_thread();
        }
    }
}

void ThreadScheduler::yield_higher_priority_thread(void)
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

uint8_t ThreadScheduler::get_lsb_index_from_runqueue(void)
{
    uint8_t index = 0;

    uint32_t queue = get_runqueue_bitcache();

    /* [IMPORTANT]: this functions assume there will be at least 1 thread on the queue, (idle) thread */

#if 1
    index = bitarithm_lsb(queue);
#else
    while ((queue & 0x01) == 0)
    {
        queue >>= 1;
        index += 1;
    }
#endif

    return index;
}

Thread *ThreadScheduler::get_next_thread_from_runqueue(void)
{
    uint8_t priority = get_lsb_index_from_runqueue();

    list_node_t *thread_ptr_in_queue = static_cast<list_node_t *>((scheduler_runqueue[priority].next)->next);

    thread_t *thread = container_of(thread_ptr_in_queue, thread_t, runqueue_entry);

    return static_cast<Thread *>(thread);
}

void ThreadScheduler::sleeping_current_thread(void)
{
    if (cpu_is_in_isr())
    {
        return;
    }

    unsigned state = cpu_irq_disable();

    set_thread_status(get_current_active_thread(), THREAD_STATUS_SLEEPING);

    cpu_irq_restore(state);

    yield_higher_priority_thread();
}

int ThreadScheduler::wakeup_thread(kernel_pid_t pid)
{
    unsigned state = cpu_irq_disable();

    Thread *thread_to_wakeup = get_thread_from_scheduler(pid);

    if (!thread_to_wakeup)
    {
        //TODO: Warning thread does not exist!
    }
    else if (thread_to_wakeup->get_status() == THREAD_STATUS_SLEEPING)
    {
        set_thread_status(thread_to_wakeup, THREAD_STATUS_RUNNING);

        cpu_irq_restore(state);

        context_switch(thread_to_wakeup->get_priority());

        return 1;
    }
    else
    {
        // TODO: Warning thread is not sleeping!
    }

    cpu_irq_restore(state);

    return (int)THREAD_STATUS_NOT_FOUND;
}

void ThreadScheduler::yield(void)
{
    unsigned state = cpu_irq_disable();

    Thread *current_thread = get_current_active_thread();

    if (current_thread->get_status() >= THREAD_STATUS_RUNNING)
    {
        scheduler_runqueue[current_thread->get_priority()].left_pop_right_push();
    }

    cpu_irq_restore(state);

    yield_higher_priority_thread();
}

void ThreadScheduler::exit_current_active_thread(void)
{
    (void) cpu_irq_disable();

    set_thread_scheduler(NULL, get_current_active_pid());

    decrement_numof_threads_in_scheduler();

    set_thread_status(get_current_active_thread(), THREAD_STATUS_STOPPED);

    set_current_active_thread(NULL);

    cpu_switch_context_exit();
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

#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE

thread_flags_t ThreadScheduler::thread_flags_clear_atomic(Thread *thread, thread_flags_t mask)
{
    unsigned state = cpu_irq_disable();

    mask &= thread->flags;

    thread->flags &= ~mask;

    cpu_irq_restore(state);

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
    Thread *current_thread = get_current_active_thread();

    unsigned state = cpu_irq_disable();

    if (!(current_thread->flags & mask))
    {
        thread_flags_wait(mask, current_thread, THREAD_STATUS_FLAG_BLOCKED_ANY, state);
    }
    else
    {
        cpu_irq_restore(state);
    }
}

void ThreadScheduler::thread_flags_set(Thread *thread, thread_flags_t mask)
{
    unsigned state = cpu_irq_disable();

    thread->flags |= mask;

    if (thread_flags_wake(thread))
    {
        cpu_irq_restore(state);

        if (!cpu_is_in_isr())
        {
            yield_higher_priority_thread();
        }
    }
    else
    {
        cpu_irq_restore(state);
    }
}

int ThreadScheduler::thread_flags_wake(Thread *thread)
{
    unsigned wakeup;

    thread_flags_t mask = thread->waited_flags;

    switch (thread->get_status())
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
        enable_context_switch_request();
    }

    return wakeup;
}

thread_flags_t ThreadScheduler::thread_flags_clear(thread_flags_t mask)
{
    Thread *current_thread = get_current_active_thread();

    mask = thread_flags_clear_atomic(current_thread, mask);

    return mask;
}

thread_flags_t ThreadScheduler::thread_flags_wait_any(thread_flags_t mask)
{
    Thread *current_thread = get_current_active_thread();

    thread_flags_wait_any_blocked(mask);

    return thread_flags_clear_atomic(current_thread, mask);
}

thread_flags_t ThreadScheduler::thread_flags_wait_all(thread_flags_t mask)
{
    unsigned state = cpu_irq_disable();

    Thread *current_thread = get_current_active_thread();

    if (!((current_thread->flags & mask) == mask))
    {
        thread_flags_wait(mask, current_thread, THREAD_STATUS_FLAG_BLOCKED_ALL, state);
    }
    else
    {
        cpu_irq_restore(state);
    }

    return thread_flags_clear_atomic(current_thread, mask);
}

thread_flags_t ThreadScheduler::thread_flags_wait_one(thread_flags_t mask)
{
    thread_flags_wait_any_blocked(mask);

    Thread *current_thread = get_current_active_thread();

    thread_flags_t tmp = current_thread->flags & mask;

    /* clear all but least significant bit */
    tmp &= (~tmp + 1);

    return thread_flags_clear_atomic(current_thread, tmp);
}

#endif // #if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE

#if VCRTOS_CONFIG_THREAD_EVENT_ENABLE

void EventQueue::event_post(Event *event, Thread *thread)
{
    vcassert(event && thread != NULL);

    unsigned state = cpu_irq_disable();

    if (!event->list_node.next)
    {
        (static_cast<Clist *>(&event_list))->right_push(static_cast<Clist *>(&event->list_node));
    }

    cpu_irq_restore(state);

    get<ThreadScheduler>().thread_flags_set(thread, THREAD_FLAG_EVENT);
}

void EventQueue::event_cancel(Event *event)
{
    vcassert(event);

    unsigned state = cpu_irq_disable();

    (static_cast<Clist *>(&event_list))->remove(static_cast<Clist *>(&event->list_node));

    event->list_node.next = NULL;

    cpu_irq_restore(state);
}

Event *EventQueue::event_get(void)
{
    unsigned state = cpu_irq_disable();

    Event *result = reinterpret_cast<Event *>((static_cast<Clist *>(&event_list))->left_pop());

    cpu_irq_restore(state);

    if (result)
    {
        result->list_node.next = NULL;
    }

    return result;
}

Event *EventQueue::event_wait(void)
{
    Event *result = NULL;

#ifdef UNITTEST

    unsigned state = cpu_irq_disable();
    result = reinterpret_cast<Event *>((static_cast<Clist *>(&event_list))->left_pop());
    cpu_irq_restore(state);

    if (result == NULL)
    {
        get<ThreadScheduler>().thread_flags_wait_any(THREAD_FLAG_EVENT);
    }

#else

    do
    {
        unsigned state = cpu_irq_disable();
        result = reinterpret_cast<Event *>((static_cast<Clist *>(&event_list))->left_pop());
        cpu_irq_restore(state);

        if (result == NULL)
        {
            get<ThreadScheduler>().thread_flags_wait_any(THREAD_FLAG_EVENT);
        }

    } while (result == NULL);

#endif

    return result;
}

void EventQueue::event_release(Event *event)
{
    /* Note: before releasing the event, make sure it's no longer in the event_queue */
    event->list_node.next = NULL;
}

int EventQueue::event_pending(void)
{
    return (static_cast<Clist *>(&event_list))->count();
}

Event *EventQueue::event_peek(void)
{
    return reinterpret_cast<Event *>((static_cast<Clist *>(&event_list))->left_peek());
}

#endif // #if VCRTOS_CONFIG_THREAD_EVENT_ENABLE

extern "C" void cpu_end_of_isr(void)
{
#if !VCRTOS_CONFIG_MULTIPLE_INSTANCE_ENABLE
    Instance &instance = *static_cast<Instance *>(instance_get());
    if (instance.get<ThreadScheduler>().is_context_switch_requested())
    {
        ThreadScheduler::yield_higher_priority_thread();
    }
#endif
}

template <> inline Instance &Thread::get(void) const
{
    return get_instance();
}

template <typename Type> inline Type &Thread::get(void) const
{
    return get_instance().get<Type>();
}

#if VCRTOS_CONFIG_THREAD_EVENT_ENABLE
template <> inline Instance &EventQueue::get(void) const
{
    return get_instance();
}

template <typename Type> inline Type &EventQueue::get(void) const
{
    return get_instance().get<Type>();
}
#endif

} // namespace vc
