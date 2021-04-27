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

#ifndef TEST_HELPER_H
#define TEST_HELPER_H

/* unit test helper functions */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void test_helper_set_cpu_in_isr(int val);

int test_helper_is_pendsv_interrupt_triggered(void);

void test_helper_reset_pendsv_trigger(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_HELPER_H */
