
#include "measurement.h"

#include "mainwin.h"
#include "../system.h"
#include "../measure.h"



    /**** global variables ****/

static GtkWidget *              measures_connect_src_combo;
static GtkWidget *              measures_connect_dst_combo;
static GtkWidget *              measures_connect_tree_view;
static GtkWidget *              measures_connect_add_button;
static GtkWidget *              measures_connect_rem_button;
static GtkWidget *              measures_connect_connected_time_label;
static GtkWidget *              measures_connect_total_time_label;
static GtkWidget *              measures_connect_measure_time_label;
static GtkWidget *              measures_connect_progress;
static GtkListStore *           measures_connect_src_store;
static GtkListStore *           measures_connect_dst_store;
static GtkListStore *           measures_connect_store;

static GtkWidget *              measures_converg_connected_nodes_label;
static GtkWidget *              measures_converg_total_nodes_label;
static GtkWidget *              measures_converg_measure_time_label;
static GtkWidget *              measures_converg_progress;

static GtkWidget *              measures_sp_comp_src_combo;
static GtkWidget *              measures_sp_comp_dst_combo;
static GtkWidget *              measures_sp_comp_tree_view;
static GtkWidget *              measures_sp_comp_add_button;
static GtkWidget *              measures_sp_comp_rem_button;
static GtkWidget *              measures_sp_comp_progress;
static GtkWidget *              measures_sp_comp_rpl_cost_label;
static GtkWidget *              measures_sp_comp_sp_cost_label;
static GtkWidget *              measures_sp_comp_measure_time_label;
static GtkListStore *           measures_sp_comp_src_store;
static GtkListStore *           measures_sp_comp_dst_store;
static GtkListStore *           measures_sp_comp_store;


    /**** local function prototypes ****/

void                            cb_measures_connect_add_button_clicked(GtkWidget *widget, gpointer data);
void                            cb_measures_connect_rem_button_clicked(GtkWidget *widget, gpointer data);
void                            cb_measures_connect_tree_view_cursor_changed(GtkWidget *widget, gpointer data);

void                            cb_measures_sp_comp_add_button_clicked(GtkWidget *widget, gpointer data);
void                            cb_measures_sp_comp_rem_button_clicked(GtkWidget *widget, gpointer data);
void                            cb_measures_sp_comp_tree_view_cursor_changed(GtkWidget *widget, gpointer data);

static void                     update_sensitivity();
static int32                    connect_tree_viee_get_selected_index();
static int32                    sp_comp_tree_viee_get_selected_index();


    /**** exported functions ****/

