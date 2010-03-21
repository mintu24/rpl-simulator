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

#ifndef PHY_H_
#define PHY_H_

#include "../base.h"
#include "../node.h"


typedef struct phy_mobility_t {

    sim_time_t          trigger_time;
    sim_time_t          duration;
    coord_t             dest_x;
    coord_t             dest_y;

} phy_mobility_t;


    /* info that a node supporting PHY layer should store */
typedef struct phy_node_info_t {

    char *              name;
    coord_t             cx;
    coord_t             cy;

    percent_t           battery_level;
    percent_t           tx_power;
    bool                mains_powered;

    coord_t             mobility_start_x;
    coord_t             mobility_start_y;
    sim_time_t          mobility_start_time;
    sim_time_t          mobility_stop_time;
    double              mobility_cos_alpha;
    double              mobility_sin_alpha;
    double              mobility_speed;

    node_t **           neighbor_list;
    uint16              neighbor_count;

    phy_mobility_t **   mobility_list;
    uint16              mobility_count;

} phy_node_info_t;


    /* fields contained in a PHY message */
typedef struct phy_pdu_t {

    void *              sdu;

} phy_pdu_t;


extern uint16           phy_event_node_wake;
extern uint16           phy_event_node_kill;

extern uint16           phy_event_pdu_send;
extern uint16           phy_event_pdu_receive;

extern uint16           phy_event_neighbor_attach;
extern uint16           phy_event_neighbor_detach;

extern uint16           phy_event_change_mobility;


bool                    phy_init();
bool                    phy_done();

phy_pdu_t *             phy_pdu_create();
void                    phy_pdu_destroy(phy_pdu_t *pdu);
phy_pdu_t *             phy_pdu_duplicate(phy_pdu_t *pdu);
void                    phy_pdu_set_sdu(phy_pdu_t *pdu, void *sdu);

void                    phy_node_init(node_t *node, char *name, coord_t cx, coord_t cy);
void                    phy_node_done(node_t *node);

void                    phy_node_set_name(node_t *node, const char *name);
void                    phy_node_set_coords(node_t* node, coord_t cx, coord_t cy);
void                    phy_node_set_tx_power(node_t* node, percent_t tw_power);
void                    phy_node_add_mobility(node_t *node, sim_time_t trigger_time, sim_time_t duration, coord_t dest_x, coord_t dest_y);
void                    phy_node_rem_mobility(node_t *node, uint16 index);
void                    phy_node_update_mobility_coords(node_t *node);
void                    phy_node_update_neighbors(node_t *node);

bool                    phy_node_send(node_t *node, node_t *outgoing_node, void *sdu);
bool                    phy_node_receive(node_t *node, node_t *incoming_node, phy_pdu_t *pdu);

bool                    phy_node_add_neighbor(node_t* node, node_t* neighbor_node);
bool                    phy_node_rem_neighbor(node_t* node, node_t *neighbor_node);
bool                    phy_node_has_neighbor(node_t* node, node_t *neighbor_node);

#endif /* PHY_H_ */
