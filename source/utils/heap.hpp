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

#ifndef VCRTOS_HEAP_HPP
#define VCRTOS_HEAP_HPP

#include <stddef.h>
#include <stdint.h>

#include <vcrtos/config.h>

namespace vc {
namespace utils {

class Block
{
    friend class Heap;

public:
    uint16_t get_size() const { return _size; }
    void set_size(uint16_t size) { _size = size; }

    uint16_t get_next() const
    {
        return *reinterpret_cast<const uint16_t *>(
            reinterpret_cast<const void *>(reinterpret_cast<const uint8_t *>(this) + sizeof(_size) + _size));
    }

    void set_next(uint16_t next)
    {
        *reinterpret_cast<uint16_t *>(
            reinterpret_cast<void *>(reinterpret_cast<uint8_t *>(this) + sizeof(_size) + _size)) = next;
    }

    void *get_pointer() { return &_memory; }
    uint16_t get_left_next() const { return *(&_size - 1); }
    bool is_left_free() const { return get_left_next() != 0; }
    bool is_free() const { return _size != GUARD_BLOCK_SIZE && get_next() != 0; }

private:
    enum
    {
        GUARD_BLOCK_SIZE = 0xffff,
    };

    uint16_t _size;

    uint8_t _memory[sizeof(uint16_t)];
};

class Heap
{
public:
    Heap();

    void *calloc(size_t count, size_t size);
    void free(void *ptr);

    bool is_clean() const
    {
        Heap &self = *const_cast<Heap *>(this);
        const Block &super = self.block_super();
        const Block &first = self.block_right(super);
        return super.get_next() == self.block_offset(first) && first.get_size() == FIRST_BLOCK_SIZE;
    }

    size_t get_capacity() const { return FIRST_BLOCK_SIZE; }
    size_t get_free_size() const { return _memory.mfree_size; }

private:
    enum
    {
        MEMORY_SIZE = VCRTOS_CONFIG_HEAP_SIZE,
        ALIGN_SIZE = sizeof(void *),
        BLOCK_REMAINDER_SIZE = ALIGN_SIZE - sizeof(uint16_t) * 2,
        SUPER_BLOCK_SIZE = ALIGN_SIZE - sizeof(Block),
        FIRST_BLOCK_SIZE = MEMORY_SIZE - ALIGN_SIZE * 3 + BLOCK_REMAINDER_SIZE,
        SUPER_BLOCK_OFFSET = ALIGN_SIZE - sizeof(uint16_t),
        FIRST_BLOCK_OFFSET = ALIGN_SIZE * 2 - sizeof(uint16_t),
        GUARD_BLOCK_OFFSET = MEMORY_SIZE - sizeof(uint16_t),
    };

    static_assert(MEMORY_SIZE % ALIGN_SIZE == 0, "The memory size is not aligned to ALIGN_SIZE!");

    Block &block_at(uint16_t offset) { return *reinterpret_cast<Block *>(&_memory.m16[offset / 2]); }

    Block &block_of(void *ptr)
    {
        uint16_t offset = static_cast<uint16_t>(reinterpret_cast<uint8_t *>(ptr) - _memory.m8);
        offset -= sizeof(uint16_t);
        return block_at(offset);
    }

    Block &block_super() { return block_at(SUPER_BLOCK_OFFSET); }

    Block &block_next(const Block &block) { return block_at(block.get_next()); }

    Block &block_right(const Block &block)
    {
        return block_at(block_offset(block) + sizeof(Block) + block.get_size());
    }

    Block &block_prev(const Block &block);

    bool is_left_free(const Block &block)
    {
        return (block_offset(block) != FIRST_BLOCK_OFFSET && block.is_left_free());
    }

    uint16_t block_offset(const Block &block)
    {
        return static_cast<uint16_t>(reinterpret_cast<const uint8_t *>(&block) - _memory.m8);
    }

    void block_insert(Block &prev, Block &block);

    union
    {
        uint16_t mfree_size;
        long mlong[MEMORY_SIZE / sizeof(long)];
        uint8_t m8[MEMORY_SIZE];
        uint16_t m16[MEMORY_SIZE / sizeof(uint16_t)];
    } _memory;
};

} // namespace utils
} // namespace vc

#endif /* VCRTOS_HEAP_HPP */
