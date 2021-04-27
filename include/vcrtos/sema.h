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

#ifndef VCRTOS_SEMA_H
#define VCRTOS_SEMA_H

#include <stdint.h>

#include <vcrtos/config.h>
#include <vcrtos/mutex.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SEMA_OK = 0,
    SEMA_DESTROY,
} sema_state_t;

typedef struct
{
    unsigned int value;
    sema_state_t state;
    mutex_t mutex;
} sema_t;

void sema_create(sema_t *sema, unsigned int value);
void sema_destroy(sema_t *sema);
int sema_post(sema_t *sema);

#ifdef __cplusplus
}
#endif

#endif /* VCRTOS_SEMA_H */
