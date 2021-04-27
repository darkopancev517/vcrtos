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

#include "core/code_utils.h"
#include "core/thread.hpp"
#include "core/msg.hpp"

#include "test-helper.h"

using namespace vc;

class TestMsg : public testing::Test
{
    protected:

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(TestMsg, singleMsgTest)
{
    ThreadScheduler *scheduler = &ThreadScheduler::init();

    EXPECT_NE(scheduler, nullptr);
    EXPECT_EQ(scheduler->numof_threads(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    Msg main_thread_msgqueue[4];
    Msg thread1_msgqueue[4];

    char idle_stack[128];

    Thread *idle_thread = Thread::init(idle_stack, sizeof(idle_stack), nullptr, "idle", KERNEL_THREAD_PRIORITY_IDLE);

    EXPECT_EQ(idle_thread->has_msg_queue(), 0);
    EXPECT_EQ(idle_thread->numof_msg_in_queue(), -1);

    EXPECT_NE(idle_thread, nullptr);
    EXPECT_EQ(idle_thread->get_pid(), 1);
    EXPECT_EQ(idle_thread->get_priority(), KERNEL_THREAD_PRIORITY_IDLE);
    EXPECT_EQ(idle_thread->get_name(), "idle");
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    char main_stack[128];

    Thread *main_thread = Thread::init(main_stack, sizeof(main_stack), nullptr, "main");

    main_thread->init_msg_queue(main_thread_msgqueue, ARRAY_LENGTH(main_thread_msgqueue));

    EXPECT_EQ(main_thread->has_msg_queue(), 1);
    EXPECT_EQ(main_thread->numof_msg_in_queue(), 0);

    EXPECT_NE(main_thread, nullptr);
    EXPECT_EQ(main_thread->get_pid(), 2);
    EXPECT_EQ(main_thread->get_priority(), KERNEL_THREAD_PRIORITY_MAIN);
    EXPECT_EQ(main_thread->get_name(), "main");
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);

    char stack1[128];

    Thread *thread1 = Thread::init(stack1, sizeof(stack1), nullptr, "thread1", KERNEL_THREAD_PRIORITY_MAIN - 1);

    thread1->init_msg_queue(thread1_msgqueue, ARRAY_LENGTH(thread1_msgqueue));

    EXPECT_EQ(thread1->has_msg_queue(), 1);
    EXPECT_EQ(thread1->numof_msg_in_queue(), 0);

    EXPECT_NE(thread1, nullptr);
    EXPECT_EQ(thread1->get_pid(), 3);
    EXPECT_EQ(thread1->get_priority(), KERNEL_THREAD_PRIORITY_MAIN - 1);
    EXPECT_EQ(thread1->get_name(), "thread1");
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(scheduler->numof_threads(), 3);
    EXPECT_EQ(scheduler->get_thread_from_container(idle_thread->get_pid()), idle_thread);
    EXPECT_EQ(scheduler->get_thread_from_container(main_thread->get_pid()), main_thread);
    EXPECT_EQ(scheduler->get_thread_from_container(thread1->get_pid()), thread1);
    EXPECT_EQ(scheduler->requested_context_switch(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] send message to a thread without msgQueue
     * -------------------------------------------------------------------------
     **/

    Msg msg = Msg();

    EXPECT_EQ(msg.send(idle_thread->get_pid()), -1);
    EXPECT_EQ(msg.send(idle_thread->get_pid()), -1);
    EXPECT_EQ(msg.send(idle_thread->get_pid()), -1);
    EXPECT_EQ(msg.send(idle_thread->get_pid()), -1);

    // idle_thread doesn't have  msgQueue, send message to idle_thread will failed

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] basic send and receive message
     * -------------------------------------------------------------------------
     **/

    Msg msg1 = Msg();

    msg1.receive();

    EXPECT_EQ(msg1.sender_pid, KERNEL_PID_UNDEF);
    EXPECT_EQ(msg1.type, 0);
    EXPECT_EQ(msg1.content.ptr, nullptr);
    EXPECT_EQ(msg1.content.value, 0);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RECEIVE_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RECEIVE_BLOCKED);

    Msg msg2 = Msg();

    EXPECT_EQ(msg2.sender_pid, KERNEL_PID_UNDEF);
    EXPECT_EQ(msg2.type, 0);
    EXPECT_EQ(msg2.content.ptr, nullptr);
    EXPECT_EQ(msg2.content.value, 0);

    uint32_t msg_value = 0xdeadbeef;

    msg2.type = 0x20;
    msg2.content.ptr = static_cast<void *>(&msg_value);

    EXPECT_EQ(msg2.send(thread1->get_pid()), 1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    // msg2 immediately received because thread1 was in RECEIVE BLOCKED status

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(msg1.sender_pid, main_thread->get_pid());
    EXPECT_EQ(msg1.type, 0x20);
    EXPECT_NE(msg1.content.ptr, nullptr);
    EXPECT_EQ(*static_cast<uint32_t *>(msg1.content.ptr), 0xdeadbeef);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] send message (blocking)
     * -------------------------------------------------------------------------
     **/

    Msg msg3 = Msg();

    EXPECT_EQ(msg3.sender_pid, KERNEL_PID_UNDEF);
    EXPECT_EQ(msg3.type, 0);
    EXPECT_EQ(msg3.content.ptr, nullptr);
    EXPECT_EQ(msg3.content.value, 0);

    msg3.type = 0x21;
    msg3.content.value = 0x12345678;

    EXPECT_EQ(msg3.send(main_thread->get_pid()), 1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(main_thread->numof_msg_in_queue(), 1);

    scheduler->sleep();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    Msg msg4 = Msg();

    EXPECT_EQ(msg4.sender_pid, KERNEL_PID_UNDEF);
    EXPECT_EQ(msg4.type, 0);
    EXPECT_EQ(msg4.content.ptr, nullptr);
    EXPECT_EQ(msg4.content.value, 0);

    EXPECT_EQ(msg4.receive(), 1);

    // Expected to receive msg3 content that was sent by thread1

    EXPECT_EQ(msg4.sender_pid, thread1->get_pid());
    EXPECT_EQ(msg4.type, 0x21);
    EXPECT_EQ(msg4.content.value, 0x12345678);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    EXPECT_EQ(main_thread->numof_msg_in_queue(), 0);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    EXPECT_EQ(scheduler->wakeup_thread(thread1->get_pid()), 1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] try send message (non-blocking)
     * -------------------------------------------------------------------------
     **/

    msg3.type = 0x22;
    msg3.content.value = 0xaaaaaaaa;

    EXPECT_EQ(msg3.try_send(main_thread->get_pid()), 1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(main_thread->numof_msg_in_queue(), 1);

    scheduler->sleep();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    EXPECT_EQ(msg4.receive(), 1);

    // Expected to receive msg3 content that was sent by thread1

    EXPECT_EQ(msg4.sender_pid, thread1->get_pid());
    EXPECT_EQ(msg4.type, 0x22);
    EXPECT_EQ(msg4.content.value, 0xaaaaaaaa);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    EXPECT_EQ(main_thread->numof_msg_in_queue(), 0);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    EXPECT_EQ(scheduler->wakeup_thread(thread1->get_pid()), 1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] send message from ISR
     * -------------------------------------------------------------------------
     **/
    msg3.type = 0x23;
    msg3.content.value = 0xbbbbbbbb;

    test_helper_set_cpu_in_isr(1);
    EXPECT_EQ(cpu_is_in_isr(), 1);
    EXPECT_EQ(msg3.send(thread1->get_pid()), 1);
    test_helper_set_cpu_in_isr(0);
    EXPECT_EQ(cpu_is_in_isr(), 0);

    EXPECT_EQ(thread1->numof_msg_in_queue(), 1);

    EXPECT_EQ(msg4.receive(), 1);

    EXPECT_EQ(thread1->numof_msg_in_queue(), 0);

    EXPECT_EQ(msg4.sender_pid, KERNEL_PID_ISR);
    EXPECT_EQ(msg4.type, 0x23);
    EXPECT_EQ(msg4.content.value, 0xbbbbbbbb);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] send and then immediately set to receive state by calling
     * sendReceive function.
     * -------------------------------------------------------------------------
     **/

    msg3.type = 0x24;
    msg3.content.value = 0xcccccccc;

    EXPECT_EQ(msg3.send_receive(&msg4, main_thread->get_pid()), 1);

    // Note: replied message will be update on msg4

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_REPLY_BLOCKED);

    EXPECT_EQ(main_thread->numof_msg_in_queue(), 1);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_REPLY_BLOCKED);

