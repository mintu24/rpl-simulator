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

#ifndef NODE_H_
#define NODE_H_

#include "base.h"


    /* a node in the simulated network */
typedef struct node_t {

    struct measure_node_info_t *measure_info;
    struct phy_node_info_t *    phy_info;
    struct mac_node_info_t *    mac_info;
    struct ip_node_info_t *     ip_info;
    struct icmp_node_info_t *   icmp_info;
    struct rpl_node_info_t *    rpl_info;

    bool                        alive;

} node_t;


node_t *                        node_create();
bool                            node_destroy(node_t* node);

bool                            node_wake(node_t* node);
bool                            node_kill(node_t* node);


#endif /* NODE_H_ */
