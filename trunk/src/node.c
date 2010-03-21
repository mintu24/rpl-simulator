/*
   RPL Simulator.

   Copyright (c) Calin Crisan 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <unistd.h>

#include "node.h"
#include "system.h"


    /**** local function prototypes ****/

    /**** exported functions ****/

node_t *node_create()
{
    node_t *node = malloc(sizeof(node_t));

    node->measure_info = NULL;
    node->phy_info = NULL;
    node->mac_info = NULL;
    node->ip_info = NULL;
    node->icmp_info = NULL;
    node->rpl_info = NULL;

    node->alive = FALSE;
    return node;
}

bool node_destroy(node_t *node)
{
    rs_assert(node != NULL);

    if (node->alive) {
        if (!node_kill(node)) {
            rs_error("failed to kill node '%s'", node->phy_info->name);
            return FALSE;
        }
    }

    rpl_node_done(node);
    icmp_node_done(node);
    ip_node_done(node);
    mac_node_done(node);
    phy_node_done(node);
    measure_node_done(node);

    free(node);

    return TRUE;
}

bool node_wake(node_t* node)
{
    rs_assert(node != NULL);

    if (node->alive) {
        rs_error("node '%s': already alive", node->phy_info->name);
        return FALSE;
    }

    node->alive = TRUE;

    rs_system_schedule_event(node, sys_event_node_wake, NULL, NULL, 0);

    return TRUE;
}

bool node_kill(node_t* node)
{
    rs_assert(node != NULL);

    if (!node->alive) {
        rs_error("node '%s': already dead", node->phy_info->name);
        return FALSE;
    }

    if (rs_system->started) {
        rs_system_schedule_event(node, sys_event_node_kill, NULL, NULL, 0);
    }
    else {
        event_execute(sys_event_node_kill, node, NULL, NULL);
    }

    node->alive = FALSE;

    return TRUE;
}


/**** local functions ****/