    Msg msg5 = Msg();

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.type, 0x24);
    EXPECT_EQ(msg5.content.value, 0xcccccccc);
    EXPECT_EQ(msg5.sender_pid, thread1->get_pid());

    EXPECT_EQ(msg5.try_receive(), -1);
    EXPECT_EQ(main_thread->numof_msg_in_queue(), 0);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_REPLY_BLOCKED);

    Msg msg6 = Msg();

    msg6.type = 0xff;
    msg6.content.value = 0xaaaacccc;

    EXPECT_EQ(msg5.reply(&msg6), 1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();    

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    // replied message received by msg4 as expected

    EXPECT_EQ(msg4.type, 0xff);
    EXPECT_EQ(msg4.content.value, 0xaaaacccc);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] send and then immediately set to receive state by calling
     * sendReceive function. reply function would be in Isr.
     * -------------------------------------------------------------------------
     **/

    msg3.type = 0x24;
    msg3.content.value = 0xddddcccc;

    EXPECT_EQ(msg3.send_receive(&msg4, main_thread->get_pid()), 1);

    // Note: replied message will be update on msg4

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_REPLY_BLOCKED);

    EXPECT_EQ(main_thread->numof_msg_in_queue(), 1);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_REPLY_BLOCKED);

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.type, 0x24);
    EXPECT_EQ(msg5.content.value, 0xddddcccc);
    EXPECT_EQ(msg5.sender_pid, thread1->get_pid());

    EXPECT_EQ(msg5.try_receive(), -1);
    EXPECT_EQ(main_thread->numof_msg_in_queue(), 0);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_REPLY_BLOCKED);

    msg6.type = 0xee;
    msg6.content.value = 0xccccdddd;

    test_helper_set_cpu_in_isr(1);
    EXPECT_EQ(cpu_is_in_isr(), 1);

    EXPECT_EQ(msg5.reply_in_isr(&msg6), 1);

    test_helper_set_cpu_in_isr(0);
    EXPECT_EQ(cpu_is_in_isr(), 0);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(scheduler->requested_context_switch(), 1);

    scheduler->run();    

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    // replied message received by msg4 as expected

    EXPECT_EQ(msg4.type, 0xee);
    EXPECT_EQ(msg4.content.value, 0xccccdddd);
}

