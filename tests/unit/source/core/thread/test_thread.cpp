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

#include "gtest/gtest.h"

#include "core/thread.hpp"

#include "test-helper.h"

using namespace vc;

class TestThread : public testing::Test
{
    protected:

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(TestThread, basicThreadTest)
{
    ThreadScheduler *scheduler = &ThreadScheduler::init();

    EXPECT_NE(scheduler, nullptr);
    EXPECT_NE(scheduler, nullptr);
    EXPECT_EQ(scheduler->numof_threads(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    EXPECT_EQ(ThreadScheduler::is_initialized(), 1);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] create single thread and run the thread scheduler, that
     * thread is expected to be in running state and become current active
     * thread
     * -------------------------------------------------------------------------
     **/

    char stack1[128];

    Thread *thread1 = Thread::init(stack1, sizeof(stack1), nullptr, "thread1");

    EXPECT_NE(thread1, nullptr);
    EXPECT_EQ(thread1->get_pid(), 1);
    EXPECT_EQ(thread1->get_priority(), KERNEL_THREAD_PRIORITY_MAIN);
    EXPECT_EQ(thread1->get_name(), "thread1");
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(scheduler->numof_threads(), 1);
    EXPECT_EQ(scheduler->get_thread_from_container(thread1->get_pid()), thread1);
    EXPECT_EQ(scheduler->requested_context_switch(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    scheduler->run();

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(sched_active_thread, thread1);
    EXPECT_EQ(sched_active_pid, thread1->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 1);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] create new thread and exit current active thread
     * -------------------------------------------------------------------------
     **/

    char stack2[128];

    Thread *thread2 = Thread::init(stack2, sizeof(stack2), nullptr, "thread2", KERNEL_THREAD_PRIORITY_MAIN - 1);

    EXPECT_NE(thread2, nullptr);
    EXPECT_EQ(thread2->get_pid(), 2);
    EXPECT_EQ(thread2->get_priority(), KERNEL_THREAD_PRIORITY_MAIN - 1);
    EXPECT_EQ(thread2->get_name(), "thread2");
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(sched_active_thread, thread2);
    EXPECT_EQ(sched_active_pid, thread2->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 2);

    // Exit current active thread

    scheduler->exit();
    cpu_switch_context_exit();

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_STOPPED);

    scheduler->run();

    EXPECT_EQ(sched_active_thread, thread1);
    EXPECT_EQ(sched_active_pid, thread1->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 1);

    // Try to get removed thread from scheduler

    Thread *thread = scheduler->get_thread_from_container(thread2->get_pid());

    EXPECT_EQ(thread, nullptr);
}

TEST_F(TestThread, multipleThreadTest)
{
    ThreadScheduler *scheduler = &ThreadScheduler::init();

    EXPECT_NE(scheduler, nullptr);
    EXPECT_EQ(scheduler->numof_threads(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] create multiple thread ("idle" and "main" thread) and make
     * sure the thread with higher priority will be in running state and the
     * thread with lower priority ("idle" thread) is in pending state
     * -------------------------------------------------------------------------
     **/

    char idle_stack[128];

    Thread *idle_thread = Thread::init(idle_stack, sizeof(idle_stack), nullptr, "idle", KERNEL_THREAD_PRIORITY_IDLE);

    EXPECT_NE(idle_thread, nullptr);
    EXPECT_EQ(idle_thread->get_pid(), 1);
    EXPECT_EQ(idle_thread->get_priority(), KERNEL_THREAD_PRIORITY_IDLE);
    EXPECT_EQ(idle_thread->get_name(), "idle");
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    char main_stack[128];

    Thread *main_thread = Thread::init(main_stack, sizeof(main_stack), nullptr, "main");

    EXPECT_NE(main_thread, nullptr);
    EXPECT_EQ(main_thread->get_pid(), 2);
    EXPECT_EQ(main_thread->get_priority(), KERNEL_THREAD_PRIORITY_MAIN);
    EXPECT_EQ(main_thread->get_name(), "main");
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(scheduler->numof_threads(), 2);
    EXPECT_EQ(scheduler->get_thread_from_container(idle_thread->get_pid()), idle_thread);
    EXPECT_EQ(scheduler->get_thread_from_container(main_thread->get_pid()), main_thread);
    EXPECT_EQ(scheduler->requested_context_switch(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(sched_active_thread, main_thread);
    EXPECT_EQ(sched_active_pid, main_thread->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 2);

    // At this point "main" thread alread in running state as expected

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] set the higher priority thread ("main" thread) to blocked
     * state and lower priority thread ("idle" thread) should be in in running
     * state
     * -------------------------------------------------------------------------
     **/

    scheduler->set_thread_status(main_thread, THREAD_STATUS_MUTEX_BLOCKED);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(sched_active_thread, main_thread);

    // At this point "idle" thread in running state as expected after "main"
    // thread set to blocked state

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] create new thread with higher priority than main and idle
     * thread and yield immediately
     * -------------------------------------------------------------------------
     **/

    char stack1[128];

    Thread *thread1 = Thread::init(stack1, sizeof(stack1), nullptr, "thread1", KERNEL_THREAD_PRIORITY_MAIN - 1, nullptr, THREAD_FLAGS_CREATE_STACKMARKER);

    // Create thread without 'THREAD_FLAGS_CREATE_WOUT_YIELD'

    EXPECT_NE(thread1, nullptr);
    EXPECT_EQ(thread1->get_pid(), 3);
    EXPECT_EQ(thread1->get_priority(), KERNEL_THREAD_PRIORITY_MAIN - 1);
    EXPECT_EQ(thread1->get_name(), "thread1");
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    // At this point cpu should immediately yield the "thread1" by triggering
    // PendSV interrupt and context switch from Isr is not requested

    EXPECT_EQ(test_helper_is_pendsv_interrupt_triggered(), 1);
    EXPECT_EQ(scheduler->requested_context_switch(), 0);

    scheduler->run();

    // After run the scheduler current active thread is expected to be "thread1"

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(sched_active_thread, thread1);
    EXPECT_EQ(sched_active_pid, thread1->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 3);

    scheduler->yield();

    // "thread1" alread the highest priority, nothing to be change is expected

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] try to set current active thread to sleep and wakeup
     * -------------------------------------------------------------------------
     **/

    scheduler->sleep();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    // Note: at this point both main_thread and thread1 are in blocking
    // status, so the next expected thread to run is idle_thread because
    // idle_thread was in pending status

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    EXPECT_EQ(sched_active_thread, idle_thread);
    EXPECT_EQ(sched_active_pid, idle_thread->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 3);

    EXPECT_EQ(scheduler->wakeup_thread(main_thread->get_pid()), 0);

    // main_thread wasn't in sleeping state

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    // Wake up thread1 which was on sleeping status

    EXPECT_EQ(scheduler->wakeup_thread(thread1->get_pid()), 1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] try to set sleep in Isr
     * -------------------------------------------------------------------------
     **/

    test_helper_set_cpu_in_isr(1);

    scheduler->sleep();

    // We can't sleep in ISR function, nothing is expected to happen

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    test_helper_set_cpu_in_isr(0);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] try to context swithing to lower priority thread than current
     * running thread
     * -------------------------------------------------------------------------
     **/

    test_helper_reset_pendsv_trigger();

    EXPECT_EQ(test_helper_is_pendsv_interrupt_triggered(), 0);

    EXPECT_EQ(idle_thread->get_priority(), KERNEL_THREAD_PRIORITY_IDLE);
    EXPECT_EQ(main_thread->get_priority(), KERNEL_THREAD_PRIORITY_MAIN);
    EXPECT_EQ(thread1->get_priority(), KERNEL_THREAD_PRIORITY_MAIN - 1);

    scheduler->context_switch(main_thread->get_priority());

    // Note: because "main_thread" priority is lower than current running thread
    // and current running thread is still in running status, nothing should
    // happened

    EXPECT_EQ(test_helper_is_pendsv_interrupt_triggered(), 0);

    EXPECT_EQ(scheduler->requested_context_switch(), 0);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(sched_active_thread, thread1);
    EXPECT_EQ(sched_active_pid, thread1->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 3);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] request context swicth inside Isr (Interrupt Service Routine)
     * and current running thread is not in running status, it expected to see
     * context switch is requested instead of yielding immediately to the next
     * thread
     * -------------------------------------------------------------------------
     **/

