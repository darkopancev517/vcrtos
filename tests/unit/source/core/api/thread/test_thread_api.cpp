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

#include <vcrtos/thread.h>

#include "core/thread.hpp"

#include "test-helper.h"

using namespace vc;

class TestThreadApi : public testing::Test
{
    protected:

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(TestThreadApi, basicThreadApiTest)
{
    ThreadScheduler *scheduler = &ThreadScheduler::init();

    EXPECT_NE(scheduler, nullptr);
    EXPECT_NE(scheduler, nullptr);
    EXPECT_EQ(scheduler->numof_threads(), 0);
    EXPECT_EQ(thread_current(), nullptr);
    EXPECT_EQ(thread_current_pid(), KERNEL_PID_UNDEF);

    EXPECT_EQ(thread_scheduler_is_initialized(), 1);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] make sure Thread class size is correct
     * -------------------------------------------------------------------------
     **/

    EXPECT_EQ(sizeof(Thread), sizeof(thread_t));

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] create single thread and run the thread scheduler, that
     * thread is expected to be in running state and become current active
     * thread
     * -------------------------------------------------------------------------
     **/

    char stack1[128];

    kernel_pid_t thread1_pid = thread_create(stack1, sizeof(stack1), nullptr, "thread1",
                                             KERNEL_THREAD_PRIORITY_MAIN,
                                             nullptr,
                                             THREAD_FLAGS_CREATE_WOUT_YIELD | THREAD_FLAGS_CREATE_STACKMARKER);

    EXPECT_NE(thread1_pid, KERNEL_PID_UNDEF);
    EXPECT_EQ(thread1_pid, 1);

    thread_t *thread1 = thread_get_from_scheduler(thread1_pid);

    EXPECT_NE(thread1, nullptr);
    EXPECT_EQ(thread1->pid, 1);
    EXPECT_EQ(thread1->priority, KERNEL_THREAD_PRIORITY_MAIN);
    EXPECT_EQ(thread1->name, "thread1");
    EXPECT_EQ(thread1->status, THREAD_STATUS_PENDING);

    EXPECT_EQ(scheduler->numof_threads(), 1);
    EXPECT_EQ(thread_get_from_scheduler(thread1->pid), thread1);
    EXPECT_EQ(thread_scheduler_requested_context_switch(), 0);
    EXPECT_EQ(thread_current(), nullptr);
    EXPECT_EQ(thread_current_pid(), KERNEL_PID_UNDEF);

    thread_scheduler_run();

    EXPECT_EQ(thread1->status, THREAD_STATUS_RUNNING);

    EXPECT_EQ(thread_current(), thread1);
    EXPECT_EQ(thread_current_pid(), thread1->pid);
    EXPECT_EQ(scheduler->numof_threads(), 1);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] create new thread and exit current active thread
     * -------------------------------------------------------------------------
     **/

    char stack2[128];

    kernel_pid_t thread2_pid = thread_create(stack2, sizeof(stack2), nullptr, "thread2",
                                             KERNEL_THREAD_PRIORITY_MAIN - 1,
                                             nullptr,
                                             THREAD_FLAGS_CREATE_WOUT_YIELD | THREAD_FLAGS_CREATE_STACKMARKER);

    EXPECT_NE(thread2_pid, KERNEL_PID_UNDEF);
    EXPECT_EQ(thread2_pid, 2);

    thread_t *thread2 = thread_get_from_scheduler(thread2_pid);

    EXPECT_NE(thread2, nullptr);
    EXPECT_EQ(thread2->pid, 2);
    EXPECT_EQ(thread2->priority, KERNEL_THREAD_PRIORITY_MAIN - 1);
    EXPECT_EQ(thread2->name, "thread2");
    EXPECT_EQ(thread2->status, THREAD_STATUS_PENDING);

    thread_scheduler_run();

    EXPECT_EQ(thread1->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->status, THREAD_STATUS_RUNNING);

    EXPECT_EQ(thread_current(), thread2);
    EXPECT_EQ(thread_current_pid(), thread2->pid);
    EXPECT_EQ(scheduler->numof_threads(), 2);

    // Exit current active thread

    thread_exit();
    cpu_switch_context_exit();

    EXPECT_EQ(thread1->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->status, THREAD_STATUS_STOPPED);

    thread_scheduler_run();

    EXPECT_EQ(thread_current(), thread1);
    EXPECT_EQ(thread_current_pid(), thread1->pid);
    EXPECT_EQ(scheduler->numof_threads(), 1);

    // Try to get removed thread from scheduler

    thread_t *thread = thread_get_from_scheduler(thread2->pid);

    EXPECT_EQ(thread, nullptr);
}