TEST_F(TestMsg, multipleMsgTest)
{
    ThreadScheduler *scheduler = &ThreadScheduler::init();

    EXPECT_NE(scheduler, nullptr);
    EXPECT_EQ(scheduler->numof_threads(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    Msg idle_thread_msgqueue[4];
    Msg main_thread_msgqueue[4];
    Msg thread1_msgqueue[4];

    char idle_stack[128];

    Thread *idle_thread = Thread::init(idle_stack, sizeof(idle_stack), nullptr, "idle", KERNEL_THREAD_PRIORITY_IDLE);

    idle_thread->init_msg_queue(idle_thread_msgqueue, ARRAY_LENGTH(idle_thread_msgqueue));

    EXPECT_EQ(idle_thread->has_msg_queue(), 1);
    EXPECT_EQ(idle_thread->numof_msg_in_queue(), 0);

    EXPECT_NE(idle_thread, nullptr);
    EXPECT_EQ(idle_thread->get_pid(), 1);
    EXPECT_EQ(idle_thread->get_priority(), KERNEL_THREAD_PRIORITY_IDLE);
    EXPECT_EQ(idle_thread->get_name(), "idle");
    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);

    char main_stack[128];

    Thread *main_thread = Thread::init(main_stack, sizeof(main_stack), nullptr, "main");

    main_thread->init_msg_queue(main_thread_msgqueue, ARRAY_LENGTH(main_thread_msgqueue));

    EXPECT_EQ(main_thread->has_msg_queue(), 1);
    EXPECT_EQ(main_thread->numof_msg_in_queue(), 0);

    EXPECT_NE(main_thread, nullptr);
    EXPECT_EQ(main_thread->get_pid(), 2);
    EXPECT_EQ(main_thread->get_priority(), KERNEL_THREAD_PRIORITY_MAIN);
    EXPECT_EQ(main_thread->get_name(), "main");
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);

    char stack1[128];

    Thread *thread1 = Thread::init(stack1, sizeof(stack1), nullptr, "thread1", KERNEL_THREAD_PRIORITY_MAIN - 1);

    thread1->init_msg_queue(thread1_msgqueue, ARRAY_LENGTH(thread1_msgqueue));

    EXPECT_EQ(thread1->has_msg_queue(), 1);
    EXPECT_EQ(thread1->numof_msg_in_queue(), 0);

    EXPECT_NE(thread1, nullptr);
    EXPECT_EQ(thread1->get_pid(), 3);
    EXPECT_EQ(thread1->get_priority(), KERNEL_THREAD_PRIORITY_MAIN - 1);
    EXPECT_EQ(thread1->get_name(), "thread1");
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    EXPECT_EQ(scheduler->numof_threads(), 3);
    EXPECT_EQ(scheduler->get_thread_from_container(idle_thread->get_pid()), idle_thread);
    EXPECT_EQ(scheduler->get_thread_from_container(main_thread->get_pid()), main_thread);
    EXPECT_EQ(scheduler->get_thread_from_container(thread1->get_pid()), thread1);
    EXPECT_EQ(scheduler->requested_context_switch(), 0);
    EXPECT_EQ(sched_active_thread, nullptr);
    EXPECT_EQ(sched_active_pid, KERNEL_PID_UNDEF);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] set current active thread to sleep status and send msg to it,
     * it expected to queued the msg.
     * -------------------------------------------------------------------------
     **/

    scheduler->sleep();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    Msg msg1 = Msg();
    Msg msg2 = Msg();
    Msg msg3 = Msg();
    Msg msg4 = Msg();

    msg1.type = 0xff;
    msg1.content.value = 0x1;

    msg2.type = 0xff;
    msg2.content.value = 0x2;

    msg3.type = 0xff;
    msg3.content.value = 0x3;

    msg4.type = 0xff;
    msg4.content.value = 0x4;

    EXPECT_EQ(msg1.send(thread1->get_pid()), 1);
    EXPECT_EQ(msg2.send(thread1->get_pid()), 1);
    EXPECT_EQ(msg3.send(thread1->get_pid()), 1);
    EXPECT_EQ(msg4.send(thread1->get_pid()), 1);

    EXPECT_EQ(thread1->numof_msg_in_queue(), 4);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    EXPECT_EQ(scheduler->wakeup_thread(thread1->get_pid()), 1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    Msg msg5 = Msg();

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.sender_pid, main_thread->get_pid());
    EXPECT_EQ(msg5.type, 0xff);
    EXPECT_EQ(msg5.content.value, 0x1);

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.sender_pid, main_thread->get_pid());
    EXPECT_EQ(msg5.type, 0xff);
    EXPECT_EQ(msg5.content.value, 0x2);

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.sender_pid, main_thread->get_pid());
    EXPECT_EQ(msg5.type, 0xff);
    EXPECT_EQ(msg5.content.value, 0x3);

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.sender_pid, main_thread->get_pid());
    EXPECT_EQ(msg5.type, 0xff);
    EXPECT_EQ(msg5.content.value, 0x4);

    EXPECT_EQ(thread1->numof_msg_in_queue(), 0);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    scheduler->sleep();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    // Send message to thread1 until it message queue overflow

    EXPECT_EQ(msg1.send(thread1->get_pid()), 1);
    EXPECT_EQ(msg2.send(thread1->get_pid()), 1);
    EXPECT_EQ(msg3.send(thread1->get_pid()), 1);
    EXPECT_EQ(msg4.send(thread1->get_pid()), 1);

    EXPECT_EQ(thread1->numof_msg_in_queue(), 4);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    Msg msg6 = Msg();

    msg6.type = 0xff;
    msg6.content.value = 0xdeadbeef;

    EXPECT_EQ(msg6.send(thread1->get_pid()), 1);

    EXPECT_EQ(thread1->numof_msg_in_queue(), 4);

    // Notice that num of message in thread1 msgQueue doesn't increase, it
    // already reach it's limit

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_SEND_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    // thread1 msgQueue was full, so the main_thread will go into SEND BLOCKED
    // state to send msg6 until all messages in thread1 msgQueue is receive

    EXPECT_EQ(scheduler->wakeup_thread(thread1->get_pid()), 1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_SEND_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_SEND_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.sender_pid, main_thread->get_pid());
    EXPECT_EQ(msg5.type, 0xff);
    EXPECT_EQ(msg5.content.value, 0x1); // msg1

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.sender_pid, main_thread->get_pid());
    EXPECT_EQ(msg5.type, 0xff);
    EXPECT_EQ(msg5.content.value, 0x2); // msg2

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.sender_pid, main_thread->get_pid());
    EXPECT_EQ(msg5.type, 0xff);
    EXPECT_EQ(msg5.content.value, 0x3); // msg3

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.sender_pid, main_thread->get_pid());
    EXPECT_EQ(msg5.type, 0xff);
    EXPECT_EQ(msg5.content.value, 0x4); // msg4

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_SEND_BLOCKED);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(thread1->numof_msg_in_queue(), 0);

    // All messages have been received from thread1 msgQueue, to receive pending
    // msg6 we need to call receive() once again and sender thread (main_thread)
    // should no longer in SEND BLOCKED state if msg6 receive successfully

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.sender_pid, main_thread->get_pid());
    EXPECT_EQ(msg5.type, 0xff);
    EXPECT_EQ(msg5.content.value, 0xdeadbeef); // msg6

    // Now msg6 was successfully sent and main_thread (sender) will no longer in
    // SEND BLOCKED state

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    msg5.type = 0;
    msg5.content.value = 0;

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.type, 0);
    EXPECT_EQ(msg5.content.value, 0);

    // Nothing was on the queue, thread1 will go into RECEIVE BLOCKED state

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RECEIVE_BLOCKED);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RECEIVE_BLOCKED);

    EXPECT_EQ(msg1.send(thread1->get_pid()), 1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_PENDING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_RUNNING);

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.type, 0xff);
    EXPECT_EQ(msg5.content.value, 0x1);

    EXPECT_EQ(thread1->numof_msg_in_queue(), 0);

    scheduler->sleep();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    scheduler->run();

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    // Use try_send() function (non-blocking) when target msgQueue is full

    EXPECT_EQ(msg1.send(thread1->get_pid()), 1);
    EXPECT_EQ(msg2.send(thread1->get_pid()), 1);
    EXPECT_EQ(msg3.send(thread1->get_pid()), 1);
    EXPECT_EQ(msg4.send(thread1->get_pid()), 1);

    EXPECT_EQ(thread1->numof_msg_in_queue(), 4);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    EXPECT_EQ(msg6.try_send(thread1->get_pid()), 0);
    EXPECT_EQ(msg6.try_send(thread1->get_pid()), 0);
    EXPECT_EQ(msg6.try_send(thread1->get_pid()), 0);
    EXPECT_EQ(msg6.try_send(thread1->get_pid()), 0);
    EXPECT_EQ(msg6.try_send(thread1->get_pid()), 0);
    EXPECT_EQ(msg6.try_send(thread1->get_pid()), 0);

    // thread1 msgQueue is full, so all try_send attempt was failed as expected
    // and main_thread should still in RUNNING state

    EXPECT_EQ(thread1->numof_msg_in_queue(), 4);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    /**
     * -------------------------------------------------------------------------
     * [TEST CASE] send message to itself
     * -------------------------------------------------------------------------
     **/

    EXPECT_EQ(msg6.send_to_self_queue(), 1);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);

    EXPECT_EQ(main_thread->numof_msg_in_queue(), 1);

    msg5.type = 0;
    msg5.content.value = 0;

    EXPECT_EQ(msg5.receive(), 1);

    EXPECT_EQ(msg5.type, 0xff);
    EXPECT_EQ(msg5.content.value, 0xdeadbeef); // msg6

    EXPECT_EQ(main_thread->numof_msg_in_queue(), 0);

    EXPECT_EQ(idle_thread->get_status(), THREAD_STATUS_PENDING);
    EXPECT_EQ(main_thread->get_status(), THREAD_STATUS_RUNNING);
    EXPECT_EQ(thread1->get_status(), THREAD_STATUS_SLEEPING);
}
