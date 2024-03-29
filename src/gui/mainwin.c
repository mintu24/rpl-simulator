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

#include <gdk/gdk.h>
#include <cairo.h>
#include <libgen.h> /* for basename() */

#include "mainwin.h"

#include "simfield.h"
#include "legend.h"
#include "dialogs.h"

#include "../main.h"
#include "../system.h"

#define signal_enter()          { if (signals_disabled) return; signals_disabled = TRUE; }
#define signal_leave()          { signals_disabled = FALSE; }

#define signals_disable()       { signals_disabled = TRUE; }
#define signals_enable()        { signals_disabled = FALSE; }


    /**** global variables ****/

GtkBuilder *                    gtk_builder = NULL;

static display_params_t         display_params;
static GtkWidget *              main_window = NULL;
static GtkWidget *              legend_widget = NULL;
static GtkWidget *              sim_status_bar = NULL;
static GtkWidget *              nodes_status_bar = NULL;
static GtkWidget *              sim_time_status_bar = NULL;
static GtkWidget *              xy_status_bar = NULL;


    /* params widgets */
static GtkWidget *              params_config_button = NULL;
static GtkWidget *              params_config_vbox = NULL;

static GtkWidget *              params_system_auto_wake_check = NULL;
static GtkWidget *              params_system_deterministic_random_check = NULL;
static GtkWidget *              params_system_real_time_sim_check = NULL;
static GtkWidget *              params_system_sim_second_spin = NULL;

static GtkWidget *              params_system_width_spin = NULL;
static GtkWidget *              params_system_height_spin = NULL;
static GtkWidget *              params_system_no_link_dist_spin = NULL;
static GtkWidget *              params_system_no_link_quality_spin = NULL;
static GtkWidget *              params_system_transmission_time_spin = NULL;

static GtkWidget *              params_system_mac_pdu_timeout_spin = NULL;

static GtkWidget *              params_system_ip_queue_size = NULL;
static GtkWidget *              params_system_ip_pdu_timeout_spin = NULL;
static GtkWidget *              params_system_ip_neighbor_timeout_spin = NULL;

static GtkWidget *              params_rpl_trickle_min_spin = NULL;
static GtkWidget *              params_rpl_trickle_doublings_spin = NULL;
static GtkWidget *              params_rpl_trickle_redundancy_spin = NULL;
static GtkWidget *              params_rpl_dao_root_delay_spin = NULL;
static GtkWidget *              params_rpl_dao_remove_timeout_spin = NULL;
static GtkWidget *              params_rpl_max_rank_inc_spin = NULL;
static GtkWidget *              params_rpl_dao_supported_check = NULL;
static GtkWidget *              params_rpl_dao_trigger_check = NULL;
static GtkWidget *              params_rpl_probe_check = NULL;
static GtkWidget *              params_rpl_prefer_floating_check = NULL;
static GtkWidget *              params_rpl_autoinc_sn_check = NULL;
static GtkWidget *              params_rpl_autoinc_sn_spin = NULL;
static GtkWidget *              params_rpl_poison_count_spin = NULL;

static GtkWidget *              params_system_measure_pdu_timeout_spin = NULL;

static GtkWidget *              params_display_show_node_names_check = NULL;
static GtkWidget *              params_display_show_node_addresses_check = NULL;
static GtkWidget *              params_display_show_node_tx_power_check = NULL;
static GtkWidget *              params_display_show_node_ranks_check = NULL;
static GtkWidget *              params_display_show_preferred_parent_arrows_check = NULL;
static GtkWidget *              params_display_show_parent_arrows_check = NULL;
static GtkWidget *              params_display_show_sibling_arrows_check = NULL;

static GtkWidget *              params_events_sys_node_wake_check = NULL;
static GtkWidget *              params_events_sys_node_kill_check = NULL;
static GtkWidget *              params_events_sys_pdu_receive_check = NULL;
static GtkWidget *              params_events_phy_node_wake_check = NULL;
static GtkWidget *              params_events_phy_node_kill_check = NULL;
static GtkWidget *              params_events_phy_pdu_send_check = NULL;
static GtkWidget *              params_events_phy_pdu_receive_check = NULL;
static GtkWidget *              params_events_phy_neighbor_attach_check = NULL;
static GtkWidget *              params_events_phy_neighbor_detach_check = NULL;
static GtkWidget *              params_events_phy_change_mobility_check = NULL;
static GtkWidget *              params_events_mac_node_wake_check = NULL;
static GtkWidget *              params_events_mac_node_kill_check = NULL;
static GtkWidget *              params_events_mac_pdu_send_check = NULL;
static GtkWidget *              params_events_mac_pdu_send_timeout_check_check = NULL;
static GtkWidget *              params_events_mac_pdu_receive_check = NULL;
static GtkWidget *              params_events_ip_node_wake_check = NULL;
static GtkWidget *              params_events_ip_node_kill_check = NULL;
static GtkWidget *              params_events_ip_pdu_send_check = NULL;
static GtkWidget *              params_events_ip_pdu_send_timeout_check_check = NULL;
static GtkWidget *              params_events_ip_pdu_receive_check = NULL;
static GtkWidget *              params_events_ip_neighbor_cache_timeout_check_check = NULL;
static GtkWidget *              params_events_icmp_node_wake_check = NULL;
static GtkWidget *              params_events_icmp_node_kill_check = NULL;
static GtkWidget *              params_events_icmp_pdu_send_check = NULL;
static GtkWidget *              params_events_icmp_pdu_receive_check = NULL;
static GtkWidget *              params_events_icmp_ping_request_check = NULL;
static GtkWidget *              params_events_icmp_ping_reply_check = NULL;
static GtkWidget *              params_events_icmp_ping_timeout_check = NULL;
static GtkWidget *              params_events_rpl_node_wake_check = NULL;
static GtkWidget *              params_events_rpl_node_kill_check = NULL;
static GtkWidget *              params_events_rpl_dis_pdu_send_check = NULL;
static GtkWidget *              params_events_rpl_dis_pdu_receive_check = NULL;
static GtkWidget *              params_events_rpl_dio_pdu_send_check = NULL;
static GtkWidget *              params_events_rpl_dio_pdu_receive_check = NULL;
static GtkWidget *              params_events_rpl_dao_pdu_send_check = NULL;
static GtkWidget *              params_events_rpl_dao_pdu_receive_check = NULL;
static GtkWidget *              params_events_rpl_neighbor_attach_check = NULL;
static GtkWidget *              params_events_rpl_neighbor_detach_check = NULL;
static GtkWidget *              params_events_rpl_forward_failure_check = NULL;
static GtkWidget *              params_events_rpl_forward_inconsistency_check = NULL;
static GtkWidget *              params_events_rpl_trickle_t_timeout_check = NULL;
static GtkWidget *              params_events_rpl_trickle_i_timeout_check = NULL;
static GtkWidget *              params_events_rpl_dao_send_check = NULL;
static GtkWidget *              params_events_rpl_dao_timeout_check_check = NULL;
static GtkWidget *              params_events_rpl_seq_num_autoinc_check = NULL;
static GtkWidget *              params_events_measure_node_wake_check = NULL;
static GtkWidget *              params_events_measure_node_kill_check = NULL;
static GtkWidget *              params_events_measure_pdu_send_check = NULL;
static GtkWidget *              params_events_measure_pdu_receive_check = NULL;
static GtkWidget *              params_events_measure_connect_update_check = NULL;
static GtkWidget *              params_events_measure_connect_hop_passed_check = NULL;
static GtkWidget *              params_events_measure_connect_hop_failed_check = NULL;
static GtkWidget *              params_events_measure_connect_hop_timeout_check = NULL;
static GtkWidget *              params_events_measure_connect_established_check = NULL;
static GtkWidget *              params_events_measure_connect_lost_check = NULL;

static GtkWidget *              params_nodes_button = NULL;
static GtkWidget *              params_nodes_vbox = NULL;

static GtkWidget *              params_nodes_name_entry = NULL;
static GtkWidget *              params_nodes_x_spin = NULL;
static GtkWidget *              params_nodes_y_spin = NULL;
static GtkWidget *              params_nodes_tx_power_spin = NULL;
static GtkWidget *              params_nodes_bat_level_spin = NULL;
static GtkWidget *              params_nodes_mains_powered_check = NULL;
static GtkWidget *              params_nodes_mobility_trigger_time_spin = NULL;
static GtkWidget *              params_nodes_mobility_duration_spin = NULL;
static GtkWidget *              params_nodes_mobility_dx_spin = NULL;
static GtkWidget *              params_nodes_mobility_dy_spin = NULL;
static GtkWidget *              params_nodes_mobility_tree_view = NULL;
static GtkWidget *              params_nodes_mobility_add_button = NULL;
static GtkWidget *              params_nodes_mobility_rem_button = NULL;
static GtkListStore *           params_nodes_mobility_store = NULL;

static GtkWidget *              params_nodes_mac_address_entry = NULL;

static GtkWidget *              params_nodes_ip_address_entry = NULL;
static GtkWidget *              params_nodes_route_dst_combo = NULL;
static GtkWidget *              params_nodes_route_prefix_len_spin = NULL;
static GtkWidget *              params_nodes_route_next_hop_combo = NULL;
static GtkWidget *              params_nodes_route_add_button = NULL;
static GtkWidget *              params_nodes_route_rem_button = NULL;
static GtkWidget *              params_nodes_route_tree_view = NULL;
static GtkListStore *           params_nodes_route_dst_store = NULL;
static GtkListStore *           params_nodes_route_next_hop_store = NULL;
static GtkListStore *           params_nodes_route_store = NULL;

static GtkWidget *              params_nodes_enable_ping_measurements_check = NULL;
static GtkWidget *              params_nodes_ping_interval_spin = NULL;
static GtkWidget *              params_nodes_ping_timeout_spin = NULL;
static GtkWidget *              params_nodes_ping_address_combo = NULL;
static GtkListStore *           params_nodes_ping_address_store = NULL;

static GtkWidget *              params_nodes_storing_check = NULL;
static GtkWidget *              params_nodes_grounded_check = NULL;
static GtkWidget *              params_nodes_dao_enabled_check = NULL;
static GtkWidget *              params_nodes_dao_trigger_check = NULL;
static GtkWidget *              params_nodes_dag_pref_spin = NULL;
static GtkWidget *              params_nodes_dag_id_entry = NULL;
static GtkWidget *              params_nodes_seq_num_entry = NULL;
static GtkWidget *              params_nodes_rank_entry = NULL;
static GtkWidget *              params_nodes_trickle_int_entry = NULL;
static GtkWidget *              params_nodes_root_button = NULL;
static GtkWidget *              params_nodes_isolate_button = NULL;

static GtkWidget *              params_nodes_measure_connect_dst_combo;
static GtkWidget *              params_nodes_measure_connect_connected_now_label;
static GtkWidget *              params_nodes_measure_connect_progress;
static GtkListStore *           params_nodes_measure_connect_dst_store;

static GtkWidget *              params_nodes_measure_stat_forward_inconsistencies_label;
static GtkWidget *              params_nodes_measure_stat_forward_failures_label;
static GtkWidget *              params_nodes_measure_stat_rpl_dis_messages_label;
static GtkWidget *              params_nodes_measure_stat_rpl_dio_messages_label;
static GtkWidget *              params_nodes_measure_stat_rpl_dao_messages_label;
static GtkWidget *              params_nodes_measure_stat_rpl_parents_siblings_label;
static GtkWidget *              params_nodes_measure_stat_ping_progress;
static GtkWidget *              params_nodes_measure_stat_ip_packets_label;
static GtkWidget *              params_nodes_measure_stat_queued_ip_packets_label;

static GtkWidget *              params_nodes_measure_converg_connected_progress;
static GtkWidget *              params_nodes_measure_converg_stable_progress;
static GtkWidget *              params_nodes_measure_converg_floating_progress;



//    /* measures widgets */

//static GtkWidget *              measures_button = NULL;
//static GtkWidget *              measures_vbox = NULL;


    /* other widgets */

static GtkWidget *              main_win_first_hpaned;
static GtkWidget *              main_win_second_hpaned;

static GtkWidget *              log_tree_view;
static GtkListStore *           log_store;
static GtkWidget *              log_to_console_check;
static GtkWidget *              log_events_to_keep_spin;

static GtkWidget *              new_menu_item = NULL;
static GtkWidget *              open_menu_item = NULL;
static GtkWidget *              save_menu_item = NULL;
static GtkWidget *              save_as_menu_item = NULL;
static GtkWidget *              quit_menu_item = NULL;
static GtkWidget *              start_menu_item = NULL;
static GtkWidget *              pause_menu_item = NULL;
static GtkWidget *              step_menu_item = NULL;
static GtkWidget *              stop_menu_item = NULL;
static GtkWidget *              add_menu_item = NULL;
static GtkWidget *              rem_menu_item = NULL;
static GtkWidget *              wake_menu_item = NULL;
static GtkWidget *              kill_menu_item = NULL;
static GtkWidget *              add_all_menu_item = NULL;
static GtkWidget *              remove_all_menu_item = NULL;
static GtkWidget *              wake_all_menu_item = NULL;
static GtkWidget *              kill_all_menu_item = NULL;
static GtkWidget *              about_menu_item = NULL;

static GtkWidget *              add_node_toolbar_item = NULL;
static GtkWidget *              rem_node_toolbar_item = NULL;
static GtkWidget *              start_toolbar_item = NULL;
static GtkWidget *              pause_toolbar_item = NULL;
static GtkWidget *              step_toolbar_item = NULL;
static GtkWidget *              stop_toolbar_item = NULL;
static GtkWidget *              wake_toolbar_item = NULL;
static GtkWidget *              kill_toolbar_item = NULL;

