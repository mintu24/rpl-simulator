#include <gdk/gdk.h>
#include <cairo.h>

#include "mainwin.h"
#include "simfield.h"
#include "../system.h"
#include "../main.h"

#define signal_enter()          { if (signals_disabled) return; signals_disabled = TRUE; }
#define signal_leave()          { signals_disabled = FALSE; }

#define signals_disable()       { signals_disabled = TRUE; }
#define signals_enable()        { signals_disabled = FALSE; }

// todo graphically represent packets in nodes on the sim field
// todo check for duplicate node names
// todo rename display to configuration or smtn
// todo add node life core sleep as a display param
// todo check for thread count limit
// todo see why the threads don't actually die

    /**** global variables ****/

display_params_t *              display_params = NULL;

static GtkBuilder *             gtk_builder = NULL;

static GtkWidget *              main_window = NULL;

    /* params widgets */
static GtkWidget *              params_system_button = NULL;
static GtkWidget *              params_system_vbox = NULL;
static GtkWidget *              params_system_no_link_dist_spin = NULL;
static GtkWidget *              params_system_no_link_quality_spin = NULL;
static GtkWidget *              params_transmit_mode_block_radio = NULL;
static GtkWidget *              params_transmit_mode_reject_radio = NULL;
static GtkWidget *              params_transmit_mode_queue_radio = NULL;
static GtkWidget *              params_system_width_spin = NULL;
static GtkWidget *              params_system_height_spin = NULL;

static GtkWidget *              params_nodes_button = NULL;
static GtkWidget *              params_nodes_vbox = NULL;
static GtkWidget *              params_nodes_name_entry = NULL;
static GtkWidget *              params_nodes_x_spin = NULL;
static GtkWidget *              params_nodes_y_spin = NULL;
static GtkWidget *              params_nodes_tx_power_spin = NULL;
static GtkWidget *              params_nodes_bat_level_spin = NULL;
static GtkWidget *              params_nodes_mains_powered_check = NULL;
static GtkWidget *              params_nodes_mac_address_entry = NULL;
static GtkWidget *              params_nodes_ip_address_entry = NULL;
static GtkWidget *              params_nodes_route_dst_combo = NULL;
static GtkListStore *           params_nodes_route_dst_store = NULL;
static GtkWidget *              params_nodes_route_prefix_len_spin = NULL;
static GtkWidget *              params_nodes_route_next_hop_combo = NULL;
static GtkListStore *           params_nodes_route_next_hop_store = NULL;
static GtkWidget *              params_nodes_route_type_combo = NULL;
static GtkListStore *           params_nodes_route_type_store = NULL;
static GtkWidget *              params_nodes_route_add_button = NULL;
static GtkWidget *              params_nodes_route_rem_button = NULL;
static GtkWidget *              params_nodes_route_upd_button = NULL;
static GtkWidget *              params_nodes_route_tree_view = NULL;
static GtkListStore *           params_nodes_route_store = NULL;
static GtkWidget *              params_nodes_enable_ping_measurements_check = NULL;
static GtkWidget *              params_nodes_ping_interval_spin = NULL;
static GtkWidget *              params_nodes_ping_timeout_spin = NULL;
static GtkWidget *              params_nodes_ping_node_combo = NULL;
static GtkListStore *           params_nodes_ping_node_store = NULL;
static GtkWidget *              params_nodes_rank_spin = NULL;
static GtkWidget *              params_nodes_seq_num_spin = NULL;

static GtkWidget *              params_display_button = NULL;
static GtkWidget *              params_display_vbox = NULL;
static GtkWidget *              params_display_show_node_names_check = NULL;
static GtkWidget *              params_display_show_node_addresses_check = NULL;
static GtkWidget *              params_display_show_node_tx_power_check = NULL;
static GtkWidget *              params_display_show_node_ranks_check = NULL;
static GtkWidget *              params_display_show_parent_arrows_check = NULL;
static GtkWidget *              params_display_show_sibling_arrows_check = NULL;

    /* other widgets */
static GtkWidget *              open_menu_item = NULL;
static GtkWidget *              save_menu_item = NULL;
static GtkWidget *              quit_menu_item = NULL;
static GtkWidget *              start_menu_item = NULL;
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
static GtkWidget *              stop_toolbar_item = NULL;
static GtkWidget *              wake_toolbar_item = NULL;
static GtkWidget *              kill_toolbar_item = NULL;

