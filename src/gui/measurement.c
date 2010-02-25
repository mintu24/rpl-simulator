
#include "measurement.h"

#include "mainwin.h"
#include "../system.h"



    /**** global variables ****/

    /* connectivity */
    /* sp comparison */
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

    /* convergence */
static GtkWidget *              measures_converg_connected_progress;
static GtkWidget *              measures_converg_stable_progress;
static GtkWidget *              measures_converg_floating_progress;
static GtkWidget *              measures_converg_measure_time_label;

    /* statistics */
static GtkWidget *              measures_stat_node_combo;
static GtkWidget *              measures_stat_tree_view;
static GtkWidget *              measures_stat_add_button;
static GtkWidget *              measures_stat_rem_button;
static GtkWidget *              measures_stat_forward_inconsistencies_label;
static GtkWidget *              measures_stat_forward_failures_label;
static GtkWidget *              measures_stat_rpl_events_label;
static GtkWidget *              measures_stat_rpl_dis_messages_label;
static GtkWidget *              measures_stat_rpl_dio_messages_label;
static GtkWidget *              measures_stat_rpl_dao_messages_label;
static GtkWidget *              measures_stat_ping_progress;
static GtkWidget *              measures_stat_measure_time_label;
static GtkListStore *           measures_stat_node_store;
static GtkListStore *           measures_stat_store;


    /**** local function prototypes ****/

void                            cb_measures_sp_comp_add_button_clicked(GtkWidget *widget, gpointer data);
void                            cb_measures_sp_comp_rem_button_clicked(GtkWidget *widget, gpointer data);
void                            cb_measures_sp_comp_tree_view_cursor_changed(GtkWidget *widget, gpointer data);

void                            cb_measures_stat_add_button_clicked(GtkWidget *widget, gpointer data);
void                            cb_measures_stat_rem_button_clicked(GtkWidget *widget, gpointer data);
void                            cb_measures_stat_tree_view_cursor_changed(GtkWidget *widget, gpointer data);

static void                     update_sensitivity();

static int32                    sp_comp_tree_viee_get_selected_index();
static int32                    stat_tree_viee_get_selected_index();


    /**** exported functions ****/

GtkWidget *measurement_widget_create()
{
    GtkWidget *measures_widget = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_vbox");

    /* sp comparison */
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
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(measures_sp_comp_src_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(measures_sp_comp_src_combo), renderer, "text", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(measures_sp_comp_dst_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(measures_sp_comp_dst_combo), renderer, "text", 0, NULL);

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

    /* convergence */
    measures_converg_connected_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_converg_connected_progress");
    measures_converg_stable_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_converg_stable_progress");
    measures_converg_floating_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_converg_floating_progress");
    measures_converg_measure_time_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_converg_measure_time_label");

    /* statistics */
    measures_stat_node_combo = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_node_combo");
    measures_stat_tree_view = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_tree_view");
    measures_stat_add_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_add_button");
    measures_stat_rem_button = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_rem_button");
    measures_stat_forward_inconsistencies_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_forward_inconsistencies_label");
    measures_stat_forward_failures_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_forward_failures_label");
    measures_stat_rpl_events_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_rpl_events_label");
    measures_stat_rpl_dis_messages_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_rpl_dis_messages_label");
    measures_stat_rpl_dio_messages_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_rpl_dio_messages_label");
    measures_stat_rpl_dao_messages_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_rpl_dao_messages_label");
    measures_stat_ping_progress = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_ping_progress");
    measures_stat_measure_time_label = (GtkWidget *) gtk_builder_get_object(gtk_builder, "measures_stat_measure_time_label");
    measures_stat_node_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "measures_stat_node_store");
    measures_stat_store = (GtkListStore *) gtk_builder_get_object(gtk_builder, "measures_stat_store");

    renderer = gtk_cell_renderer_text_new();
    gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(measures_stat_node_combo), renderer, FALSE);
    gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(measures_stat_node_combo), renderer, "text", 0, NULL);

    renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(
            GTK_TREE_VIEW(measures_stat_tree_view),
            -1,
            "Node",
            renderer,
            "text", 0,
            NULL);


    update_sensitivity();
    measurement_output_to_gui();

    return measures_widget;
}