static node_t *                 selected_node = NULL;

static bool                     signals_disabled = FALSE;


    /**** local function prototypes ****/

void                            cb_params_system_button_clicked(GtkWidget *button, gpointer data);
void                            cb_params_nodes_button_clicked(GtkWidget *button, gpointer data);
void                            cb_gui_system_updated(GtkSpinButton *spin, gpointer data);
void                            cb_gui_node_updated(GtkWidget *widget, gpointer data);
void                            cb_gui_display_updated(GtkWidget *widget, gpointer data);
void                            cb_gui_events_updated(GtkWidget *widget, gpointer data);
void                            cb_params_nodes_mobility_tree_view_cursor_changed(GtkWidget *widget, gpointer data);
void                            cb_params_nodes_route_tree_view_cursor_changed(GtkWidget *widget, gpointer data);
void                            cb_params_nodes_mobility_add_button_clicked(GtkButton *button, gpointer data);
void                            cb_params_nodes_mobility_rem_button_clicked(GtkButton *button, gpointer data);
void                            cb_params_nodes_route_add_button_clicked(GtkButton *button, gpointer data);
void                            cb_params_nodes_route_rem_button_clicked(GtkButton *button, gpointer data);
void                            cb_params_nodes_route_dst_combo_changed(GtkComboBoxEntry *combo_box, gpointer data);
void                            cb_params_nodes_root_button_clicked(GtkButton *button, gpointer data);
void                            cb_params_nodes_isolate_button_clicked(GtkButton *button, gpointer data);

static void                     cb_log_to_console_check_toggle(GtkWidget *widget, gpointer *data);
static void                     cb_new_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_open_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_save_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_save_as_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_quit_menu_item_activate(GtkMenuItem *widget, gpointer user_data);
static void                     cb_start_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_pause_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_step_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_stop_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_add_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_rem_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_wake_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_kill_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_add_all_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_remove_all_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_wake_all_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_kill_all_menu_item_activate(GtkWidget *widget, gpointer *data);
static void                     cb_about_menu_item_activate(GtkWidget *widget, gpointer *data);

static void                     cb_main_window_delete();

static GtkWidget *              create_params_widget();
static GtkWidget *              create_menu_bar();
static GtkWidget *              create_tool_bar();
static GtkWidget *              create_log_console();
static GtkWidget *              create_status_bar();
static GtkWidget *              create_content_widget();

static void                     initialize_widgets();
static void                     update_sensitivity();
static int32                    route_tree_viee_get_selected_index();
static int32                    mobility_tree_viee_get_selected_index();

static void                     gui_to_system();
static void                     gui_to_node(node_t *node);
static void                     gui_to_display();
static void                     gui_to_events();

static gboolean                 gui_update_wrapper(void *data);
static gboolean                 status_bar_update_wrapper(void *data);
static gboolean                 log_wrapper(void *data);

static void                     update_rpl_root_configurations();


    /**** exported functions ****/

bool main_win_init()
{
    display_params.show_node_names = TRUE;
    display_params.show_node_addresses = TRUE;
    display_params.show_node_tx_power = TRUE;
    display_params.show_node_ranks = TRUE;
    display_params.show_preferred_parent_arrows = TRUE;
    display_params.show_parent_arrows = TRUE;
    display_params.show_sibling_arrows = TRUE;

    gtk_builder = gtk_builder_new();

    /* glade/gtkbuilder UI */
    char path[256];
    snprintf(path, sizeof(path), "%s/%s/mainwin.glade", rs_app_dir, RES_DIR);

    GError *error = NULL;
    gtk_builder_add_from_file(gtk_builder, path, &error);
    if (error != NULL) {
        rs_error("failed to load params ui: %s", error->message);
        return FALSE;
    }

    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(main_window), MAIN_WIN_WIDTH, MAIN_WIN_HEIGHT);
    gtk_window_set_title(GTK_WINDOW(main_window), "RPL Simulator");
    gtk_signal_connect(GTK_OBJECT(main_window), "delete-event", GTK_SIGNAL_FUNC(cb_main_window_delete), NULL);

    /* icon */
    snprintf(path, sizeof(path), "%s/%s/%s", rs_app_dir, RES_DIR, MAIN_WIN_ICON);
    gtk_window_set_icon_from_file(GTK_WINDOW(main_window), path, NULL);

    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

    GtkWidget *menu_bar = create_menu_bar();
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, TRUE, 0);

    GtkWidget *tool_bar = create_tool_bar();
    gtk_box_pack_start(GTK_BOX(vbox), tool_bar, FALSE, TRUE, 0);

    GtkWidget *content_widget = create_content_widget();
    if (content_widget == NULL) {
        rs_error("failed to create content widget");
        return FALSE;
    }

    gtk_box_pack_start(GTK_BOX(vbox), content_widget, TRUE, TRUE, 0);

    GtkWidget *status_bar_box = create_status_bar();
    gtk_box_pack_start(GTK_BOX(vbox), status_bar_box, FALSE, TRUE, 0);

    if (!dialogs_init()) {
        rs_error("failed to initialize dialogs");
        return FALSE;
    }

    gtk_widget_show_all(main_window);

    main_win_system_to_gui();
    main_win_display_to_gui();
    main_win_events_to_gui();

    gtk_builder_connect_signals(gtk_builder, NULL);
    initialize_widgets();

    g_timeout_add(SIM_FIELD_REDRAW_INTERVAL, gui_update_wrapper, NULL);

    main_win_update_sim_status();
    main_win_update_nodes_status();
    main_win_update_sim_time_status();
    main_win_update_xy_status(0, 0);

    return TRUE;
}

node_t *main_win_get_selected_node()
{
    return selected_node;
}

void main_win_set_selected_node(node_t *node)
{
    selected_node = node;

    if (node != NULL) {
        main_win_node_to_gui(node, MAIN_WIN_NODE_TO_GUI_ALL);
    }

    sim_field_redraw();
    update_sensitivity();
}

void main_win_system_to_gui()
{
    signals_disable();

    rs_assert(rs_system != NULL);

    /* system */
    if (rs_system->simulation_second >= 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_system_real_time_sim_check), TRUE);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_sim_second_spin), rs_system->simulation_second);
    }
    else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_system_real_time_sim_check), FALSE);
    }

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_system_auto_wake_check), rs_system->auto_wake_nodes);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_system_deterministic_random_check), rs_system->deterministic_random);

    /* phy */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_width_spin), rs_system->width);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_height_spin), rs_system->height);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_no_link_dist_spin), rs_system->no_link_dist_thresh);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_no_link_quality_spin), rs_system->no_link_quality_thresh * 100);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_transmission_time_spin), rs_system->transmission_time);

    /* mac */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_mac_pdu_timeout_spin), rs_system->mac_pdu_timeout);

    /* ip */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_ip_queue_size), rs_system->ip_queue_size);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_ip_pdu_timeout_spin), rs_system->ip_pdu_timeout);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_ip_neighbor_timeout_spin), rs_system->ip_neighbor_timeout);

    /* rpl */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_rpl_trickle_min_spin), rs_system->rpl_dio_interval_min);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_rpl_trickle_doublings_spin), rs_system->rpl_dio_interval_doublings);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_rpl_trickle_redundancy_spin), rs_system->rpl_dio_redundancy_constant);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_rpl_dao_root_delay_spin), rs_system->rpl_dao_root_delay);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_rpl_dao_remove_timeout_spin), rs_system->rpl_dao_remove_timeout);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_rpl_max_rank_inc_spin), rs_system->rpl_max_inc_rank);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_rpl_dao_supported_check), rs_system->rpl_dao_supported);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_rpl_dao_trigger_check), rs_system->rpl_dao_trigger);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_rpl_probe_check), rs_system->rpl_startup_probe_for_dodags);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_rpl_prefer_floating_check), rs_system->rpl_prefer_floating);

    if (rs_system->rpl_auto_sn_inc_interval > 0) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_rpl_autoinc_sn_check), TRUE);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_rpl_autoinc_sn_spin), rs_system->rpl_auto_sn_inc_interval);
    }
    else {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_rpl_autoinc_sn_check), FALSE);
    }

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_rpl_poison_count_spin), rs_system->rpl_poison_count);

    /* measure */
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_measure_pdu_timeout_spin), rs_system->measure_pdu_timeout);

    /* add all the current possible next-hops,
     * and all the possible destination addresses */

    gtk_list_store_clear(params_nodes_route_next_hop_store);
    gtk_list_store_clear(params_nodes_route_dst_store);
    gtk_list_store_clear(params_nodes_ping_address_store);
    gtk_list_store_clear(params_nodes_measure_connect_dst_store);

    gtk_list_store_insert_with_values(params_nodes_measure_connect_dst_store, NULL, -1, 0, "Disabled", -1);

    nodes_lock();

    uint16 i;
    for (i = 0; i < rs_system->node_count; i++) {
        node_t *node = rs_system->node_list[i];

        gtk_list_store_insert_with_values(params_nodes_route_next_hop_store, NULL, -1, 0, node->phy_info->name, -1);
        gtk_list_store_insert_with_values(params_nodes_route_dst_store, NULL, -1, 0, node->ip_info->address, -1);
        gtk_list_store_insert_with_values(params_nodes_ping_address_store, NULL, -1, 0, node->ip_info->address, -1);
        gtk_list_store_insert_with_values(params_nodes_measure_connect_dst_store, NULL, -1, 0, node->phy_info->name, -1);
    }

    nodes_unlock();

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(params_nodes_route_next_hop_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_route_next_hop_combo), 0);
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(params_nodes_route_dst_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_route_dst_combo), 0);
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(params_nodes_ping_address_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_ping_address_combo), 0);
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(params_nodes_measure_connect_dst_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_measure_connect_dst_combo), 0);

    update_sensitivity();

    char title[256];
    char *scenario_file_name = rs_scenario_file_name != NULL ? rs_scenario_file_name : MAIN_WIN_TITLE_UNSAVED;

    snprintf(title, sizeof(title), MAIN_WIN_TITLE, scenario_file_name);
    gtk_window_set_title(GTK_WINDOW(main_window), title);

    signals_enable();
}

