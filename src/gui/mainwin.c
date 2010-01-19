#include <gdk/gdk.h>
#include <cairo.h>

#include "mainwin.h"
#include "simfield.h"
#include "../node.h"
#include "../system.h"
#include "../main.h"


    /**** global variables ****/

static GtkBuilder *gtk_builder = NULL;

    /* params widgets */
static GtkWidget *params_system_button = NULL;
static GtkWidget *params_system_no_link_dist_spin = NULL;
static GtkWidget *params_transmit_mode_block_radio = NULL;
static GtkWidget *params_transmit_mode_reject_radio = NULL;
static GtkWidget *params_transmit_mode_queue_radio = NULL;
static GtkWidget *params_system_width_spin = NULL;
static GtkWidget *params_system_height_spin = NULL;

static GtkWidget *params_nodes_button = NULL;
static GtkWidget *params_nodes_name_entry = NULL;
static GtkWidget *params_nodes_x_spin = NULL;
static GtkWidget *params_nodes_y_spin = NULL;
static GtkWidget *params_nodes_tx_power_spin = NULL;
static GtkWidget *params_nodes_bat_level_spin = NULL;
static GtkWidget *params_nodes_mains_powered_check = NULL;
static GtkWidget *params_nodes_mac_address_entry = NULL;
static GtkWidget *params_nodes_ip_address_entry = NULL;

static GtkWidget *params_display_button = NULL;


    /**** local function prototypes ****/

void                cb_expander_toggle_button_clicked(GtkWidget *widget, GtkToggleButton *button);
void                cb_mains_powered_check_button_toggled(GtkToggleButton *button, gpointer data);
void                cb_width_height_spin_value_changed(GtkSpinButton *spin, gpointer data);

static void         cb_add_node_tool_button_clicked(GtkWidget *widget, gpointer *data);
static void         cb_rem_node_tool_button_clicked(GtkWidget *widget, gpointer *data);
static void         cb_start_tool_button_clicked(GtkWidget *widget, gpointer *data);
static void         cb_stop_tool_button_clicked(GtkWidget *widget, gpointer *data);

static void         cb_quit_menu_item_activate(GtkMenuItem *widget, gpointer user_data);
static void         cb_main_window_delete();

static GtkWidget *  create_params_widget();
static GtkWidget *  create_monitoring_widget();
static GtkWidget *  create_menu_bar();
static GtkWidget *  create_tool_bar();
static GtkWidget *  create_status_bar();
static GtkWidget *  create_content_widget();

static void         initialize_widgets();
static void         update_sensitivity();

static void         system_to_gui();
static void         gui_to_system();

static void         node_to_gui(node_t *node);
static void         gui_to_node(node_t *node);


    /**** exported functions ****/

void main_win_init()
{
    gtk_builder = gtk_builder_new();

    GError *error = NULL;
    gtk_builder_add_from_file(gtk_builder, "resources/params.glade", &error);
    if (error != NULL) {
        rs_error("failed to load params ui: %s", error->message);
        return;
    }

    GtkWidget *main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
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

    initialize_widgets();
    system_to_gui();

    gtk_builder_connect_signals(gtk_builder, NULL);
}


    /**** local functions ****/

void cb_expander_toggle_button_clicked(GtkWidget *widget, GtkToggleButton *button)
{
    gtk_widget_set_visible(widget, gtk_toggle_button_get_active(button));
}

void cb_mains_powered_check_button_toggled(GtkToggleButton *button, gpointer data)
{
    update_sensitivity();
}

void cb_width_height_spin_value_changed(GtkSpinButton *spin, gpointer data)
{
    gui_to_system();

    sim_field_redraw();
}


static void cb_quit_menu_item_activate(GtkMenuItem *widget, gpointer user_data)
{
    rs_debug(NULL);

    rs_quit();
}

static void cb_add_node_tool_button_clicked(GtkWidget *widget, gpointer *data)
{
    rs_debug(NULL);
}

static void cb_rem_node_tool_button_clicked(GtkWidget *widget, gpointer *data)
{
    rs_debug(NULL);
}

static void cb_start_tool_button_clicked(GtkWidget *widget, gpointer *data)
{
    rs_debug(NULL);
}

