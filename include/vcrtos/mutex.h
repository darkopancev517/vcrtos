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

#ifndef VCRTOS_MUTEX_H
#define VCRTOS_MUTEX_H

#include <stdint.h>

#include <vcrtos/config.h>
#include <vcrtos/list.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MUTEX_LOCKED ((list_node_t *)-1)

#define MUTEX_INIT_UNLOCKED 0
#define MUTEX_INIT_LOCKED   1

typedef struct mutex
{
    list_node_t queue;
} mutex_t;

void mutex_init(mutex_t *mutex);
void mutex_init_unlocked(mutex_t *mutex);
void mutex_lock(mutex_t *mutex);
int mutex_try_lock(mutex_t *mutex);
void mutex_unlock(mutex_t *mutex);
void mutex_unlock_and_sleep(mutex_t *mutex);

#ifdef __cplusplus
}
#endif

#endif /* VCRTOS_MUTEX_H */