void main_win_node_to_gui(node_t *node, uint32 what)
{
    signals_disable();
    events_lock();

    if ((what & MAIN_WIN_NODE_TO_GUI_PHY) && (node != NULL)) {
        gtk_entry_set_text(GTK_ENTRY(params_nodes_name_entry), node->phy_info->name);

        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_x_spin), node->phy_info->cx);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_y_spin), node->phy_info->cy);

        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_tx_power_spin), node->phy_info->tx_power * 100);

        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_bat_level_spin), node->phy_info->battery_level * 100);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_mains_powered_check), node->phy_info->mains_powered);

        int16 prev_sel_index = mobility_tree_viee_get_selected_index();

        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_mobility_dx_spin), node->phy_info->cx);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_mobility_dy_spin), node->phy_info->cy);

        if (node->phy_info->mobility_count > 0) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_mobility_trigger_time_spin),
                    node->phy_info->mobility_list[node->phy_info->mobility_count - 1]->trigger_time +
                    node->phy_info->mobility_list[node->phy_info->mobility_count - 1]->duration);
        }
        else {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_mobility_trigger_time_spin), rs_system->now);
        }

        gtk_list_store_clear(params_nodes_mobility_store);
        uint16 i;
        char dest_x_str[256], dest_y_str[256];
        char *trigger_str_time, *duration_str_time;
        GtkTreeIter iter;
        for (i = 0; i < node->phy_info->mobility_count; i++) {
            phy_mobility_t *mobility = node->phy_info->mobility_list[i];

            trigger_str_time = rs_system_sim_time_to_string(mobility->trigger_time, TRUE);
            duration_str_time = rs_system_sim_time_to_string(mobility->duration, TRUE);
            snprintf(dest_x_str, sizeof(dest_x_str), "%.02f", mobility->dest_x);
            snprintf(dest_y_str, sizeof(dest_x_str), "%.02f", mobility->dest_y);

            gtk_list_store_append(params_nodes_mobility_store, &iter);
            gtk_list_store_set(params_nodes_mobility_store, &iter,
                    0, trigger_str_time,
                    1, duration_str_time,
                    2, dest_x_str,
                    3, dest_y_str,
                    -1);

            free(trigger_str_time);
            free(duration_str_time);
        }

        if (prev_sel_index >= node->phy_info->mobility_count) {
            prev_sel_index = node->phy_info->mobility_count - 1;
        }

        if (prev_sel_index >= 0) {
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(params_nodes_mobility_tree_view));
            GtkTreePath *path = gtk_tree_path_new_from_indices(prev_sel_index, -1);
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);
        }
    }

    if ((what & MAIN_WIN_NODE_TO_GUI_MAC) && (node != NULL)) {
        gtk_entry_set_text(GTK_ENTRY(params_nodes_mac_address_entry), node->mac_info->address);
    }

    if ((what & MAIN_WIN_NODE_TO_GUI_IP) && (node != NULL)) {
        int16 prev_sel_index = route_tree_viee_get_selected_index();

        gtk_entry_set_text(GTK_ENTRY(params_nodes_ip_address_entry), node->ip_info->address);

        gtk_list_store_clear(params_nodes_route_store);
        uint16 i;
        GtkTreeIter iter;
        char dest[256];
        char *type[] = {"Connected", "Manual", "DAO", "DIO"};
        for (i = 0; i < node->ip_info->route_count; i++) {
            ip_route_t *route = node->ip_info->route_list[i];

            snprintf(dest, sizeof(dest), "%s/%d", route->dst, route->prefix_len);

            gtk_list_store_append(params_nodes_route_store, &iter);
            gtk_list_store_set(params_nodes_route_store, &iter,
                    0, dest,
                    1, route->next_hop->phy_info->name,
                    2, type[route->type],
                    -1);
        }

        if (prev_sel_index >= node->ip_info->route_count) {
            prev_sel_index = node->ip_info->route_count - 1;
        }

        if (prev_sel_index >= 0) {
            GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(params_nodes_route_tree_view));
            GtkTreePath *path = gtk_tree_path_new_from_indices(prev_sel_index, -1);
            gtk_tree_selection_select_path(selection, path);
            gtk_tree_path_free(path);
        }
    }

    if ((what & MAIN_WIN_NODE_TO_GUI_ICMP) && (node != NULL)) {
        if (node->icmp_info->ping_ip_address != NULL) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_enable_ping_measurements_check), TRUE);

            GtkTreeIter iter;
            bool more = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(params_nodes_ping_address_store), &iter);
            GValue value = {0, };
            int16 pos = -1;
            while (more) {
                pos++;

                gtk_tree_model_get_value(GTK_TREE_MODEL(params_nodes_ping_address_store), &iter, 0, &value);

                const char *str = g_value_get_string(&value);
                if (strcmp(str, node->icmp_info->ping_ip_address) == 0) {
                    g_value_unset(&value);
                    break;
                }
                g_value_unset(&value);
                more = gtk_tree_model_iter_next(GTK_TREE_MODEL(params_nodes_ping_address_store), &iter);
            }

            if (pos == -1) {
                gtk_list_store_insert_with_values(params_nodes_ping_address_store, NULL, -1, 0, node->icmp_info->ping_ip_address, -1);
                pos = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(params_nodes_ping_address_store), NULL) - 1;
            }

            gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_ping_address_combo), pos);
        }
        else {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_enable_ping_measurements_check), FALSE);
            gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_ping_address_combo), -1);
        }

        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_ping_interval_spin), node->icmp_info->ping_interval);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_ping_timeout_spin), node->icmp_info->ping_timeout);
    }

    if ((what & MAIN_WIN_NODE_TO_GUI_RPL) && (node != NULL)) {
        rpl_root_info_t *root_info = node->rpl_info->root_info;
        rpl_dodag_t *dodag = node->rpl_info->joined_dodag;

        char temp[256];

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_storing_check), node->rpl_info->storing);

        snprintf(temp, sizeof(temp), "%d", node->rpl_info->trickle_i);
        gtk_entry_set_text(GTK_ENTRY(params_nodes_trickle_int_entry), temp);

        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_grounded_check), root_info->grounded);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_dao_enabled_check), root_info->dao_supported);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_dao_trigger_check), root_info->dao_trigger);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_dag_pref_spin), root_info->dodag_pref);

        gtk_entry_set_editable(GTK_ENTRY(params_nodes_dag_id_entry), !rpl_node_is_poisoning(node) && !rpl_node_is_joined(node));

        if (rpl_node_is_joined(node)) {
            gtk_entry_set_text(GTK_ENTRY(params_nodes_dag_id_entry), dodag->dodag_id);

            snprintf(temp, sizeof(temp), "%d", dodag->seq_num);
            gtk_entry_set_text(GTK_ENTRY(params_nodes_seq_num_entry), temp);

            snprintf(temp, sizeof(temp), "%d", dodag->rank);
            gtk_entry_set_text(GTK_ENTRY(params_nodes_rank_entry), temp);
        }
        else if (rpl_node_is_root(node)) {
            gtk_entry_set_text(GTK_ENTRY(params_nodes_dag_id_entry), root_info->dodag_id);
            snprintf(temp, sizeof(temp), "%d", rpl_seq_num_get(node->rpl_info->root_info->dodag_id));
            gtk_entry_set_text(GTK_ENTRY(params_nodes_seq_num_entry), temp);

            snprintf(temp, sizeof(temp), "%d", RPL_RANK_ROOT);
            gtk_entry_set_text(GTK_ENTRY(params_nodes_rank_entry), temp);
        }
        else if (rpl_node_is_poisoning(node)) {
            gtk_entry_set_text(GTK_ENTRY(params_nodes_dag_id_entry), dodag->dodag_id);

            gtk_entry_set_text(GTK_ENTRY(params_nodes_seq_num_entry), "0");

            snprintf(temp, sizeof(temp), "%d", RPL_RANK_INFINITY);
            gtk_entry_set_text(GTK_ENTRY(params_nodes_rank_entry), temp);
        }
        else { /* isolated */
            if (root_info->configured_dodag_id != NULL)
                gtk_entry_set_text(GTK_ENTRY(params_nodes_dag_id_entry), root_info->configured_dodag_id);
            else
                gtk_entry_set_text(GTK_ENTRY(params_nodes_dag_id_entry), "");

            gtk_entry_set_text(GTK_ENTRY(params_nodes_seq_num_entry), "0");

            snprintf(temp, sizeof(temp), "%d", RPL_RANK_INFINITY);
            gtk_entry_set_text(GTK_ENTRY(params_nodes_rank_entry), "0");
        }
    }

    if (what & MAIN_WIN_NODE_TO_GUI_MEASURE) {
        char temp[256];
        char *conn_str_time, *total_str_time;
        percent_t fraction;

        if (node != NULL) {
            /* connectivity */
            if (node->measure_info->connect_dst_node != NULL) {
                uint16 pos = rs_system_get_node_pos(node->measure_info->connect_dst_node);
                gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_measure_connect_dst_combo), pos + 1);
                gtk_label_set_text(GTK_LABEL(params_nodes_measure_connect_connected_now_label),
                        node->measure_info->connect_dst_reachable ? "Yes" : "No");

                percent_t fraction = 0;
				sim_time_t connected_time = node->measure_info->connect_connected_time;
				sim_time_t total_time = rs_system->now - node->measure_info->connect_global_start_time;

				if (node->measure_info->connect_last_establish_time != -1) {
					connected_time += rs_system->now - node->measure_info->connect_last_establish_time;
				}

				if (total_time > 0 && total_time >= connected_time)
					fraction = (percent_t) connected_time / total_time;

				conn_str_time = rs_system_sim_time_to_string(connected_time, FALSE);
				total_str_time = rs_system_sim_time_to_string(total_time, FALSE);

                snprintf(temp, sizeof(temp), "%s/%s (%.0f%%)", conn_str_time, total_str_time, fraction * 100);

                free(conn_str_time);
                free(total_str_time);

                gtk_progress_bar_set_text(GTK_PROGRESS_BAR(params_nodes_measure_connect_progress), temp);
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(params_nodes_measure_connect_progress), fraction);
            }
            else {
                gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_measure_connect_dst_combo), 0); /* disabled */
                gtk_label_set_text(GTK_LABEL(params_nodes_measure_connect_connected_now_label), "N/A");
                gtk_progress_bar_set_text(GTK_PROGRESS_BAR(params_nodes_measure_connect_progress), "N/A");
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(params_nodes_measure_connect_progress), 0);
            }

            /* statistics */
            snprintf(temp, sizeof(temp), "%d", node->measure_info->forward_inconsistency_count);
            gtk_label_set_text(GTK_LABEL(params_nodes_measure_stat_forward_inconsistencies_label), temp);

            snprintf(temp, sizeof(temp), "%d", node->measure_info->forward_failure_count);
            gtk_label_set_text(GTK_LABEL(params_nodes_measure_stat_forward_failures_label), temp);

            snprintf(temp, sizeof(temp), "%d/%d", node->measure_info->rpl_s_dis_message_count, node->measure_info->rpl_r_dis_message_count);
            gtk_label_set_text(GTK_LABEL(params_nodes_measure_stat_rpl_dis_messages_label), temp);

            snprintf(temp, sizeof(temp), "%d/%d", node->measure_info->rpl_s_dio_message_count, node->measure_info->rpl_r_dio_message_count);
            gtk_label_set_text(GTK_LABEL(params_nodes_measure_stat_rpl_dio_messages_label), temp);

            snprintf(temp, sizeof(temp), "%d/%d", node->measure_info->rpl_s_dao_message_count, node->measure_info->rpl_r_dao_message_count);
            gtk_label_set_text(GTK_LABEL(params_nodes_measure_stat_rpl_dao_messages_label), temp);

            if (rpl_node_is_joined(node)) {
                snprintf(temp, sizeof(temp), "%d/%d", node->rpl_info->joined_dodag->parent_count, node->rpl_info->joined_dodag->sibling_count);
                gtk_label_set_text(GTK_LABEL(params_nodes_measure_stat_rpl_parents_siblings_label), temp);
            }
            else {
                snprintf(temp, sizeof(temp), "%d/%d", 0, 0);
                gtk_label_set_text(GTK_LABEL(params_nodes_measure_stat_rpl_parents_siblings_label), temp);
            }

            snprintf(temp, sizeof(temp), "%d/%d", node->measure_info->gen_ip_packet_count, node->measure_info->fwd_ip_packet_count);
            gtk_label_set_text(GTK_LABEL(params_nodes_measure_stat_ip_packets_label), temp);

            snprintf(temp, sizeof(temp), "%d", node->ip_info->enqueued_count);
            gtk_label_set_text(GTK_LABEL(params_nodes_measure_stat_queued_ip_packets_label), temp);

            float percent;
            if (node->measure_info->ping_successful_count > 0) {
                percent = (float) node->measure_info->ping_successful_count / (node->measure_info->ping_successful_count + node->measure_info->ping_timeout_count);
            }
            else {
                percent = 0;
            }

            if (percent > 1) {
                percent = 1;
            }

            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(params_nodes_measure_stat_ping_progress), percent);

            snprintf(temp, sizeof(temp), "%d/%d (%.0f%%)", node->measure_info->ping_successful_count, node->measure_info->ping_successful_count + node->measure_info->ping_timeout_count, percent * 100);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(params_nodes_measure_stat_ping_progress), temp);
        }
        else {
            /* convergence */
            measure_converg_t *measure_converg = measure_converg_get();

            if (measure_converg->total_node_count > 0 && measure_converg->total_node_count >= measure_converg->connected_node_count) {
                fraction = (float) measure_converg->connected_node_count / measure_converg->total_node_count;
            }
            else {
                fraction = 0;
            }
            snprintf(temp, sizeof(temp), "%d/%d (%.0f%%)", measure_converg->connected_node_count, measure_converg->total_node_count, fraction * 100);
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(params_nodes_measure_converg_connected_progress), fraction);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(params_nodes_measure_converg_connected_progress), temp);

            if (measure_converg->total_node_count > 0 && measure_converg->total_node_count >= measure_converg->stable_node_count) {
                fraction = (float) measure_converg->stable_node_count / measure_converg->total_node_count;
            }
            else {
                fraction = 0;
            }

            snprintf(temp, sizeof(temp), "%d/%d (%.0f%%)", measure_converg->stable_node_count, measure_converg->total_node_count, fraction * 100);
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(params_nodes_measure_converg_stable_progress), fraction);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(params_nodes_measure_converg_stable_progress), temp);

            if (measure_converg->total_node_count > 0 && measure_converg->total_node_count>= measure_converg->floating_node_count) {
                fraction = (float) measure_converg->floating_node_count / measure_converg->total_node_count;
            }
            else {
                fraction = 0;
            }

            snprintf(temp, sizeof(temp), "%d/%d (%.0f%%)", measure_converg->floating_node_count, measure_converg->total_node_count, fraction * 100);
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(params_nodes_measure_converg_floating_progress), fraction);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(params_nodes_measure_converg_floating_progress), temp);
        }
    }

    update_sensitivity();

    events_unlock();
    signals_enable();
}

void main_win_display_to_gui()
{
    signals_disable();

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_node_names_check), display_params.show_node_names);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_node_addresses_check), display_params.show_node_addresses);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_node_tx_power_check), display_params.show_node_tx_power);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_node_ranks_check), display_params.show_node_ranks);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_preferred_parent_arrows_check), display_params.show_preferred_parent_arrows);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_parent_arrows_check), display_params.show_parent_arrows);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_sibling_arrows_check), display_params.show_sibling_arrows);

    signals_enable();
}

