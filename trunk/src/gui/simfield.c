
#include <math.h>
#include <gdk/gdk.h>

#include "simfield.h"
#include "mainwin.h"
#include "../main.h"
#include "../system.h"


    /*** global variables ***/

static GdkWindow *          sim_field_window = NULL;
static GdkGC *              sim_field_gc = NULL;

static char *               node_colors[SIM_FIELD_NODE_COLOR_COUNT] = {"blue", "green", "red", "magenta", "brown", "gray"};
static cairo_surface_t *    node_round_images[SIM_FIELD_NODE_COLOR_COUNT][SIM_FIELD_TX_POWER_STEP_COUNT];
static cairo_surface_t *    node_square_images[SIM_FIELD_NODE_COLOR_COUNT][SIM_FIELD_TX_POWER_STEP_COUNT];

static GtkWidget *          sim_field_drawing_area = NULL;
static GtkWidget *          sim_field_vruler = NULL;
static GtkWidget *          sim_field_hruler = NULL;

static node_t *             hover_node = NULL;
static node_t *             moving_node = NULL;


    /**** local function prototypes ****/

static gboolean             cb_sim_field_drawing_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data);
static gboolean             cb_sim_field_drawing_area_button_press(GtkDrawingArea *widget, GdkEventButton *event, gpointer data);
static gboolean             cb_sim_field_drawing_area_button_release(GtkDrawingArea *widget, GdkEventButton *event, gpointer data);
static gboolean             cb_sim_field_drawing_area_motion_notify(GtkDrawingArea *widget, GdkEventMotion *event, gpointer data);
static gboolean             cb_sim_field_drawing_area_scroll(GtkDrawingArea *widget, GdkEventScroll *event, gpointer data);

static gboolean             draw_sim_field(void *data);
static gboolean             update_rulers_wrapper(void *data);

static node_t *             find_node_under_coords(gint x, gint y, float scale_x, float scale_y);


    /**** exported functions ****/

GtkWidget *sim_field_create()
{
    /* load node images */
    char filename[256];
    int power_step, color_index;

    for (power_step = 0; power_step < SIM_FIELD_TX_POWER_STEP_COUNT; power_step++) {
        for (color_index = 0; color_index < SIM_FIELD_NODE_COLOR_COUNT; color_index++) {
            /* round images */
            snprintf(filename, sizeof(filename),
                    "%s/%s/node-round-%s-%d.png",
                    rs_app_dir,
                    RES_DIR,
                    node_colors[color_index],
                    power_step * (100 / (SIM_FIELD_TX_POWER_STEP_COUNT - 1)));

            cairo_surface_t *image = cairo_image_surface_create_from_png(filename);
            if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
                rs_error("failed to load png image '%s': %s", filename, cairo_status_to_string(cairo_surface_status(image)));
                return NULL;
            }

            node_round_images[color_index][power_step] = image;

            /* square images */
            snprintf(filename, sizeof(filename),
                    "%s/%s/node-square-%s-%d.png",
                    rs_app_dir,
                    RES_DIR,
                    node_colors[color_index],
                    power_step * (100 / (SIM_FIELD_TX_POWER_STEP_COUNT - 1)));

            image = cairo_image_surface_create_from_png(filename);
            if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
                rs_error("failed to load png image '%s': %s", filename, cairo_status_to_string(cairo_surface_status(image)));
                return NULL;
            }

            node_square_images[color_index][power_step] = image;
        }
    }

    GtkWidget *table = gtk_table_new(2, 2, FALSE);

    sim_field_hruler = gtk_hruler_new();
    gtk_ruler_set_metric(GTK_RULER(sim_field_hruler), GTK_PIXELS);
    gtk_table_attach(GTK_TABLE(table), sim_field_hruler, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 1);

    sim_field_vruler = gtk_vruler_new();
    gtk_ruler_set_metric(GTK_RULER(sim_field_vruler), GTK_PIXELS);
    gtk_table_attach(GTK_TABLE(table), sim_field_vruler, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 1, 0);

    sim_field_drawing_area = gtk_drawing_area_new();
    gtk_widget_add_events(sim_field_drawing_area, GDK_ALL_EVENTS_MASK);
    gtk_signal_connect(GTK_OBJECT(sim_field_drawing_area), "expose-event", GTK_SIGNAL_FUNC(cb_sim_field_drawing_area_expose), NULL);
    gtk_signal_connect(GTK_OBJECT(sim_field_drawing_area), "button-press-event", GTK_SIGNAL_FUNC(cb_sim_field_drawing_area_button_press), NULL);
    gtk_signal_connect(GTK_OBJECT(sim_field_drawing_area), "button-release-event", GTK_SIGNAL_FUNC(cb_sim_field_drawing_area_button_release), NULL);
    gtk_signal_connect(GTK_OBJECT(sim_field_drawing_area), "motion-notify-event", GTK_SIGNAL_FUNC(cb_sim_field_drawing_area_motion_notify), NULL);
    gtk_signal_connect(GTK_OBJECT(sim_field_drawing_area), "scroll-event", GTK_SIGNAL_FUNC(cb_sim_field_drawing_area_scroll), NULL);
    gtk_table_attach(GTK_TABLE(table), sim_field_drawing_area, 1, 2, 1, 2, GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);

    return table;
}