GtkWidget *measurement_widget_create()
{
    GtkWidget *measures_widget = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_vbox");

    measures_connect_src_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_connect_src_combo");
    measures_connect_dst_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_connect_dst_combo");
    measures_connect_tree_view = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_connect_tree_view");
    measures_connect_add_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_connect_add_button");
    measures_connect_rem_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_connect_rem_button");
    measures_connect_connected_time_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_connect_connected_time_label");
    measures_connect_total_time_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_connect_total_time_label");
    measures_connect_measure_time_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_connect_measure_time_label");
    measures_connect_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_connect_progress");
    measures_connect_src_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "measures_connect_src_store");
    measures_connect_dst_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "measures_connect_dst_store");
    measures_connect_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "measures_connect_store");

    measures_converg_connected_nodes_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_converg_connected_nodes_label");
    measures_converg_total_nodes_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_converg_total_nodes_label");
    measures_converg_measure_time_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_converg_measure_time_label");
    measures_converg_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_converg_progress");

    measures_sp_comp_src_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_src_combo");
    measures_sp_comp_dst_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_dst_combo");
    measures_sp_comp_tree_view = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_tree_view");
    measures_sp_comp_add_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_add_button");
    measures_sp_comp_rem_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_rem_button");
    measures_sp_comp_rpl_cost_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_rpl_cost_label");
    measures_sp_comp_sp_cost_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_sp_cost_label");
    measures_sp_comp_measure_time_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_measure_time_label");
    measures_sp_comp_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_progress");
    measures_sp_comp_src_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_src_store");
    measures_sp_comp_dst_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_dst_store");
    measures_sp_comp_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "measures_sp_comp_store");

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(measures_connect_src_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(measures_connect_src_combo), renderer, "text", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(measures_connect_dst_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(measures_connect_dst_combo), renderer, "text", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(measures_sp_comp_src_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(measures_sp_comp_src_combo), renderer, "text", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(measures_sp_comp_dst_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(measures_sp_comp_dst_combo), renderer, "text", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(measures_connect_tree_view),
            -1,
            "Source",
            renderer,
            "text", 0,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(measures_connect_tree_view),
            -1,
            "Destination",
            renderer,
            "text", 1,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(measures_sp_comp_tree_view),
            -1,
            "Source",
            renderer,
            "text", 0,
            NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(measures_sp_comp_tree_view),
            -1,
            "Destination",
            renderer,
            "text", 1,
            NULL);

    update_sensitivity();
    measurement_output_to_gui();

    return measures_widget;
}

void measurement_system_to_gui()
{
    rs_assert(rs_system != NULL);

    /* add all the possible node sources ad destinations */

    gtk_list_store_clear(measures_connect_src_store);
    gtk_list_store_clear(measures_connect_dst_store);
    gtk_list_store_clear(measures_connect_store);

    gtk_list_store_clear(measures_sp_comp_src_store);
    gtk_list_store_clear(measures_sp_comp_dst_store);
    gtk_list_store_clear(measures_sp_comp_store);

    gtk_list_store_insert_with_values(measures_connect_src_store, NULL, -1, 0, "All", -1);
    gtk_list_store_insert_with_values(measures_sp_comp_src_store, NULL, -1, 0, "All", -1);

    nodes_lock();

    uint16 i;
    for (i = 0; i < rs_system->node_count; i++) {
        node_t *node = rs_system->node_list[i];

        gtk_list_store_insert_with_values(measures_connect_src_store, NULL, -1, 0, node->phy_info->name, -1);
        gtk_list_store_insert_with_values(measures_connect_dst_store, NULL, -1, 0, node->phy_info->name, -1);

        gtk_list_store_insert_with_values(measures_sp_comp_src_store, NULL, -1, 0, node->phy_info->name, -1);
        gtk_list_store_insert_with_values(measures_sp_comp_dst_store, NULL, -1, 0, node->phy_info->name, -1);
    }

    nodes_unlock();

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(measures_connect_src_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(measures_connect_src_combo), 0);
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(measures_connect_dst_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(measures_connect_dst_combo), 0);
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(measures_sp_comp_src_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(measures_sp_comp_src_combo), 0);
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(measures_sp_comp_dst_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(measures_sp_comp_dst_combo), 0);

    measurement_entries_to_gui();
}

void measurement_entries_to_gui()
{
    measures_lock();

    gtk_list_store_clear(measures_connect_store);

    uint16 i;
    GtkTreeIter iter;
    for (i = 0; i < measure_connect_entry_get_count(); i++) {
        measure_connect_t *measure = measure_connect_entry_get(i);

        gtk_list_store_append(measures_connect_store, &iter);
        gtk_list_store_set(measures_connect_store, &iter,
                0, (measure->src_node != NULL ? measure->src_node->phy_info->name : "All"),
                1, measure->dst_node->phy_info->name,
                -1);
    }

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(measures_connect_tree_view));
    GtkTreePath *path = gtk_tree_path_new_first();
    gtk_tree_selection_select_path(selection, path);
    gtk_tree_path_free(path);
    cb_measures_connect_tree_view_cursor_changed(measures_connect_tree_view, NULL);

    gtk_list_store_clear(measures_sp_comp_store);

    for (i = 0; i < measure_sp_comp_entry_get_count(); i++) {
        measure_sp_comp_t *measure = measure_sp_comp_entry_get(i);

        gtk_list_store_append(measures_sp_comp_store, &iter);
        gtk_list_store_set(measures_sp_comp_store, &iter,
                0, (measure->src_node != NULL ? measure->src_node->phy_info->name : "All"),
                1, measure->dst_node->phy_info->name,
                -1);
    }

    measures_unlock();

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(measures_sp_comp_tree_view));
    path = gtk_tree_path_new_first();
    gtk_tree_selection_select_path(selection, path);
    gtk_tree_path_free(path);
    cb_measures_sp_comp_tree_view_cursor_changed(measures_sp_comp_tree_view, NULL);

    update_sensitivity();
}