void main_win_events_to_gui()
{
    signals_disable();

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_sys_node_wake_check), event_get_logging(sys_event_node_wake));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_sys_node_wake_check), event_get_logging(sys_event_node_wake));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_sys_node_kill_check), event_get_logging(sys_event_node_kill));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_sys_pdu_receive_check), event_get_logging(sys_event_pdu_receive));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_phy_node_wake_check), event_get_logging(phy_event_node_wake));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_phy_node_kill_check), event_get_logging(phy_event_node_kill));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_phy_pdu_send_check), event_get_logging(phy_event_pdu_send));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_phy_pdu_receive_check), event_get_logging(phy_event_pdu_receive));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_phy_neighbor_attach_check), event_get_logging(phy_event_neighbor_attach));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_phy_neighbor_detach_check), event_get_logging(phy_event_neighbor_detach));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_phy_change_mobility_check), event_get_logging(phy_event_change_mobility));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_mac_node_wake_check), event_get_logging(mac_event_node_wake));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_mac_node_kill_check), event_get_logging(mac_event_node_kill));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_mac_pdu_send_check), event_get_logging(mac_event_pdu_send));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_mac_pdu_send_timeout_check_check), event_get_logging(mac_event_pdu_send_timeout_check));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_mac_pdu_receive_check), event_get_logging(mac_event_pdu_receive));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_ip_node_wake_check), event_get_logging(ip_event_node_wake));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_ip_node_kill_check), event_get_logging(ip_event_node_kill));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_ip_pdu_send_check), event_get_logging(ip_event_pdu_send));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_ip_pdu_send_timeout_check_check), event_get_logging(ip_event_pdu_send_timeout_check));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_ip_pdu_receive_check), event_get_logging(ip_event_pdu_receive));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_ip_neighbor_cache_timeout_check_check), event_get_logging(ip_event_neighbor_cache_timeout_check));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_icmp_node_wake_check), event_get_logging(icmp_event_node_wake));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_icmp_node_kill_check), event_get_logging(icmp_event_node_kill));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_icmp_pdu_send_check), event_get_logging(icmp_event_pdu_send));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_icmp_pdu_receive_check), event_get_logging(icmp_event_pdu_receive));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_icmp_ping_request_check), event_get_logging(icmp_event_ping_request));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_icmp_ping_reply_check), event_get_logging(icmp_event_ping_reply));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_icmp_ping_timeout_check), event_get_logging(icmp_event_ping_timeout));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_node_wake_check), event_get_logging(rpl_event_node_wake));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_node_kill_check), event_get_logging(rpl_event_node_kill));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_dis_pdu_send_check), event_get_logging(rpl_event_dis_pdu_send));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_dis_pdu_receive_check), event_get_logging(rpl_event_dis_pdu_receive));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_dio_pdu_send_check), event_get_logging(rpl_event_dio_pdu_send));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_dio_pdu_receive_check), event_get_logging(rpl_event_dio_pdu_receive));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_dao_pdu_send_check), event_get_logging(rpl_event_dao_pdu_send));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_dao_pdu_receive_check), event_get_logging(rpl_event_dao_pdu_receive));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_neighbor_attach_check), event_get_logging(rpl_event_neighbor_attach));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_neighbor_detach_check), event_get_logging(rpl_event_neighbor_detach));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_forward_failure_check), event_get_logging(rpl_event_forward_failure));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_forward_inconsistency_check), event_get_logging(rpl_event_forward_inconsistency));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_trickle_t_timeout_check), event_get_logging(rpl_event_trickle_t_timeout));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_trickle_i_timeout_check), event_get_logging(rpl_event_trickle_i_timeout));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_dao_send_check), event_get_logging(rpl_event_dao_send));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_dao_timeout_check_check), event_get_logging(rpl_event_dao_timeout_check));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_rpl_seq_num_autoinc_check), event_get_logging(rpl_event_seq_num_autoinc));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_measure_node_wake_check), event_get_logging(measure_event_node_wake));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_measure_node_kill_check), event_get_logging(measure_event_node_kill));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_measure_pdu_send_check), event_get_logging(measure_event_pdu_send));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_measure_pdu_receive_check), event_get_logging(measure_event_pdu_receive));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_update_check), event_get_logging(measure_event_connect_update));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_hop_passed_check), event_get_logging(measure_event_connect_hop_passed));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_hop_failed_check), event_get_logging(measure_event_connect_hop_failed));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_hop_timeout_check), event_get_logging(measure_event_connect_hop_timeout));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_established_check), event_get_logging(measure_event_connect_established));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_lost_check), event_get_logging(measure_event_connect_lost));

    signals_enable();
}

display_params_t *main_win_get_display_params()
{
    return &display_params;
}

void main_win_add_log_line(uint32 no, char *str_time, char *node_name, char *layer, char *event_name, char *str1, char *str2)
{
    struct {
        uint32 no;
        char *str_time;
        char *node_name;
        char *layer;
        char *event_name;
        char *str1;
        char *str2;
    } * params = malloc(6 * sizeof(char *) + sizeof(uint32));

    params->no = no;
    params->str_time = strdup(str_time);
    params->node_name = strdup(node_name);
    params->layer = strdup(layer);
    params->event_name = strdup(event_name);
    params->str1 = strdup(str1);
    params->str2 = strdup(str2);

    gdk_threads_add_idle(log_wrapper, params);
}

void main_win_clear_log()
{
    gdk_threads_add_idle(log_wrapper, NULL);
}

void main_win_update_sim_status()
{
    char *text = malloc(256);
    snprintf(text, 256, "%s", rs_system->started ? (rs_system->paused ? "Paused" : "Running") : "Stopped");

    void **data = malloc(2 * sizeof(void *));
    data[0] = sim_status_bar;
    data[1] = text;

    gdk_threads_add_idle(status_bar_update_wrapper, data);
}

void main_win_update_nodes_status()
{
    nodes_lock();

    uint16 i, alive_count = 0;
    for (i = 0; i < rs_system->node_count; i++) {
        node_t *node = rs_system->node_list[i];
        if (node->alive) {
            alive_count++;
        }
    }

    nodes_unlock();

    char *text = malloc(256);
    snprintf(text, 256, "Nodes (alive/total): %d/%d", alive_count, rs_system->node_count);

    void **data = malloc(2 * sizeof(void *));
    data[0] = nodes_status_bar;
    data[1] = text;

    gdk_threads_add_idle(status_bar_update_wrapper, data);
}

void main_win_update_sim_time_status()
{
    char *text = malloc(256);
    char *time_str = rs_system_sim_time_to_string(rs_system->now, TRUE);

    snprintf(text, 256, "Simulation time: %s, Events (scheduled/total): %d/%d", time_str, rs_system->schedule_count, rs_system->event_count);
    free(time_str);

    void **data = malloc(2 * sizeof(void *));
    data[0] = sim_time_status_bar;
    data[1] = text;

    gdk_threads_add_idle(status_bar_update_wrapper, data);
}

void main_win_update_xy_status(coord_t x, coord_t y)
{
    char *text = malloc(256);
    snprintf(text, 256, "X: %.2fm, Y:%.2fm", x, y);

    void **data = malloc(2 * sizeof(void *));
    data[0] = xy_status_bar;
    data[1] = text;

    gdk_threads_add_idle(status_bar_update_wrapper, data);
}


    /**** local functions ****/

void cb_params_system_button_clicked(GtkWidget *widget, gpointer data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    gtk_widget_set_visible(params_config_vbox, TRUE);
    gtk_widget_set_visible(params_nodes_vbox, FALSE);
//    gtk_widget_set_visible(measures_vbox, FALSE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_config_button), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_button), FALSE);
//    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(measures_button), FALSE);

    signal_leave();
}

void cb_params_nodes_button_clicked(GtkWidget *widget, gpointer data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    gtk_widget_set_visible(params_config_vbox, FALSE);
    gtk_widget_set_visible(params_nodes_vbox, TRUE);
//    gtk_widget_set_visible(measures_vbox, FALSE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_config_button), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_button), TRUE);
//    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(measures_button), FALSE);

    signal_leave();
}

void cb_gui_system_updated(GtkSpinButton *spin, gpointer data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    gui_to_system();
    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

void cb_gui_node_updated(GtkWidget *widget, gpointer data)
{
    signal_enter();

    rs_assert(selected_node != NULL);
    rs_debug(DEBUG_GUI, NULL);

    gui_to_node(selected_node);
    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

void cb_gui_display_updated(GtkWidget *widget, gpointer data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    gui_to_display();
    sim_field_redraw();

    signal_leave();
}

void cb_gui_events_updated(GtkWidget *widget, gpointer data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    gui_to_events();

    signal_leave();
}

void cb_params_nodes_mobility_tree_view_cursor_changed(GtkWidget *widget, gpointer data)
{
    signal_enter();

    update_sensitivity();

    signal_leave();
}

void cb_params_nodes_route_tree_view_cursor_changed(GtkWidget *widget, gpointer data)
{
    signal_enter();

    update_sensitivity();

    signal_leave();
}

void cb_params_nodes_mobility_add_button_clicked(GtkButton *button, gpointer data)
{
    rs_assert(selected_node != NULL);
    rs_debug(DEBUG_GUI, NULL);

    sim_time_t trigger_time = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_mobility_trigger_time_spin));
    sim_time_t duration = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_mobility_duration_spin));
    coord_t dest_x = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_mobility_dx_spin));
    coord_t dest_y = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_mobility_dy_spin));

    phy_node_add_mobility(selected_node, trigger_time, duration, dest_x, dest_y);

    main_win_node_to_gui(selected_node, MAIN_WIN_NODE_TO_GUI_PHY);
}

void cb_params_nodes_mobility_rem_button_clicked(GtkButton *button, gpointer data)
{
    rs_assert(selected_node != NULL);
    rs_debug(DEBUG_GUI, NULL);

    int32 index = mobility_tree_viee_get_selected_index();
    rs_assert(index >= 0 && index < selected_node->phy_info->mobility_count);

    phy_node_rem_mobility(selected_node, index);

    main_win_node_to_gui(selected_node, MAIN_WIN_NODE_TO_GUI_PHY);
}

void cb_params_nodes_route_add_button_clicked(GtkButton *button, gpointer data)
{
    signal_enter();
    events_lock();

    rs_assert(selected_node != NULL);
    rs_debug(DEBUG_GUI, NULL);

    const char *dst = gtk_combo_box_get_active_text(GTK_COMBO_BOX(params_nodes_route_dst_combo));
    uint8 prefix_len = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_route_prefix_len_spin));
    int32 next_hop_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(params_nodes_route_next_hop_combo));

    if (strlen(dst) == 0) {
        dst = "0";
        prefix_len = 0;
    }

    rs_assert(next_hop_pos >= 0 && next_hop_pos < rs_system->node_count);

    node_t *next_hop = rs_system->node_list[next_hop_pos];

    ip_node_add_route(selected_node, (char *) dst, prefix_len, next_hop, IP_ROUTE_TYPE_MANUAL, NULL);

    events_unlock();
    signal_leave();

    main_win_node_to_gui(selected_node, MAIN_WIN_NODE_TO_GUI_IP);
}

void cb_params_nodes_route_rem_button_clicked(GtkButton *button, gpointer data)
{
    signal_enter();
    events_lock();

    rs_assert(selected_node != NULL);
    rs_debug(DEBUG_GUI, NULL);

    int32 index = route_tree_viee_get_selected_index();
    rs_assert(index >= 0 && index < selected_node->ip_info->route_count);

    ip_route_t *route = selected_node->ip_info->route_list[index];
    rs_system_cancel_event(selected_node, rpl_event_dao_timeout_check, route, NULL, 0);
    ip_node_rem_route(selected_node, route);

    events_unlock();
    signal_leave();

    main_win_node_to_gui(selected_node, MAIN_WIN_NODE_TO_GUI_IP);
}

void cb_params_nodes_route_dst_combo_changed(GtkComboBoxEntry *combo_box, gpointer data)
{
    rs_debug(DEBUG_GUI, NULL);

    char *dst = gtk_combo_box_get_active_text(GTK_COMBO_BOX(params_nodes_route_dst_combo));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_route_prefix_len_spin), strlen(dst) * 4);
}

void cb_params_nodes_root_button_clicked(GtkButton *button, gpointer data)
{
    signal_enter();

    rs_assert(selected_node != NULL);
    rs_debug(DEBUG_GUI, NULL);

    rpl_root_info_t *root_info = selected_node->rpl_info->root_info;
    if (root_info->configured_dodag_id != NULL) {
        free(root_info->configured_dodag_id);
    }
    if (root_info->configured_dodag_id == NULL) {
        root_info->dodag_id = strdup(selected_node->ip_info->address);
    }
    else {
        root_info->dodag_id = strdup(root_info->configured_dodag_id);
    }

    rpl_node_start_as_root(selected_node);
    sim_field_redraw();

    signal_leave();

    main_win_node_to_gui(selected_node, MAIN_WIN_NODE_TO_GUI_RPL);
}

void cb_params_nodes_isolate_button_clicked(GtkButton *button, gpointer data)
{
    signal_enter();

    rs_assert(selected_node != NULL);
    rs_debug(DEBUG_GUI, NULL);

    rpl_node_isolate(selected_node);
    sim_field_redraw();

    signal_leave();

    main_win_node_to_gui(selected_node, MAIN_WIN_NODE_TO_GUI_RPL);
}

void cb_params_nodes_ping_timeout_spin_changed(GtkSpinButton *spin, gpointer data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    update_sensitivity();

    signal_leave();
}

static void cb_log_to_console_check_toggle(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(log_to_console_check))) {
        gtk_list_store_clear(log_store);
    }

    update_sensitivity();

    signal_leave();
}