void sim_field_redraw()
{
    rs_assert(rs_system != NULL);

    /* assure thread safety, performing all the drawing ops in the GUI thread */
    gdk_threads_add_idle(draw_sim_field, NULL);
    gdk_threads_add_idle(update_rulers_wrapper, NULL);
}

void sim_field_draw_node(node_t *node, cairo_t *cr, double pixel_x, double pixel_y)
{
    percent_t tx_power = node->phy_info->tx_power;
    uint8 sequence_number = node->rpl_info->seq_num;

    cairo_surface_t **images;

    /* node itself */
    if (!main_win_get_display_params()->show_node_tx_power || !node->alive) {
        tx_power = 0.0;
    }

    if (node->alive) {
        if (rpl_node_is_root(node)) {
            images = node_square_images[sequence_number % (SIM_FIELD_NODE_COLOR_COUNT - 1)];
        }
        else {
            images = node_round_images[sequence_number % (SIM_FIELD_NODE_COLOR_COUNT - 1)];
        }
    }
    else {
        images = node_round_images[SIM_FIELD_NODE_COLOR_COUNT - 1];
    }

    cairo_surface_t *image = images[(int) round(tx_power * 10)];
    int image_width = cairo_image_surface_get_width(image);
    int image_height = cairo_image_surface_get_height(image);

    cairo_set_source_surface(cr, image, pixel_x - image_width / 2, pixel_y - image_height / 2);
    cairo_paint(cr);

    /* node name */
    if (main_win_get_display_params()->show_node_names && strlen(node->phy_info->name) > 0) {
        sim_field_draw_text(cr, node->phy_info->name,
                pixel_x, pixel_y,
                SIM_FIELD_TEXT_NAME_FG_COLOR,
                strlen(node->phy_info->name) > 5 ? SIM_FIELD_TEXT_BG_COLOR : 0);
    }

    /* node address */
    if (main_win_get_display_params()->show_node_addresses && strlen(node->ip_info->address) > 0) {
        sim_field_draw_text(cr, node->ip_info->address,
                pixel_x, pixel_y + SIM_FIELD_NODE_RADIUS * 2,
                SIM_FIELD_TEXT_ADDRESS_FG_COLOR, SIM_FIELD_TEXT_BG_COLOR);
    }

    /* node rank */
    if (main_win_get_display_params()->show_node_ranks && node->rpl_info->rank > 0) {
        char rank_str[256];
        snprintf(rank_str, sizeof(rank_str), "%d", node->rpl_info->rank);

        sim_field_draw_text(cr, rank_str,
                pixel_x, pixel_y - SIM_FIELD_NODE_RADIUS * 2,
                SIM_FIELD_TEXT_RANK_FG_COLOR, SIM_FIELD_TEXT_BG_COLOR);
    }
}

