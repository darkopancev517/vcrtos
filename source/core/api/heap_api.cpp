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
#include <vcrtos/heap.h>
#include <vcrtos/assert.h>

#include "core/code_utils.h"
#include "core/new.hpp"

#include "utils/heap.hpp"

using namespace vc;
using namespace utils;

DEFINE_ALIGNED_VAR(heap_raw, sizeof(Heap), uint64_t);

static Heap *heap = NULL;

void *heap_init()
{
    vcassert(heap == NULL);
    heap = new (&heap_raw) Heap();
    return heap;
}

size_t heap_get_free_size()
{
    vcassert(heap != NULL);
    return heap->get_free_size();
}

size_t heap_get_capacity()
{
    vcassert(heap != NULL);
    return heap->get_capacity();
}

bool heap_is_clean()
{
    vcassert(heap != NULL);
    return heap->is_clean();
}

void heap_free(void *ptr)
{
    vcassert(heap != NULL);
    heap->free(ptr);
}

void *heap_malloc(size_t size)
{
    return heap_calloc(1, size);
}

void *heap_calloc(size_t count, size_t size)
{
    vcassert(heap != NULL);
    return heap->calloc(count, size);
}