static void cb_new_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    rs_new();

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_open_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    GtkWidget *open_dialog = gtk_file_chooser_dialog_new(
            "Load scenario",
            GTK_WINDOW(main_window),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*" SCENARIO_FILE_EXT);
    gtk_file_filter_set_name(filter, "RS Scenarios");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(open_dialog), filter);

    char path[256];
    snprintf(path, sizeof(path), "%s/%s", rs_app_dir, SCENARIO_DIR);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(open_dialog), path);

    char *filename = NULL;
    if (gtk_dialog_run(GTK_DIALOG(open_dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(open_dialog));
    }

    gtk_widget_destroy(open_dialog);

    if (filename != NULL) {
        char *msg = rs_open(filename);
        free(filename);

        if (msg != NULL) {
            GtkWidget *msg_dialog = gtk_message_dialog_new(
                    GTK_WINDOW(main_window),
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    "Error loading scenario: %s", msg);

            gtk_dialog_run(GTK_DIALOG(msg_dialog));
            gtk_widget_destroy(msg_dialog);
        }
    }

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_save_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    if (rs_scenario_file_name == NULL) { /* no scenario loaded/saved yet */
        cb_save_as_menu_item_activate(widget, data);
        return;
    }

    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    char *msg = rs_save(rs_scenario_file_name);

    if (msg != NULL) {
        GtkWidget *msg_dialog = gtk_message_dialog_new(
                GTK_WINDOW(main_window),
                GTK_DIALOG_DESTROY_WITH_PARENT,
                GTK_MESSAGE_ERROR,
                GTK_BUTTONS_CLOSE,
                "Error saving scenario: %s", msg);

        gtk_dialog_run(GTK_DIALOG(msg_dialog));
        gtk_widget_destroy(msg_dialog);
    }

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_save_as_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    GtkWidget *save_dialog = gtk_file_chooser_dialog_new(
            "Save scenario",
            GTK_WINDOW(main_window),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*" SCENARIO_FILE_EXT);
    gtk_file_filter_set_name(filter, "RS Scenarios");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(save_dialog), filter);

    if (rs_scenario_file_name != NULL) {
        gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(save_dialog), rs_scenario_file_name);
    }
    else {
        char path[256];
        snprintf(path, sizeof(path), "%s/%s", rs_app_dir, SCENARIO_DIR);

        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(save_dialog), path);
    }

    char *filename = NULL;
    if (gtk_dialog_run(GTK_DIALOG(save_dialog)) == GTK_RESPONSE_ACCEPT) {
        char *temp_filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(save_dialog));
        if ((strlen(temp_filename) < strlen(SCENARIO_FILE_EXT)) ||
                (strcmp(temp_filename + strlen(temp_filename) - strlen(SCENARIO_FILE_EXT), SCENARIO_FILE_EXT) != 0)) {

            filename = malloc(strlen(temp_filename) + strlen(SCENARIO_FILE_EXT) + 1);
            sprintf(filename, "%s" SCENARIO_FILE_EXT, temp_filename);
            free(temp_filename);
        }
        else {
            filename = temp_filename;
        }
    }
    gtk_widget_destroy(save_dialog);

    if (filename != NULL) {
        char *msg = rs_save(filename);

        free(filename);

        if (msg != NULL) {
            GtkWidget *msg_dialog = gtk_message_dialog_new(
                    GTK_WINDOW(main_window),
                    GTK_DIALOG_DESTROY_WITH_PARENT,
                    GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE,
                    "Error saving scenario: %s", msg);

            gtk_dialog_run(GTK_DIALOG(msg_dialog));
            gtk_widget_destroy(msg_dialog);
        }
    }

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_quit_menu_item_activate(GtkMenuItem *widget, gpointer user_data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);
    rs_quit();

    signal_leave();
}

static void cb_start_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    rs_start(FALSE);

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_pause_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    rs_pause();

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_step_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    rs_step();

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_stop_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    rs_stop();

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_add_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    node_t *node = rs_add_node(rs_system->width / 2, rs_system->height / 2);
    main_win_set_selected_node(node);
    sim_field_redraw();

    main_win_system_to_gui();
    main_win_update_nodes_status();

    update_sensitivity();

    signal_leave();
}

static void cb_rem_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);
    rs_assert(selected_node != NULL);

    rs_rem_node(selected_node);

    main_win_set_selected_node(NULL);

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_wake_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);
    rs_assert(selected_node != NULL);

    rs_wake_node(selected_node);

    update_sensitivity();
    sim_field_redraw();

    signal_leave();
}

static void cb_kill_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);
    rs_assert(selected_node != NULL);

    rs_kill_node(selected_node);

    update_sensitivity();
    sim_field_redraw();

    signal_leave();
}

static void cb_add_all_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    add_more_dialog_info_t *dialog_info = add_more_dialog_run();
    if (dialog_info != NULL) {
        rs_add_more_nodes(
                dialog_info->node_number,
                dialog_info->pattern,
                dialog_info->horiz_dist,
                dialog_info->vert_dist,
                dialog_info->row_length);

        free(dialog_info);
    }

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_remove_all_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    rs_rem_all_nodes();

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_wake_all_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    rs_wake_all_nodes();

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_kill_all_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    rs_kill_all_nodes();

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_about_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    GtkWidget *about_dialog = gtk_about_dialog_new();

    const gchar *authors[3] = {
            "Calin Crisan <ccrisan@gmail.com>",
            NULL
    };

    const gchar *description = "Version " RS_VERSION "\nA simple LLN/RPL Simulator using GTK+";

    const gchar *license =
            "This program is free software; you can redistribute it and/or modify "
            "it under the terms of the GNU General Public License as published by "
            "the Free Software Foundation; either version 3 of the License, or "
            "(at your option) any later version. \n\n"
            "This program is distributed in the hope that it will be useful, "
            "but WITHOUT ANY WARRANTY; without even the implied warranty of "
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
            "GNU General Public License for more details. \n\n"
            "You should have received a copy of the GNU General Public License "
            "along with this program. If not, see <http://www.gnu.org/licenses/>.";

    char path[256];
    snprintf(path, sizeof(path), "%s/%s/%s", rs_app_dir, RES_DIR, MAIN_WIN_ICON);
    GdkPixbuf *icon = gdk_pixbuf_new_from_file(path, NULL);

    gtk_window_set_icon(GTK_WINDOW(about_dialog), icon);

    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dialog), "RPL Simulator");
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dialog), "http://code.google.com/p/rpl-simulator/");
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog), "Copyright \xc2\xa9 2010 Calin Crisan <ccrisan@gmail.com>");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), authors);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), description);
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about_dialog), license);
    gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(about_dialog), TRUE);
    gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about_dialog), icon);

    gtk_dialog_run(GTK_DIALOG(about_dialog));
    gtk_widget_hide(about_dialog);

    signal_leave();
}

static void cb_main_window_delete()
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    main_window = NULL;
    rs_quit();

    signal_leave();
}


    /**** widget creation ****/

GtkWidget *create_params_widget()
{
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, 350, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *params_table = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_table");
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), params_table);

    params_config_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_button");
    params_config_vbox = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_vbox");

    params_system_auto_wake_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_auto_wake_check");
    params_system_deterministic_random_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_deterministic_random_check");
    params_system_real_time_sim_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_real_time_sim_check");
    params_system_sim_second_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_sim_second_spin");

    params_system_width_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_width_spin");
    params_system_height_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_height_spin");
    params_system_no_link_dist_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_no_link_dist_spin");
    params_system_no_link_quality_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_no_link_quality_spin");
    params_system_transmission_time_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_transmission_time_spin");

    params_system_mac_pdu_timeout_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_mac_pdu_timeout_spin");

    params_system_ip_queue_size = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_ip_queue_size");
    params_system_ip_pdu_timeout_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_ip_pdu_timeout_spin");
    params_system_ip_neighbor_timeout_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_ip_neighbor_timeout_spin");

    params_rpl_trickle_min_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_trickle_min_spin");
    params_rpl_trickle_doublings_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_trickle_doublings_spin");
    params_rpl_trickle_redundancy_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_trickle_redundancy_spin");
    params_rpl_dao_root_delay_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_dao_root_delay_spin");
    params_rpl_dao_remove_timeout_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_dao_remove_timeout_spin");
    params_rpl_max_rank_inc_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_max_rank_inc_spin");
    params_rpl_dao_supported_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_dao_supported_check");
    params_rpl_dao_trigger_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_dao_trigger_check");
    params_rpl_probe_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_probe_check");
    params_rpl_prefer_floating_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_prefer_floating_check");
    params_rpl_autoinc_sn_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_autoinc_sn_check");
    params_rpl_autoinc_sn_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_autoinc_sn_spin");
    params_rpl_poison_count_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_rpl_poison_count_spin");

    params_system_measure_pdu_timeout_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_measure_pdu_timeout_spin");

    params_display_show_node_names_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_node_names_check");
    params_display_show_node_addresses_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_node_addresses_check");
    params_display_show_node_tx_power_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_node_tx_power_check");
    params_display_show_node_ranks_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_node_ranks_check");
    params_display_show_preferred_parent_arrows_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_preferred_parent_arrows_check");
    params_display_show_parent_arrows_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_parent_arrows_check");
    params_display_show_sibling_arrows_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_sibling_arrows_check");

    params_events_sys_node_wake_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_sys_node_wake_check");
    params_events_sys_node_kill_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_sys_node_kill_check");
    params_events_sys_pdu_receive_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_sys_pdu_receive_check");
    params_events_phy_node_wake_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_phy_node_wake_check");
    params_events_phy_node_kill_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_phy_node_kill_check");
    params_events_phy_pdu_send_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_phy_pdu_send_check");
    params_events_phy_pdu_receive_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_phy_pdu_receive_check");
    params_events_phy_neighbor_attach_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_phy_neighbor_attach_check");
    params_events_phy_neighbor_detach_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_phy_neighbor_detach_check");
    params_events_phy_change_mobility_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_phy_change_mobility_check");
    params_events_mac_node_wake_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_mac_node_wake_check");
    params_events_mac_node_kill_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_mac_node_kill_check");
    params_events_mac_pdu_send_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_mac_pdu_send_check");
    params_events_mac_pdu_send_timeout_check_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_mac_pdu_send_timeout_check_check");
    params_events_mac_pdu_receive_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_mac_pdu_receive_check");
    params_events_ip_node_wake_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_ip_node_wake_check");
    params_events_ip_node_kill_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_ip_node_kill_check");
    params_events_ip_pdu_send_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_ip_pdu_send_check");
    params_events_ip_pdu_send_timeout_check_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_ip_pdu_send_timeout_check_check");
    params_events_ip_pdu_receive_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_ip_pdu_receive_check");
    params_events_ip_neighbor_cache_timeout_check_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_ip_neighbor_cache_timeout_check_check");
    params_events_icmp_node_wake_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_icmp_node_wake_check");
    params_events_icmp_node_kill_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_icmp_node_kill_check");
    params_events_icmp_pdu_send_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_icmp_pdu_send_check");
    params_events_icmp_pdu_receive_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_icmp_pdu_receive_check");
    params_events_icmp_ping_request_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_icmp_ping_request_check");
    params_events_icmp_ping_reply_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_icmp_ping_reply_check");
    params_events_icmp_ping_timeout_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_icmp_ping_timeout_check");
    params_events_rpl_node_wake_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_node_wake_check");
    params_events_rpl_node_kill_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_node_kill_check");
    params_events_rpl_dis_pdu_send_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_dis_pdu_send_check");
    params_events_rpl_dis_pdu_receive_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_dis_pdu_receive_check");
    params_events_rpl_dio_pdu_send_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_dio_pdu_send_check");
    params_events_rpl_dio_pdu_receive_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_dio_pdu_receive_check");
    params_events_rpl_dao_pdu_send_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_dao_pdu_send_check");
    params_events_rpl_dao_pdu_receive_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_dao_pdu_receive_check");
    params_events_rpl_neighbor_attach_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_neighbor_attach_check");
    params_events_rpl_neighbor_detach_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_neighbor_detach_check");
    params_events_rpl_forward_failure_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_forward_failure_check");
    params_events_rpl_forward_inconsistency_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_forward_inconsistency_check");
    params_events_rpl_trickle_t_timeout_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_trickle_t_timeout_check");
    params_events_rpl_trickle_i_timeout_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_trickle_i_timeout_check");
    params_events_rpl_dao_send_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_dao_send_check");
    params_events_rpl_dao_timeout_check_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_dao_timeout_check_check");
    params_events_rpl_seq_num_autoinc_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_rpl_seq_num_autoinc_check");
    params_events_measure_node_wake_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_measure_node_wake_check");
    params_events_measure_node_kill_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_measure_node_kill_check");
    params_events_measure_pdu_send_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_measure_pdu_send_check");
    params_events_measure_pdu_receive_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_measure_pdu_receive_check");
    params_events_measure_connect_update_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_measure_connect_update_check");
    params_events_measure_connect_hop_passed_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_measure_connect_hop_passed_check");
    params_events_measure_connect_hop_failed_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_measure_connect_hop_failed_check");
    params_events_measure_connect_hop_timeout_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_measure_connect_hop_timeout_check");
    params_events_measure_connect_established_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_measure_connect_established_check");
    params_events_measure_connect_lost_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_events_measure_connect_lost_check");

    params_nodes_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_button");
    params_nodes_vbox = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_vbox");

    params_nodes_name_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_name_entry");
    params_nodes_x_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_x_spin");
    params_nodes_y_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_y_spin");
    params_nodes_tx_power_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_tx_power_spin");
    params_nodes_bat_level_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_bat_level_spin");
    params_nodes_mains_powered_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mains_powered_check");
    params_nodes_mobility_trigger_time_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mobility_trigger_time_spin");
    params_nodes_mobility_duration_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mobility_duration_spin");
    params_nodes_mobility_dx_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mobility_dx_spin");
    params_nodes_mobility_dy_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mobility_dy_spin");
    params_nodes_mobility_tree_view = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mobility_tree_view");
    params_nodes_mobility_add_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mobility_add_button");
    params_nodes_mobility_rem_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mobility_rem_button");
    params_nodes_mobility_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "params_nodes_mobility_store");

    params_nodes_mac_address_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mac_address_entry");

    params_nodes_ip_address_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_ip_address_entry");
    params_nodes_route_dst_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_dst_combo");
    params_nodes_route_prefix_len_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_prefix_len_spin");
    params_nodes_route_next_hop_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_next_hop_combo");
    params_nodes_route_add_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_add_button");
    params_nodes_route_rem_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_rem_button");
    params_nodes_route_tree_view = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_tree_view");
    params_nodes_route_next_hop_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "params_nodes_route_next_hop_store");
    params_nodes_route_dst_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "params_nodes_route_dst_store");
    params_nodes_route_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "params_nodes_route_store");

    params_nodes_enable_ping_measurements_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_enable_ping_measurements_check");
    params_nodes_ping_interval_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_ping_interval_spin");
    params_nodes_ping_timeout_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_ping_timeout_spin");
    params_nodes_ping_address_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_ping_address_combo");
    params_nodes_ping_address_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "params_nodes_ping_address_store");

    params_nodes_storing_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_storing_check");
    params_nodes_grounded_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_grounded_check");
    params_nodes_dao_enabled_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_dao_enabled_check");
    params_nodes_dao_trigger_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_dao_trigger_check");
    params_nodes_dag_pref_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_dag_pref_spin");
    params_nodes_dag_id_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_dag_id_entry");
    params_nodes_seq_num_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_seq_num_entry");
    params_nodes_rank_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_rank_entry");
    params_nodes_trickle_int_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_trickle_int_entry");
    params_nodes_root_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_root_button");
    params_nodes_isolate_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_isolate_button");

    params_nodes_measure_connect_dst_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_connect_dst_combo");
    params_nodes_measure_connect_connected_now_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_connect_connected_now_label");
    params_nodes_measure_connect_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_connect_progress");
    params_nodes_measure_connect_dst_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_connect_dst_store");

    params_nodes_measure_stat_forward_inconsistencies_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_stat_forward_inconsistencies_label");
    params_nodes_measure_stat_forward_failures_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_stat_forward_failures_label");
    params_nodes_measure_stat_rpl_dis_messages_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_stat_rpl_dis_messages_label");
    params_nodes_measure_stat_rpl_dio_messages_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_stat_rpl_dio_messages_label");
    params_nodes_measure_stat_rpl_dao_messages_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_stat_rpl_dao_messages_label");
    params_nodes_measure_stat_rpl_parents_siblings_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_stat_rpl_parents_siblings_label");
    params_nodes_measure_stat_ping_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_stat_ping_progress");
    params_nodes_measure_stat_ip_packets_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_stat_ip_packets_label");
    params_nodes_measure_stat_queued_ip_packets_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_stat_queued_ip_packets_label");

    params_nodes_measure_converg_connected_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_converg_connected_progress");
    params_nodes_measure_converg_stable_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_converg_stable_progress");
    params_nodes_measure_converg_floating_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_measure_converg_floating_progress");


