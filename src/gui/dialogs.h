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

#ifndef DIALOGS_H_
#define DIALOGS_H_

#include <gtk/gtk.h>

#include "../base.h"

#define ADD_MORE_DIALOG_PATTERN_RECTANGULAR         0
#define ADD_MORE_DIALOG_PATTERN_TRIANGULAR          1
#define ADD_MORE_DIALOG_PATTERN_RANDOM              2


typedef struct add_more_dialog_info_t {

    uint16          node_number;
    uint8           pattern;
    coord_t         horiz_dist;
    coord_t         vert_dist;
    uint16          row_length;

} add_more_dialog_info_t;


bool                            dialogs_init();
bool                            dialogs_done();

add_more_dialog_info_t *        add_more_dialog_run();

#endif /* DIALOGS_H_ */
