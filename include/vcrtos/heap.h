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

#ifndef VCRTOS_HEAP_H
#define VCRTOS_HEAP_H

#include <stdlib.h>

#include <vcrtos/config.h>

#ifdef __cplusplus
extern "C" {
#endif

void *heap_init();
size_t heap_get_free_size();
size_t heap_get_capacity();
bool heap_is_clean();
void heap_free(void *ptr);
void *heap_malloc(size_t size);
void *heap_calloc(size_t count, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* VCRTOS_HEAP_H */