//    measures_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_button");

//    measures_vbox = measurement_widget_create();
//    if (measures_vbox == NULL) {
//        rs_error("failed to create measurements widget");
//        return NULL;
//    }

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(params_nodes_mobility_tree_view),
            -1,
            "Trigger Time",
            renderer,
            "text", 0,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(params_nodes_mobility_tree_view),
            -1,
            "Duration",
            renderer,
            "text", 1,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(params_nodes_mobility_tree_view),
            -1,
            "Dest. X",
            renderer,
            "text", 2,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(params_nodes_mobility_tree_view),
            -1,
            "Dest. Y",
            renderer,
            "text", 3,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(params_nodes_route_next_hop_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(params_nodes_route_next_hop_combo), renderer, "text", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(params_nodes_route_tree_view),
            -1,
            "Destination",
            renderer,
            "text", 0,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(params_nodes_route_tree_view),
            -1,
            "Next Hop",
            renderer,
            "text", 1,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(params_nodes_route_tree_view),
            -1,
            "Type",
            renderer,
            "text", 2,
            NULL);

    GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(params_nodes_route_tree_view), 0);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 0);

    column = gtk_tree_view_get_column(GTK_TREE_VIEW(params_nodes_route_tree_view), 1);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 1);

    column = gtk_tree_view_get_column(GTK_TREE_VIEW(params_nodes_route_tree_view), 2);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 2);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(params_nodes_measure_connect_dst_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(params_nodes_measure_connect_dst_combo), renderer, "text", 0, NULL);

    return scrolled_window;
}

GtkWidget *create_menu_bar()
{
    GtkWidget *menu_bar = gtk_menu_bar_new();

    /* file menu */
    GtkWidget *file_menu = gtk_menu_new();


    new_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_NEW, NULL);
    gtk_signal_connect(GTK_OBJECT(new_menu_item), "activate", GTK_SIGNAL_FUNC(cb_new_menu_item_activate), NULL);
    gtk_menu_append(file_menu, new_menu_item);

    open_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
    gtk_signal_connect(GTK_OBJECT(open_menu_item), "activate", GTK_SIGNAL_FUNC(cb_open_menu_item_activate), NULL);
    gtk_menu_append(file_menu, open_menu_item);

    save_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE, NULL);
    gtk_signal_connect(GTK_OBJECT(save_menu_item), "activate", GTK_SIGNAL_FUNC(cb_save_menu_item_activate), NULL);
    gtk_menu_append(file_menu, save_menu_item);

    save_as_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE_AS, NULL);
    gtk_signal_connect(GTK_OBJECT(save_as_menu_item), "activate", GTK_SIGNAL_FUNC(cb_save_as_menu_item_activate), NULL);
    gtk_menu_append(file_menu, save_as_menu_item);

    gtk_menu_append(file_menu, gtk_separator_menu_item_new());

    quit_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    gtk_signal_connect(GTK_OBJECT(quit_menu_item), "activate", GTK_SIGNAL_FUNC(cb_quit_menu_item_activate), NULL);
    gtk_menu_append(file_menu, quit_menu_item);

    GtkWidget *file_menu_item = gtk_menu_item_new_with_mnemonic("_File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu_item), file_menu);
    gtk_menu_bar_append(menu_bar, file_menu_item);

    /* simulation menu */
    GtkWidget *simulation_menu = gtk_menu_new();

    start_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(start_menu_item), "_Start Simulation");
    gtk_signal_connect(GTK_OBJECT(start_menu_item), "activate", GTK_SIGNAL_FUNC(cb_start_menu_item_activate), NULL);
    gtk_menu_append(simulation_menu, start_menu_item);

    pause_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PAUSE, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(pause_menu_item), "_Pause Simulation");
    gtk_signal_connect(GTK_OBJECT(pause_menu_item), "activate", GTK_SIGNAL_FUNC(cb_pause_menu_item_activate), NULL);
    gtk_menu_append(simulation_menu, pause_menu_item);

    step_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_FORWARD, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(step_menu_item), "_One Step");
    gtk_signal_connect(GTK_OBJECT(step_menu_item), "activate", GTK_SIGNAL_FUNC(cb_step_menu_item_activate), NULL);
    gtk_menu_append(simulation_menu, step_menu_item);

    stop_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_STOP, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(stop_menu_item), "S_top Simulation");
    gtk_signal_connect(GTK_OBJECT(stop_menu_item), "activate", GTK_SIGNAL_FUNC(cb_stop_menu_item_activate), NULL);
    gtk_menu_append(simulation_menu, stop_menu_item);

    GtkWidget *simulation_menu_item = gtk_menu_item_new_with_mnemonic("_Simulation");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(simulation_menu_item), simulation_menu);
    gtk_menu_bar_append(menu_bar, simulation_menu_item);

    /* node menu */
    GtkWidget *node_menu = gtk_menu_new();

    add_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(add_menu_item), "_Add Node");
    gtk_signal_connect(GTK_OBJECT(add_menu_item), "activate", GTK_SIGNAL_FUNC(cb_add_menu_item_activate), NULL);
    gtk_menu_append(node_menu, add_menu_item);

    rem_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_REMOVE, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(rem_menu_item), "_Remove Node");
    gtk_signal_connect(GTK_OBJECT(rem_menu_item), "activate", GTK_SIGNAL_FUNC(cb_rem_menu_item_activate), NULL);
    gtk_menu_append(node_menu, rem_menu_item);

    gtk_menu_append(node_menu, gtk_separator_menu_item_new());

    wake_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_APPLY, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(wake_menu_item), "_Wake Node");
    gtk_signal_connect(GTK_OBJECT(wake_menu_item), "activate", GTK_SIGNAL_FUNC(cb_wake_menu_item_activate), NULL);
    gtk_menu_append(node_menu, wake_menu_item);

    kill_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CANCEL, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(kill_menu_item), "_Kill Node");
    gtk_signal_connect(GTK_OBJECT(kill_menu_item), "activate", GTK_SIGNAL_FUNC(cb_kill_menu_item_activate), NULL);
    gtk_menu_append(node_menu, kill_menu_item);

    gtk_menu_append(node_menu, gtk_separator_menu_item_new());

    add_all_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(add_all_menu_item), "Add _More...");
    gtk_signal_connect(GTK_OBJECT(add_all_menu_item), "activate", GTK_SIGNAL_FUNC(cb_add_all_menu_item_activate), NULL);
    gtk_menu_append(node_menu, add_all_menu_item);

    remove_all_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CLEAR, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(remove_all_menu_item), "R_emove All");
    gtk_signal_connect(GTK_OBJECT(remove_all_menu_item), "activate", GTK_SIGNAL_FUNC(cb_remove_all_menu_item_activate), NULL);
    gtk_menu_append(node_menu, remove_all_menu_item);

    wake_all_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_APPLY, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(wake_all_menu_item), "Wa_ke All");
    gtk_signal_connect(GTK_OBJECT(wake_all_menu_item), "activate", GTK_SIGNAL_FUNC(cb_wake_all_menu_item_activate), NULL);
    gtk_menu_append(node_menu, wake_all_menu_item);

    kill_all_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_CANCEL, NULL);
    gtk_menu_item_set_label(GTK_MENU_ITEM(kill_all_menu_item), "Ki_ll All");
    gtk_signal_connect(GTK_OBJECT(kill_all_menu_item), "activate", GTK_SIGNAL_FUNC(cb_kill_all_menu_item_activate), NULL);
    gtk_menu_append(node_menu, kill_all_menu_item);

    GtkWidget *node_menu_item = gtk_menu_item_new_with_mnemonic("_Node");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(node_menu_item), node_menu);
    gtk_menu_bar_append(menu_bar, node_menu_item);

    /* help menu */
    GtkWidget *help_menu = gtk_menu_new();

    about_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
    gtk_signal_connect(GTK_OBJECT(about_menu_item), "activate", GTK_SIGNAL_FUNC(cb_about_menu_item_activate), NULL);
    gtk_menu_append(help_menu, about_menu_item);

    GtkWidget *help_menu_item = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_menu_item), help_menu);
    gtk_menu_bar_append(menu_bar, help_menu_item);

    return menu_bar;
}

GtkWidget *create_tool_bar()
{
    GtkWidget *toolbar = gtk_toolbar_new();
    gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_BOTH_HORIZ);

    add_node_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_ADD);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(add_node_toolbar_item), "Add a new node");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(add_node_toolbar_item), "Add Node");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(add_node_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(add_node_toolbar_item), "clicked", G_CALLBACK(cb_add_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(add_node_toolbar_item), -1);

    rem_node_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_REMOVE);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(rem_node_toolbar_item), "Remove the selected node");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(rem_node_toolbar_item), "Remove Node");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(rem_node_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(rem_node_toolbar_item), "clicked", G_CALLBACK(cb_rem_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(rem_node_toolbar_item), -1);

    GtkToolItem *sep_toolbar_item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep_toolbar_item, -1);

    start_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(start_toolbar_item), "Start simulation");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(start_toolbar_item), "Start");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(start_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(start_toolbar_item), "clicked", G_CALLBACK(cb_start_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(start_toolbar_item), -1);

    pause_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PAUSE);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(pause_toolbar_item), "Pause simulation");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(pause_toolbar_item), "Pause");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(pause_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(pause_toolbar_item), "clicked", G_CALLBACK(cb_pause_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(pause_toolbar_item), -1);

    step_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_FORWARD);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(step_toolbar_item), "Go one step forward");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(step_toolbar_item), "Step");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(step_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(step_toolbar_item), "clicked", G_CALLBACK(cb_step_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(step_toolbar_item), -1);

    stop_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(stop_toolbar_item), "Stop simulation");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(stop_toolbar_item), "Stop");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(stop_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(stop_toolbar_item), "clicked", G_CALLBACK(cb_stop_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(stop_toolbar_item), -1);

    sep_toolbar_item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep_toolbar_item, -1);

    wake_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_APPLY);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(wake_toolbar_item), "Wake Node");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(wake_toolbar_item), "Wake");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(wake_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(wake_toolbar_item), "clicked", G_CALLBACK(cb_wake_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(wake_toolbar_item), -1);

    kill_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(kill_toolbar_item), "Kill Node");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(kill_toolbar_item), "Kill");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(kill_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(kill_toolbar_item), "clicked", G_CALLBACK(cb_kill_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(kill_toolbar_item), -1);

    return toolbar;
}