static node_t *                 selected_node = NULL;

static bool                     signals_disabled = FALSE;


    /**** local function prototypes ****/

void                cb_params_system_button_clicked(GtkWidget *button, gpointer data);
void                cb_params_nodes_button_clicked(GtkWidget *button, gpointer data);
void                cb_params_display_button_clicked(GtkWidget *button, gpointer data);
void                cb_gui_system_updated(GtkSpinButton *spin, gpointer data);
void                cb_gui_node_updated(GtkWidget *widget, gpointer data);
void                cb_gui_display_updated(GtkWidget *widget, gpointer data);
void                cb_params_nodes_route_tree_view_cursor_changed(GtkTreeView *tree_view, gpointer data);
void                cb_params_nodes_route_add_button_clicked(GtkButton *button, gpointer data);
void                cb_params_nodes_route_rem_button_clicked(GtkButton *button, gpointer data);
void                cb_params_nodes_route_upd_button_clicked(GtkButton *button, gpointer data);
void                cb_params_nodes_route_dst_combo_changed(GtkComboBoxEntry *combo_box, gpointer data);

static void         cb_open_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_save_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_quit_menu_item_activate(GtkMenuItem *widget, gpointer user_data);
static void         cb_start_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_stop_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_add_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_rem_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_wake_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_kill_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_add_all_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_remove_all_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_wake_all_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_kill_all_menu_item_activate(GtkWidget *widget, gpointer *data);
static void         cb_about_menu_item_activate(GtkWidget *widget, gpointer *data);

static void         cb_main_window_delete();

static GtkWidget *  create_params_widget();
//static GtkWidget *  create_monitoring_widget();
static GtkWidget *  create_menu_bar();
static GtkWidget *  create_tool_bar();
static GtkWidget *  create_status_bar();
static GtkWidget *  create_content_widget();

static void         initialize_widgets();
static void         update_sensitivity();
static int32        route_tree_viee_get_selected_index();

static void         gui_to_system();
static void         gui_to_node(node_t *node);
static void         gui_to_display();


    /**** exported functions ****/

void main_win_init()
{
    display_params = malloc(sizeof(display_params_t));
    display_params->show_node_names = TRUE;
    display_params->show_node_addresses = TRUE;
    display_params->show_node_tx_power = TRUE;
    display_params->show_node_ranks = TRUE;
    display_params->show_parent_arrows = TRUE;
    display_params->show_sibling_arrows = TRUE;

    gtk_builder = gtk_builder_new();

    GError *error = NULL;
    gtk_builder_add_from_file(gtk_builder, "resources/params.glade", &error);
    if (error != NULL) {
        rs_error("failed to load params ui: %s", error->message);
        return;
    }

    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(main_window), MAIN_WIN_WIDTH, MAIN_WIN_HEIGHT);
    gtk_window_set_title(GTK_WINDOW(main_window), "RPL Simulator");
    gtk_signal_connect(GTK_OBJECT(main_window), "delete-event", GTK_SIGNAL_FUNC(cb_main_window_delete), NULL);

    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(main_window), vbox);

    GtkWidget *menu_bar = create_menu_bar();
    gtk_box_pack_start(GTK_BOX(vbox), menu_bar, FALSE, TRUE, 0);

    GtkWidget *tool_bar = create_tool_bar();
    gtk_box_pack_start(GTK_BOX(vbox), tool_bar, FALSE, TRUE, 0);

    GtkWidget *content_widget = create_content_widget();
    gtk_box_pack_start(GTK_BOX(vbox), content_widget, TRUE, TRUE, 0);

    GtkWidget *status_bar = create_status_bar();
    gtk_box_pack_start(GTK_BOX(vbox), status_bar, FALSE, TRUE, 0);


    gtk_widget_show_all(main_window);

    main_win_system_to_gui();
    main_win_display_to_gui();

    gtk_builder_connect_signals(gtk_builder, NULL);
    initialize_widgets();
}

node_t *main_win_get_selected_node()
{
    return selected_node;
}

void main_win_set_selected_node(node_t *node)
{
    selected_node = node;

    if (node != NULL) {
        main_win_node_to_gui(node);
    }

    sim_field_redraw();
    update_sensitivity();
}

