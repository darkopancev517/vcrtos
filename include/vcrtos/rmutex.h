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

#ifndef VCRTOS_RMUTEX_H
#define VCRTOS_RMUTEX_H

#include <stdint.h>
#include <stdatomic.h>

#include <vcrtos/config.h>
#include <vcrtos/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    mutex_t mutex;
    uint16_t refcount;
    atomic_int_least16_t owner;
} rmutex_t;

void rmutex_init(rmutex_t *rmutex);
void rmutex_lock(rmutex_t *rmutex);
int rmutex_trylock(rmutex_t *rmutex);
void rmutex_unlock(rmutex_t *rmutex);

#ifdef __cplusplus
}
#endif

#endif /* VCRTOS_RMUTEX_H */
