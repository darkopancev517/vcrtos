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

#ifndef CORE_THREAD_HPP
#define CORE_THREAD_HPP

#include <vcrtos/config.h>
#include <vcrtos/thread.h>
#include <vcrtos/stat.h>
#include <vcrtos/cpu.h>

#if VCRTOS_CONFIG_THREAD_EVENT_ENABLE
#include <vcrtos/event.h>
#endif

#include "core/msg.hpp"
#include "core/cib.hpp"
#include "core/clist.hpp"

namespace vc {

class ThreadScheduler;
class Instance;

#if !VCRTOS_CONFIG_MULTIPLE_INSTANCE_ENABLE
extern uint64_t instance_raw[];
#endif

class Thread : public thread_t
{
public:
    static Thread *init(Instance &instances, char *stack, int size, unsigned priority, int flags,
                        thread_handler_func_t handler_func, void *arg, const char *name);

    kernel_pid_t get_pid(void) { return pid; }

    void set_pid(kernel_pid_t pids) { pid = pids; }

    thread_status_t get_status(void) { return status; }

    void set_status(thread_status_t new_status) { status = new_status; }

    uint8_t get_priority(void) { return priority; }

    void set_priority(uint8_t new_priority) { priority = new_priority; }

    list_node_t *get_runqueue_entry(void) { return &runqueue_entry; }

    const char *get_name(void) { return name; }

    void add_to_list(List *list);

    static Thread *get_thread_pointer_from_list_member(List *list);

    static int is_pid_valid(kernel_pid_t pid);

    void init_msg_queue(Msg *msg, int num);

    int queued_msg(Msg *msg);

    int get_numof_msg_in_queue(void);

    int has_msg_queue(void);

private:
    void init_runqueue_entry(void) { runqueue_entry.next = NULL; }

    void init_msg(void);

#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
    void init_flags(void);
#endif

    void set_stack_start(char *ptr) { stack_start = ptr; }

    void set_stack_size(int size) { stack_size = size; }

    void set_name(const char *new_name) { name = new_name; }

    void set_stack_pointer(char *ptr) { stack_pointer = ptr; }

    void stack_init(thread_handler_func_t func, void *arg, void *ptr, int size);

    template <typename Type> inline Type &get(void) const;

#if VCRTOS_CONFIG_MULTIPLE_INSTANCE_ENABLE
    Instance &get_instance(void) const { return *static_cast<Instance *>(instance); }
#else
    Instance &get_instance(void) const { return *reinterpret_cast<Instance *>(&instance_raw); }
#endif
};

#if VCRTOS_CONFIG_THREAD_EVENT_ENABLE
class Event : public event_t
{
public:
    Event(void)
    {
        list_node.next = NULL;
    }
};

class EventQueue : public event_queue_t
{
public:
    explicit EventQueue(Instance &instances)
    {
        event_list.next = NULL;
#if VCRTOS_CONFIG_MULTIPLE_INSTANCE_ENABLE
        instance = static_cast<void *>(&instances);
#else
        (void) instances;
#endif
    }

    void event_post(Event *event, Thread *thread);

    void event_cancel(Event *event);

    Event *event_get(void);

    Event *event_wait(void);

    static void event_release(Event *event);

    int event_pending(void);

    Event *event_peek(void);

private:
    template <typename Type> inline Type &get(void) const;

#if VCRTOS_CONFIG_MULTIPLE_INSTANCE_ENABLE
    Instance &get_instance(void) const { return *static_cast<Instance *>(instance); }
#else
    Instance &get_instance(void) const { return *reinterpret_cast<Instance *>(&instance_raw); }
#endif
};
#endif // #if VCRTOS_CONFIG_THREAD_EVENT_ENABLE

class ThreadScheduler : public Clist
{
public:
    ThreadScheduler(Instance &instances)
        : numof_threads_in_scheduler(0)
        , context_switch_request(0)
        , current_active_thread(NULL)
        , current_active_pid(KERNEL_PID_UNDEF)
        , runqueue_bitcache(0)
    {
        for (kernel_pid_t i = KERNEL_PID_FIRST; i <= KERNEL_PID_LAST; ++i)
        {
            scheduled_threads[i] = NULL;
        }

        instance = static_cast<void *>(&instances);
    }

    Thread *get_thread_from_scheduler(kernel_pid_t pid) { return scheduled_threads[pid]; }

    void set_thread_scheduler(Thread *thread, kernel_pid_t pid) { scheduled_threads[pid] = thread; }

    unsigned int is_context_switch_requested(void) { return context_switch_request; }

    void enable_context_switch_request(void) { context_switch_request = 1; }

    void disable_context_switch_request(void) { context_switch_request = 0; }

    int get_numof_threads_in_scheduler(void) { return numof_threads_in_scheduler; }

    void increment_numof_threads_in_scheduler(void) { numof_threads_in_scheduler++; }

    void decrement_numof_threads_in_scheduler(void) { numof_threads_in_scheduler--; }

    Thread *get_current_active_thread(void) { return current_active_thread; }

    void set_current_active_thread(Thread *thread) { current_active_thread = thread; }

    kernel_pid_t get_current_active_pid(void) { return current_active_pid; }

    void set_current_active_pid(kernel_pid_t pid) { current_active_pid = pid; }

    uint64_t get_thread_runtime_ticks(kernel_pid_t pid) { return scheduler_stats[pid].runtime_ticks; }

    unsigned get_thread_schedules_stat(kernel_pid_t pid) { return scheduler_stats[pid].schedules; }

    void run(void);

    void set_thread_status(Thread *thread, thread_status_t status);

    void context_switch(uint8_t priority_to_switch);

    void sleeping_current_thread(void);

    int wakeup_thread(kernel_pid_t pid);

    void yield(void);

    void exit_current_active_thread(void);

    static void yield_higher_priority_thread(void);

    static const char *thread_status_to_string(thread_status_t status);

#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
    void thread_flags_set(Thread *thread, thread_flags_t mask);

    thread_flags_t thread_flags_clear(thread_flags_t mask);

    thread_flags_t thread_flags_wait_any(thread_flags_t mask);

    thread_flags_t thread_flags_wait_all(thread_flags_t mask);

    thread_flags_t thread_flags_wait_one(thread_flags_t mask);

    int thread_flags_wake(Thread *thread);
#endif

private:
    uint32_t get_runqueue_bitcache(void) { return runqueue_bitcache; }

    void set_runqueue_bitcache(uint8_t priority) { runqueue_bitcache |= 1 << priority; }

    void reset_runqueue_bitcache(uint8_t priority) { runqueue_bitcache &= ~(1 << priority); }

    Thread *get_next_thread_from_runqueue(void);

    uint8_t get_lsb_index_from_runqueue(void);

    static unsigned bitarithm_lsb(unsigned v);

#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
    thread_flags_t thread_flags_clear_atomic(Thread *thread, thread_flags_t mask);

    void thread_flags_wait(thread_flags_t mask, Thread *thread, thread_status_t thread_status, unsigned irqstate);

    void thread_flags_wait_any_blocked(thread_flags_t mask);
#endif

    int numof_threads_in_scheduler;

    unsigned int context_switch_request;

    Thread *scheduled_threads[KERNEL_PID_LAST + 1];

    Thread *current_active_thread;

    kernel_pid_t current_active_pid;

    Clist scheduler_runqueue[VCRTOS_CONFIG_THREAD_PRIORITY_LEVELS];

    uint32_t runqueue_bitcache;

    scheduler_stat_t scheduler_stats[KERNEL_PID_LAST + 1];

    void *instance;
};

} // namespace vc

#endif /* CORE_THREAD_HPP */
