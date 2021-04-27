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

#include "utils/isrpipe.hpp"

namespace vc {
namespace utils {

Isrpipe::Isrpipe(char *buf, unsigned int size)
    : _mutex()
    , _tsrb(buf, size)
{
}

int Isrpipe::write_one(char byte)
{
    int res = get_tsrb().add_one(byte);
    get_mutex().unlock();
    return res;
}

int Isrpipe::read(char *buf, size_t size)
{
    int res;
#ifdef UNITTEST
    if (!(res = get_tsrb().get(buf, size)))
    {
        get_mutex().lock();
    }
#else
    while (!(res = get_tsrb().get(buf, size)))
    {
        get_mutex().lock();
    }
#endif
    return res;
}

int Tsrb::get_one()
{
    if (!is_empty())
    {
        return pop();
    }
    else
    {
        return -1;
    }
}

int Tsrb::get(char *buf, size_t size)
{
    size_t tmp = size;
    while (tmp && !is_empty())
    {
        *buf++ = pop();
        tmp--;
    }
    return (size - tmp);
}

int Tsrb::drop(size_t size)
{
    size_t tmp = size;
    while (tmp && !is_empty())
    {
        pop();
        tmp--;
    }
    return (size - tmp);
}

int Tsrb::add_one(char byte)
{
    if (!is_full())
    {
        push(byte);
        return 0;
    }
    else
    {
        return -1;
    }
}

int Tsrb::add(const char *buf, size_t size)
{
    size_t tmp = size;
    while (tmp && !is_full())
    {
        push(*buf++);
        tmp--;
    }
    return (size - tmp);
}

} // namespace utils
} // namespace vc
