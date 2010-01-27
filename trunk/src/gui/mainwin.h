#ifndef MAINWIN_H_
#define MAINWIN_H_

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>

#include "../base.h"
#include "../node.h"

#define MAIN_WIN_WIDTH                              1200
#define MAIN_WIN_HEIGHT	                            750

#ifndef gtk_widget_set_visible
#define gtk_widget_set_visible(widget, visible)     { if (visible) gtk_widget_show(widget); else gtk_widget_hide(widget); }
#endif


typedef struct display_params_t {

    bool            show_node_names;
    bool            show_node_addresses;
    bool            show_node_tx_power;
    bool            show_node_ranks;
    bool            show_parent_arrows;
    bool            show_sibling_arrows;

} display_params_t;


void                main_win_init();

node_t *            main_win_get_selected_node();
void                main_win_set_selected_node(node_t *node);

void                main_win_system_to_gui();
void                main_win_node_to_gui(node_t *node);
void                main_win_display_to_gui();

display_params_t *  main_win_get_display_params();

    /* events */
void                main_win_event_after_node_wake(node_t *node);
void                main_win_event_before_node_kill(node_t *node);


#endif /* MAINWIN_H_ */
