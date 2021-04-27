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

#ifndef UTILS_ISRPIPPE_HPP
#define UTILS_ISRPIPPE_HPP

#include <stdint.h>

#include <vcrtos/config.h>
#include <vcrtos/assert.h>

#include "core/mutex.hpp"

namespace vc {

namespace utils {

class Tsrb
{
public:
    explicit Tsrb(char *buf, unsigned int size)
        : _buf(buf)
        , _size(size)
        , _reads(0)
        , _writes(0)
    {
        vcassert((size != 0) && ((size & (~size + 1)) == size));
    }

    int get_one();
    int get(char *buf, size_t size);
    int drop(size_t size);
    int add_one(char byte);
    int add(const char *buf, size_t size);
    int avail() { return _writes - _reads; }
    int is_empty() const { return _reads == _writes; }
    int is_full() const { return (_writes - _reads) == _size; }
    int free() { return (_size - _writes + _reads); }
    void push(char byte) { _buf[_writes++ & (_size - 1)] = byte; }
    char pop() { return _buf[_reads++ & (_size - 1)]; }

private:
    char *_buf;
    unsigned int _size;
    volatile unsigned _reads;
    volatile unsigned _writes;
};

class Isrpipe
{
public:
    explicit Isrpipe(char *buf, unsigned int size);

    int write_one(char byte);
    int read(char *buf, size_t size);
    Mutex &get_mutex() { return _mutex; }
    Tsrb &get_tsrb() { return _tsrb; }

private:
    Mutex _mutex;
    Tsrb _tsrb;
};

class UartIsrpipe : public Isrpipe
{
public:
    UartIsrpipe()
        : Isrpipe(_buf, sizeof(_buf))
    {
    }

private:
    char _buf[VCRTOS_CONFIG_UTILS_UART_TSRB_ISRPIPE_SIZE];
};

} // namespace utils
} // namespace vc

#endif /* UTILS_ISRPIPPE_HPP */
