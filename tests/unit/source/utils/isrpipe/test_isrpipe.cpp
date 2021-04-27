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

#include "utils/isrpipe.hpp"

using namespace vc;
using namespace utils;

class TestUtilsTsrb : public testing::Test
{
protected:
    char buffer[8];

    Tsrb *tsrb;

    virtual void SetUp()
    {
        tsrb = new Tsrb(buffer, sizeof(buffer));
    }

    virtual void TearDown()
    {
        delete tsrb;
    }
};

class TestUtilsUartIsrpipe : public testing::Test
{
protected:
    UartIsrpipe *uart_isrpipe;

    virtual void SetUp()
    {
        uart_isrpipe = new UartIsrpipe();
    }

    virtual void TearDown()
    {
        delete uart_isrpipe;
    }
};

TEST_F(TestUtilsTsrb, tsrbConstructorTest)
{
    EXPECT_TRUE(tsrb);
}

TEST_F(TestUtilsUartIsrpipe, isrpipeConstructorTest)
{
    EXPECT_TRUE(uart_isrpipe);
}

TEST_F(TestUtilsTsrb, tsrbFunctionTest)
{
    EXPECT_EQ(tsrb->avail(), 0);
    EXPECT_TRUE(tsrb->is_empty());
    EXPECT_FALSE(tsrb->is_full());
    EXPECT_EQ(tsrb->free(), 8);

    tsrb->add_one(0x1);
    tsrb->add_one(0x2);
    tsrb->add_one(0x3);
    tsrb->add_one(0x4);

    EXPECT_EQ(tsrb->avail(), 4);
    EXPECT_FALSE(tsrb->is_empty());
    EXPECT_FALSE(tsrb->is_full());
    EXPECT_EQ(tsrb->free(), 4);

    tsrb->push(0x5);
    tsrb->push(0x6);
    tsrb->push(0x7);
    tsrb->push(0x8);

    EXPECT_EQ(tsrb->avail(), 8);
    EXPECT_FALSE(tsrb->is_empty());
    EXPECT_TRUE(tsrb->is_full());
    EXPECT_EQ(tsrb->free(), 0);

    EXPECT_EQ(tsrb->pop(), 0x1);
    EXPECT_EQ(tsrb->pop(), 0x2);
    EXPECT_EQ(tsrb->pop(), 0x3);
    EXPECT_EQ(tsrb->pop(), 0x4);

    EXPECT_EQ(tsrb->free(), 4);

    tsrb->drop(2);

    EXPECT_EQ(tsrb->free(), 6);
    EXPECT_EQ(tsrb->avail(), 2);

    EXPECT_EQ(tsrb->get_one(), 0x7);
    EXPECT_EQ(tsrb->get_one(), 0x8);

    EXPECT_EQ(tsrb->avail(), 0);
    EXPECT_TRUE(tsrb->is_empty());
    EXPECT_FALSE(tsrb->is_full());
    EXPECT_EQ(tsrb->free(), 8);

    char data[4] = {(char)0xfa, (char)0xfb, (char)0xfc, (char)0xfd};

    tsrb->add(data, sizeof(data));

    EXPECT_EQ(tsrb->avail(), 4);
    EXPECT_FALSE(tsrb->is_empty());
    EXPECT_FALSE(tsrb->is_full());
    EXPECT_EQ(tsrb->free(), 4);

    char result[4];

    tsrb->get(result, sizeof(result));

    EXPECT_EQ(tsrb->avail(), 0);
    EXPECT_TRUE(tsrb->is_empty());
    EXPECT_FALSE(tsrb->is_full());
    EXPECT_EQ(tsrb->free(), 8);

    EXPECT_EQ(result[0], (char)0xfa);
    EXPECT_EQ(result[1], (char)0xfb);
    EXPECT_EQ(result[2], (char)0xfc);
    EXPECT_EQ(result[3], (char)0xfd);
}

TEST_F(TestUtilsUartIsrpipe, uartIsrpipeFunctionsTest)
{
    ThreadScheduler *scheduler = &ThreadScheduler::init();

    EXPECT_NE(scheduler, nullptr);
    EXPECT_EQ(scheduler->numof_threads(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    char stack1[128];

    Thread *thread1 = Thread::init(stack1, sizeof(stack1), nullptr, "thread1", KERNEL_THREAD_PRIORITY_MAIN);

    EXPECT_NE(thread1, nullptr);
    EXPECT_EQ(thread1->get_pid(), 1);
    EXPECT_EQ(thread1->get_priority(), KERNEL_THREAD_PRIORITY_MAIN);
    EXPECT_EQ(thread1->get_name(), "thread1");
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    char stack2[128];

    Thread *thread2 = Thread::init(stack2, sizeof(stack2), nullptr, "thread2", KERNEL_THREAD_PRIORITY_MAIN - 1);

    EXPECT_NE(thread2, nullptr);
    EXPECT_EQ(thread2->get_pid(), 2);
    EXPECT_EQ(thread2->get_priority(), KERNEL_THREAD_PRIORITY_MAIN - 1);
    EXPECT_EQ(thread2->get_name(), "thread2");
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(scheduler->numof_threads(), 2);
    EXPECT_EQ(scheduler->get_thread_from_container(thread1->get_pid()), thread1);
    EXPECT_EQ(scheduler->get_thread_from_container(thread2->get_pid()), thread2);
    EXPECT_EQ(scheduler->requested_context_switch(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(sched_active_thread, thread2);
    EXPECT_EQ(sched_active_pid, thread2->get_pid());
    EXPECT_EQ(scheduler->numof_threads(), 2);

    uart_isrpipe->write_one(0x1);
    uart_isrpipe->write_one(0x2);
    uart_isrpipe->write_one(0x3);

    EXPECT_EQ(uart_isrpipe->get_tsrb().avail(), 3);

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    char data;

    uart_isrpipe->read(&data, 1);

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(data, 0x1);

    uart_isrpipe->read(&data, 1);

    EXPECT_EQ(data, 0x2);

    EXPECT_EQ(uart_isrpipe->get_tsrb().avail(), 1);

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    uart_isrpipe->read(&data, 1);

    EXPECT_EQ(data, 0x3);

    EXPECT_EQ(uart_isrpipe->get_tsrb().avail(), 0);

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    uart_isrpipe->read(&data, 1);

    /* Note: mutex was unlocked, set to locked for the first time and still
     * running current thread */

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    uart_isrpipe->read(&data, 1);

    /* Note: second time call read, mutex was locked and no data in buffer
     * it will set current thread to mutex blocked status*/

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    scheduler->run();

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_MUTEX_BLOCKED);

    uart_isrpipe->write_one(0xa);

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_PENDING);

    /* Note: we get new data in uart_isrpipe so set thread2 to pending */

    scheduler->run();

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(uart_isrpipe->get_tsrb().avail(), 1); /* one data available to read */

    uart_isrpipe->read(&data, 1);

    EXPECT_EQ(data, 0xa);

    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread2->get_status(), THREAD_STATUS_RUNNING);
}