    // Set "thread1" at blocked state first and is expected to be the next
    // thread to run is "idle" thread

    scheduler->set_thread_status(thread1, THREAD_STATUS_RECEIVE_BLOCKED);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RECEIVE_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RECEIVE_BLOCKED);

    EXPECT_EQ(sched_active_thread, idle_thread);
    EXPECT_EQ(sched_active_pid, idle_thread->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 3);

    // At this point idle thread is run as expected, because other
    // higher priority threads is in blocked state

    test_helper_set_cpu_in_isr(1);

    EXPECT_EQ(cpu_is_in_isr(), 1);

    scheduler->context_switch(thread1->get_priority());

    EXPECT_EQ(test_helper_is_pendsv_interrupt_triggered(), 0);
    EXPECT_EQ(scheduler->requested_context_switch(), 1);

    // Because it is in ISR at this point context switch is requested instead
    // of immediatelly yield to "thread1"

    // In real cpu implementation, before exiting the Isr function it will
    // check this flag and trigger the PendSV interrupt if context switch is
    // requested, therefore after exiting Isr function PendSV interrupt will be
    // triggered and run thread scheduler

    // This equal to cpu_end_of_isr();

    if (scheduler->requested_context_switch())
        cpu_trigger_pendsv_interrupt();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RECEIVE_BLOCKED);

    test_helper_set_cpu_in_isr(0);

    EXPECT_EQ(cpu_is_in_isr(), 0);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RECEIVE_BLOCKED);

    // At this point the current active thread is still "idle_thread" because
    // "thread1" is still in receive blocked state even though it was try to
    // context switch to "thread1", now try set "thread1" to pending state and
    // context switch to "thread1" priority */

    scheduler->set_thread_status(thread1, THREAD_STATUS_PENDING);

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    scheduler->context_switch(thread1->get_priority());

    EXPECT_EQ(test_helper_is_pendsv_interrupt_triggered(), 1);
    EXPECT_EQ(scheduler->requested_context_switch(), 0);

    scheduler->run();

    test_helper_reset_pendsv_trigger();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    // At this point it succesfully switched to "thread1"

    EXPECT_EQ(sched_active_thread, thread1);
    EXPECT_EQ(sched_active_pid, thread1->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 3);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] create a thread with highest priority but with THREAD_FLAGS
     * sleeping and not using THREAD_FLAGS create stack marker.
     * -------------------------------------------------------------------------
     **/

    char stack2[128];

    // Intentionally create thread with misaligment stack boundary on
    // 16/32 bit boundary (&stack2[1]) will do the job

    Thread *thread2 = Thread::init(&stack2[1], sizeof(stack2) - 4, nullptr, "thread2",
                                   KERNEL_THREAD_PRIORITY_MAIN - 2,
                                   nullptr, THREAD_FLAGS_CREATE_SLEEPING);

    EXPECT_NE(thread2, nullptr);

    EXPECT_EQ(thread2->get_pid(), 4);
    EXPECT_EQ(thread2->get_priority(), KERNEL_THREAD_PRIORITY_MAIN - 2);
    EXPECT_EQ(thread2->get_name(), "thread2");
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_SLEEPING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_SLEEPING);

    scheduler->wakeup_thread(thread2->get_pid());

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);
}