void main_win_system_to_gui()
{
    signals_disable();

    rs_assert(rs_system != NULL);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_no_link_dist_spin), rs_system_get_no_link_dist_thresh());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_no_link_quality_spin), rs_system_get_no_link_quality_thresh() * 100);

    switch (rs_system_get_transmit_mode()) {
        case PHY_TRANSMIT_MODE_BLOCK :
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_transmit_mode_block_radio), TRUE);
            break;

        case PHY_TRANSMIT_MODE_REJECT :
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_transmit_mode_reject_radio), TRUE);
            break;

        case PHY_TRANSMIT_MODE_QUEUE :
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_transmit_mode_queue_radio), TRUE);
            break;
    }

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_width_spin), rs_system_get_width());
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_height_spin), rs_system_get_height());

    /* add all the current possible next-hops,
     * and all the possible destination addresses */
    system_lock();

    uint16 node_count, i;
    node_t **node_list = rs_system_get_node_list(&node_count);
    gtk_list_store_clear(params_nodes_route_next_hop_store);
    gtk_list_store_clear(params_nodes_route_dst_store);
    gtk_list_store_clear(params_nodes_ping_node_store);

    gtk_list_store_insert_with_values(params_nodes_ping_node_store, NULL, -1, 0, "Any", -1);

    for (i = 0; i < node_count; i++) {
        node_t *node = node_list[i];

        gtk_list_store_insert_with_values(params_nodes_route_next_hop_store, NULL, -1, 0, phy_node_get_name(node), -1);
        gtk_list_store_insert_with_values(params_nodes_route_dst_store, NULL, -1, 0, ip_node_get_address(node), -1);
        gtk_list_store_insert_with_values(params_nodes_ping_node_store, NULL, -1, 0, phy_node_get_name(node), -1);
    }

    system_unlock();

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(params_nodes_route_next_hop_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_route_next_hop_combo), 0);
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(params_nodes_route_dst_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_route_dst_combo), 0);
    gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_route_type_combo), 0);

    signals_enable();
}

void main_win_node_to_gui(node_t *node)
{
    signals_disable();

    /* phy */
    phy_node_lock(node);

    gtk_entry_set_text(GTK_ENTRY(params_nodes_name_entry), phy_node_get_name(node));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_x_spin), phy_node_get_x(node));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_y_spin), phy_node_get_y(node));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_tx_power_spin), phy_node_get_tx_power(node) * 100);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_bat_level_spin), phy_node_get_battery_level(node) * 100);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_mains_powered_check), phy_node_is_mains_powered(node));

    phy_node_unlock(node);

    /* mac */
    mac_node_lock(node);

    gtk_entry_set_text(GTK_ENTRY(params_nodes_mac_address_entry), mac_node_get_address(node));

    mac_node_unlock(node);

    /* ip */
    ip_node_lock(node);

    gtk_entry_set_text(GTK_ENTRY(params_nodes_ip_address_entry), ip_node_get_address(node));

    /* ip route */
    gtk_list_store_clear(params_nodes_route_store);
    uint16 route_count, i;
    ip_route_t **route_list = ip_node_get_route_list(node, &route_count);
    GtkTreeIter iter;
    char destination[256];
    for (i = 0; i < route_count; i++) {
        ip_route_t *route = route_list[i];

        snprintf(destination, sizeof(destination), "%s/%d", route->dst, route->prefix_len);

        gtk_list_store_append(params_nodes_route_store, &iter);
        gtk_list_store_set(params_nodes_route_store, &iter, 0, destination, -1);
    }

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(params_nodes_route_tree_view));
    GtkTreePath *path = gtk_tree_path_new_first();
    gtk_tree_selection_select_path(selection, path);
    gtk_tree_path_free(path);
    cb_params_nodes_route_tree_view_cursor_changed(GTK_TREE_VIEW(params_nodes_route_tree_view), NULL);

    ip_node_unlock(node);

    /* icmp */
    icmp_node_lock(node);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_enable_ping_measurements_check), icmp_node_is_enabled_ping_measurement(node));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_ping_interval_spin), icmp_node_get_ping_interval(node) / 1000);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_ping_timeout_spin), icmp_node_get_ping_timeout(node) / 1000);

    if (icmp_node_get_ping_node(node) == NULL) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_ping_node_combo), 0);
    }
    else {
        system_lock();

        int pos = rs_system_get_node_pos(icmp_node_get_ping_node(node));
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_ping_node_combo), pos + 1);

        system_unlock();
    }

    icmp_node_unlock(node);

    /* rpl */
    rpl_node_lock(node);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_rank_spin), rpl_node_get_rank(node));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_seq_num_spin), rpl_node_get_seq_num(node));

    rpl_node_unlock(node);

    update_sensitivity();

    signals_enable();
}