static void cb_stop_tool_button_clicked(GtkWidget *widget, gpointer *data)
{
    rs_debug(NULL);
}

static void cb_main_window_delete()
{
    rs_debug(NULL);

    rs_quit();
}

static void update_sensitivity()
{
    gtk_widget_set_sensitive(params_nodes_bat_level_spin, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_mains_powered_check)));
}

static void initialize_widgets()
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_system_button), TRUE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_button), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_display_button), FALSE);

    /* force a visibility update in the params panel */
    gtk_signal_emit_by_name(GTK_OBJECT(params_system_button), "toggled");
    gtk_signal_emit_by_name(GTK_OBJECT(params_nodes_button), "toggled");
    gtk_signal_emit_by_name(GTK_OBJECT(params_display_button), "toggled");

    update_sensitivity();
}

static void system_to_gui()
{
    rs_assert(rs_system != NULL);

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_system_no_link_dist_spin), rs_system_get_no_link_dist_thresh());

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
}

static void gui_to_system()
{
    rs_assert(rs_system != NULL);

    rs_system_set_no_link_dist_thresh(gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_system_no_link_dist_spin)));

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

static void node_to_gui(node_t *node)
{
    /* phy */
    gtk_entry_set_text(GTK_ENTRY(params_nodes_name_entry), phy_node_get_name(node));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_x_spin), phy_node_get_x(node));
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_y_spin), phy_node_get_y(node));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_tx_power_spin), phy_node_get_tx_power(node));

    gtk_spin_button_set_value(GTK_SPIN_BUTTON(params_nodes_bat_level_spin), phy_node_get_battery_level(node));

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(params_nodes_mains_powered_check), phy_node_is_mains_powered(node));

    /* mac */
    gtk_entry_set_text(GTK_ENTRY(params_nodes_mac_address_entry), mac_node_get_address(node));

    /* ip */
    gtk_entry_set_text(GTK_ENTRY(params_nodes_ip_address_entry), ip_node_get_address(node));
}

static void gui_to_node(node_t *node)
{
    /* phy */
    phy_node_set_name(node, gtk_entry_get_text(GTK_ENTRY(params_nodes_name_entry)));

    phy_node_set_xy(node,
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_x_spin)),
            gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_y_spin))
    );

    phy_node_set_tx_power(node, gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_tx_power_spin)));

    phy_node_set_battery_level(node, gtk_spin_button_get_value(GTK_SPIN_BUTTON(params_nodes_bat_level_spin)));

    phy_node_set_mains_powered(node, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(params_nodes_mains_powered_check)));

    /* mac */
    mac_node_set_address(node, gtk_entry_get_text(GTK_ENTRY(params_nodes_mac_address_entry)));

    /* ip */
    ip_node_set_address(node, gtk_entry_get_text(GTK_ENTRY(params_nodes_ip_address_entry)));
}


    /**** widget creation ****/

GtkWidget *create_params_widget()
{
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, 250, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *params_table = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_table");
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), params_table);

    params_system_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_button");
    params_system_no_link_dist_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_no_link_dist_spin");
    params_transmit_mode_block_radio = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_transmit_mode_block_radio");
    params_transmit_mode_reject_radio = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_transmit_mode_reject_radio");
    params_transmit_mode_queue_radio = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_transmit_mode_queue_radio");
    params_system_width_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_width_spin");
    params_system_height_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_system_height_spin");

    params_nodes_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_button");
    params_nodes_name_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_name_entry");
    params_nodes_x_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_x_spin");
    params_nodes_y_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_y_spin");
    params_nodes_tx_power_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_tx_power_spin");
    params_nodes_bat_level_spin = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_bat_level_spin");
    params_nodes_mains_powered_check = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mains_powered_check");
    params_nodes_mac_address_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_mac_address_entry");
    params_nodes_ip_address_entry = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_nodes_ip_address_entry");

    params_display_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "params_display_button");

    return scrolled_window;
}

GtkWidget *create_monitoring_widget()
{
    GtkWidget *label = gtk_label_new("Monitoring");

    return label;
}