GtkWidget *create_log_console()
{
    GtkWidget *log_console = gtk_vbox_new(FALSE, 2);
    gtk_widget_set_size_request(log_console, -1, 100);

    GtkWidget *hbox = gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(log_console), hbox, FALSE, TRUE, 0);

    log_to_console_check = gtk_check_button_new_with_label("Log to console");
    gtk_signal_connect(GTK_OBJECT(log_to_console_check), "toggled", G_CALLBACK(cb_log_to_console_check_toggle), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), log_to_console_check, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Events to keep: "), TRUE, TRUE, 0);

    GtkAdjustment *adjustment = (GtkAdjustment *) gtk_adjustment_new(100, 1, 100000, 100, 1000, 0);
    log_events_to_keep_spin = gtk_spin_button_new(adjustment, 0, 0);
    gtk_box_pack_start(GTK_BOX(hbox), log_events_to_keep_spin, TRUE, TRUE, 0);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, -1, 150);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled_window), GTK_SHADOW_ETCHED_IN);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(log_console), scrolled_window, TRUE, TRUE, 0);

    log_store = gtk_list_store_new(7, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    log_tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(log_store));
    gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(log_tree_view), TRUE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), log_tree_view);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(log_tree_view),
            -1,
            "No",
            renderer,
            "text", 0,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(log_tree_view),
            -1,
            "Time",
            renderer,
            "text", 1,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(log_tree_view),
            -1,
            "Node",
            renderer,
            "text", 2,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(log_tree_view),
            -1,
            "Layer",
            renderer,
            "text", 3,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(log_tree_view),
            -1,
            "Event",
            renderer,
            "text", 4,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(log_tree_view),
            -1,
            "Data1",
            renderer,
            "text", 5,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(log_tree_view),
            -1,
            "Data2",
            renderer,
            "text", 6,
            NULL);

    GtkTreeViewColumn *column = gtk_tree_view_get_column(GTK_TREE_VIEW(log_tree_view), 0);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 0);

    column = gtk_tree_view_get_column(GTK_TREE_VIEW(log_tree_view), 1);
    gtk_tree_view_column_set_resizable(column, TRUE);

    column = gtk_tree_view_get_column(GTK_TREE_VIEW(log_tree_view), 2);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 2);

    column = gtk_tree_view_get_column(GTK_TREE_VIEW(log_tree_view), 3);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 3);

    column = gtk_tree_view_get_column(GTK_TREE_VIEW(log_tree_view), 4);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 4);

    column = gtk_tree_view_get_column(GTK_TREE_VIEW(log_tree_view), 5);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 5);

    column = gtk_tree_view_get_column(GTK_TREE_VIEW(log_tree_view), 6);
    gtk_tree_view_column_set_clickable(column, TRUE);
    gtk_tree_view_column_set_resizable(column, TRUE);
    gtk_tree_view_column_set_sort_column_id(column, 6);

    return log_console;
}

GtkWidget *create_status_bar()
{
    GtkWidget *hbox = gtk_hbox_new(FALSE, 2);

    sim_status_bar = gtk_statusbar_new();
    gtk_widget_set_size_request(sim_status_bar, 100, -1);

    nodes_status_bar = gtk_statusbar_new();
    gtk_widget_set_size_request(nodes_status_bar, 200, -1);

    sim_time_status_bar = gtk_statusbar_new();
    gtk_widget_set_size_request(sim_time_status_bar, 500, -1);

    xy_status_bar = gtk_statusbar_new();
    gtk_widget_set_size_request(xy_status_bar, 150, -1);

    gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(sim_status_bar), FALSE);
    gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(nodes_status_bar), FALSE);
    gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(sim_time_status_bar), FALSE);
    gtk_statusbar_set_has_resize_grip(GTK_STATUSBAR(xy_status_bar), TRUE);

    gtk_box_pack_start(GTK_BOX(hbox), sim_status_bar, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), nodes_status_bar, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), sim_time_status_bar, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), xy_status_bar, TRUE, TRUE, 0);

    return hbox;
}

GtkWidget *create_content_widget()
{
    main_win_first_hpaned = gtk_hpaned_new();
    main_win_second_hpaned = gtk_hpaned_new();

    gtk_paned_pack2(GTK_PANED(main_win_first_hpaned), main_win_second_hpaned, TRUE, TRUE);

    GtkWidget *params_widget = create_params_widget();
    if (params_widget == NULL) {
        rs_error("failed to create params widget");
        return NULL;
    }

    gtk_paned_pack1(GTK_PANED(main_win_first_hpaned), params_widget, FALSE, TRUE);

    GtkWidget *center_paned = gtk_vpaned_new();

    GtkWidget *sim_field = sim_field_create();
    if (sim_field == NULL) {
        rs_error("failed to create simulation field");
        return NULL;
    }

    GtkWidget *log_console = create_log_console();
    if (log_console == NULL) {
        rs_error("failed to create log console");
        return NULL;
    }

    gtk_paned_pack1(GTK_PANED(center_paned), sim_field, TRUE, TRUE);
    gtk_paned_pack2(GTK_PANED(center_paned), log_console, FALSE, FALSE);

    gtk_paned_pack1(GTK_PANED(main_win_second_hpaned), center_paned, TRUE, TRUE);

    legend_widget = legend_create();
    if (legend_widget == NULL) {
        rs_error("failed to create legend widget");
        return NULL;
    }

    gtk_paned_pack2(GTK_PANED(main_win_second_hpaned), legend_widget, FALSE, TRUE);

    gtk_paned_set_position(GTK_PANED(main_win_first_hpaned), 350);
    gtk_paned_set_position(GTK_PANED(main_win_second_hpaned), -1);

    return main_win_first_hpaned;
}

static void initialize_widgets()
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_config_button), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_button), FALSE);
//    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(measures_button), FALSE);

    sim_field_redraw();

    update_sensitivity();
}

static void update_sensitivity()
{
    bool real_time_sim = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_system_real_time_sim_check));
    bool mains_powered = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_mains_powered_check));
    bool sn_autoinc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_rpl_autoinc_sn_check));
    bool log_to_console = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(log_to_console_check));
    bool node_selected = selected_node != NULL;
    bool has_nodes = rs_system->node_count > 0;
    bool node_alive = node_selected && selected_node->alive;
    bool route_selected = gtk_tree_selection_count_selected_rows(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(params_nodes_route_tree_view))) > 0;
    bool mobility_selected = gtk_tree_selection_count_selected_rows(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(params_nodes_mobility_tree_view))) > 0;
    bool sim_started = rs_system->started;
    bool sim_paused = rs_system->paused;

    gtk_widget_set_sensitive(params_system_sim_second_spin, real_time_sim);
    gtk_widget_set_sensitive(params_rpl_autoinc_sn_spin, sn_autoinc);

    gtk_widget_set_sensitive(params_nodes_name_entry, node_selected);
    gtk_widget_set_sensitive(params_nodes_x_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_y_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_tx_power_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_bat_level_spin, node_selected && !mains_powered);
    gtk_widget_set_sensitive(params_nodes_mains_powered_check, node_selected);
    gtk_widget_set_sensitive(params_nodes_mobility_trigger_time_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_mobility_duration_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_mobility_dx_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_mobility_dy_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_mobility_tree_view, node_selected);
    gtk_widget_set_sensitive(params_nodes_mobility_add_button, node_selected);
    gtk_widget_set_sensitive(params_nodes_mobility_rem_button, node_selected && mobility_selected);

    gtk_widget_set_sensitive(params_nodes_mac_address_entry, node_selected);

    gtk_widget_set_sensitive(params_nodes_ip_address_entry, node_selected);
    gtk_widget_set_sensitive(params_nodes_route_dst_combo, node_selected);
    gtk_widget_set_sensitive(params_nodes_route_prefix_len_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_route_next_hop_combo, node_selected);
    gtk_widget_set_sensitive(params_nodes_route_add_button, node_selected);
    gtk_widget_set_sensitive(params_nodes_route_rem_button, node_selected && route_selected);
    gtk_widget_set_sensitive(params_nodes_route_tree_view, node_selected);

    gtk_widget_set_sensitive(params_nodes_enable_ping_measurements_check, node_selected);
    gtk_widget_set_sensitive(params_nodes_ping_interval_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_ping_timeout_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_ping_address_combo, node_selected);

    gtk_widget_set_sensitive(params_nodes_storing_check, node_selected);
    gtk_widget_set_sensitive(params_nodes_grounded_check, node_selected);
    gtk_widget_set_sensitive(params_nodes_dao_enabled_check, node_selected);
    gtk_widget_set_sensitive(params_nodes_dao_trigger_check, node_selected);
    gtk_widget_set_sensitive(params_nodes_dag_pref_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_dag_id_entry, node_selected);
    gtk_widget_set_sensitive(params_nodes_seq_num_entry, node_selected);
    gtk_widget_set_sensitive(params_nodes_rank_entry, node_selected);
    gtk_widget_set_sensitive(params_nodes_trickle_int_entry, node_selected);
    gtk_widget_set_sensitive(params_nodes_root_button, node_selected);
    gtk_widget_set_sensitive(params_nodes_isolate_button, node_selected);

    gtk_widget_set_sensitive(params_nodes_measure_connect_dst_combo, node_selected);

    gtk_widget_set_sensitive(log_events_to_keep_spin, log_to_console);

    gtk_widget_set_sensitive(new_menu_item, !sim_started);
    gtk_widget_set_sensitive(open_menu_item, !sim_started);
    gtk_widget_set_sensitive(save_menu_item, !sim_started);
    gtk_widget_set_sensitive(save_as_menu_item, !sim_started);

    gtk_widget_set_sensitive(rem_node_toolbar_item, node_selected && !sim_started);
    gtk_widget_set_sensitive(rem_menu_item, node_selected &&  !sim_started);

    gtk_widget_set_sensitive(remove_all_menu_item, has_nodes && !sim_started);
    gtk_widget_set_sensitive(kill_all_menu_item, has_nodes && sim_started && !sim_paused);
    gtk_widget_set_sensitive(wake_all_menu_item, has_nodes && sim_started && !sim_paused);

    gtk_widget_set_sensitive(wake_menu_item, node_selected && !node_alive && sim_started && !sim_paused);
    gtk_widget_set_sensitive(wake_toolbar_item, node_selected && !node_alive && sim_started && !sim_paused);
    gtk_widget_set_sensitive(kill_menu_item, node_selected && node_alive && sim_started && !sim_paused);
    gtk_widget_set_sensitive(kill_toolbar_item, node_selected && node_alive && sim_started && !sim_paused);

    gtk_widget_set_sensitive(start_menu_item, !sim_started || sim_paused);
    gtk_widget_set_sensitive(start_toolbar_item, !sim_started || sim_paused);

    gtk_widget_set_sensitive(pause_menu_item, sim_started && !sim_paused);
    gtk_widget_set_sensitive(pause_toolbar_item, sim_started && !sim_paused);

    gtk_widget_set_sensitive(step_menu_item, !sim_started || sim_paused);
    gtk_widget_set_sensitive(step_toolbar_item, !sim_started || sim_paused);

    gtk_widget_set_sensitive(stop_menu_item, sim_started);
    gtk_widget_set_sensitive(stop_toolbar_item, sim_started);
}

static int32 route_tree_viee_get_selected_index()
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(params_nodes_route_tree_view));
    GList *path_list = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (path_list == NULL) {
        return -1;
    }
    else {
        int32 index = gtk_tree_path_get_indices((GtkTreePath *) g_list_nth_data(path_list, 0))[0];

        g_list_foreach(path_list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(path_list);

        return index;
    }
}

static int32 mobility_tree_viee_get_selected_index()
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(params_nodes_mobility_tree_view));
    GList *path_list = gtk_tree_selection_get_selected_rows(selection, NULL);

    if (path_list == NULL) {
        return -1;
    }
    else {
        int32 index = gtk_tree_path_get_indices((GtkTreePath *) g_list_nth_data(path_list, 0))[0];

        g_list_foreach(path_list, (GFunc) gtk_tree_path_free, NULL);
        g_list_free(path_list);

        return index;
    }
}

static void gui_to_system()
{
    rs_assert(rs_system != NULL);

    /* system */
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_system_real_time_sim_check))) {
        rs_system->simulation_second = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_sim_second_spin));
    }
    else {
        rs_system->simulation_second = -1;
    }

    rs_system->auto_wake_nodes = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_system_auto_wake_check));
    rs_system->deterministic_random = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_system_deterministic_random_check));

    /* phy */
    rs_system->width = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_width_spin));
    rs_system->height = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_height_spin));
    rs_system->no_link_dist_thresh = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_no_link_dist_spin));
    rs_system->no_link_quality_thresh = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_no_link_quality_spin)) / 100.0;
    rs_system->transmission_time = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_transmission_time_spin));

    /* mac */
    rs_system->mac_pdu_timeout = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_mac_pdu_timeout_spin));

    /* ip */
    rs_system->ip_queue_size = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_ip_queue_size));
    rs_system->ip_pdu_timeout = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_ip_pdu_timeout_spin));
    rs_system->ip_neighbor_timeout = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_ip_neighbor_timeout_spin));

    /* rpl */
    rs_system->rpl_dio_interval_min = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_rpl_trickle_min_spin));
    rs_system->rpl_dio_interval_doublings = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_rpl_trickle_doublings_spin));
    rs_system->rpl_dio_redundancy_constant = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_rpl_trickle_redundancy_spin));
    rs_system->rpl_dao_root_delay = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_rpl_dao_root_delay_spin));
    rs_system->rpl_dao_remove_timeout = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_rpl_dao_remove_timeout_spin));
    rs_system->rpl_max_inc_rank = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_rpl_max_rank_inc_spin));
    rs_system->rpl_dao_supported = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_rpl_dao_supported_check));
    rs_system->rpl_dao_trigger = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_rpl_dao_trigger_check));
    rs_system->rpl_startup_probe_for_dodags = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_rpl_probe_check));
    rs_system->rpl_prefer_floating = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_rpl_prefer_floating_check));

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_rpl_autoinc_sn_check))) {
        if (gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_rpl_autoinc_sn_spin)) <= 0) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_rpl_autoinc_sn_spin), 10000);
        }

        rs_system->rpl_auto_sn_inc_interval = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_rpl_autoinc_sn_spin));
    }
    else {
        rs_system->rpl_auto_sn_inc_interval = -1;
    }

    rs_system->rpl_poison_count = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_rpl_poison_count_spin));

    /* measure */
    rs_system->measure_pdu_timeout = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_measure_pdu_timeout_spin));

    /* update all existing nodes' root info */
    update_rpl_root_configurations();

    /* reschedule the auto incrementing of seq num mechanism */
    rs_system_cancel_event(NULL, rpl_event_seq_num_autoinc, NULL, NULL, 0);
    if (rs_system->rpl_auto_sn_inc_interval > 0 ) {
        rs_system_schedule_event(NULL, rpl_event_seq_num_autoinc, NULL, NULL, rs_system->rpl_auto_sn_inc_interval);
    }

    uint16 i;
    for (i = 0; i < rs_system->node_count; i++) {
        phy_node_update_neighbors(rs_system->node_list[i]);
    }
}

