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

#ifndef CORE_CLIST_HPP
#define CORE_CLIST_HPP

#include <stddef.h>

#include <vcrtos/clist.h>

#include "core/list.hpp"

namespace vc {

class Clist : public List
{
public:
    Clist() { next = nullptr; }

    void right_push(Clist *node)
    {
        if (this->next)
        {
            node->next = this->next->next;
            this->next->next = node;
        }
        else
        {
            node->next = node;
        }
        this->next = node;
    }

    void left_push(Clist *node)
    {
        if (this->next)
        {
            node->next = this->next->next;
            this->next->next = node;
        }
        else
        {
            node->next = node;
            this->next = node;
        }
    }

    Clist *left_pop()
    {
        if (this->next)
        {
            Clist *first = static_cast<Clist *>(this->next->next);

            if (this->next == first)
            {
                this->next = nullptr;
            }
            else
            {
                this->next->next = first->next;
            }

            return first;
        }
        else
        {
            return nullptr;
        }
    }

    void left_pop_right_push()
    {
        if (this->next)
        {
            this->next = this->next->next;
        }
    }

    Clist *left_peek()
    {
        if (this->next)
        {
            return static_cast<Clist *>(this->next->next);
        }

        return nullptr;
    }

    Clist *right_peek() { return static_cast<Clist *>(this->next); }

    Clist *right_pop()
    {
        if (this->next)
        {
            List *last = static_cast<List *>(this->next);

            while (this->next->next != last)
            {
                this->left_pop_right_push();
            }

            return this->left_pop();
        }
        else
        {
            return nullptr;
        }
    }

    Clist *find_before(const Clist *node)
    {
        Clist *pos = static_cast<Clist *>(this->next);

        if (!pos)
            return nullptr;

        do
        {
            pos = static_cast<Clist *>(pos->next);

            if (pos->next == node)
            {
                return pos;
            }

        } while (pos != this->next);

        return nullptr;
    }

    Clist *find(const Clist *node)
    {
        Clist *tmp = this->find_before(node);

        if (tmp)
        {
            return static_cast<Clist *>(tmp->next);
        }
        else
        {
            return nullptr;
        }
    }

    Clist *remove(Clist *node)
    {
        if (this->next)
        {
            if (this->next->next == node)
            {
                return this->left_pop();
            }
            else
            {
                Clist *tmp = this->find_before(node);

                if (tmp)
                {
                    tmp->next = tmp->next->next;

                    if (node == this->next)
                    {
                        this->next = tmp;
                    }
                    return node;
                }
            }
        }
        return nullptr;
    }

    size_t count()
    {
        Clist *node = static_cast<Clist *>(this->next);
        size_t cnt  = 0;
        if (node)
        {
            do
            {
                node = static_cast<Clist *>(node->next);
                ++cnt;
            } while (node != this->next);
        }
        return cnt;
    }
};

} // namespace vc

#endif /* CORE_CLIST_HPP */