void sim_field_draw_parent_arrow(cairo_t *cr, double start_pixel_x, double start_pixel_y, double stop_pixel_x, double stop_pixel_y, uint32 color, bool packet)
{
    double thickness = (packet ? 3.0 : 1.0);
    sim_field_draw_arrow(cr, start_pixel_x, start_pixel_y, stop_pixel_x, stop_pixel_y, SIM_FIELD_NODE_RADIUS, color, TRUE, thickness, FALSE);
}

void sim_field_draw_sibling_arrow(cairo_t *cr, double start_pixel_x, double start_pixel_y, double stop_pixel_x, double stop_pixel_y, uint32 color, bool packet, bool draw_line)
{
    double thickness = (packet ? 3.0 : 1.0);
    sim_field_draw_arrow(cr, start_pixel_x, start_pixel_y, stop_pixel_x, stop_pixel_y, SIM_FIELD_NODE_RADIUS, color, draw_line, thickness, TRUE);
}

void sim_field_draw_arrow(cairo_t *cr, double start_x, double start_y, double end_x, double end_y, double radius, uint32 color, bool draw_line, double thickness, bool dashed)
{
    const double arrow_length = 12;
    const double arrow_angle = M_PI / 10;

    double angle = atan2(end_y - start_y, end_x - start_x) + M_PI;

    double xa = end_x + radius * cos(angle);
    double ya = end_y + radius * sin(angle);
    double xb = end_x + arrow_length * cos(angle + arrow_angle) + radius * cos(angle);
    double yb = end_y + arrow_length * sin(angle + arrow_angle) + radius * sin(angle);
    double xc = end_x + arrow_length * cos(angle - arrow_angle) + radius * cos(angle);
    double yc = end_y + arrow_length * sin(angle - arrow_angle) + radius * sin(angle);
    double xd = start_x - radius * cos(angle);
    double yd = start_y - radius * sin(angle);

    double r, g, b, a;
    EXPAND_COLOR(color, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_set_line_width(cr, 1.0);

    cairo_move_to(cr, xb, yb);
    cairo_line_to(cr, xa, ya);
    cairo_line_to(cr, xc, yc);

    cairo_fill(cr);

    if (draw_line) {
        if (dashed) {
            double dashes[] = {5.0, 5.0};
            int len = sizeof(dashes) / sizeof(dashes[0]);

            cairo_set_dash(cr, dashes, len, 0.0);
        }
        else {
            cairo_set_dash(cr, NULL, 0, 0.0);
        }

        cairo_set_line_width(cr, thickness);

        cairo_move_to(cr, xa, ya);
        cairo_line_to(cr, xd, yd);

        cairo_stroke(cr);
    }
}

void sim_field_draw_text(cairo_t *cr, char *text, double x, double y, uint32 fg_color, uint32 bg_color)
{
    double r, g, b, a;
    cairo_text_extents_t te;
    double tx, ty, tw, th;

    cairo_select_font_face(cr, SIM_FIELD_TEXT_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, SIM_FIELD_TEXT_SIZE);
    cairo_text_extents(cr, text, &te);
    tx = x - te.width / 2;
    ty = y - te.height / 2;
    tw = te.width;
    th = te.height;

    if (bg_color != 0) {
        cairo_set_dash(cr, NULL, 0, 0);

        EXPAND_COLOR(bg_color, r, g, b, a);
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_rectangle(cr, tx - 2, ty - 2, tw + 5, th + 5);
        cairo_fill(cr);

        r += 0.4 * r;
        g += 0.4 * g;
        b += 0.4 * b;
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_set_line_width(cr, 1);
        cairo_rectangle(cr, tx - 2, ty - 2, tw + 5, th + 5);
        cairo_stroke(cr);
    }

    EXPAND_COLOR(fg_color, r, g, b, a);
    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_move_to(cr, tx - te.x_bearing, ty - te.y_bearing);
    cairo_show_text(cr, text);
}


    /**** local functions ****/

static gboolean cb_sim_field_drawing_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    draw_sim_field(NULL);

    return TRUE;
}

