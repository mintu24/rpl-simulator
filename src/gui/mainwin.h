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

#ifndef MAINWIN_H_
#define MAINWIN_H_

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "../base.h"
#include "../node.h"

#define MAIN_WIN_WIDTH                              1200
#define MAIN_WIN_HEIGHT	                            750

#define MAIN_WIN_TITLE                              "RPL Simulator [%s]"
#define MAIN_WIN_TITLE_UNSAVED                      "unsaved scenario"
#define MAIN_WIN_ICON                               "icon.png"

#ifndef gtk_widget_set_visible /* gtk <=2.16 compatibility */
#define gtk_widget_set_visible(widget, visible)     { if (visible) gtk_widget_show(widget); else gtk_widget_hide(widget); }
#endif


#define MAIN_WIN_NODE_TO_GUI_MEASURE                (1 << 0)
#define MAIN_WIN_NODE_TO_GUI_PHY                    (1 << 1)
#define MAIN_WIN_NODE_TO_GUI_MAC                    (1 << 2)
#define MAIN_WIN_NODE_TO_GUI_IP                     (1 << 3)
#define MAIN_WIN_NODE_TO_GUI_ICMP                   (1 << 4)
#define MAIN_WIN_NODE_TO_GUI_RPL                    (1 << 5)
#define MAIN_WIN_NODE_TO_GUI_ALL                    0xFFFF


typedef struct display_params_t {

    bool            show_node_names;
    bool            show_node_addresses;
    bool            show_node_tx_power;
    bool            show_node_ranks;
    bool            show_parent_arrows;
    bool            show_preferred_parent_arrows;
    bool            show_sibling_arrows;

} display_params_t;


extern GtkBuilder * gtk_builder;


bool                main_win_init();

node_t *            main_win_get_selected_node();
void                main_win_set_selected_node(node_t *node);

void                main_win_system_to_gui();
void                main_win_node_to_gui(node_t *node, uint32 what);
void                main_win_display_to_gui();
void                main_win_events_to_gui();

display_params_t *  main_win_get_display_params();

void                main_win_add_log_line(uint32 no, char *str_time, char *node_name, char *layer, char *event_name, char *str1, char *str2);
void                main_win_clear_log();

void                main_win_update_sim_status();
void                main_win_update_nodes_status();
void                main_win_update_sim_time_status();
void                main_win_update_xy_status(coord_t x, coord_t y);


#endif /* MAINWIN_H_ */