void main_win_display_to_gui()
{
    signals_disable();

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_node_names_check), display_params->show_node_names);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_node_addresses_check), display_params->show_node_addresses);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_node_tx_power_check), display_params->show_node_tx_power);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_node_ranks_check), display_params->show_node_ranks);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_parent_arrows_check), display_params->show_parent_arrows);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_show_sibling_arrows_check), display_params->show_sibling_arrows);

    signals_enable();
}

display_params_t *main_win_get_display_params()
{
    return display_params;
}

void main_win_event_after_node_wake(node_t *node)
{
    sim_field_redraw();

    if (main_window != NULL)
        update_sensitivity();
}

void main_win_event_before_node_kill(node_t *node)
{
    sim_field_redraw();

    if (main_window != NULL)
        update_sensitivity();
}


    /**** local functions ****/

void cb_params_system_button_clicked(GtkWidget *widget, gpointer data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    gtk_widget_set_visible(params_system_vbox, TRUE);
    gtk_widget_set_visible(params_nodes_vbox, FALSE);
    gtk_widget_set_visible(params_display_vbox, FALSE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_system_button), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_button), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_button), FALSE);

    signal_leave();
}

void cb_params_nodes_button_clicked(GtkWidget *widget, gpointer data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    gtk_widget_set_visible(params_system_vbox, FALSE);
    gtk_widget_set_visible(params_nodes_vbox, TRUE);
    gtk_widget_set_visible(params_display_vbox, FALSE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_system_button), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_button), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_button), FALSE);

    signal_leave();
}

void cb_params_display_button_clicked(GtkWidget *widget, gpointer data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    gtk_widget_set_visible(params_system_vbox, FALSE);
    gtk_widget_set_visible(params_nodes_vbox, FALSE);
    gtk_widget_set_visible(params_display_vbox, TRUE);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_system_button), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_button), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_button), TRUE);

    signal_leave();
}

void cb_gui_system_updated(GtkSpinButton *spin, gpointer data)
{
    signal_enter();

    gui_to_system();
    sim_field_redraw();

    signal_leave();
}

void cb_gui_node_updated(GtkWidget *widget, gpointer data)
{
    signal_enter();

    rs_assert(selected_node != NULL);

    gui_to_node(selected_node);
    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

void cb_gui_display_updated(GtkWidget *widget, gpointer data)
{
    signal_enter();

    gui_to_display();
    sim_field_redraw();

    signal_leave();
}

void cb_params_nodes_route_tree_view_cursor_changed(GtkTreeView *tree_view, gpointer data)
{
    int32 index = route_tree_viee_get_selected_index();

    if (index < 0 || selected_node == NULL) {
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_route_dst_combo), 0);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_route_prefix_len_spin), 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_route_type_combo), 0);
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_route_next_hop_combo), 0);

        return;
    }
    else {
        ip_node_lock(selected_node);

        uint16 route_count;
        ip_route_t **route_list = ip_node_get_route_list(selected_node, &route_count);
        ip_route_t *route = route_list[index];

        int32 pos = -1;
        node_t *node = rs_system_find_node_by_ip_address(route->dst);
        if (node != NULL)
            pos = rs_system_get_node_pos(node);
        if (pos == -1) {
            gtk_list_store_insert_with_values(params_nodes_route_dst_store, NULL, -1, 0, route->dst, -1);
            pos = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(params_nodes_route_dst_store), NULL) - 1;
        }

        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_route_dst_combo), pos);

        gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_route_prefix_len_spin), route->prefix_len);
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_route_type_combo), route->type);

        pos = -1;
        if (route->next_hop != NULL)
            pos = rs_system_get_node_pos(route->next_hop);
        gtk_combo_box_set_active(GTK_COMBO_BOX(params_nodes_route_next_hop_combo), pos);

        ip_node_unlock(selected_node);
    }
}