static gboolean cb_sim_field_drawing_area_button_press(GtkDrawingArea *widget, GdkEventButton *event, gpointer data)
{
    main_win_set_selected_node(hover_node);

    if (hover_node != NULL) {
        moving_node = hover_node;

        gint pixel_width, pixel_height;
        gdk_drawable_get_size(sim_field_window, &pixel_width, &pixel_height);

        float scale_x = pixel_width / rs_system->width;
        float scale_y = pixel_height / rs_system->height;

        coord_t current_x = event->x / scale_x;
        coord_t current_y = event->y / scale_y;

        moving_node->phy_info->cx = current_x;
        moving_node->phy_info->cy = current_y;

        main_win_node_to_gui(moving_node);
        sim_field_redraw();
    }

    return TRUE;
}

static gboolean cb_sim_field_drawing_area_button_release(GtkDrawingArea *widget, GdkEventButton *event, gpointer data)
{
    moving_node = NULL;

    return TRUE;
}

static gboolean cb_sim_field_drawing_area_motion_notify(GtkDrawingArea *widget, GdkEventMotion *event, gpointer data)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(sim_field_window, &pixel_width, &pixel_height);

    coord_t system_width = rs_system->width;
    coord_t system_height = rs_system->height;

    float scale_x = pixel_width / system_width;
    float scale_y = pixel_height / system_height;

    node_t *node = find_node_under_coords(event->x, event->y, scale_x, scale_y);
    if (node == NULL && moving_node != NULL) {  /* if we're moving a node but the mouse went out of its area... */
        node = moving_node;
    }

    hover_node = node;

    if (node != NULL) {
        GdkCursor* cursor = cursor = gdk_cursor_new(GDK_HAND2);
        gdk_window_set_cursor(sim_field_window, cursor);
        gdk_cursor_destroy(cursor);
    }
    else {
        GdkCursor* cursor = cursor = gdk_cursor_new(GDK_ARROW);
        gdk_window_set_cursor(sim_field_window, cursor);
        gdk_cursor_destroy(cursor);
    }

    coord_t current_x = event->x / scale_x;
    coord_t current_y = event->y / scale_y;

    if (moving_node != NULL) {
        moving_node->phy_info->cx = current_x;
        moving_node->phy_info->cy = current_y;

        main_win_node_to_gui(moving_node);
    }

    draw_sim_field(NULL);

    main_win_update_xy_status(current_x, current_y);

    gtk_ruler_set_range(GTK_RULER(sim_field_hruler), 0, system_width, current_x, system_width);
    gtk_ruler_set_range(GTK_RULER(sim_field_vruler), 0, system_height, current_y, system_height);

    return TRUE;
}

static gboolean cb_sim_field_drawing_area_scroll(GtkDrawingArea *widget, GdkEventScroll *event, gpointer data)
{
    node_t *node = main_win_get_selected_node();
    if (node == NULL) {
        return FALSE;
    }

    if (event->direction == GDK_SCROLL_UP) {
        if (node->phy_info->tx_power + 0.1 <= 1.0) {
            node->phy_info->tx_power += 0.1;
        }
        else {
            node->phy_info->tx_power = 1.0;
        }
    }
    else /* (event->direction == GDK_SCROLL_DOWN) */{
        if (node->phy_info->tx_power - 0.1 >= 0.0) {
            node->phy_info->tx_power -= 0.1;
        }
        else {
            node->phy_info->tx_power = 0.0;
        }
    }

    main_win_node_to_gui(node);
    sim_field_redraw();

    return TRUE;
}

