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
class Mutex;
class Msg;

class Thread : public thread_t
{
    friend class ThreadScheduler;
    friend class Mutex;
    friend class Msg;

public:
    Thread();

    static Thread *init(char *allocated_stack,
                        int size,
                        thread_handler_func_t handler_func,
                        const char *name,
                        unsigned priority = KERNEL_THREAD_PRIORITY_MAIN,
                        void *arg = nullptr,
                        int flags = THREAD_FLAGS_CREATE_WOUT_YIELD | THREAD_FLAGS_CREATE_STACKMARKER);

    list_node_t *get_runqueue_entry() { return &runqueue_entry; }
    void add_to_list(List *list);
    static Thread *get_thread_pointer_from_list_member(List *list);
    static int is_pid_valid(kernel_pid_t pid);
    void init_msg_queue(Msg *msg, int num);
    int queued_msg(Msg *msg);
    int numof_msg_in_queue();
    int has_msg_queue();

    kernel_pid_t get_pid() { return pid; }
    unsigned get_priority() { return priority; }
    const char *get_name() { return name; }
    thread_status_t get_status() { return status; }
};

#if VCRTOS_CONFIG_THREAD_EVENT_ENABLE
class Event : public event_t
{
public:
    Event()
    {
        this->list_node.next = nullptr;
    }
};

class EventQueue : public event_queue_t
{
    friend class ThreadScheduler;

public:
    EventQueue()
    {
        this->event_list.next = nullptr;
    }

    void event_post(Event *event, Thread *thread);
    void event_cancel(Event *event);
    Event *event_get();
    Event *event_wait();
    static void event_release(Event *event);
    int event_pending();
    Event *event_peek();
};
#endif // #if VCRTOS_CONFIG_THREAD_EVENT_ENABLE

class ThreadScheduler
{
public:
    ThreadScheduler()
        : numof_threads_in_container(0)
        , context_switch_request(0)
        , current_active_thread(nullptr)
        , current_active_pid(KERNEL_PID_UNDEF)
        , runqueue_bitcache(0)
    {
        for (kernel_pid_t i = KERNEL_PID_FIRST; i <= KERNEL_PID_LAST; ++i)
        {
            this->threads_container[i] = nullptr;

            this->scheduler_stats[i].last_start = 0;
            this->scheduler_stats[i].schedules = 0;
            this->scheduler_stats[i].runtime_ticks = 0;
        }
        for (uint8_t prio = 0; prio < KERNEL_THREAD_PRIORITY_LEVELS; prio++)
        {
            this->scheduler_runqueue[prio].next = nullptr;
        }
    }

    static ThreadScheduler &init();
    static ThreadScheduler &get();
    static int is_initialized();

    Thread *get_thread_from_container(kernel_pid_t pid) { return threads_container[pid]; }
    void add_thread(Thread *thread, kernel_pid_t pid) { threads_container[pid] = thread; }
    void add_numof_threads() { numof_threads_in_container += 1; }
    int requested_context_switch() { return context_switch_request; }
    void request_context_switch() { context_switch_request = 1; }
    void set_context_switch_request(unsigned state) { context_switch_request = state; }
    int numof_threads() { return numof_threads_in_container; }
    void run();
    void set_thread_status(Thread *thread, thread_status_t status);
    void context_switch(uint8_t priority_to_switch);
    void sleep();
    int wakeup_thread(kernel_pid_t pid);
    void yield();
    void exit();
    static void yield_higher_priority_thread();
    static const char *thread_status_to_string(thread_status_t status);
#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
    void thread_flags_set(Thread *thread, thread_flags_t mask);
    thread_flags_t thread_flags_clear(thread_flags_t mask);
    thread_flags_t thread_flags_wait_any(thread_flags_t mask);
    thread_flags_t thread_flags_wait_all(thread_flags_t mask);
    thread_flags_t thread_flags_wait_one(thread_flags_t mask);
    int thread_flags_wake(Thread *thread);
#endif
    uint64_t get_thread_runtime_ticks(kernel_pid_t pid);
    uint32_t get_thread_schedules_stat(kernel_pid_t pid);

private:
    Thread *get_next_thread_from_runqueue();
    static unsigned bitarithm_lsb(unsigned v);
#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
    thread_flags_t thread_flags_clear_atomic(Thread *thread, thread_flags_t mask);
    void thread_flags_wait(thread_flags_t mask, Thread *thread, thread_status_t thread_status, unsigned irqstate);
    void thread_flags_wait_any_blocked(thread_flags_t mask);
#endif

    int numof_threads_in_container;
    unsigned int context_switch_request;
    Thread *threads_container[KERNEL_PID_LAST + 1];
    Thread *current_active_thread;
    kernel_pid_t current_active_pid;
    Clist scheduler_runqueue[VCRTOS_CONFIG_THREAD_PRIORITY_LEVELS];
    uint32_t runqueue_bitcache;
    scheduler_stat_t scheduler_stats[KERNEL_PID_LAST + 1];
};

} // namespace vc

#endif /* CORE_THREAD_HPP */
