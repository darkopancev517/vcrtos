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

#ifndef VCRTOS_KERNEL_H
#define VCRTOS_KERNEL_H

#include <stdint.h>
#include <inttypes.h>

#include <vcrtos/config.h>

#ifdef __cplusplus
extern "C" {
#endif

#define KERNEL_MAXTHREADS (32)

#define KERNEL_PID_UNDEF (0)
#define KERNEL_PID_FIRST (KERNEL_PID_UNDEF + 1)
#define KERNEL_PID_LAST (KERNEL_PID_FIRST + KERNEL_MAXTHREADS - 1)

#define KERNEL_PID_ISR (KERNEL_PID_LAST - 1)

#define KERNEL_THREAD_PRIORITY_LEVELS VCRTOS_CONFIG_THREAD_PRIORITY_LEVELS
#define KERNEL_THREAD_PRIORITY_MIN (KERNEL_THREAD_PRIORITY_LEVELS - 1)
#define KERNEL_THREAD_PRIORITY_IDLE KERNEL_THREAD_PRIORITY_MIN
#define KERNEL_THREAD_PRIORITY_MAIN (KERNEL_THREAD_PRIORITY_MIN - (KERNEL_THREAD_PRIORITY_LEVELS / 2))

#define PRIkernel_pid PRIi16

typedef int16_t kernel_pid_t;

void kernel_init();

#ifdef __cplusplus
}
#endif

#endif /* VCRTOS_KERNEL_H */
