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

#include <errno.h>
#include <limits.h>

#include <vcrtos/sema.h>
#include <vcrtos/cpu.h>
#include <vcrtos/assert.h>
#include <vcrtos/thread.h>
#include <vcrtos/mutex.h>

void sema_create(sema_t *sema, unsigned int value)
{
    vcassert(sema != NULL);
    sema->value = value;
    sema->state = SEMA_OK;
    mutex_init_unlocked(&sema->mutex);
    if (value == 0)
        mutex_lock(&sema->mutex);
}

void sema_destroy(sema_t *sema)
{
    vcassert(sema != NULL);
    sema->state = SEMA_DESTROY;
    mutex_unlock(&sema->mutex);
}

int sema_post(sema_t *sema)
{
    vcassert(sema != NULL);
    unsigned irqstate = cpu_irq_disable();
    if (sema->value == UINT_MAX)
    {
        cpu_irq_restore(irqstate);
        return -EOVERFLOW;
    }
    unsigned value = sema->value++;
    cpu_irq_restore(irqstate);

    if (value == 0)
        mutex_unlock(&sema->mutex);

    return 0;
}
