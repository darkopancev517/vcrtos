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

#ifndef VCRTOS_CPU_H
#define VCRTOS_CPU_H

#include <stdint.h>

#include <vcrtos/config.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This marker is used to identify the stacks beginning */
#define STACK_MARKER (0x77777777)

/* Initial program status register value for a newly created thread */
#define INITIAL_XPSR (0x01000000)

/* ARM Cortex-M specific exception return value, that triggers the return to the
 * task mode stack pointer */
#define EXCEPT_RET_TASK_MODE (0xfffffffd)

/* Interrupt stack canary value 0xe7fe is the ARM Thumb machine code equivalent
 * of asm("bl #-2\n") or 'while (1);', i.e. an infinite loop */
#define STACK_CANARY_WORD (0xE7FEE7FEu)

unsigned cpu_irq_disable();
unsigned cpu_irq_enable();
void cpu_irq_restore(unsigned state);
int cpu_is_in_isr();
extern void cpu_end_of_isr();
void cpu_trigger_pendsv_interrupt();
void cpu_switch_context_exit();
void cpu_print_last_instruction();
void cpu_sleep_until_event();
void cpu_sleep(int deep);
void cpu_jump_to_image(uint32_t image_addr);
uint32_t cpu_get_image_base_addr();
void *cpu_get_msp();

#ifdef __cplusplus
}
#endif

#endif /* VCRTOS_CPU_H */
