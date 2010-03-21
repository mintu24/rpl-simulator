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

#ifndef MAIN_H_
#define MAIN_H_

#include "base.h"
#include "node.h"

#define                 AUTOINC_ADDRESS_PART        16

#define                 SCENARIO_FILE_EXT           ".scenario"

void                    rs_new();
char *                  rs_open();
char *                  rs_save();
void                    rs_quit();

void                    rs_start();
void                    rs_pause();
void                    rs_step();
void                    rs_stop();

node_t *                rs_add_node(coord_t x, coord_t y);
void                    rs_rem_node(node_t *node);
void                    rs_wake_node(node_t *node);
void                    rs_kill_node(node_t *node);

void                    rs_add_more_nodes(uint16 node_number, uint8 pattern, coord_t horiz_dist, coord_t vert_dist, uint16 row_length);
void                    rs_rem_all_nodes();
void                    rs_wake_all_nodes();
void                    rs_kill_all_nodes();

#endif /* MAIN_H_ */
