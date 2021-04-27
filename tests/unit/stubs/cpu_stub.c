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

#include "test-helper.h"

static int is_cpu_in_isr = 0;
static int is_pendsv_interrupt_triggered = 0;

void test_helper_set_cpu_in_isr(int val)
{
    is_cpu_in_isr = val;
}

int test_helper_is_pendsv_interrupt_triggered(void)
{
    return is_pendsv_interrupt_triggered;
}

void test_helper_reset_pendsv_trigger(void)
{
    is_pendsv_interrupt_triggered = 0;
}

unsigned cpu_irq_disable(void)
{
    return 0;
}

unsigned cpu_irq_enable(void)
{
    return 0;
}

void cpu_irq_restore(unsigned state)
{
    (void) state;
}

int cpu_is_in_isr(void)
{
    return is_cpu_in_isr;
}

void cpu_trigger_pendsv_interrupt(void)
{
    is_pendsv_interrupt_triggered = 1;
}

void thread_arch_yield_higher(void)
{
    cpu_trigger_pendsv_interrupt();
}

void cpu_switch_context_exit(void)
{
}