TEST_F(TestThread, threadFlagsTest)
{
    ThreadScheduler *scheduler = &ThreadScheduler::init();

    EXPECT_NE(scheduler, nullptr);
    EXPECT_EQ(scheduler->numof_threads(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    char idle_stack[128];

    Thread *idle_thread = Thread::init(idle_stack, sizeof(idle_stack), nullptr, "idle", KERNEL_THREAD_PRIORITY_IDLE);

    EXPECT_NE(idle_thread, nullptr);
    EXPECT_EQ(idle_thread->get_pid(), 1);
    EXPECT_EQ(idle_thread->get_priority(), KERNEL_THREAD_PRIORITY_IDLE);
    EXPECT_EQ(idle_thread->get_name(), "idle");
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    char main_stack[128];

    Thread *main_thread = Thread::init(main_stack, sizeof(main_stack), nullptr, "main");

    EXPECT_NE(main_thread, nullptr);
    EXPECT_EQ(main_thread->get_pid(), 2);
    EXPECT_EQ(main_thread->get_priority(), KERNEL_THREAD_PRIORITY_MAIN);
    EXPECT_EQ(main_thread->get_name(), "main");
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(scheduler->numof_threads(), 2);
    EXPECT_EQ(scheduler->get_thread_from_container(idle_thread->get_pid()), idle_thread);
    EXPECT_EQ(scheduler->get_thread_from_container(main_thread->get_pid()), main_thread);
    EXPECT_EQ(scheduler->requested_context_switch(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(sched_active_thread, main_thread);
    EXPECT_EQ(sched_active_pid, main_thread->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 2);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] thread flags wait any
     * -------------------------------------------------------------------------
     **/

    scheduler->thread_flags_wait_any(0xf);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(main_thread->flags, 0);
    EXPECT_EQ(main_thread->waited_flags, 0xf);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->thread_flags_set(main_thread, 0x1);

    // any flag set to thread flags will immediately change the thread status to
    // pending status.

    EXPECT_EQ(main_thread->flags, 0x1);
    EXPECT_EQ(main_thread->waited_flags, 0xf);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->thread_flags_clear(0xf);

    // at this point we already clear thread->flags so calling
    // thread_flags_wait_any will make the current thread in FLAG_BLOCKED_ANY
    // status.

    EXPECT_EQ(main_thread->flags, 0);
    EXPECT_EQ(main_thread->waited_flags, 0xf);

    scheduler->thread_flags_wait_any(0xf);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(main_thread->flags, 0);
    EXPECT_EQ(main_thread->waited_flags, 0xf);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->thread_flags_set(main_thread, 0x3);

    // now we use different flag to unlocked main_thread, any flag will unlocked
    // main_thread.

    EXPECT_EQ(main_thread->flags, 0x3);
    EXPECT_EQ(main_thread->waited_flags, 0xf);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(main_thread->flags, 0x3);
    EXPECT_EQ(main_thread->waited_flags, 0xf);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] thread flags wait all
     * -------------------------------------------------------------------------
     **/

    scheduler->thread_flags_wait_all(0xff);

    // note: wait all will clear the previous main_thread flags.

    EXPECT_EQ(main_thread->flags, 0);
    EXPECT_EQ(main_thread->waited_flags, 0xff);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ALL);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ALL);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->thread_flags_set(main_thread, 0x1);
    scheduler->thread_flags_set(main_thread, 0x2);
    scheduler->thread_flags_set(main_thread, 0x4);
    scheduler->thread_flags_set(main_thread, 0x8);

    // the thread should remain in FLAG_BLOCKED_ALL status until all flags is
    // filled.

    EXPECT_EQ(main_thread->flags, 0xf);
    EXPECT_EQ(main_thread->waited_flags, 0xff);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ALL);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->thread_flags_set(main_thread, 0x10);
    scheduler->thread_flags_set(main_thread, 0x20);
    scheduler->thread_flags_set(main_thread, 0x40);
    scheduler->thread_flags_set(main_thread, 0x80);

    // at this point all flags is filled.

    EXPECT_EQ(main_thread->flags, 0xff);
    EXPECT_EQ(main_thread->waited_flags, 0xff);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->thread_flags_clear(0xf);

    // clear half of the flags in main_thread->flags.

    EXPECT_EQ(main_thread->flags, 0xf0);
    EXPECT_EQ(main_thread->waited_flags, 0xff);

    scheduler->thread_flags_wait_all(0xff);

    // this function will clear all the remain flags in main_thread->flags to
    // wait all flags to be received again and set to FLAG_BLOCKED_ALL status.

    EXPECT_EQ(main_thread->flags, 0);
    EXPECT_EQ(main_thread->waited_flags, 0xff);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ALL);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ALL);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->thread_flags_set(main_thread, 0x1);
    scheduler->thread_flags_set(main_thread, 0x2);
    scheduler->thread_flags_set(main_thread, 0x4);
    scheduler->thread_flags_set(main_thread, 0x8);
    scheduler->thread_flags_set(main_thread, 0x10);
    scheduler->thread_flags_set(main_thread, 0x20);
    scheduler->thread_flags_set(main_thread, 0x40);
    scheduler->thread_flags_set(main_thread, 0x80);

    EXPECT_EQ(main_thread->flags, 0xff);
    EXPECT_EQ(main_thread->waited_flags, 0xff);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->run();

    EXPECT_EQ(main_thread->flags, 0xff);
    EXPECT_EQ(main_thread->waited_flags, 0xff);

    scheduler->thread_flags_wait_all(0xff); // this will take no effect.

    // Note: previous main_thread->flags is 0xff and calling this function again
    // will assume that the flags has been filled and clear the
    // main_thread->flags and will not blocked main_thread. to make
    // thread_flags_wait_all immediately taking effect we must clear the
    // main_thread->flags first.

    EXPECT_EQ(main_thread->flags, 0x0);
    EXPECT_EQ(main_thread->waited_flags, 0xff);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    // main_thread->flags is cleared but main_thread is still in running status.

    /**
     * ------------------------------------------------------------------------------
     * [TEST CASE] thread flags wait one
     * ------------------------------------------------------------------------------
     **/

    EXPECT_EQ(main_thread->flags, 0x0);
    EXPECT_EQ(main_thread->waited_flags, 0xff);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->thread_flags_wait_one(0x3);

    EXPECT_EQ(main_thread->flags, 0x0);
    EXPECT_EQ(main_thread->waited_flags, 0x3);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->thread_flags_set(main_thread, 0x10);

    // Note: we set the flag that is not being wait.

    EXPECT_EQ(main_thread->flags, 0x10);
    EXPECT_EQ(main_thread->waited_flags, 0x3);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->thread_flags_set(main_thread, 0x1);

    EXPECT_EQ(main_thread->flags, 0x11);
    EXPECT_EQ(main_thread->waited_flags, 0x3);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->run();

    EXPECT_EQ(main_thread->flags, 0x11);
    EXPECT_EQ(main_thread->waited_flags, 0x3);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->thread_flags_wait_one(0x3);

    // previous flags is not being cleared, calling this function will assume
    // the flags has been filled and clear the flags corresponding to waited_one 
    // mask.

    EXPECT_EQ(main_thread->flags, 0x10);
    EXPECT_EQ(main_thread->waited_flags, 0x3);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->thread_flags_wait_one(0x3);

    // now it will blocked main_thread.

    EXPECT_EQ(main_thread->flags, 0x10);
    EXPECT_EQ(main_thread->waited_flags, 0x3);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->thread_flags_set(main_thread, 0x2);

    EXPECT_EQ(main_thread->flags, 0x12);
    EXPECT_EQ(main_thread->waited_flags, 0x3);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(main_thread->flags, 0x12);
    EXPECT_EQ(main_thread->waited_flags, 0x3);

    scheduler->thread_flags_clear(0xff);

    // clear main_thread (current thread) flags.

    EXPECT_EQ(main_thread->flags, 0);
    EXPECT_EQ(main_thread->waited_flags, 0x3);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
}