void measurement_system_to_gui()
{
    rs_assert(rs_system != NULL);

    /* add all the possible node sources ad destinations */

    gtk_list_store_clear(measures_sp_comp_src_store);
    gtk_list_store_clear(measures_sp_comp_dst_store);
    gtk_list_store_clear(measures_sp_comp_store);

    gtk_list_store_clear(measures_stat_node_store);
    gtk_list_store_clear(measures_stat_store);

    gtk_list_store_insert_with_values(measures_stat_node_store, NULL, -1, 0, "Average", -1);
    gtk_list_store_insert_with_values(measures_stat_node_store, NULL, -1, 0, "Total", -1);

    nodes_lock();

    uint16 i;
    for (i = 0; i < rs_system->node_count; i++) {
        node_t *node = rs_system->node_list[i];

        gtk_list_store_insert_with_values(measures_sp_comp_src_store, NULL, -1, 0, node->phy_info->name, -1);
        gtk_list_store_insert_with_values(measures_sp_comp_dst_store, NULL, -1, 0, node->phy_info->name, -1);

        gtk_list_store_insert_with_values(measures_stat_node_store, NULL, -1, 0, node->phy_info->name, -1);
    }

    nodes_unlock();

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(measures_sp_comp_src_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(measures_sp_comp_src_combo), 0);
    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(measures_sp_comp_dst_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(measures_sp_comp_dst_combo), 0);

    if (gtk_tree_model_iter_n_children(GTK_TREE_MODEL(measures_stat_node_store), NULL) > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(measures_stat_node_combo), 0);

    measurement_entries_to_gui();
}

void measurement_entries_to_gui()
{
    measures_lock();

    /* sp comparison */
    gtk_list_store_clear(measures_sp_comp_store);

    GtkTreeIter iter;
    uint16 i;
    for (i = 0; i < measure_sp_comp_entry_get_count(); i++) {
        measure_sp_comp_t *measure = measure_sp_comp_entry_get(i);

        gtk_list_store_append(measures_sp_comp_store, &iter);
        gtk_list_store_set(measures_sp_comp_store, &iter,
                0, (measure->src_node != NULL ? measure->src_node->phy_info->name : "All"),
                1, measure->dst_node->phy_info->name,
                -1);
    }

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(measures_sp_comp_tree_view));
    GtkTreePath *path = gtk_tree_path_new_first();
    gtk_tree_selection_select_path(selection, path);
    gtk_tree_path_free(path);
    cb_measures_sp_comp_tree_view_cursor_changed(measures_sp_comp_tree_view, NULL);

    /* sp comparison */
    gtk_list_store_clear(measures_stat_store);

    for (i = 0; i < measure_stat_entry_get_count(); i++) {
        measure_stat_t *measure = measure_stat_entry_get(i);

        gtk_list_store_append(measures_stat_store, &iter);

        switch (measure->type) {

            case MEASURE_STAT_TYPE_AVG:
                gtk_list_store_set(measures_stat_store, &iter,
                        0, "Average",
                        -1);
                break;

            case MEASURE_STAT_TYPE_TOTAL:
                gtk_list_store_set(measures_stat_store, &iter,
                        0, "Total",
                        -1);
                break;

            case MEASURE_STAT_TYPE_NODE:
                gtk_list_store_set(measures_stat_store, &iter,
                        0, measure->node->phy_info->name,
                        -1);
                break;

        }
    }

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(measures_stat_tree_view));
    path = gtk_tree_path_new_first();
    gtk_tree_selection_select_path(selection, path);
    gtk_tree_path_free(path);
    cb_measures_stat_tree_view_cursor_changed(measures_stat_tree_view, NULL);

    measures_unlock();

    update_sensitivity();
}

