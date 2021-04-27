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

#ifndef CORE_MSG_HPP
#define CORE_MSG_HPP

#include <stdint.h>

#include <vcrtos/assert.h>
#include <vcrtos/config.h>
#include <vcrtos/kernel.h>
#include <vcrtos/msg.h>

#include "core/list.hpp"

namespace vc {

class Thread;

class Msg : public msg_t
{
public:
    Msg()
    {
        this->sender_pid = KERNEL_PID_UNDEF;
        this->type = 0;
        this->content.ptr = nullptr;
        this->content.value = 0;
    }

    int queued_msg(Thread *target);
    int send(kernel_pid_t target_pid);
    int try_send(kernel_pid_t target_pid);
    int send_to_self_queue();
    int send_in_isr(kernel_pid_t target_pid);
    int is_sent_by_isr() { return sender_pid == KERNEL_PID_ISR; }
    int receive() { return receive(1); }
    int try_receive() { return receive(0); }
    int send_receive(Msg *reply, kernel_pid_t target_pid);
    int reply(Msg *reply);
    int reply_in_isr(Msg *reply);

private:
    int send(kernel_pid_t target_pid, int blocking, unsigned state);
    int receive(int blocking);
};

} // namespace vc

#endif /* CORE_MSG_HPP */