void measurement_output_to_gui()
{
    measures_lock();

    cb_measures_connect_tree_view_cursor_changed(measures_connect_tree_view, NULL);
    cb_measures_sp_comp_tree_view_cursor_changed(measures_sp_comp_tree_view, NULL);

    measure_converg_t *measure = measure_converg_entry_get();
    measure_converg_output_t output = measure->output;

    char temp[256];
    char *time_str;

    snprintf(temp, sizeof(temp), "%d", output.converged_node_count);
    gtk_label_set_text(GTK_LABEL(measures_converg_connected_nodes_label), temp);

    snprintf(temp, sizeof(temp), "%d", output.total_node_count);
    gtk_label_set_text(GTK_LABEL(measures_converg_total_nodes_label), temp);

    time_str = rs_system_sim_time_to_string(output.measure_time);
    gtk_label_set_text(GTK_LABEL(measures_converg_measure_time_label), time_str);
    free(time_str);

    float percent;
    if (output.total_node_count > 0)
        percent = (float) output.converged_node_count / output.total_node_count;
    else
        percent = 0;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(measures_converg_progress), percent);

    snprintf(temp, sizeof(temp), "%.0f%%", percent);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(measures_converg_progress), temp);

    measures_unlock();
}


    /**** local functions ****/

void cb_measures_connect_add_button_clicked(GtkWidget *widget, gpointer data)
{
    rs_debug(DEBUG_GUI, NULL);

    int32 src_node_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(measures_connect_src_combo));
    int32 dst_node_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(measures_connect_dst_combo));

    rs_assert(src_node_pos >= 0 && src_node_pos <= rs_system->node_count); /* <= including the "All" item */
    rs_assert(dst_node_pos >= 0 && dst_node_pos < rs_system->node_count);

    node_t *src_node = NULL;
    node_t *dst_node = NULL;

    nodes_lock();

    if (src_node_pos > 0) { /* pos == 0 corresponds to the "All" item */
        src_node = rs_system->node_list[src_node_pos - 1];
    }
    dst_node = rs_system->node_list[dst_node_pos];

    nodes_unlock();

    measure_connect_entry_add(src_node, dst_node);

    measurement_entries_to_gui();

    update_sensitivity();
}

void cb_measures_connect_rem_button_clicked(GtkWidget *widget, gpointer data)
{
    rs_debug(DEBUG_GUI, NULL);

    int32 index = connect_tree_viee_get_selected_index();
    rs_assert(index >= 0 && index < measure_connect_entry_get_count());

    measure_connect_entry_remove(index);

    measurement_entries_to_gui();

    update_sensitivity();
}

void cb_measures_connect_tree_view_cursor_changed(GtkWidget *widget, gpointer data)
{
    rs_debug(DEBUG_GUI, NULL);

    int32 index = connect_tree_viee_get_selected_index();
    rs_assert(index < measure_connect_entry_get_count());

    if (index == -1) {
        gtk_label_set_text(GTK_LABEL(measures_connect_connected_time_label), "N/A");
        gtk_label_set_text(GTK_LABEL(measures_connect_total_time_label), "N/A");
        gtk_label_set_text(GTK_LABEL(measures_connect_measure_time_label), "N/A");

        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(measures_connect_progress), 0.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(measures_connect_progress), "N/A");
    }
    else {
        measures_lock();

        measure_connect_t *measure = measure_connect_entry_get(index);
        measure_connect_output_t output = measure->output;

        char temp[256];
        char *time_str;

        time_str = rs_system_sim_time_to_string(output.connected_time);
        gtk_label_set_text(GTK_LABEL(measures_connect_connected_time_label), time_str);
        free(time_str);

        time_str = rs_system_sim_time_to_string(output.total_time);
        gtk_label_set_text(GTK_LABEL(measures_connect_total_time_label), time_str);
        free(time_str);

        time_str = rs_system_sim_time_to_string(output.measure_time);
        gtk_label_set_text(GTK_LABEL(measures_connect_measure_time_label), time_str);
        free(time_str);

        float percent;
        if (output.total_time > 0)
            percent = (float) output.connected_time / output.total_time;
        else
            percent = 0;

        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(measures_connect_progress), percent);

        snprintf(temp, sizeof(temp), "%.0f%%", percent);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(measures_connect_progress), temp);

        measures_unlock();
    }

    update_sensitivity();
}