void cb_params_nodes_route_add_button_clicked(GtkButton *button, gpointer data)
{
    signal_enter();

    // todo check for duplicates & validity

    rs_assert(selected_node != NULL);
    rs_debug(DEBUG_GUI, NULL);

    const char *dst = gtk_combo_box_get_active_text(GTK_COMBO_BOX(params_nodes_route_dst_combo));
    uint8 prefix_len = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_route_prefix_len_spin));
    uint8 type = gtk_combo_box_get_active(GTK_COMBO_BOX(params_nodes_route_type_combo));
    int32 next_hop_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(params_nodes_route_next_hop_combo));

    if (strlen(dst) == 0) {
        dst = "0";
        prefix_len = 0;
        type = IP_ROUTE_TYPE_MANUAL;
    }

    system_lock();

    uint16 node_count;
    node_t **node_list = rs_system_get_node_list(&node_count);
    node_t *next_hop = NULL;

    rs_assert(next_hop_pos < node_count);
    if (next_hop_pos >= 0) {
        next_hop = node_list[next_hop_pos];
    }

    system_unlock();

    ip_node_add_route(selected_node, type, (char *) dst, prefix_len, next_hop, FALSE);

    signal_leave();

    main_win_node_to_gui(selected_node);
}

void cb_params_nodes_route_rem_button_clicked(GtkButton *button, gpointer data)
{
    signal_enter();

    rs_assert(selected_node != NULL);
    rs_debug(DEBUG_GUI, NULL);

    int32 index = route_tree_viee_get_selected_index();

    ip_node_lock(selected_node);

    uint16 route_count;
    ip_route_t **route_list = ip_node_get_route_list(selected_node, &route_count);
    rs_assert(index < route_count);
    rs_assert(index >= 0);

    ip_route_t *route = route_list[index];
    ip_node_rem_route(selected_node, route->dst, route->prefix_len);

    ip_node_unlock(selected_node);

    signal_leave();

    main_win_node_to_gui(selected_node);
}

void cb_params_nodes_route_upd_button_clicked(GtkButton *button, gpointer data)
{
    signal_enter();

    rs_assert(selected_node != NULL);
    rs_debug(DEBUG_GUI, NULL);

    const char *dst = gtk_combo_box_get_active_text(GTK_COMBO_BOX(params_nodes_route_dst_combo));
    uint8 prefix_len = gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_route_prefix_len_spin));
    uint8 type = gtk_combo_box_get_active(GTK_COMBO_BOX(params_nodes_route_type_combo));
    int32 next_hop_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(params_nodes_route_next_hop_combo));

    system_lock();

    uint16 node_count;
    node_t **node_list = rs_system_get_node_list(&node_count);
    node_t *next_hop = NULL;

    rs_assert(next_hop_pos < node_count);
    if (next_hop_pos >= 0) {
        next_hop = node_list[next_hop_pos];
    }

    system_unlock();

    int32 index = route_tree_viee_get_selected_index();

    ip_node_lock(selected_node);

    uint16 route_count;
    ip_route_t **route_list = ip_node_get_route_list(selected_node, &route_count);
    rs_assert(index < route_count);
    rs_assert(index >= 0);

    ip_route_t *route = route_list[index];


    if (route->dst != NULL)
        free(route->dst);
    route->dst = strdup(dst);
    route->prefix_len = prefix_len;
    route->next_hop = next_hop;
    route->type = type;

    ip_node_unlock(selected_node);

    signal_leave();

    main_win_node_to_gui(selected_node);
}

void cb_params_nodes_route_dst_combo_changed(GtkComboBoxEntry *combo_box, gpointer data)
{
    char *dst = gtk_combo_box_get_active_text(GTK_COMBO_BOX(params_nodes_route_dst_combo));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_route_prefix_len_spin), strlen(dst) * 4);
}

void cb_params_nodes_ping_timeout_spin_changed(GtkSpinButton *spin, gpointer data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    update_sensitivity();

    signal_leave();
}


static void cb_open_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_save_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

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

    rs_start();

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

    node_t *node = rs_add_node();
    main_win_set_selected_node(node);
    sim_field_redraw();

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

    rs_add_more_nodes();

    sim_field_redraw();
    update_sensitivity();

    signal_leave();
}

