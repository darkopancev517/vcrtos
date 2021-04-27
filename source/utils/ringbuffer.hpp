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

#ifndef UTILS_RINGBUFFER_HPP
#define UTILS_RINGBUFFER_HPP

#include <vcrtos/config.h>

namespace vc {
namespace utils {

class RingBuffer
{
public:
    explicit RingBuffer(char *buf, unsigned size)
        : _buf(buf)
        , _size(size)
        , _start(0)
        , _avail(0)
    {
    }

    int add_one(char byte);
    unsigned add(const char *buf, unsigned size);
    int get_one();
    unsigned get(char *buf, unsigned size);
    unsigned remove(unsigned size);
    int is_empty() { return _avail == 0; }
    int is_full() { return _avail == _size; }
    unsigned int free() { return _size - _avail; }
    unsigned int avail() { return _avail; }
    int peek_one();
    unsigned peek(char *buf, unsigned size);

private:
    void add_tail(char byte);
    char get_head();

    char *_buf;
    unsigned int _size;
    unsigned int _start;
    unsigned int _avail;
};

} // namespace utils
} // namespace vc

#endif /* UTILS_RINGBUFFER_HPP */
