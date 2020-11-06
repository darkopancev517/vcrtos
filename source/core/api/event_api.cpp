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

#include <vcrtos/config.h>
#include <vcrtos/event.h>
#include <vcrtos/thread.h>

#include "core/code_utils.h"
#include "core/instance.hpp"
#include "core/new.hpp"
#include "core/thread.hpp"

#if VCRTOS_CONFIG_THREAD_EVENT_ENABLE

using namespace vc;

void event_init(event_t *event)
{
    event = new (event) Event();
}

void event_queue_init(void *instances, event_queue_t *queue)
{
    Instance &instance = *static_cast<Instance *>(instances);
    queue = new (queue) EventQueue(instance);
}

void event_post(event_queue_t *queue, event_t *event, thread_t *thread)
{
    EventQueue &event_queue = *static_cast<EventQueue *>(queue);
    event_queue.event_post(reinterpret_cast<Event *>(event), static_cast<Thread *>(thread));
}

void event_cancel(event_queue_t *queue, event_t *event)
{
    EventQueue &event_queue = *static_cast<EventQueue *>(queue);
    event_queue.event_cancel(reinterpret_cast<Event *>(event));
}

event_t *event_get(event_queue_t *queue)
{
    EventQueue &event_queue = *static_cast<EventQueue *>(queue);
    return event_queue.event_get();
}

event_t *event_wait(event_queue_t *queue)
{
    EventQueue &event_queue = *static_cast<EventQueue *>(queue);
    return event_queue.event_wait();
}

void event_release(event_t *event)
{
    EventQueue::event_release(reinterpret_cast<Event *>(event));
}

int event_pending(event_queue_t *queue)
{
    EventQueue &event_queue = *static_cast<EventQueue *>(queue);
    return event_queue.event_pending();
}

event_t *event_peek(event_queue_t *queue)
{
    EventQueue &event_queue = *static_cast<EventQueue *>(queue);
    return event_queue.event_peek();
}

#endif // #if VCRTOS_CONFIG_THREAD_EVENT_ENABLE