void measurement_output_to_gui()
{
    measures_lock();

    char temp[256];
    char *time_str;
    float fraction = 0;

    /* sp comp */
    cb_measures_sp_comp_tree_view_cursor_changed(measures_sp_comp_tree_view, NULL);

    /* convergence */
    measure_converg_t *converg_measure = measure_converg_entry_get();
    measure_converg_output_t converg_output = converg_measure->output;

    if (converg_output.total_node_count > 0) {
        fraction = (float) converg_output.connected_node_count / converg_output.total_node_count;
    }
    snprintf(temp, sizeof(temp), "%d/%d (%.0f%%)", converg_output.connected_node_count, converg_output.total_node_count, fraction * 100);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(measures_converg_connected_progress), fraction);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(measures_converg_connected_progress), temp);

    if (converg_output.total_node_count > 0) {
        fraction = (float) converg_output.stable_node_count / converg_output.total_node_count;
    }
    snprintf(temp, sizeof(temp), "%d/%d (%.0f%%)", converg_output.stable_node_count, converg_output.total_node_count, fraction * 100);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(measures_converg_stable_progress), fraction);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(measures_converg_stable_progress), temp);

    if (converg_output.total_node_count > 0) {
        fraction = (float) converg_output.floating_node_count / converg_output.total_node_count;
    }
    snprintf(temp, sizeof(temp), "%d/%d (%.0f%%)", converg_output.floating_node_count, converg_output.total_node_count, fraction * 100);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(measures_converg_floating_progress), fraction);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(measures_converg_floating_progress), temp);

    time_str = rs_system_sim_time_to_string(converg_output.measure_time);
    gtk_label_set_text(GTK_LABEL(measures_converg_measure_time_label), time_str);
    free(time_str);

    /* statistics */
    cb_measures_stat_tree_view_cursor_changed(measures_stat_tree_view, NULL);

    measures_unlock();
}


    /**** local functions ****/

void cb_measures_sp_comp_add_button_clicked(GtkWidget *widget, gpointer data)
{
    rs_debug(DEBUG_GUI, NULL);

    int32 src_node_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(measures_sp_comp_src_combo));
    int32 dst_node_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(measures_sp_comp_dst_combo));

    //rs_assert(src_node_pos >= 0 && src_node_pos <= rs_system->node_count); /* <= including the "All" item */
    rs_assert(src_node_pos >= 0 && src_node_pos < rs_system->node_count);
    rs_assert(dst_node_pos >= 0 && dst_node_pos < rs_system->node_count);

    node_t *src_node = NULL;
    node_t *dst_node = NULL;

    nodes_lock();

    // if (src_node_pos > 0) { /* pos == 0 corresponds to the "All" item */
    src_node = rs_system->node_list[src_node_pos];
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

        snprintf(temp, sizeof(temp), "%.0f%%", percent * 100);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(measures_sp_comp_progress), temp);

        measures_unlock();
    }

    update_sensitivity();
}

void cb_measures_stat_add_button_clicked(GtkWidget *widget, gpointer data)
{
    rs_debug(DEBUG_GUI, NULL);

    int32 node_pos = gtk_combo_box_get_active(GTK_COMBO_BOX(measures_stat_node_combo));

    rs_assert(node_pos >= 0 && node_pos <= rs_system->node_count + 1); /* <= including the "Avg" & "Total" items */


    if (node_pos > 1) { /* pos == 0 corresponds to the "Avg" item, while pos == 1 to "Total" item */
        node_t *node = rs_system->node_list[node_pos - 2];

        measure_stat_entry_add(node, MEASURE_STAT_TYPE_NODE);
    }
    else if (node_pos == 0) {
        measure_stat_entry_add(NULL, MEASURE_STAT_TYPE_AVG);
    }
    else {
        measure_stat_entry_add(NULL, MEASURE_STAT_TYPE_TOTAL);
    }

    measurement_entries_to_gui();

    update_sensitivity();
}

void cb_measures_stat_rem_button_clicked(GtkWidget *widget, gpointer data)
{
    rs_debug(DEBUG_GUI, NULL);

    int32 index = stat_tree_viee_get_selected_index();
    rs_assert(index >= 0 && index < measure_stat_entry_get_count());

    measure_stat_entry_remove(index);

    measurement_entries_to_gui();

    update_sensitivity();
}