static void cb_remove_all_menu_item_activate(GtkWidget *widget, gpointer *data)
{
    signal_enter();

    rs_debug(DEBUG_GUI, NULL);

    rs_rem_all_nodes();

    main_win_set_selected_node(NULL);

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
            "Ali Moshfegh <ali.moshfegh@gmail.com>",
            "Calin Crisan <ccrisan@gmail.com>",
            NULL
    };

    const gchar *description = "A simple LLN/RPL Simulator using GTK+";

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

    gtk_about_dialog_set_name(GTK_ABOUT_DIALOG(about_dialog), "RPL Simulator");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dialog), RS_VERSION);
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dialog), "Copyright \xc2\xa9 2010 Calin Crisan <ccrisan@gmail.com>");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dialog), authors);
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dialog), description);
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about_dialog), license);
    gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(about_dialog), TRUE);

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
    gtk_widget_set_size_request(scrolled_window, 300, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *params_table = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_table");
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), params_table);

    params_system_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_button");
    params_system_vbox = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_vbox");
    params_system_no_link_dist_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_no_link_dist_spin");
    params_system_no_link_quality_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_no_link_quality_spin");
    params_transmit_mode_block_radio = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_transmit_mode_block_radio");
    params_transmit_mode_reject_radio = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_transmit_mode_reject_radio");
    params_transmit_mode_queue_radio = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_transmit_mode_queue_radio");
    params_system_width_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_width_spin");
    params_system_height_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_height_spin");

    params_nodes_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_button");
    params_nodes_vbox = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_vbox");
    params_nodes_name_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_name_entry");
    params_nodes_x_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_x_spin");
    params_nodes_y_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_y_spin");
    params_nodes_tx_power_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_tx_power_spin");
    params_nodes_bat_level_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_bat_level_spin");
    params_nodes_mains_powered_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mains_powered_check");
    params_nodes_mac_address_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mac_address_entry");
    params_nodes_ip_address_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_ip_address_entry");
    params_nodes_route_dst_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_dst_combo");
    params_nodes_route_dst_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "params_nodes_route_dst_store");
    params_nodes_route_prefix_len_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_prefix_len_spin");
    params_nodes_route_next_hop_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_next_hop_combo");
    params_nodes_route_next_hop_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "params_nodes_route_next_hop_store");
    params_nodes_route_type_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_type_combo");
    params_nodes_route_type_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "params_nodes_route_type_store");
    params_nodes_route_add_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_add_button");
    params_nodes_route_rem_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_rem_button");
    params_nodes_route_upd_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_upd_button");
    params_nodes_route_tree_view = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_route_tree_view");
    params_nodes_route_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "params_nodes_route_store");
    params_nodes_enable_ping_measurements_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_enable_ping_measurements_check");
    params_nodes_ping_interval_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_ping_interval_spin");
    params_nodes_ping_timeout_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_ping_timeout_spin");
    params_nodes_ping_node_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_ping_node_combo");
    params_nodes_ping_node_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "params_nodes_ping_node_store");
    params_nodes_rank_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_rank_spin");
    params_nodes_seq_num_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_seq_num_spin");

    params_display_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_button");
    params_display_vbox = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_vbox");
    params_display_show_node_names_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_node_names_check");
    params_display_show_node_addresses_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_node_addresses_check");
    params_display_show_node_tx_power_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_node_tx_power_check");
    params_display_show_node_ranks_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_node_ranks_check");
    params_display_show_parent_arrows_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_parent_arrows_check");
    params_display_show_sibling_arrows_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_show_sibling_arrows_check");

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(params_nodes_route_type_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(params_nodes_route_type_combo), renderer, "text", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(params_nodes_route_next_hop_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(params_nodes_route_next_hop_combo), renderer, "text", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(params_nodes_ping_node_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(params_nodes_ping_node_combo), renderer, "text", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(params_nodes_route_tree_view),
            -1,
            "Destination",
            renderer,
            "text", 0,
            NULL);

    return scrolled_window;
}

//GtkWidget *create_monitoring_widget()
//{
//    GtkWidget *label = gtk_label_new("Monitoring");
//
//    return label;
//}

GtkWidget *create_menu_bar()
{
    GtkWidget *menu_bar = gtk_menu_bar_new();

    /* file menu */
    GtkWidget *file_menu = gtk_menu_new();


    open_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_OPEN, NULL);
    gtk_signal_connect(GTK_OBJECT(open_menu_item), "activate", GTK_SIGNAL_FUNC(cb_open_menu_item_activate), NULL);
    gtk_menu_append(file_menu, open_menu_item);

    save_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_SAVE_AS, NULL);
    gtk_signal_connect(GTK_OBJECT(save_menu_item), "activate", GTK_SIGNAL_FUNC(cb_save_menu_item_activate), NULL);
    gtk_menu_append(file_menu, save_menu_item);

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
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(add_node_toolbar_item), 0);

    rem_node_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_REMOVE);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(rem_node_toolbar_item), "Remove the selected node");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(rem_node_toolbar_item), "Remove Node");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(rem_node_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(rem_node_toolbar_item), "clicked", G_CALLBACK(cb_rem_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(rem_node_toolbar_item), 1);

    GtkToolItem *sep_toolbar_item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep_toolbar_item, 2);

    start_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(start_toolbar_item), "Start simulation");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(start_toolbar_item), "Start");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(start_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(start_toolbar_item), "clicked", G_CALLBACK(cb_start_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(start_toolbar_item), 3);

    stop_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(stop_toolbar_item), "Stop simulation");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(stop_toolbar_item), "Stop");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(stop_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(stop_toolbar_item), "clicked", G_CALLBACK(cb_stop_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(stop_toolbar_item), 4);

    sep_toolbar_item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep_toolbar_item, 5);

    wake_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_APPLY);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(wake_toolbar_item), "Wake Node");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(wake_toolbar_item), "Wake");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(wake_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(wake_toolbar_item), "clicked", G_CALLBACK(cb_wake_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(wake_toolbar_item), 6);

    kill_toolbar_item = (GtkWidget *) gtk_tool_button_new_from_stock(GTK_STOCK_CANCEL);
    gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(kill_toolbar_item), "Kill Node");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(kill_toolbar_item), "Kill");
    gtk_tool_item_set_is_important(GTK_TOOL_ITEM(kill_toolbar_item), TRUE);
    gtk_signal_connect(GTK_OBJECT(kill_toolbar_item), "clicked", G_CALLBACK(cb_kill_menu_item_activate), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(kill_toolbar_item), 7);

    return toolbar;
}

