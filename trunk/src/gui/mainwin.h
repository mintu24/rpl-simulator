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

void                main_win_update_sim_status();
void                main_win_update_nodes_status();
void                main_win_update_sim_time_status();
void                main_win_update_xy_status(coord_t x, coord_t y);

    /* events */
void                main_win_event_after_node_wake(node_t *node);
void                main_win_event_before_node_kill(node_t *node);


#endif /* MAINWIN_H_ */
