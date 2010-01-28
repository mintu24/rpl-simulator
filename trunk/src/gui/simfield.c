
#include <math.h>
#include <gdk/gdk.h>

#include "simfield.h"
#include "mainwin.h"
#include "../main.h"
#include "../system.h"

// todo: node image legend

#define EXPAND_COLOR(color, r, g, b, a) {   \
        r = (((color) & 0x00FF0000) >> 16) / 255.0;  \
        g = (((color) & 0x0000FF00) >> 8) / 255.0;  \
        b = (((color) & 0x000000FF)) / 255.0;  \
        a = (((color) & 0xFF000000) >> 24) / 255.0; }


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
static void                 draw_node(node_t *node, cairo_t *cr, double pixel_x, double pixel_y);
static void                 draw_parent_arrow(cairo_t *cr, double start_pixel_x, double start_pixel_y, double stop_pixel_x, double stop_pixel_y, uint32 color, bool packet);
static void                 draw_sibling_arrow(cairo_t *cr, double start_pixel_x, double start_pixel_y, double stop_pixel_x, double stop_pixel_y, uint32 color, bool packet, bool draw_line);
static gboolean             update_rulers_wrapper(void *data);

static void                 draw_arrow(cairo_t *cr, double start_x, double start_y, double end_x, double end_y, double radius, uint32 color, bool draw_line, double thickness, bool dashed);
static void                 draw_text(cairo_t *cr, char *text, double x, double y, uint32 fg_color, uint32 bg_color);

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
                    "%s/node-round-%s-%d.png",
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
                    "%s/node-square-%s-%d.png",
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

        float scale_x = pixel_width / rs_system_get_width();
        float scale_y = pixel_height / rs_system_get_height();

        coord_t current_x = event->x / scale_x;
        coord_t current_y = event->y / scale_y;

        phy_node_set_xy(moving_node, current_x, current_y);

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

    coord_t system_width = rs_system_get_width();
    coord_t system_height = rs_system_get_height();

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
        phy_node_set_xy(moving_node, current_x, current_y);

        main_win_node_to_gui(moving_node);
    }

    draw_sim_field(NULL);

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
        if (phy_node_get_tx_power(node) + 0.1 <= 1.0) {
            phy_node_set_tx_power(node, phy_node_get_tx_power(node) + 0.1);
        }
        else {
            phy_node_set_tx_power(node, 1.0);
        }
    }
    else /* (event->direction == GDK_SCROLL_DOWN) */{
        if (phy_node_get_tx_power(node) - 0.1 >= 0.0) {
            phy_node_set_tx_power(node, phy_node_get_tx_power(node) - 0.1);
        }
        else {
            phy_node_set_tx_power(node, 0.0);
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

    /* nodes */
    uint16 node_count, parent_count, sibling_count, node_index, parent_index, sibling_index;
    node_t **node_list;
    rpl_remote_node_t **parent_list, **sibling_list;
    node_list = rs_system_get_node_list_copy(&node_count);

    float scale_x = pixel_width / rs_system_get_width();
    float scale_y = pixel_height / rs_system_get_height();

    int pixel_x, start_pixel_x, end_pixel_x;
    int pixel_y, start_pixel_y, end_pixel_y;

    for (node_index = 0; node_index < node_count; node_index++) {
        node_t *node = node_list[node_index];

        phy_node_lock(node);
        mac_node_lock(node);
        ip_node_lock(node);
        icmp_node_lock(node);
        rpl_node_lock(node);

        pixel_x = phy_node_get_x(node) * scale_x;
        pixel_y = phy_node_get_y(node) * scale_y;

        draw_node(node, cr, pixel_x, pixel_y);

        rpl_node_unlock(node);
        icmp_node_unlock(node);
        ip_node_unlock(node);
        mac_node_unlock(node);
        phy_node_unlock(node);
    }

    /* arrows */
    if (main_win_get_display_params()->show_parent_arrows || main_win_get_display_params()->show_sibling_arrows) {
        for (node_index = 0; node_index < node_count; node_index++) {
            node_t *node = node_list[node_index];

            /* don't draw arrows of dead nodes */
            if (!node->alive) {
                continue;
            }

            phy_node_lock(node);
            ip_node_lock(node);
            rpl_node_lock(node);

            start_pixel_x = phy_node_get_x(node) * scale_x;
            start_pixel_y = phy_node_get_y(node) * scale_y;

            if (main_win_get_display_params()->show_parent_arrows) {
                parent_list = rpl_node_get_parent_list(node, &parent_count);

//                printf("%s.parent_list = [", node->phy_info->name);
//                for (parent_index = 0; parent_index < parent_count; parent_index++) {
//                    node_t *parent = parent_list[parent_index]->node;
//                    if (parent == NULL)
//                        printf(" (NULL) ");
//                    else
//                        printf(" %s ", parent->phy_info->name);
//                }
//                printf("]\n");

                for (parent_index = 0; parent_index < parent_count; parent_index++) {
                    node_t *parent = parent_list[parent_index]->node;

                    phy_node_lock(parent);
                    ip_node_lock(parent);
                    rpl_node_lock(parent);

                    end_pixel_x = phy_node_get_x(parent) * scale_x;
                    end_pixel_y = phy_node_get_y(parent) * scale_y;

                    uint32 color;
                    if (parent->alive) {
                        if (rpl_node_get_pref_parent(node) == parent_list[parent_index]) {
                            color = SIM_FIELD_PREF_PARENT_ARROW_COLOR;
                        }
                        else {
                            color = SIM_FIELD_PARENT_ARROW_COLOR;
                        }
                    }
                    else {
                        color = SIM_FIELD_DEAD_ARROW_COLOR;
                    }

                    rpl_node_unlock(parent);
                    ip_node_unlock(parent);
                    phy_node_unlock(parent);

                    bool packet = FALSE; //node_has_pdu_from(parent, node);
                    draw_parent_arrow(cr, start_pixel_x, start_pixel_y, end_pixel_x, end_pixel_y, color, packet);
                }
            }

            if (main_win_get_display_params()->show_sibling_arrows) {
                sibling_list = rpl_node_get_sibling_list(node, &sibling_count);

                for (sibling_index = 0; sibling_index < sibling_count; sibling_index++) {
                    node_t *sibling = sibling_list[sibling_index]->node;

                    phy_node_lock(sibling);
                    ip_node_lock(sibling);
                    rpl_node_lock(sibling);

                    end_pixel_x = phy_node_get_x(sibling) * scale_x;
                    end_pixel_y = phy_node_get_y(sibling) * scale_y;

                    uint32 color = sibling->alive ? SIM_FIELD_SIBLING_ARROW_COLOR : SIM_FIELD_DEAD_ARROW_COLOR;

                    bool packet = FALSE;    // node_has_pdu_from(sibling, node);

                    /* in case of mutual siblingness, only the "lowest" node draws the line */
                    if (node < sibling) {
                        draw_sibling_arrow(cr, start_pixel_x, start_pixel_y, end_pixel_x, end_pixel_y, color, packet, TRUE);
                    }
                    else {
                        if (rpl_node_has_sibling(sibling, node) &&
                                sibling->alive &&
                                rs_system_has_node(sibling)) {
                            draw_sibling_arrow(cr, start_pixel_x, start_pixel_y, end_pixel_x, end_pixel_y, color, packet, FALSE);
                        }
                        else {
                            draw_sibling_arrow(cr, start_pixel_x, start_pixel_y, end_pixel_x, end_pixel_y, color, packet, TRUE);
                        }
                    }

                    rpl_node_unlock(sibling);
                    ip_node_unlock(sibling);
                    phy_node_unlock(sibling);
                }
            }

            rpl_node_unlock(node);
            ip_node_unlock(node);
            phy_node_unlock(node);
        }
    }

    free(node_list);

    /* decorate the hovered node */
    if (hover_node != NULL) {
        pixel_x = phy_node_get_x(hover_node) * scale_x;
        pixel_y = phy_node_get_y(hover_node) * scale_y;

        draw_arrow(cr,
                pixel_x - SIM_FIELD_NODE_RADIUS * 4, pixel_y, pixel_x - SIM_FIELD_NODE_RADIUS * 1, pixel_y, 0,
                SIM_FIELD_HOVER_COLOR, TRUE, 1.0, TRUE);
        draw_arrow(cr,
                pixel_x, pixel_y - SIM_FIELD_NODE_RADIUS * 4, pixel_x, pixel_y - SIM_FIELD_NODE_RADIUS * 1, 0,
                SIM_FIELD_HOVER_COLOR, TRUE, 1.0, TRUE);
        draw_arrow(cr,
                pixel_x + SIM_FIELD_NODE_RADIUS * 4, pixel_y, pixel_x + SIM_FIELD_NODE_RADIUS * 1, pixel_y, 0,
                SIM_FIELD_HOVER_COLOR, TRUE, 1.0, TRUE);
        draw_arrow(cr,
                pixel_x, pixel_y + SIM_FIELD_NODE_RADIUS * 4, pixel_x, pixel_y + SIM_FIELD_NODE_RADIUS * 1, 0,
                SIM_FIELD_HOVER_COLOR, TRUE, 1.0, TRUE);
    }

    /* decorate the selected node */
    node_t *selected_node = main_win_get_selected_node();
    if (selected_node != NULL) {
        pixel_x = phy_node_get_x(selected_node) * scale_x;
        pixel_y = phy_node_get_y(selected_node) * scale_y;

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

    /* do the actual double-buffered paint */
    cairo_t *sim_field_cr = gdk_cairo_create(sim_field_window);
    cairo_set_source_surface(sim_field_cr, surface, 0, 0);
    cairo_paint(sim_field_cr);
    cairo_destroy(sim_field_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    return FALSE;
}

static void draw_node(node_t *node, cairo_t *cr, double pixel_x, double pixel_y)
{
    percent_t tx_power = phy_node_get_tx_power(node);
    uint8 sequence_number = rpl_node_get_seq_num(node);

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
    if (main_win_get_display_params()->show_node_names) {
        draw_text(cr, phy_node_get_name(node),
                pixel_x, pixel_y,
                SIM_FIELD_TEXT_NAME_FG_COLOR,
                strlen(phy_node_get_name(node)) > 2 ? SIM_FIELD_TEXT_BG_COLOR : 0);
    }

    /* node address */
    if (main_win_get_display_params()->show_node_addresses) {
        draw_text(cr, ip_node_get_address(node),
                pixel_x, pixel_y + SIM_FIELD_NODE_RADIUS * 2,
                SIM_FIELD_TEXT_ADDRESS_FG_COLOR, SIM_FIELD_TEXT_BG_COLOR);
    }

    /* node rank */
    if (main_win_get_display_params()->show_node_ranks) {
        char rank_str[256];
        snprintf(rank_str, sizeof(rank_str), "%d", rpl_node_get_rank(node));

        draw_text(cr, rank_str,
                pixel_x + SIM_FIELD_NODE_RADIUS * 3, pixel_y,
                SIM_FIELD_TEXT_RANK_FG_COLOR, SIM_FIELD_TEXT_BG_COLOR);
    }
}

static void draw_parent_arrow(cairo_t *cr, double start_pixel_x, double start_pixel_y, double stop_pixel_x, double stop_pixel_y, uint32 color, bool packet)
{
    double thickness = (packet ? 3.0 : 1.0);
    draw_arrow(cr, start_pixel_x, start_pixel_y, stop_pixel_x, stop_pixel_y, SIM_FIELD_NODE_RADIUS, color, TRUE, thickness, FALSE);
}

static void draw_sibling_arrow(cairo_t *cr, double start_pixel_x, double start_pixel_y, double stop_pixel_x, double stop_pixel_y, uint32 color, bool packet, bool draw_line)
{
    double thickness = (packet ? 3.0 : 1.0);
    draw_arrow(cr, start_pixel_x, start_pixel_y, stop_pixel_x, stop_pixel_y, SIM_FIELD_NODE_RADIUS, color, draw_line, thickness, TRUE);
}

static gboolean update_rulers_wrapper(void *data)
{
    coord_t width = rs_system_get_width();
    coord_t height = rs_system_get_height();

    gtk_ruler_set_range(GTK_RULER(sim_field_hruler), 0, width, 0, width);
    gtk_ruler_set_range(GTK_RULER(sim_field_vruler), 0, height, 0, height);

    return FALSE;
}

static void draw_arrow(cairo_t *cr, double start_x, double start_y, double end_x, double end_y, double radius, uint32 color, bool draw_line, double thickness, bool dashed)
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
            static double dashes[] = {5.0, 5.0};
            static int len = sizeof(dashes) / sizeof(dashes[0]);

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

static void draw_text(cairo_t *cr, char *text, double x, double y, uint32 fg_color, uint32 bg_color)
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

static node_t *find_node_under_coords(gint x, gint y, float scale_x, float scale_y)
{
    system_lock();

    uint16 node_count;
    int32 index;
    node_t **node_list;
    node_list = rs_system_get_node_list(&node_count);

    coord_t system_x = x / scale_x;
    coord_t system_y = y / scale_y;
    coord_t system_radius = SIM_FIELD_NODE_RADIUS * 2 / scale_y ;

    for (index = node_count - 1; index >= 0; index--) {
        node_t *node = node_list[index];

        coord_t node_x = phy_node_get_x(node);
        coord_t node_y = phy_node_get_y(node);

        if ((system_x > node_x - system_radius) &&
            (system_x < node_x + system_radius) &&
            (system_y > node_y - system_radius) &&
            (system_y < node_y + system_radius)) {

            system_unlock();

            return node;
        }
    }

    system_unlock();

    return NULL;
}
