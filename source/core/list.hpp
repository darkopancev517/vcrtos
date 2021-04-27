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

#ifndef CORE_LIST_HPP
#define CORE_LIST_HPP

#include <vcrtos/list.h>

namespace vc {

class List : public list_node_t
{
public:
    List() { next = nullptr; }

    void add(List *node)
    {
        node->next = this->next;
        this->next = node;
    }

    List *remove_head()
    {
        List *head = static_cast<List *>(this->next);
        if (head)
        {
            this->next = head->next;
        }
        return head;
    }

    static List *remove(List *list, List *node)
    {
        while (list->next)
        {
            if (list->next == node)
            {
                list->next = node->next;
                return node;
            }
            list = static_cast<List *>(list->next);
        }
        return static_cast<List *>(list->next);
    }
};

} // namespace vc

#endif /* CORE_LIST_HPP */
