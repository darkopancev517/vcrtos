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
#include <vcrtos/mutex.h>

#include "core/thread.hpp"

#include "test-helper.h"

using namespace vc;

class TestMutexApi : public testing::Test
{
    protected:

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(TestMutexApi, mutexApiFunctionTest)
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

    char stack1[128];

    Thread *thread1 = Thread::init(stack1, sizeof(stack1), nullptr, "thread1", KERNEL_THREAD_PRIORITY_MAIN - 1);

    EXPECT_NE(thread1, nullptr);
    EXPECT_EQ(thread1->get_pid(), 3);
    EXPECT_EQ(thread1->get_priority(), KERNEL_THREAD_PRIORITY_MAIN - 1);
    EXPECT_EQ(thread1->get_name(), "thread1");
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    char stack2[128];

    Thread *thread2 = Thread::init(stack2, sizeof(stack2), nullptr, "thread2", KERNEL_THREAD_PRIORITY_MAIN - 1);

    EXPECT_NE(thread2, nullptr);
    EXPECT_EQ(thread2->get_pid(), 4);
    EXPECT_EQ(thread2->get_priority(), KERNEL_THREAD_PRIORITY_MAIN - 1);
    EXPECT_EQ(thread2->get_name(), "thread2");
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(scheduler->numof_threads(), 4);
    EXPECT_EQ(scheduler->get_thread_from_container(idle_thread->get_pid()), idle_thread);
    EXPECT_EQ(scheduler->get_thread_from_container(main_thread->get_pid()), main_thread);
    EXPECT_EQ(scheduler->get_thread_from_container(thread1->get_pid()), thread1);
    EXPECT_EQ(scheduler->get_thread_from_container(thread2->get_pid()), thread2);
    EXPECT_EQ(scheduler->requested_context_switch(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();
    scheduler->run();
    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] single mutex
     * -------------------------------------------------------------------------
     **/

    mutex_t mutex;

    mutex_init_unlocked(&mutex);

    mutex_lock(&mutex);

    // Mutex was unlocked when set lock for the first time, therefore current
    // thread (thread1) expected still running state

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    mutex_try_lock(&mutex);
    mutex_try_lock(&mutex);
    mutex_try_lock(&mutex);
    mutex_try_lock(&mutex);
    mutex_try_lock(&mutex);
    mutex_try_lock(&mutex);

    // Mutex was set to lock, nothing will happen when calling try_lock()

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    mutex_lock(&mutex);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    mutex_lock(&mutex); // this will blocked thread2

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    mutex_lock(&mutex); // this will blocked main_thread

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    mutex_unlock(&mutex); // this expected to unlock thread1

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    mutex_unlock(&mutex); // this will unlock thread2
    mutex_unlock(&mutex); // this will unlock main_thread

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] multiple mutexes with LOCKED initial value
     * -------------------------------------------------------------------------
     **/

    mutex_t mutex1;
    mutex_t mutex2;
    mutex_t mutex3;

    mutex_init(&mutex1);
    mutex_init(&mutex2);
    mutex_init(&mutex3);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    mutex_lock(&mutex1); // mutex1 locked thread1

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    mutex_lock(&mutex2);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    mutex_lock(&mutex3);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    mutex_unlock(&mutex2); // this will unlocked thread2

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    mutex_unlock(&mutex3);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    mutex_unlock(&mutex1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] unlock mutex in ISR
     * -------------------------------------------------------------------------
     **/

    mutex_lock(&mutex3);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    mutex_unlock(&mutex3);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    mutex_lock(&mutex1); // this will lock thread1

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    mutex_lock(&mutex2); // this will lock thread2

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    mutex_lock(&mutex3); // this will lock main_thread

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    test_helper_set_cpu_in_isr(1);
    EXPECT_EQ(cpu_is_in_isr(), 1);

    mutex_unlock(&mutex3);

    test_helper_set_cpu_in_isr(0);
    EXPECT_EQ(cpu_is_in_isr(), 0);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    mutex_unlock(&mutex1);
    mutex_unlock(&mutex2);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    mutex_lock(&mutex1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_MUTEX_BLOCKED);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    mutex_unlock_and_sleep(&mutex1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_SLEEPING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_SLEEPING);

    mutex_unlock(&mutex1);
    mutex_unlock(&mutex1);
    mutex_unlock(&mutex1);
    mutex_unlock(&mutex1);
    mutex_unlock(&mutex1);
    mutex_unlock(&mutex1);
    mutex_unlock(&mutex1);
    mutex_unlock(&mutex1);

    // mutex1 wasn't locked so nothing happen

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_SLEEPING);
}