static void gui_to_node(node_t *node)
{
    events_lock();

    /* phy */
    const char *new_name = gtk_entry_get_text(GTK_ENTRY(params_nodes_name_entry));

    if (strlen(new_name) > 0) {
        nodes_lock();
        uint16 i;
        bool exists = FALSE;
        for (i = 0; i < rs_system->node_count; i++) { /* check for name already in use */
            node_t *other_node = rs_system->node_list[i];
            if (node == other_node)
                continue;

            if (strcmp(other_node->phy_info->name, new_name) == 0) {
                exists = TRUE;
                break;
            }
        }
        nodes_unlock();

        if (!exists) {
            bool name_changed = FALSE;
            if (strcmp(gtk_entry_get_text(GTK_ENTRY(params_nodes_name_entry)), node->phy_info->name) != 0) {
                name_changed = TRUE;
            }

            phy_node_set_name(node, gtk_entry_get_text(GTK_ENTRY(params_nodes_name_entry)));

            if (name_changed) {
                main_win_system_to_gui();
            }
        }
        else {
            //gtk_entry_set_text(GTK_ENTRY(params_nodes_name_entry), node->phy_info->name);
        }
    }

    phy_node_set_coords(node, 
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_x_spin)),
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_y_spin)));

    phy_node_set_tx_power(node,
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_tx_power_spin)) / 100.0);

    node->phy_info->battery_level = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_bat_level_spin)) / 100.0;

    node->phy_info->mains_powered = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_mains_powered_check));

    /* mac */
    mac_node_set_address(node, gtk_entry_get_text(GTK_ENTRY(params_nodes_mac_address_entry)));

    /* ip */
    ip_node_set_address(node, gtk_entry_get_text(GTK_ENTRY(params_nodes_ip_address_entry)));

    /* icmp */
    bool should_start_ping = FALSE;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_enable_ping_measurements_check))) {
        node->icmp_info->ping_ip_address = gtk_combo_box_get_active_text(GTK_COMBO_BOX(params_nodes_ping_address_combo));
        should_start_ping = TRUE;
    }
    else {
        if (node->icmp_info->ping_ip_address != NULL) {
            free(node->icmp_info->ping_ip_address);
            node->icmp_info->ping_ip_address = NULL;
        }
    }
    node->icmp_info->ping_interval = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_ping_interval_spin));
    node->icmp_info->ping_timeout = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_ping_timeout_spin));

    /* rpl */
    rpl_root_info_t *root_info = node->rpl_info->root_info;

    node->rpl_info->storing = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_storing_check));
    /* node->rpl_info->trickle_i = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_trickle_spin)); */

    root_info->grounded = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_grounded_check));
    root_info->dao_supported = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_dao_enabled_check));
    root_info->dao_trigger = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_dao_trigger_check));
    root_info->dodag_pref = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_dag_pref_spin));

    if (rpl_node_is_root(node) || rpl_node_is_isolated(node)) {
        if (root_info->dodag_id != NULL) {
            free(root_info->dodag_id);
        }
        root_info->dodag_id = strdup(gtk_entry_get_text(GTK_ENTRY(params_nodes_dag_id_entry)));

        if (node->rpl_info->root_info->grounded) {
            if (root_info->configured_dodag_id != NULL) {
                free(root_info->configured_dodag_id);
            }
            root_info->configured_dodag_id = strdup(gtk_entry_get_text(GTK_ENTRY(params_nodes_dag_id_entry)));
        }
    }

    /* measure */
    bool should_start_connect_measure = FALSE;
    int32 pos = gtk_combo_box_get_active(GTK_COMBO_BOX(params_nodes_measure_connect_dst_combo));
    if (pos < 1) {
        node->measure_info->connect_dst_node = NULL;
    }
    else {
        node->measure_info->connect_dst_node = rs_system->node_list[pos - 1];
        should_start_connect_measure = TRUE;
    }

    events_unlock();

    if (node->alive && rs_system->started) {
        if (should_start_ping) {
            rs_system_cancel_event(node, icmp_event_ping_timeout, NULL, NULL, 0);
            rs_system_cancel_event(node, icmp_event_ping_request, NULL, NULL, 0);
            rs_system_schedule_event(node, icmp_event_ping_request,
                    node->icmp_info->ping_ip_address, (void *) node->icmp_info->ping_seq_num++,
                    rs_system_random() % node->icmp_info->ping_interval);
        }
        if (should_start_connect_measure) {
            measure_node_reset(node);
            measure_connect_update();
        }
    }
}

static void gui_to_display()
{
    display_params.show_node_names = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_node_names_check));
    display_params.show_node_addresses = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_node_addresses_check));
    display_params.show_node_tx_power = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_node_tx_power_check));
    display_params.show_node_ranks = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_node_ranks_check));
    display_params.show_parent_arrows = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_parent_arrows_check));
    display_params.show_preferred_parent_arrows = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_preferred_parent_arrows_check));
    display_params.show_sibling_arrows = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_sibling_arrows_check));

    gtk_widget_queue_draw(legend_widget);
}

static void gui_to_events()
{
    event_set_logging(sys_event_node_wake, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_sys_node_wake_check)));
    event_set_logging(sys_event_node_kill, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_sys_node_kill_check)));
    event_set_logging(sys_event_pdu_receive, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_sys_pdu_receive_check)));
    event_set_logging(phy_event_node_wake, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_phy_node_wake_check)));
    event_set_logging(phy_event_node_kill, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_phy_node_kill_check)));
    event_set_logging(phy_event_pdu_send, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_phy_pdu_send_check)));
    event_set_logging(phy_event_pdu_receive, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_phy_pdu_receive_check)));
    event_set_logging(phy_event_neighbor_attach, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_phy_neighbor_attach_check)));
    event_set_logging(phy_event_neighbor_detach, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_phy_neighbor_detach_check)));
    event_set_logging(phy_event_change_mobility, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_phy_change_mobility_check)));
    event_set_logging(mac_event_node_wake, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_mac_node_wake_check)));
    event_set_logging(mac_event_node_kill, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_mac_node_kill_check)));
    event_set_logging(mac_event_pdu_send, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_mac_pdu_send_check)));
    event_set_logging(mac_event_pdu_send_timeout_check, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_mac_pdu_send_timeout_check_check)));
    event_set_logging(mac_event_pdu_receive, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_mac_pdu_receive_check)));
    event_set_logging(ip_event_node_wake, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_ip_node_wake_check)));
    event_set_logging(ip_event_node_kill, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_ip_node_kill_check)));
    event_set_logging(ip_event_pdu_send, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_ip_pdu_send_check)));
    event_set_logging(ip_event_pdu_send_timeout_check, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_ip_pdu_send_timeout_check_check)));
    event_set_logging(ip_event_pdu_receive, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_ip_pdu_receive_check)));
    event_set_logging(ip_event_neighbor_cache_timeout_check, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_ip_neighbor_cache_timeout_check_check)));
    event_set_logging(icmp_event_node_wake, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_icmp_node_wake_check)));
    event_set_logging(icmp_event_node_kill, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_icmp_node_kill_check)));
    event_set_logging(icmp_event_pdu_send, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_icmp_pdu_send_check)));
    event_set_logging(icmp_event_pdu_receive, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_icmp_pdu_receive_check)));
    event_set_logging(icmp_event_ping_request, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_icmp_ping_request_check)));
    event_set_logging(icmp_event_ping_reply, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_icmp_ping_reply_check)));
    event_set_logging(icmp_event_ping_timeout, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_icmp_ping_timeout_check)));
    event_set_logging(rpl_event_node_wake, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_node_wake_check)));
    event_set_logging(rpl_event_node_kill, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_node_kill_check)));
    event_set_logging(rpl_event_dis_pdu_send, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_dis_pdu_send_check)));
    event_set_logging(rpl_event_dis_pdu_receive, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_dis_pdu_receive_check)));
    event_set_logging(rpl_event_dio_pdu_send, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_dio_pdu_send_check)));
    event_set_logging(rpl_event_dio_pdu_receive, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_dio_pdu_receive_check)));
    event_set_logging(rpl_event_dao_pdu_send, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_dao_pdu_send_check)));
    event_set_logging(rpl_event_dao_pdu_receive, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_dao_pdu_receive_check)));
    event_set_logging(rpl_event_neighbor_attach, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_neighbor_attach_check)));
    event_set_logging(rpl_event_neighbor_detach, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_neighbor_detach_check)));
    event_set_logging(rpl_event_forward_failure, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_forward_failure_check)));
    event_set_logging(rpl_event_forward_inconsistency, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_forward_inconsistency_check)));
    event_set_logging(rpl_event_trickle_t_timeout, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_trickle_t_timeout_check)));
    event_set_logging(rpl_event_trickle_i_timeout, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_trickle_i_timeout_check)));
    event_set_logging(rpl_event_dao_send, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_dao_send_check)));
    event_set_logging(rpl_event_dao_timeout_check, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_dao_timeout_check_check)));
    event_set_logging(rpl_event_seq_num_autoinc, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_rpl_seq_num_autoinc_check)));
    event_set_logging(measure_event_node_wake, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_measure_node_wake_check)));
    event_set_logging(measure_event_node_kill, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_measure_node_kill_check)));
    event_set_logging(measure_event_pdu_send, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_measure_pdu_send_check)));
    event_set_logging(measure_event_pdu_receive, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_measure_pdu_receive_check)));
    event_set_logging(measure_event_connect_update, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_update_check)));
    event_set_logging(measure_event_connect_hop_passed, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_hop_passed_check)));
    event_set_logging(measure_event_connect_hop_failed, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_hop_failed_check)));
    event_set_logging(measure_event_connect_hop_timeout, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_hop_timeout_check)));
    event_set_logging(measure_event_connect_established, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_established_check)));
    event_set_logging(measure_event_connect_lost, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_events_measure_connect_lost_check)));
}

static gboolean gui_update_wrapper(void *data)
{
    if (rs_system->started) {
        sim_field_redraw();

        main_win_node_to_gui(NULL, MAIN_WIN_NODE_TO_GUI_MEASURE);

        if (selected_node != NULL) {
            main_win_node_to_gui(selected_node, MAIN_WIN_NODE_TO_GUI_IP | MAIN_WIN_NODE_TO_GUI_RPL | MAIN_WIN_NODE_TO_GUI_MEASURE);
        }
    }

    return TRUE;
}

static gboolean status_bar_update_wrapper(void *data)
{
    GtkStatusbar *status_bar = ((void **) data)[0];
    char *text = ((void **) data)[1];

    gtk_statusbar_pop(status_bar, 0);
    gtk_statusbar_push(status_bar, 0, text);

    free(text);
    free(data);

    return FALSE;
}

static gboolean log_wrapper(void *data)
{
    if (data == NULL) {
        gtk_list_store_clear(log_store);
    }
    else {
        struct {
            uint32 no;
            char *str_time;
            char *node_name;
            char *layer;
            char *event_name;
            char *str1;
            char *str2;
        } * params = data;

        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(log_to_console_check))) {
            GtkTreeIter iter;
            gtk_list_store_insert_with_values(log_store, &iter, -1,
                    0, params->no,
                    1, params->str_time,
                    2, params->node_name,
                    3, params->layer,
                    4, params->event_name,
                    5, params->str1,
                    6, params->str2,
                    -1);

            GtkTreePath* path = gtk_tree_model_get_path(GTK_TREE_MODEL(log_store), &iter);
            gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(log_tree_view), path, NULL, FALSE, 0, 0);
            gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(log_tree_view)), &iter);

            gtk_tree_path_free(path);

            while (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(log_store), NULL) >
                    gtk_spin_button_get_value(GTK_SPIN_BUTTON(log_events_to_keep_spin))) {

                GtkTreeIter iter;
                gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(log_store), &iter, NULL, 0);
                gtk_list_store_remove(log_store, &iter);
            }
        }

        free(params->str_time);
        free(params->node_name);
        free(params->layer);
        free(params->event_name);
        free(params->str1);
        free(params->str2);
        free(params);
    }

    return FALSE;
}

static void update_rpl_root_configurations()
{
    uint16 node_count;
    node_t **node_list = rs_system_get_node_list_copy(&node_count);

    uint16 i;
    for (i = 0; i < node_count; i++) {
        node_t *node = node_list[i];
        rpl_root_info_t *root_info = node->rpl_info->root_info;

        root_info->dao_supported = rs_system->rpl_dao_supported;
        root_info->dao_trigger = rs_system->rpl_dao_trigger;
        root_info->dio_interval_doublings = rs_system->rpl_dio_interval_doublings;
        root_info->dio_interval_min = rs_system->rpl_dio_interval_min;
        root_info->dio_redundancy_constant = rs_system->rpl_dio_redundancy_constant;
        root_info->max_rank_inc = rs_system->rpl_max_inc_rank;
        // root_info->min_hop_rank_inc = rs_system->rpl_min_hop_rank_inc;

        if (rpl_node_is_root(node)) {
            rpl_node_reset_trickle_timer(node);
        }
    }

    if (node_list != NULL) {
        free(node_list);
    }
}
