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

#ifndef VCRTOS_EVENT_H
#define VCRTOS_EVENT_H

#include <vcrtos/config.h>
#include <vcrtos/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THREAD_FLAG_EVENT (0x1)

typedef struct
{
    clist_node_t list_node;
} event_t;

typedef struct
{
    clist_node_t event_list;
#if VCRTOS_CONFIG_MULTIPLE_INSTANCE_ENABLE
    void *instance;
#endif
} event_queue_t;

void event_init(event_t *event);

void event_queue_init(void *instance, event_queue_t *queue);

void event_post(event_queue_t *queue, event_t *event, thread_t *thread);

void event_cancel(event_queue_t *queue, event_t *event);

event_t *event_get(event_queue_t *queue);

event_t *event_wait(event_queue_t *queue);

void event_loop(event_queue_t *queue);

void event_release(event_t *event);

int event_pending(event_queue_t *queue);

event_t *event_peek(event_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif /* VCRTOS_EVENT_H */