static gboolean draw_sim_field(void *data)
{
    if (sim_field_drawing_area == NULL || sim_field_drawing_area->window == NULL) {
        return FALSE;
    }

    if (sim_field_window == NULL) {
        sim_field_window = sim_field_drawing_area->window;
        sim_field_gc = sim_field_drawing_area->style->fg_gc[GTK_STATE_NORMAL];
    }

    gint pixel_width, pixel_height;
    gdk_drawable_get_size(sim_field_window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    events_lock();

    /* nodes */
    uint16 node_index, parent_index, sibling_index;

    float scale_x = pixel_width / rs_system->width;
    float scale_y = pixel_height / rs_system->height;

    int pixel_x, start_pixel_x, end_pixel_x;
    int pixel_y, start_pixel_y, end_pixel_y;

    for (node_index = 0; node_index < rs_system->node_count; node_index++) {
        node_t *node = rs_system->node_list[node_index];

        pixel_x = node->phy_info->cx * scale_x;
        pixel_y = node->phy_info->cy * scale_y;

        sim_field_draw_node(node, cr, pixel_x, pixel_y);
    }

    /* arrows */
    if (main_win_get_display_params()->show_parent_arrows || main_win_get_display_params()->show_sibling_arrows) {
        for (node_index = 0; node_index < rs_system->node_count; node_index++) {
            node_t *node = rs_system->node_list[node_index];

            /* don't draw arrows of dead nodes */
            if (!node->alive) {
                continue;
            }

            start_pixel_x = node->phy_info->cx * scale_x;
            start_pixel_y = node->phy_info->cy * scale_y;

            if (main_win_get_display_params()->show_parent_arrows) {
                for (parent_index = 0; parent_index < node->rpl_info->parent_count; parent_index++) {
                    node_t *parent = node->rpl_info->parent_list[parent_index]->node;

                    if (parent == NULL) { /* if the node has been removed in the mean time */
                        continue;
                    }

                    end_pixel_x = parent->phy_info->cx * scale_x;
                    end_pixel_y = parent->phy_info->cy * scale_y;

                    uint32 color;
                    if (parent->alive) {
                        if (node->rpl_info->pref_parent == node->rpl_info->sibling_list[parent_index]) {
                            color = SIM_FIELD_PREF_PARENT_ARROW_COLOR;
                        }
                        else {
                            color = SIM_FIELD_PARENT_ARROW_COLOR;
                        }
                    }
                    else {
                        color = SIM_FIELD_DEAD_ARROW_COLOR;
                    }

                    bool packet = FALSE; //node_has_pdu_from(parent, node);
                    sim_field_draw_parent_arrow(cr, start_pixel_x, start_pixel_y, end_pixel_x, end_pixel_y, color, packet);
                }
            }

            if (main_win_get_display_params()->show_sibling_arrows) {
                for (sibling_index = 0; sibling_index < node->rpl_info->sibling_count; sibling_index++) {
                    node_t *sibling = node->rpl_info->sibling_list[sibling_index]->node;

                    if (sibling == NULL) { /* if the node has been removed in the mean time */
                        continue;
                    }

                    end_pixel_x = sibling->phy_info->cx * scale_x;
                    end_pixel_y = sibling->phy_info->cy * scale_y;

                    uint32 color = sibling->alive ? SIM_FIELD_SIBLING_ARROW_COLOR : SIM_FIELD_DEAD_ARROW_COLOR;

                    bool packet = FALSE;    // node_has_pdu_from(sibling, node);

                    /* in case of mutual siblingness, only the "lowest" node draws the line */
                    if (node < sibling) {
                        sim_field_draw_sibling_arrow(cr, start_pixel_x, start_pixel_y, end_pixel_x, end_pixel_y, color, packet, TRUE);
                    }
                    else {
                        if (rpl_node_has_sibling(sibling, node) &&
                                sibling->alive &&
                                rs_system_has_node(sibling)) {
                            sim_field_draw_sibling_arrow(cr, start_pixel_x, start_pixel_y, end_pixel_x, end_pixel_y, color, packet, FALSE);
                        }
                        else {
                            sim_field_draw_sibling_arrow(cr, start_pixel_x, start_pixel_y, end_pixel_x, end_pixel_y, color, packet, TRUE);
                        }
                    }
                }
            }
        }
    }

    /* decorate the hovered node */
    if (hover_node != NULL) {
        pixel_x = hover_node->phy_info->cx * scale_x;
        pixel_y = hover_node->phy_info->cy * scale_y;

        sim_field_draw_arrow(cr,
                pixel_x - SIM_FIELD_NODE_RADIUS * 4, pixel_y, pixel_x - SIM_FIELD_NODE_RADIUS * 1, pixel_y, 0,
                SIM_FIELD_HOVER_COLOR, TRUE, 1.0, TRUE);
        sim_field_draw_arrow(cr,
                pixel_x, pixel_y - SIM_FIELD_NODE_RADIUS * 4, pixel_x, pixel_y - SIM_FIELD_NODE_RADIUS * 1, 0,
                SIM_FIELD_HOVER_COLOR, TRUE, 1.0, TRUE);
        sim_field_draw_arrow(cr,
                pixel_x + SIM_FIELD_NODE_RADIUS * 4, pixel_y, pixel_x + SIM_FIELD_NODE_RADIUS * 1, pixel_y, 0,
                SIM_FIELD_HOVER_COLOR, TRUE, 1.0, TRUE);
        sim_field_draw_arrow(cr,
                pixel_x, pixel_y + SIM_FIELD_NODE_RADIUS * 4, pixel_x, pixel_y + SIM_FIELD_NODE_RADIUS * 1, 0,
                SIM_FIELD_HOVER_COLOR, TRUE, 1.0, TRUE);
    }

    /* decorate the selected node */
    node_t *selected_node = main_win_get_selected_node();
    if (selected_node != NULL) {
        pixel_x = selected_node->phy_info->cx * scale_x;
        pixel_y = selected_node->phy_info->cy * scale_y;

        double r, g, b, a;
        EXPAND_COLOR(SIM_FIELD_SELECTED_COLOR, r, g, b, a);

        static double dashes[] = {5.0, 5.0};
        static int len = sizeof(dashes) / sizeof(dashes[0]);

        cairo_set_dash(cr, dashes, len, 0.0);
        cairo_set_source_rgba(cr, r, g, b, a);
        cairo_set_line_width(cr, 2.0);

        cairo_move_to(cr, pixel_x + SIM_FIELD_NODE_RADIUS * 2, pixel_y);
        cairo_arc(cr, pixel_x, pixel_y, SIM_FIELD_NODE_RADIUS * 2, 0, M_PI * 2);
        cairo_stroke(cr);
    }

    events_unlock();

    /* do the actual double-buffered paint */
    cairo_t *sim_field_cr = gdk_cairo_create(sim_field_window);
    cairo_set_source_surface(sim_field_cr, surface, 0, 0);
    cairo_paint(sim_field_cr);
    cairo_destroy(sim_field_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return FALSE;
}

static gboolean update_rulers_wrapper(void *data)
{
    coord_t width = rs_system->width;
    coord_t height = rs_system->height;

    gtk_ruler_set_range(GTK_RULER(sim_field_hruler), 0, width, 0, width);
    gtk_ruler_set_range(GTK_RULER(sim_field_vruler), 0, height, 0, height);

    return FALSE;
}

static node_t *find_node_under_coords(gint x, gint y, float scale_x, float scale_y)
{
    nodes_lock();

    int32 index;

    coord_t system_x = x / scale_x;
    coord_t system_y = y / scale_y;
    coord_t system_radius = SIM_FIELD_NODE_RADIUS * 2 / scale_y ;

    for (index = rs_system->node_count - 1; index >= 0; index--) {
        node_t *node = rs_system->node_list[index];

        coord_t node_x = node->phy_info->cx;
        coord_t node_y = node->phy_info->cy;

        if ((system_x > node_x - system_radius) &&
            (system_x < node_x + system_radius) &&
            (system_y > node_y - system_radius) &&
            (system_y < node_y + system_radius)) {

            nodes_unlock();

            return node;
        }
    }

    nodes_unlock();

    return NULL;
}
