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

#ifndef VCRTOS_THREAD_H
#define VCRTOS_THREAD_H

#include <stdlib.h>

#include <vcrtos/config.h>
#include <vcrtos/kernel.h>
#include <vcrtos/cib.h>
#include <vcrtos/clist.h>
#include <vcrtos/msg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void *sched_active_thread;
extern int16_t sched_active_pid;

typedef void *(*thread_handler_func_t)(void *arg);

typedef enum
{
    THREAD_STATUS_STOPPED,
    THREAD_STATUS_SLEEPING,
    THREAD_STATUS_MUTEX_BLOCKED,
    THREAD_STATUS_RECEIVE_BLOCKED,
    THREAD_STATUS_SEND_BLOCKED,
    THREAD_STATUS_REPLY_BLOCKED,
    THREAD_STATUS_FLAG_BLOCKED_ANY,
    THREAD_STATUS_FLAG_BLOCKED_ALL,
    THREAD_STATUS_MBOX_BLOCKED,
    THREAD_STATUS_COND_BLOCKED,
    THREAD_STATUS_RUNNING,
    THREAD_STATUS_PENDING,
    THREAD_STATUS_NUMOF
} thread_status_t;

#define THREAD_STATUS_NOT_FOUND ((thread_status_t)-1)

#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
typedef uint16_t thread_flags_t;
#endif

typedef struct thread
{
    char *stack_pointer;
    thread_status_t status;
    uint8_t priority;
    kernel_pid_t pid;
#if VCRTOS_CONFIG_THREAD_FLAGS_ENABLE
    thread_flags_t flags;
    thread_flags_t waited_flags;
#endif
    list_node_t runqueue_entry;
    void *wait_data;
    list_node_t msg_waiters;
    cib_t msg_queue;
    msg_t *msg_array;
    char *stack_start;
    const char *name;
    int stack_size;
} thread_t;

#define THREAD_FLAGS_CREATE_SLEEPING (0x1)
#define THREAD_FLAGS_CREATE_WOUT_YIELD (0x2)
#define THREAD_FLAGS_CREATE_STACKMARKER (0x4)

kernel_pid_t thread_create(char *stack,
                           int size,
                           thread_handler_func_t func,
                           const char *name,
                           char priority,
                           void *arg,
                           int flags);

void thread_scheduler_init();
int thread_scheduler_is_initialized();
int thread_scheduler_requested_context_switch();
void thread_scheduler_context_switch_request(unsigned state);
void thread_scheduler_run();
void thread_scheduler_set_status(thread_t *thread, thread_status_t status);
void thread_scheduler_switch(uint8_t priority);
void thread_exit();
void thread_terminate(kernel_pid_t pid);
int thread_pid_is_valid(kernel_pid_t pid);
void thread_yield();
thread_t *thread_current();
void thread_sleep();
int thread_wakeup(kernel_pid_t pid);
kernel_pid_t thread_current_pid();
thread_t *thread_get_from_scheduler(kernel_pid_t pid);
uint64_t thread_get_runtime_ticks(kernel_pid_t pid);
const char *thread_status_to_string(thread_status_t status);
uintptr_t thread_measure_stack_free(char *stack);
uint32_t thread_get_schedules_stat(kernel_pid_t pid);
void thread_add_to_list(list_node_t *list, thread_t *thread);

char *thread_arch_stack_init(thread_handler_func_t func, void *arg, void *stack_start, int size);
void thread_arch_stack_print();
int thread_arch_isr_stack_usage();
void *thread_arch_isr_stack_pointer();
void *thread_arch_isr_stack_start();
void thread_arch_yield_higher();

#ifdef __cplusplus
}
#endif

#endif /* VCRTOS_THREAD_H */
