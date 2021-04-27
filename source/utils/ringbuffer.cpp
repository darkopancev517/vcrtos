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

#include <string.h>

#include "utils/ringbuffer.hpp"

namespace vc {
namespace utils {

void RingBuffer::add_tail(char byte)
{
    unsigned pos = _start + _avail++;
    if (pos >= _size)
    {
        pos -= _size;
    }
    _buf[pos] = byte;
}

char RingBuffer::get_head()
{
    char result = _buf[_start];
    if ((--_avail == 0) || (++_start == _size))
    {
        _start = 0;
    }
    return result;
}

int RingBuffer::add_one(char byte)
{
    int result = -1;
    if (is_full())
    {
        result = (unsigned char)get_head();
    }
    add_tail(byte);
    return result;
}

unsigned RingBuffer::add(const char *buf, unsigned size)
{
    unsigned i;
    for (i = 0; i < size; i++)
    {
        if (is_full())
            break;

        add_tail(buf[i]);
    }
    return i;
}

int RingBuffer::get_one()
{
    if (!is_empty())
    {
        return (unsigned char)get_head();
    }
    else
    {
        return -1;
    }
}

unsigned RingBuffer::get(char *buf, unsigned size)
{
    if (size > _avail)
        size = _avail;

    if (size > 0)
    {
        unsigned bytes_till_end = _size - _start;

        if (bytes_till_end >= size)
        {
            memcpy(buf, _buf + _start, size);

            if (bytes_till_end == size)
            {
                _start = 0;
            }
            else
            {
                _start += size;
            }
        }
        else
        {
            memcpy(buf, _buf + _start, bytes_till_end);
            _start = size - bytes_till_end;
            memcpy(buf + bytes_till_end, _buf, _start);
        }

        _avail -= size;
    }
    return size;
}

unsigned RingBuffer::remove(unsigned size)
{
    if (size > _avail)
    {
        size = _avail;
        _start = _avail = 0;
    }
    else
    {
        _start += size;
        _avail -= size;

        /* compensate undeflow */
        if (_start > _size)
        {
            _start -= _size;
        }
    }

    return size;
}

int RingBuffer::peek_one()
{
    RingBuffer rb = *this;
    return rb.get_one();
}

unsigned RingBuffer::peek(char *buf, unsigned size)
{
    RingBuffer rb = *this;
    return rb.get(buf, size);
}

} // namespace utils
} // namespace vc