TEST_F(TestThreadApi, multipleThreadApiTest)
{
    ThreadScheduler *scheduler = &ThreadScheduler::init();

    EXPECT_NE(scheduler, nullptr);
    EXPECT_EQ(scheduler->numof_threads(), 0);
    EXPECT_EQ(thread_current(), nullptr);
    EXPECT_EQ(thread_current_pid(), KERNEL_PID_UNDEF);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] create multiple thread ("idle" and "main" thread) and make
     * sure the thread with higher priority will be in running state and the
     * thread with lower priority ("idle" thread) is in pending state
     * -------------------------------------------------------------------------
     **/

    char idle_stack[128];

    kernel_pid_t idle_thread_pid = thread_create(idle_stack, sizeof(idle_stack), nullptr, "idle",
                                             KERNEL_THREAD_PRIORITY_IDLE,
                                             nullptr,
                                             THREAD_FLAGS_CREATE_WOUT_YIELD | THREAD_FLAGS_CREATE_STACKMARKER);

    EXPECT_NE(idle_thread_pid, KERNEL_PID_UNDEF);
    EXPECT_EQ(idle_thread_pid, 1);

    thread_t *idle_thread = thread_get_from_scheduler(idle_thread_pid);

    EXPECT_NE(idle_thread, nullptr);
    EXPECT_EQ(idle_thread->pid, 1);
    EXPECT_EQ(idle_thread->priority, KERNEL_THREAD_PRIORITY_IDLE);
    EXPECT_EQ(idle_thread->name, "idle");
    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);

    char main_stack[128];

    kernel_pid_t main_thread_pid = thread_create(main_stack, sizeof(main_stack), nullptr, "main",
                                             KERNEL_THREAD_PRIORITY_MAIN,
                                             nullptr,
                                             THREAD_FLAGS_CREATE_WOUT_YIELD | THREAD_FLAGS_CREATE_STACKMARKER);

    EXPECT_NE(main_thread_pid, KERNEL_PID_UNDEF);
    EXPECT_EQ(main_thread_pid, 2);

    thread_t *main_thread = thread_get_from_scheduler(main_thread_pid);

    EXPECT_NE(main_thread, nullptr);
    EXPECT_EQ(main_thread->pid, 2);
    EXPECT_EQ(main_thread->priority, KERNEL_THREAD_PRIORITY_MAIN);
    EXPECT_EQ(main_thread->name, "main");
    EXPECT_EQ(main_thread->status, THREAD_STATUS_PENDING);

    thread_scheduler_run();

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_RUNNING);

    EXPECT_EQ(thread_current(), main_thread);
    EXPECT_EQ(thread_current_pid(), main_thread->pid);
    EXPECT_EQ(scheduler->numof_threads(), 2);

    // At this point "main" thread alread in running state as expected

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] set the higher priority thread ("main" thread) to blocked
     * state and lower priority thread ("idle" thread) should be in in running
     * state
     * -------------------------------------------------------------------------
     **/

    thread_scheduler_set_status(main_thread, THREAD_STATUS_MUTEX_BLOCKED);

    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(thread_current(), main_thread);

    // At this point "idle" thread in running state as expected after "main"
    // thread set to blocked state

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] create new thread with higher priority than main and idle
     * thread and yield immediately
     * -------------------------------------------------------------------------
     **/

    char stack1[128];

    kernel_pid_t thread1_pid = thread_create(stack1, sizeof(stack1), nullptr, "thread1",
                                             KERNEL_THREAD_PRIORITY_MAIN - 1,
                                             nullptr,
                                             THREAD_FLAGS_CREATE_STACKMARKER);

    EXPECT_NE(thread1_pid, KERNEL_PID_UNDEF);
    EXPECT_EQ(thread1_pid, 3);

    thread_t *thread1 = thread_get_from_scheduler(thread1_pid);

    // Create thread without 'THREAD_FLAGS_CREATE_WOUT_YIELD'

    EXPECT_NE(thread1, nullptr);
    EXPECT_EQ(thread1->pid, 3);
    EXPECT_EQ(thread1->priority, KERNEL_THREAD_PRIORITY_MAIN - 1);
    EXPECT_EQ(thread1->name, "thread1");
    EXPECT_EQ(thread1->status, THREAD_STATUS_PENDING);

    // At this point cpu should immediately yield the "thread1" by triggering
    // PendSV interrupt and context switch from Isr is not requested

    EXPECT_EQ(test_helper_is_pendsv_interrupt_triggered(), 1);
    EXPECT_EQ(thread_scheduler_requested_context_switch(), 0);

    thread_scheduler_run();

    // After run the scheduler current active thread is expected to be "thread1"

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RUNNING);

    EXPECT_EQ(thread_current(), thread1);
    EXPECT_EQ(thread_current_pid(), thread1->pid);
    EXPECT_EQ(scheduler->numof_threads(), 3);

    scheduler->yield();

    // "thread1" alread the highest priority, nothing to be change is expected

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RUNNING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] try to set current active thread to sleep and wakeup
     * -------------------------------------------------------------------------
     **/

    thread_sleep();

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_SLEEPING);

    // Note: at this point both main_thread and thread1 are in blocking
    // status, so the next expected thread to run is idle_thread because
    // idle_thread was in pending status

    thread_scheduler_run();

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_SLEEPING);

    EXPECT_EQ(thread_current(), idle_thread);
    EXPECT_EQ(thread_current_pid(), idle_thread->pid);
    EXPECT_EQ(scheduler->numof_threads(), 3);

    EXPECT_EQ(thread_wakeup(main_thread->pid), 0);

    // main_thread wasn't in sleeping state

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_SLEEPING);

    thread_scheduler_run();

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_SLEEPING);

    // Wake up thread1 which was on sleeping status

    EXPECT_EQ(thread_wakeup(thread1->pid), 1);

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_PENDING);

    thread_scheduler_run();

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RUNNING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] try to set sleep in Isr
     * -------------------------------------------------------------------------
     **/

    test_helper_set_cpu_in_isr(1);

    thread_sleep();

    // We can't sleep in ISR function, nothing is expected to happen

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RUNNING);

    test_helper_set_cpu_in_isr(0);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] try to context swithing to lower priority thread than current
     * running thread
     * -------------------------------------------------------------------------
     **/

    test_helper_reset_pendsv_trigger();

    EXPECT_EQ(test_helper_is_pendsv_interrupt_triggered(), 0);

    EXPECT_EQ(idle_thread->priority, KERNEL_THREAD_PRIORITY_IDLE);
    EXPECT_EQ(main_thread->priority, KERNEL_THREAD_PRIORITY_MAIN);
    EXPECT_EQ(thread1->priority, KERNEL_THREAD_PRIORITY_MAIN - 1);

    thread_scheduler_switch(main_thread->priority);

    // Note: because "main_thread" priority is lower than current running thread
    // and current running thread is still in running status, nothing should
    // happened

    EXPECT_EQ(test_helper_is_pendsv_interrupt_triggered(), 0);

    EXPECT_EQ(thread_scheduler_requested_context_switch(), 0);

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RUNNING);

    EXPECT_EQ(sched_active_thread, thread1);
    EXPECT_EQ(sched_active_pid, thread1->pid);
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

    thread_scheduler_set_status(thread1, THREAD_STATUS_RECEIVE_BLOCKED);

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RECEIVE_BLOCKED);

    thread_scheduler_run();

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RECEIVE_BLOCKED);

    EXPECT_EQ(sched_active_thread, idle_thread);
    EXPECT_EQ(sched_active_pid, idle_thread->pid);
    EXPECT_EQ(scheduler->numof_threads(), 3);

    // At this point idle thread is run as expected, because other
    // higher priority threads is in blocked state

    test_helper_set_cpu_in_isr(1);

    EXPECT_EQ(cpu_is_in_isr(), 1);

    thread_scheduler_switch(thread1->priority);

    EXPECT_EQ(test_helper_is_pendsv_interrupt_triggered(), 0);
    EXPECT_EQ(thread_scheduler_requested_context_switch(), 1);

    // Because it is in ISR at this point context switch is requested instead
    // of immediatelly yield to "thread1"

    // In real cpu implementation, before exiting the Isr function it will
    // check this flag and trigger the PendSV interrupt if context switch is
    // requested, therefore after exiting Isr function PendSV interrupt will be
    // triggered and run thread scheduler

    // This equal to cpu_end_of_isr();

    if (thread_scheduler_requested_context_switch())
        cpu_trigger_pendsv_interrupt();

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RECEIVE_BLOCKED);

    test_helper_set_cpu_in_isr(0);

    EXPECT_EQ(cpu_is_in_isr(), 0);

    thread_scheduler_run();

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RECEIVE_BLOCKED);

    // At this point the current active thread is still "idle_thread" because
    // "thread1" is still in receive blocked state even though it was try to
    // context switch to "thread1", now try set "thread1" to pending state and
    // context switch to "thread1" priority */

    thread_scheduler_set_status(thread1, THREAD_STATUS_PENDING);

    EXPECT_EQ(thread1->status, THREAD_STATUS_PENDING);

    thread_scheduler_switch(thread1->priority);

    EXPECT_EQ(test_helper_is_pendsv_interrupt_triggered(), 1);
    EXPECT_EQ(thread_scheduler_requested_context_switch(), 0);

    thread_scheduler_run();

    test_helper_reset_pendsv_trigger();

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RUNNING);

    // At this point it succesfully switched to "thread1"

    EXPECT_EQ(sched_active_thread, thread1);
    EXPECT_EQ(sched_active_pid, thread1->pid);
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

    kernel_pid_t thread2_pid = thread_create(&stack2[1], sizeof(stack2) - 4, nullptr, "thread2",
                                   KERNEL_THREAD_PRIORITY_MAIN - 2,
                                   nullptr,
                                   THREAD_FLAGS_CREATE_SLEEPING);

    EXPECT_NE(thread2_pid, KERNEL_PID_UNDEF);
    EXPECT_EQ(thread2_pid, 4);

    thread_t *thread2 = thread_get_from_scheduler(thread2_pid);

    EXPECT_NE(thread2, nullptr);

    EXPECT_EQ(thread2->pid, 4);
    EXPECT_EQ(thread2->priority, KERNEL_THREAD_PRIORITY_MAIN - 2);
    EXPECT_EQ(thread2->name, "thread2");
    EXPECT_EQ(thread2->status, THREAD_STATUS_SLEEPING);

    thread_scheduler_run();

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->status, THREAD_STATUS_SLEEPING);

    thread_wakeup(thread2->pid);

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->status, THREAD_STATUS_PENDING);

    thread_scheduler_run();

    EXPECT_EQ(idle_thread->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->status, THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->status, THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->status, THREAD_STATUS_RUNNING);
}