GtkWidget *create_status_bar()
{
    GtkWidget *status_bar = gtk_statusbar_new();

    // todo: statusbar: node count, alive, dead, removed
    // todo: statusbar: status (running/stopped)
    // todo: statusbar: current field (x, y)
    // todo: statusbar: real to simulated time ratio


    return status_bar;
}

GtkWidget *create_content_widget()
{
    GtkWidget *hbox = gtk_hbox_new(FALSE, 5);

    GtkWidget *params_widget = create_params_widget();
    gtk_box_pack_start(GTK_BOX(hbox), params_widget, FALSE, TRUE, 0);

    GtkWidget *sim_field = sim_field_create();
    gtk_box_pack_start(GTK_BOX(hbox), sim_field, TRUE, TRUE, 0);

    // todo this should be a legend
//    GtkWidget *monitoring_widget = create_monitoring_widget();
//    gtk_box_pack_start(GTK_BOX(hbox), monitoring_widget, FALSE, TRUE, 0);

    return hbox;
}

static void initialize_widgets()
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_system_button), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_button), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_button), FALSE);

    update_sensitivity();
}

static void update_sensitivity()
{
    uint16 node_count;
    rs_system_get_node_list(&node_count);

    bool mains_powered = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_mains_powered_check));
    bool node_selected = selected_node != NULL;
    bool has_nodes = node_count > 0;
    bool node_alive = node_selected && selected_node->alive;
    bool route_selected = gtk_tree_selection_count_selected_rows(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(params_nodes_route_tree_view))) > 0;
    bool ping_enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_enable_ping_measurements_check));

    gtk_widget_set_sensitive(params_nodes_name_entry, node_selected);
    gtk_widget_set_sensitive(params_nodes_x_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_y_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_tx_power_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_bat_level_spin, node_selected && !mains_powered);
    gtk_widget_set_sensitive(params_nodes_mains_powered_check, node_selected);
    gtk_widget_set_sensitive(params_nodes_mac_address_entry, node_selected);
    gtk_widget_set_sensitive(params_nodes_ip_address_entry, node_selected);
    gtk_widget_set_sensitive(params_nodes_route_dst_combo, node_selected);
    gtk_widget_set_sensitive(params_nodes_route_prefix_len_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_route_next_hop_combo, node_selected);
    gtk_widget_set_sensitive(params_nodes_route_type_combo, node_selected);
    gtk_widget_set_sensitive(params_nodes_route_add_button, node_selected);
    gtk_widget_set_sensitive(params_nodes_route_rem_button, node_selected && route_selected);
    gtk_widget_set_sensitive(params_nodes_route_upd_button, node_selected && route_selected);
    gtk_widget_set_sensitive(params_nodes_route_tree_view, node_selected);
    gtk_widget_set_sensitive(params_nodes_enable_ping_measurements_check, node_selected);
    gtk_widget_set_sensitive(params_nodes_ping_interval_spin, node_selected && ping_enabled);
    gtk_widget_set_sensitive(params_nodes_ping_timeout_spin, node_selected && ping_enabled);
    gtk_widget_set_sensitive(params_nodes_ping_node_combo, node_selected && ping_enabled);
    gtk_widget_set_sensitive(params_nodes_rank_spin, node_selected);
    gtk_widget_set_sensitive(params_nodes_seq_num_spin, node_selected);

    gtk_widget_set_sensitive(rem_node_toolbar_item, node_selected);
    gtk_widget_set_sensitive(rem_menu_item, node_selected);

    gtk_widget_set_sensitive(remove_all_menu_item, has_nodes);
    gtk_widget_set_sensitive(kill_all_menu_item, has_nodes);
    gtk_widget_set_sensitive(wake_all_menu_item, has_nodes);

    gtk_widget_set_sensitive(wake_menu_item, node_selected && !node_alive);
    gtk_widget_set_sensitive(wake_toolbar_item, node_selected && !node_alive);
    gtk_widget_set_sensitive(kill_menu_item, node_selected && node_alive);
    gtk_widget_set_sensitive(kill_toolbar_item, node_selected && node_alive);
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