void cb_measures_stat_tree_view_cursor_changed(GtkWidget *widget, gpointer data)
{
    rs_debug(DEBUG_GUI, NULL);

    int32 index = stat_tree_viee_get_selected_index();
    rs_assert(index < measure_stat_entry_get_count());

    if (index == -1) {
        gtk_label_set_text(GTK_LABEL(measures_stat_forward_inconsistencies_label), "N/A");
        gtk_label_set_text(GTK_LABEL(measures_stat_forward_failures_label), "N/A");
        gtk_label_set_text(GTK_LABEL(measures_stat_rpl_events_label), "N/A");
        gtk_label_set_text(GTK_LABEL(measures_stat_rpl_dis_messages_label), "N/A");
        gtk_label_set_text(GTK_LABEL(measures_stat_rpl_dio_messages_label), "N/A");
        gtk_label_set_text(GTK_LABEL(measures_stat_rpl_dao_messages_label), "N/A");
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(measures_stat_ping_progress), "N/A");
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(measures_stat_ping_progress), 0);
        gtk_label_set_text(GTK_LABEL(measures_stat_measure_time_label), "N/A");
    }
    else {
        measures_lock();

        measure_stat_t *measure = measure_stat_entry_get(index);
        measure_stat_output_t output = measure->output;

        char temp[256];
        char *time_str;

        snprintf(temp, sizeof(temp), "%d", output.forward_error_inconsistency);
        gtk_label_set_text(GTK_LABEL(measures_stat_forward_inconsistencies_label), temp);

        snprintf(temp, sizeof(temp), "%d", output.forward_failure_count);
        gtk_label_set_text(GTK_LABEL(measures_stat_forward_failures_label), temp);

        snprintf(temp, sizeof(temp), "%d", output.rpl_event_count);
        gtk_label_set_text(GTK_LABEL(measures_stat_rpl_events_label), temp);

        snprintf(temp, sizeof(temp), "%d/%d", output.rpl_s_dis_message_count, output.rpl_r_dis_message_count);
        gtk_label_set_text(GTK_LABEL(measures_stat_rpl_dis_messages_label), temp);

        snprintf(temp, sizeof(temp), "%d/%d", output.rpl_s_dio_message_count, output.rpl_r_dio_message_count);
        gtk_label_set_text(GTK_LABEL(measures_stat_rpl_dio_messages_label), temp);

        snprintf(temp, sizeof(temp), "%d/%d", output.rpl_s_dao_message_count, output.rpl_r_dao_message_count);
        gtk_label_set_text(GTK_LABEL(measures_stat_rpl_dao_messages_label), temp);

        float percent;
        if (output.ping_successful_count > 0)
            percent = (float) output.ping_successful_count / (output.ping_successful_count + output.ping_timeout_count);
        else
            percent = 0;

        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(measures_stat_ping_progress), percent);

        snprintf(temp, sizeof(temp), "%d/%d (%.0f%%)", output.ping_successful_count, output.ping_successful_count + output.ping_timeout_count, percent * 100);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(measures_stat_ping_progress), temp);

        time_str = rs_system_sim_time_to_string(output.measure_time);
        gtk_label_set_text(GTK_LABEL(measures_stat_measure_time_label), time_str);
        free(time_str);

        measures_unlock();
    }

    update_sensitivity();
}

static void update_sensitivity()
{
    bool measure_sp_comp_selected = gtk_tree_selection_count_selected_rows(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(measures_sp_comp_tree_view))) > 0;
    bool measure_stat_selected = gtk_tree_selection_count_selected_rows(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(measures_stat_tree_view))) > 0;
    bool has_nodes = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(measures_sp_comp_dst_store), NULL) > 0;

    gtk_widget_set_sensitive(measures_sp_comp_add_button, has_nodes);
    gtk_widget_set_sensitive(measures_sp_comp_rem_button, measure_sp_comp_selected);
    gtk_widget_set_sensitive(measures_stat_add_button, has_nodes);
    gtk_widget_set_sensitive(measures_stat_rem_button, measure_stat_selected);
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

static int32 stat_tree_viee_get_selected_index()
{
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(measures_stat_tree_view));
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
