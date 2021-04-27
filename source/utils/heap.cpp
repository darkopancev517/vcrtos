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

#include "utils/heap.hpp"

#include <string.h>

#include "core/code_utils.h"

namespace vc {
namespace utils {

Heap::Heap()
{
    Block &super = block_at(SUPER_BLOCK_OFFSET);
    super.set_size(SUPER_BLOCK_SIZE);

    Block &first = block_right(super);
    first.set_size(FIRST_BLOCK_SIZE);

    Block &guard = block_right(first);
    guard.set_size(Block::GUARD_BLOCK_SIZE);

    super.set_next(block_offset(first));
    first.set_next(block_offset(guard));

    _memory.mfree_size = FIRST_BLOCK_SIZE;
}

void *Heap::calloc(size_t count, size_t asize)
{
    void *ret = nullptr;
    Block *prev = nullptr;
    Block *curr = nullptr;
    uint16_t size = static_cast<uint16_t>(count * asize);

    VERIFY_OR_EXIT(size);

    size += ALIGN_SIZE - 1 - BLOCK_REMAINDER_SIZE;
    size &= ~(ALIGN_SIZE - 1);
    size += BLOCK_REMAINDER_SIZE;

    prev = &block_super();
    curr = &block_next(*prev);

    while (curr->get_size() < size)
    {
        prev = curr;
        curr = &block_next(*curr);
    }

    VERIFY_OR_EXIT(curr->is_free());

    prev->set_next(curr->get_next());

    if (curr->get_size() > size + sizeof(Block))
    {
        const uint16_t new_block_size = curr->get_size() - size - sizeof(Block);
        curr->set_size(size);

        Block &new_block = block_right(*curr);
        new_block.set_size(new_block_size);
        new_block.set_next(0);

        if (prev->get_size() < new_block_size)
        {
            block_insert(*prev, new_block);
        }
        else
        {
            block_insert(block_super(), new_block);
        }

        _memory.mfree_size -= sizeof(Block);
    }

    _memory.mfree_size -= curr->get_size();

    curr->set_next(0);

    memset(curr->get_pointer(), 0, size);
    ret = curr->get_pointer();

exit:
    return ret;
}

void Heap::block_insert(Block &aprev, Block &ablock)
{
    Block *prev = &aprev;

    for (Block *b = &block_next(*prev); b->get_size() < ablock.get_size(); b = &block_next(*b))
    {
        prev = b;
    }

    ablock.set_next(prev->get_next());
    prev->set_next(block_offset(ablock));
}

Block &Heap::block_prev(const Block &block)
{
    Block *prev = &block_super();

    while (prev->get_next() != block_offset(block))
    {
        prev = &block_next(*prev);
    }

    return *prev;
}

void Heap::free(void *ptr)
{
    if (ptr == nullptr)
    {
        return;
    }

    Block &block = block_of(ptr);
    Block &right = block_right(block);

    _memory.mfree_size += block.get_size();

    if (is_left_free(block))
    {
        Block *prev = &block_super();
        Block *left = &block_next(*prev);

        _memory.mfree_size += sizeof(Block);

        for (const uint16_t offset = block.get_left_next(); left->get_next() != offset; left = &block_next(*left))
        {
            prev = left;
        }

        // Remove left from free list
        prev->set_next(left->get_next());
        left->set_next(0);

        if (right.is_free())
        {
            _memory.mfree_size += sizeof(Block);

            if (right.get_size() > left->get_size())
            {
                for (const uint16_t offset = block_offset(right);
                     prev->get_next() != offset;
                     prev = &block_next(*prev))
                {
                }
            }
            else
            {
                prev = &block_prev(right);
            }

            // Remove right from free list
            prev->set_next(right.get_next());
            right.set_next(0);

            // Add size of right
            left->set_size(left->get_size() + right.get_size() + sizeof(Block));
        }

        // Add size of current block
        left->set_size(left->get_size() + block.get_size() + sizeof(Block));

        block_insert(*prev, *left);
    }
    else
    {
        if (right.is_free())
        {
            Block &prev = block_prev(right);
            prev.set_next(right.get_next());
            block.set_size(block.get_size() + right.get_size() + sizeof(Block));
            block_insert(prev, block);

            _memory.mfree_size += sizeof(Block);
        }
        else
        {
            block_insert(block_super(), block);
        }
    }
}

} // namespace utils
} // namespace vc