TEST_F(TestThread, threadEventTest)
{
    ThreadScheduler *scheduler = &ThreadScheduler::init();

    EXPECT_NE(scheduler, nullptr);
    EXPECT_EQ(scheduler->numof_threads(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    char idle_stack[128];

    Thread *idle_thread = Thread::init(idle_stack, sizeof(idle_stack), nullptr, "idle", KERNEL_THREAD_PRIORITY_IDLE);

    EXPECT_NE(idle_thread, nullptr);
    EXPECT_EQ(idle_thread->get_pid(), 1);
    EXPECT_EQ(idle_thread->get_priority(), KERNEL_THREAD_PRIORITY_IDLE);
    EXPECT_EQ(idle_thread->get_name(), "idle");
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    char main_stack[128];

    Thread *main_thread = Thread::init(main_stack, sizeof(main_stack), nullptr, "main");

    EXPECT_NE(main_thread, nullptr);
    EXPECT_EQ(main_thread->get_pid(), 2);
    EXPECT_EQ(main_thread->get_priority(), KERNEL_THREAD_PRIORITY_MAIN);
    EXPECT_EQ(main_thread->get_name(), "main");
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(scheduler->numof_threads(), 2);
    EXPECT_EQ(scheduler->get_thread_from_container(idle_thread->get_pid()), idle_thread);
    EXPECT_EQ(scheduler->get_thread_from_container(main_thread->get_pid()), main_thread);
    EXPECT_EQ(scheduler->requested_context_switch(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(sched_active_thread, main_thread);
    EXPECT_EQ(sched_active_pid, main_thread->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 2);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] create thread event
     * -------------------------------------------------------------------------
     **/

    char event_stack[128];

    Thread *event_thread = Thread::init(event_stack, sizeof(event_stack), nullptr, "event",
                                    KERNEL_THREAD_PRIORITY_MAIN - 1);

    EXPECT_NE(event_thread, nullptr);
    EXPECT_EQ(event_thread->get_pid(), 3);
    EXPECT_EQ(event_thread->get_priority(), KERNEL_THREAD_PRIORITY_MAIN - 1);
    EXPECT_EQ(event_thread->get_name(), "event");
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(scheduler->numof_threads(), 3);
    EXPECT_EQ(scheduler->get_thread_from_container(idle_thread->get_pid()), idle_thread);
    EXPECT_EQ(scheduler->get_thread_from_container(main_thread->get_pid()), main_thread);
    EXPECT_EQ(scheduler->get_thread_from_container(event_thread->get_pid()), event_thread);
    EXPECT_EQ(scheduler->requested_context_switch(), 0);
    EXPECT_EQ(sched_active_thread, main_thread);
    EXPECT_EQ(sched_active_pid, main_thread->get_pid());

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(sched_active_thread, event_thread);
    EXPECT_EQ(sched_active_pid, event_thread->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 3);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] wait event in event_thread
     * -------------------------------------------------------------------------
     **/

    EXPECT_EQ(sizeof(EventQueue), sizeof(event_queue_t));

    // create event queue
    EventQueue queue = EventQueue();

    EXPECT_EQ(queue.event_wait(), nullptr); // nothing in the queue, event_thread will in blocked status

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    // Note: now event_thread is already in FLAG_BLOCKED_ANY status to wait any
    // event to be received.

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] post event to event_thread
     * -------------------------------------------------------------------------
     **/

    Event event1 = Event();

    queue.event_post(&event1, event_thread);
    queue.event_post(&event1, event_thread);
    queue.event_post(&event1, event_thread);
    queue.event_post(&event1, event_thread);

    // Note: [IMPORTANT] posting same event multiple times will not increase event_queue.

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_RUNNING);

    // Note: at this point event is succesfully received and event_thread now
    // alread in running status.

    EXPECT_EQ(queue.event_wait(), &event1);
    queue.event_release(&event1);

    // Note: [IMPORTANT] event_wait() need to release the event manually before
    // it can be reuse, and make sure the event is not in the queue when you
    // release it.

    EXPECT_EQ(queue.event_wait(), nullptr); // nothing on the queue.

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(queue.event_wait(), nullptr);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    // Note: calling event_wait at line 1351 not immediately set event_thread to
    // blocked status because the thread flags was not cleared before, in real
    // device we will do while loop if event_wait result is NULL to make sure
    // the thread go into blocked status.

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    queue.event_post(&event1, event_thread);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(queue.event_wait(), &event1);

    queue.event_release(&event1); // event1 no longer in the queue, so release it for later use.

    EXPECT_NE(event_thread->flags, 0);

    // Note: calling first event_wait will not clear thread->flags.

    EXPECT_EQ(queue.event_wait(), nullptr);

    // Note: no more event in the queue, this time it will clear the flags, but
    // the event_thread is still in RUNNING status.

    EXPECT_EQ(event_thread->flags, 0);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(queue.event_wait(), nullptr);

    // Note: at this time, no event on the queue, the flags is clear, it will
    // set event thread status to FLAG_BLOCKED_ANY.

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    queue.event_post(&event1, event_thread);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(queue.event_wait(), &event1);

    queue.event_release(&event1);

    EXPECT_NE(event_thread->flags, 0);

    // Note: try to clear event thread flags first before calling event_loop().

    scheduler->thread_flags_clear(0xffff);

    EXPECT_EQ(event_thread->flags, 0);

    EXPECT_EQ(queue.event_wait(), nullptr);

    // Note: calling event_wait() now will effective immediately because
    // THREAD_FLAG_EVENT was cleared and no event in the event_queue.

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    queue.event_post(&event1, event_thread);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_RUNNING);

    event_t *ev = queue.event_get();

    // Note: event_get() will automatically release the event, so no need to
    // call event_release().

    EXPECT_NE(ev, nullptr);
    EXPECT_EQ(ev, &event1);

    EXPECT_EQ(queue.event_wait(), nullptr);
    EXPECT_EQ(queue.event_wait(), nullptr);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] post multiple different events
     * -------------------------------------------------------------------------
     **/

    Event event2 = Event();
    Event event3 = Event();
    Event event4 = Event();
    Event event5 = Event();

    queue.event_post(&event1, event_thread);
    queue.event_post(&event2, event_thread);
    queue.event_post(&event3, event_thread);
    queue.event_post(&event4, event_thread);
    queue.event_post(&event5, event_thread);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_PENDING);

    // cancel event 3.

    queue.event_cancel(&event3);

    // Note: event_cancel() will automatically release the the event.
    
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_RUNNING);

    ev = queue.event_get(); // get event1
    EXPECT_NE(ev, nullptr);
    EXPECT_EQ(ev, &event1);

    ev = queue.event_get(); // get event2
    EXPECT_NE(ev, nullptr);
    EXPECT_EQ(ev, &event2);

    ev = queue.event_get(); // get event4, event3 was canceled
    EXPECT_NE(ev, nullptr);
    EXPECT_EQ(ev, &event4);

    EXPECT_EQ(queue.event_wait(), &event5);
    queue.event_release(&event5);

    ev = queue.event_get(); // no event available

    EXPECT_EQ(ev, nullptr);

    scheduler->thread_flags_clear(0xffff);

    EXPECT_EQ(queue.event_wait(), nullptr);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] cancel event and post it again
     * -------------------------------------------------------------------------
     **/

    queue.event_post(&event3, event_thread);
    queue.event_post(&event4, event_thread);
    queue.event_post(&event5, event_thread);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_PENDING);

    queue.event_cancel(&event3);

    queue.event_post(&event3, event_thread);

    // Note: after canceled and repost again, event3 position will be at the end of the queue.

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(queue.event_wait(), &event4); // this will get event4
    queue.event_release(&event4);

    EXPECT_EQ(queue.event_wait(), &event5); // this will get event5
    queue.event_release(&event5);

    EXPECT_EQ(queue.event_wait(), &event3); // this will get event3
    queue.event_release(&event3);

    ev = queue.event_get(); // no event left

    EXPECT_EQ(ev, nullptr);

    EXPECT_EQ(queue.event_wait(), nullptr);
    EXPECT_EQ(queue.event_wait(), nullptr);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    typedef struct
    {
        event_t super;
        uint32_t data;
    } custom_event_t;

    custom_event_t custom_ev;

    custom_ev.super.list_node.next = NULL;
    custom_ev.data = 0xdeadbeef;

    queue.event_post(reinterpret_cast<Event *>(&custom_ev), event_thread);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_RUNNING);

    custom_event_t *event = (custom_event_t *)queue.event_wait();
    queue.event_release(reinterpret_cast<Event *>(event));

    EXPECT_NE(event, nullptr);

    EXPECT_EQ(event->data, 0xdeadbeef);

    EXPECT_EQ(queue.event_wait(), nullptr);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(queue.event_wait(), nullptr);

    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(event_thread->get_status(), THREAD_STATUS_FLAG_BLOCKED_ANY);

    struct process {
        thread_t super;
    };

    EXPECT_EQ(sizeof(struct process), sizeof(thread_t));
}
