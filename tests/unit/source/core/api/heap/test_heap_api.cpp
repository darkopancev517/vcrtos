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

#include "gtest/gtest.h"

#include <vcrtos/heap.h>

#include "utils/heap.hpp"

using namespace vc;
using namespace utils;

class TestHeapApi : public testing::Test
{
protected:
    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(TestHeapApi, allocate_single_test)
{
    (void) heap_init();

    const size_t total_size = heap_get_free_size();

    {
        //printf("%s allocating %zu bytes...\n", __func__, size);

        void *p = heap_calloc(1, 0);

        EXPECT_EQ(p, nullptr);
        EXPECT_EQ(total_size, heap_get_free_size());

        heap_free(p);

        p = heap_calloc(0, 1);

        EXPECT_EQ(p, nullptr);
        EXPECT_EQ(total_size, heap_get_free_size());

        heap_free(p);
    }

    for (size_t size = 1; size <= heap_get_capacity(); ++size)
    {
        void *p = heap_calloc(1, size);

        EXPECT_NE(p, nullptr);
        EXPECT_FALSE(heap_is_clean());
        EXPECT_TRUE(heap_get_free_size() + size <= total_size);

        memset(p, 0xff, size);

        heap_free(p);

        EXPECT_TRUE(heap_is_clean());
        EXPECT_TRUE(heap_get_free_size() == total_size);
    }
}

void test_allocate_randomly(size_t size_limit, unsigned int seed)
{
    struct Node
    {
        Node *next;
        size_t size;
    };

    Node head;
    size_t nnodes;

    srand(seed);

    const size_t total_size = heap_get_free_size();
    Node *last = &head;

    do
    {
        size_t size = sizeof(Node) + static_cast<size_t>(rand()) % size_limit;
        //printf("test_allocate_randomly allocating %zu bytes...\n", size);
        last->next = static_cast<Node *>(heap_calloc(1, size));

        if (last->next == nullptr)
        {
            // no more memory for allocation
            break;
        }

        EXPECT_EQ(last->next->next, nullptr);
        last = last->next;
        last->size = size;
        ++nnodes;

        // 50% probability to randomly free a node.
        size_t free_index = static_cast<size_t>(rand()) % (nnodes * 2);

        if (free_index > nnodes)
        {
            free_index /= 2;

            Node *prev = &head;

            while (free_index--)
            {
                prev = prev->next;
            }

            Node *curr = prev->next;
            //printf("test_allocate_randomly freeing %zu bytes..\n", curr->size);
            prev->next = curr->next;
            heap_free(curr);

            if (last == curr)
            {
                last = prev;
            }

            --nnodes;
        }
    } while (true);

    last = head.next;

    while (last)
    {
        Node *next = last->next;
        //printf("test_allocate_randomly freeing %zu bytes..\n", last->size);
        heap_free(last);
        last = next;
    }

    EXPECT_TRUE(heap_is_clean());
    EXPECT_TRUE(heap_get_free_size() == total_size);
}

TEST_F(TestHeapApi, allocate_multiple_test)
{
    for (unsigned int seed = 0; seed < 10; ++seed)
    {
        size_t size_limit = (1 << seed);
        //printf("test_allocate_randomly(%zu, %u)...\n", size_limit, seed);
        test_allocate_randomly(size_limit, seed);
    }
}
