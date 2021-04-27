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

#ifndef CORE_CIB_HPP
#define CORE_CIB_HPP

#include <vcrtos/assert.h>
#include <vcrtos/cib.h>

namespace vc {

class Cib : public cib_t
{
public:
    explicit Cib(unsigned int size) { init(size); }

    void init(unsigned int size)
    {
        vcassert(!(size & (size - 1)));
        read_count = 0;
        write_count = 0;
        mask = size - 1;
    }

    unsigned int avail() { return write_count - read_count; }
    unsigned int get_read_count() { return read_count; }
    unsigned int get_write_count() { return write_count; }
    unsigned int get_mask() { return mask; }
    int get_unsafe() { return static_cast<int>(read_count++ & mask); }
    int put_unsafe() { return static_cast<int>(write_count++ & mask); }
    int full() { return avail() > mask; }

    int get()
    {
        if (avail())
        {
            return static_cast<int>(read_count++ & mask);
        }
        return -1;
    }

    int peek()
    {
        if (avail())
        {
            return static_cast<int>(read_count & mask);
        }
        return -1;
    }

    int put()
    {
        int available = avail();
        if (available <= static_cast<int>(mask))
        {
            return static_cast<int>(write_count++ & mask);
        }
        return -1;
    }
};

} // namespace vc

#endif /* CORE_COMMON_CIB_HPP */