static void gui_to_system()
{
    rs_assert(rs_system != NULL);

    rs_system_set_no_link_dist_thresh(gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_no_link_dist_spin)));
    rs_system_set_no_link_quality_thresh(gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_no_link_quality_spin)) / 100.0);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_transmit_mode_block_radio))) {
        rs_system_set_transmit_mode(PHY_TRANSMIT_MODE_BLOCK);
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_transmit_mode_reject_radio))) {
        rs_system_set_transmit_mode(PHY_TRANSMIT_MODE_REJECT);
    }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_transmit_mode_queue_radio))) {
        rs_system_set_transmit_mode(PHY_TRANSMIT_MODE_QUEUE);
    }

    rs_system_set_width_height(
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_width_spin)),
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_height_spin))
    );
}

static void gui_to_node(node_t *node)
{
    /* phy */
    phy_node_lock(node);

    phy_node_set_name(node, gtk_entry_get_text(GTK_ENTRY(params_nodes_name_entry)));


    phy_node_set_xy(node,
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_x_spin)),
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_y_spin))
    );

    phy_node_set_tx_power(node, gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_tx_power_spin)) / 100.0);

    phy_node_set_battery_level(node, gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_bat_level_spin)) / 100.0);

    phy_node_set_mains_powered(node, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_mains_powered_check)));

    phy_node_unlock(node);

    /* mac */
    mac_node_lock(node);

    mac_node_set_address(node, gtk_entry_get_text(GTK_ENTRY(params_nodes_mac_address_entry)));

    mac_node_unlock(node);

    /* ip */
    ip_node_lock(node);

    ip_node_set_address(node, gtk_entry_get_text(GTK_ENTRY(params_nodes_ip_address_entry)));

    ip_node_unlock(node);

    /* icmp */
    icmp_node_lock(node);

    icmp_node_set_enable_ping_measurement(node, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_enable_ping_measurements_check)));
    icmp_node_set_ping_interval(node, gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_ping_interval_spin)) * 1000);
    icmp_node_set_ping_timeout(node, gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_ping_timeout_spin)) * 1000);

    system_lock();
    uint16 node_count;
    node_t **node_list = rs_system_get_node_list(&node_count);
    int16 pos = gtk_combo_box_get_active(GTK_COMBO_BOX(params_nodes_ping_node_combo));
    if (pos < 1 || pos > node_count)
        icmp_node_set_ping_node(node, NULL);
    else
        icmp_node_set_ping_node(node, node_list[pos - 1]);
    system_unlock();

    icmp_node_unlock(node);

    /* rpl */
    rpl_node_lock(node);

    rpl_node_set_rank(node, gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_rank_spin)));
    rpl_node_set_seq_num(node, gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_seq_num_spin)));

    rpl_node_unlock(node);
}

static void gui_to_display()
{
    display_params->show_node_names = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_node_names_check));
    display_params->show_node_addresses = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_node_addresses_check));
    display_params->show_node_tx_power = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_node_tx_power_check));
    display_params->show_node_ranks = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_node_ranks_check));
    display_params->show_parent_arrows = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_parent_arrows_check));
    display_params->show_sibling_arrows = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_display_show_sibling_arrows_check));
}