GtkWidget *create_menu_bar()
{
    GtkWidget *menu_bar = gtk_menu_bar_new();

    /* file menu */
    GtkWidget *file_menu = gtk_menu_new();

    gtk_menu_append(file_menu, gtk_separator_menu_item_new());

    GtkWidget *quit_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
    gtk_signal_connect(GTK_OBJECT(quit_menu_item), "activate", GTK_SIGNAL_FUNC(cb_quit_menu_item_activate), NULL);
    gtk_menu_append(file_menu, quit_menu_item);

    GtkWidget *file_menu_item = gtk_menu_item_new_with_mnemonic("_File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_menu_item), file_menu);
    gtk_menu_bar_append(menu_bar, file_menu_item);

    /* simulation menu */
    GtkWidget *simulation_menu = gtk_menu_new();

    GtkWidget *start_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PLAY, NULL);
    gtk_menu_append(simulation_menu, start_menu_item);

    GtkWidget *stop_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_STOP, NULL);
    gtk_menu_append(simulation_menu, stop_menu_item);

    GtkWidget *simulation_menu_item = gtk_menu_item_new_with_mnemonic("_Simulation");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(simulation_menu_item), simulation_menu);
    gtk_menu_bar_append(menu_bar, simulation_menu_item);

    /* help menu */
    GtkWidget *help_menu = gtk_menu_new();

    GtkWidget *about_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
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

    GtkToolItem *add_node_toolbar_item = gtk_tool_button_new_from_stock(GTK_STOCK_ADD);
    gtk_tool_item_set_tooltip_text(add_node_toolbar_item, "Add a new node");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(add_node_toolbar_item), "Add Node");
    gtk_tool_item_set_is_important(add_node_toolbar_item, TRUE);
    gtk_signal_connect(GTK_OBJECT(add_node_toolbar_item), "clicked", G_CALLBACK(cb_add_node_tool_button_clicked), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), add_node_toolbar_item, 0);

    GtkToolItem *rem_node_toolbar_item = gtk_tool_button_new_from_stock(GTK_STOCK_REMOVE);
    gtk_tool_item_set_tooltip_text(rem_node_toolbar_item, "Remove the selected node");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(rem_node_toolbar_item), "Remove Node");
    gtk_tool_item_set_is_important(rem_node_toolbar_item, TRUE);
    gtk_signal_connect(GTK_OBJECT(rem_node_toolbar_item), "clicked", G_CALLBACK(cb_rem_node_tool_button_clicked), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), rem_node_toolbar_item, 1);

    GtkToolItem *sep_toolbar_item = gtk_separator_tool_item_new();
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), sep_toolbar_item, 2);

    GtkToolItem *start_toolbar_item = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
    gtk_tool_item_set_tooltip_text(start_toolbar_item, "Start simulation");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(start_toolbar_item), "Start");
    gtk_tool_item_set_is_important(start_toolbar_item, TRUE);
    gtk_signal_connect(GTK_OBJECT(start_toolbar_item), "clicked", G_CALLBACK(cb_start_tool_button_clicked), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), start_toolbar_item, 3);

    GtkToolItem *stop_toolbar_item = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
    gtk_tool_item_set_tooltip_text(stop_toolbar_item, "Stop simulation");
    gtk_tool_button_set_label(GTK_TOOL_BUTTON(stop_toolbar_item), "Stop");
    gtk_tool_item_set_is_important(stop_toolbar_item, TRUE);
    gtk_signal_connect(GTK_OBJECT(stop_toolbar_item), "clicked", G_CALLBACK(cb_stop_tool_button_clicked), NULL);
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), stop_toolbar_item, 4);

    return toolbar;
}

GtkWidget *create_status_bar()
{
    GtkWidget *status_bar = gtk_statusbar_new();

    return status_bar;
}

GtkWidget *create_content_widget()
{
    GtkWidget *hbox = gtk_hbox_new(FALSE, 5);

    GtkWidget *params_widget = create_params_widget();
    gtk_box_pack_start(GTK_BOX(hbox), params_widget, FALSE, TRUE, 0);

    GtkWidget *sim_field = sim_field_create(NULL, NULL);// todo: fix this
    gtk_box_pack_start(GTK_BOX(hbox), sim_field, TRUE, TRUE, 0);

    GtkWidget *monitoring_widget = create_monitoring_widget();
    gtk_box_pack_start(GTK_BOX(hbox), monitoring_widget, FALSE, TRUE, 0);

    return hbox;
}