void cb_measures_sp_comp_add_button_clicked(GtkWidget *widget, gpointer data)
{
    rs_debug(DEBUG_GUI, NULL);

    int32 src_node_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(measures_sp_comp_src_combo));
    int32 dst_node_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(measures_sp_comp_dst_combo));

    rs_assert(src_node_pos >= 0 && src_node_pos <= rs_system->node_count); /* <= including the "All" item */
    rs_assert(dst_node_pos >= 0 && dst_node_pos < rs_system->node_count);

    node_t *src_node = NULL;
    node_t *dst_node = NULL;

    nodes_lock();

    if (src_node_pos > 0) { /* pos == 0 corresponds to the "All" item */
        src_node = rs_system->node_list[src_node_pos - 1];
    }
    dst_node = rs_system->node_list[dst_node_pos];

    nodes_unlock();

    measure_sp_comp_entry_add(src_node, dst_node);

    measurement_entries_to_gui();

    update_sensitivity();
}

void cb_measures_sp_comp_rem_button_clicked(GtkWidget *widget, gpointer data)
{
    rs_debug(DEBUG_GUI, NULL);

    int32 index = sp_comp_tree_viee_get_selected_index();
    rs_assert(index >= 0 && index < measure_sp_comp_entry_get_count());

    measure_sp_comp_entry_remove(index);

    measurement_entries_to_gui();

    update_sensitivity();
}

void cb_measures_sp_comp_tree_view_cursor_changed(GtkWidget *widget, gpointer data)
{
    rs_debug(DEBUG_GUI, NULL);

    int32 index = sp_comp_tree_viee_get_selected_index();
    rs_assert(index < measure_sp_comp_entry_get_count());

    if (index == -1) {
        gtk_label_set_text(GTK_LABEL(measures_sp_comp_rpl_cost_label), "N/A");
        gtk_label_set_text(GTK_LABEL(measures_sp_comp_sp_cost_label), "N/A");
        gtk_label_set_text(GTK_LABEL(measures_sp_comp_measure_time_label), "N/A");

        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(measures_sp_comp_progress), 0.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(measures_sp_comp_progress), "N/A");
    }
    else {
        measures_lock();

        measure_sp_comp_t *measure = measure_sp_comp_entry_get(index);
        measure_sp_comp_output_t output = measure->output;

        char temp[256];
        char *time_str;

        snprintf(temp, sizeof(temp), "%.02f", output.rpl_cost);
        gtk_label_set_text(GTK_LABEL(measures_sp_comp_rpl_cost_label), temp);

        snprintf(temp, sizeof(temp), "%.02f", output.sp_cost);
        gtk_label_set_text(GTK_LABEL(measures_sp_comp_sp_cost_label), temp);

        time_str = rs_system_sim_time_to_string(output.measure_time);
        gtk_label_set_text(GTK_LABEL(measures_sp_comp_measure_time_label), time_str);
        free(time_str);

        float percent;
        if (output.sp_cost > 0)
            percent = (float) output.rpl_cost / output.sp_cost;
        else
            percent = 0;

        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(measures_sp_comp_progress), percent);

        snprintf(temp, sizeof(temp), "%.0f%%", percent);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(measures_sp_comp_progress), temp);

        measures_unlock();
    }

    update_sensitivity();
}

static void update_sensitivity()
{
    bool measure_connect_selected = gtk_tree_selection_count_selected_rows(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(measures_connect_tree_view))) > 0;
    bool measure_sp_comp_selected = gtk_tree_selection_count_selected_rows(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(measures_sp_comp_tree_view))) > 0;
    bool has_nodes = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(measures_connect_dst_store), NULL) > 0;

    gtk_widget_set_sensitive(measures_connect_add_button, has_nodes);
    gtk_widget_set_sensitive(measures_connect_rem_button, measure_connect_selected);
    gtk_widget_set_sensitive(measures_sp_comp_add_button, has_nodes);
    gtk_widget_set_sensitive(measures_sp_comp_rem_button, measure_sp_comp_selected);
}

static int32 connect_tree_viee_get_selected_index()
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(measures_connect_tree_view));
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

static int32 sp_comp_tree_viee_get_selected_index()
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(measures_sp_comp_tree_view));
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
