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

#ifndef EVENT_H_
#define EVENT_H_

#include "base.h"
#include "node.h"


    /* a callback type representing an event handler */
typedef bool (* event_handler_t) (node_t *node, void *data1, void *data2);

    /* a callback type representing an event handler */
typedef void (* event_arg_str_t) (uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len);

    /* structure representing an event (not to be confused with an event schedule) */
typedef struct event_t {

    char *                      name;
    char *                      layer;
    event_handler_t             handler;
    event_arg_str_t             str_func;
    bool                        loggable;

} event_t;


void					event_init();
void					event_done();

uint16                  event_register(char *name, char *layer, event_handler_t handler, event_arg_str_t str_func);
bool                    event_execute(uint16 event_id, node_t *node, void *data1, void *data2);

event_t                 event_find_by_id(uint16 event_id);
uint16                  event_get_count();

void                    event_set_logging(uint16 event_id, bool loggable);
bool                    event_get_logging(uint16 event_id);
void					event_set_log_file(char *filename);

#endif /* EVENT_H_ */
