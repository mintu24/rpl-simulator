
#include <math.h>
#include <gdk/gdk.h>

#include "simfield.h"
#include "mainwin.h"
#include "../main.h"
#include "../system.h"

#define EXPAND_COLOR(color, r, g, b, a) {   \
        r = (((color) & 0x00FF0000) >> 16) / 255.0;  \
        g = (((color) & 0x0000FF00) >> 8) / 255.0;  \
        b = (((color) & 0x000000FF)) / 255.0;  \
        a = (((color) & 0xFF000000) >> 24) / 255.0; }


    /*** global variables ***/

static GdkWindow *          sim_field_window = NULL;
static GdkGC *              sim_field_gc = NULL;

static cairo_surface_t *    normal_node_images[TX_POWER_STEP_COUNT];
static cairo_surface_t *    hover_node_images[TX_POWER_STEP_COUNT];
static cairo_surface_t *    selected_node_images[TX_POWER_STEP_COUNT];
static cairo_surface_t *    hover_selected_node_images[TX_POWER_STEP_COUNT];

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

static void                 draw_arrow_line(cairo_t *cr, double start_x, double start_y, double end_x, double end_y, double radius, uint32 color);
static void                 draw_text(cairo_t *cr, char *text, double x, double y, uint32 fg_color, uint32 bg_color);
static void                 draw_node(node_t *node, cairo_t *cr, float scale_x, float scale_y);
static gboolean             draw_sim_field(void *data);

static node_t *             find_node_under_coords(gint x, gint y, gint pixel_width, gint pixel_height, float scale_x, float scale_y);


    /**** exported functions ****/

GtkWidget *sim_field_create()
{
    /* load node images */
    char filename[256];
    int power_step;

    for (power_step = 0; power_step < TX_POWER_STEP_COUNT; power_step++) {
        snprintf(filename, sizeof(filename), "%s/node-normal-%d.png", RES_DIR, power_step * (100 / (TX_POWER_STEP_COUNT - 1)));
        cairo_surface_t *image = cairo_image_surface_create_from_png(filename);
        if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
            rs_error("failed to load png image '%s': %s", filename, cairo_status_to_string(cairo_surface_status(image)));
            return NULL;
        }
        normal_node_images[power_step] = image;

        snprintf(filename, sizeof(filename), "%s/node-hover-%d.png", RES_DIR, power_step * (100 / (TX_POWER_STEP_COUNT - 1)));
        image = cairo_image_surface_create_from_png(filename);
        if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
            rs_error("failed to load png image '%s': %s", filename, cairo_status_to_string(cairo_surface_status(image)));
            return NULL;
        }
        hover_node_images[power_step] = image;

        snprintf(filename, sizeof(filename), "%s/node-selected-%d.png", RES_DIR, power_step * (100 / (TX_POWER_STEP_COUNT - 1)));
        image = cairo_image_surface_create_from_png(filename);
        if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
            rs_error("failed to load png image '%s': %s", filename, cairo_status_to_string(cairo_surface_status(image)));
            return NULL;
        }
        selected_node_images[power_step] = image;

        snprintf(filename, sizeof(filename), "%s/node-hover-selected-%d.png", RES_DIR, power_step * (100 / (TX_POWER_STEP_COUNT - 1)));
        image = cairo_image_surface_create_from_png(filename);
        if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
            rs_error("failed to load png image '%s': %s", filename, cairo_status_to_string(cairo_surface_status(image)));
            return NULL;
        }
        hover_selected_node_images[power_step] = image;
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

    coord_t width = rs_system_get_width();
    coord_t height = rs_system_get_height();

    gtk_ruler_set_range(GTK_RULER(sim_field_hruler), 0, width, width / 2, width);
    gtk_ruler_set_range(GTK_RULER(sim_field_vruler), 0, height, height / 2, height);

    /* assure thread safety, performing all the drawing ops in the GUI thread */
    gdk_threads_add_idle(draw_sim_field, NULL);
}


    /**** local functions ****/

static gboolean cb_sim_field_drawing_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    sim_field_redraw(); // todo: is this a good idea?

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

    node_t *node = find_node_under_coords(event->x, event->y, pixel_width, pixel_height, scale_x, scale_y);
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

    sim_field_redraw();

    coord_t current_x = event->x / scale_x;
    coord_t current_y = event->y / scale_y;

    if (moving_node != NULL) {
        phy_node_set_xy(moving_node, current_x, current_y);

        main_win_node_to_gui(moving_node);
        sim_field_redraw();
    }

    gtk_ruler_set_range(GTK_RULER(sim_field_hruler), 0, system_width, current_x, system_width);
    gtk_ruler_set_range(GTK_RULER(sim_field_vruler), 0, system_height, current_y, system_height);

    return TRUE;
}

static gboolean cb_sim_field_drawing_area_scroll(GtkDrawingArea *widget, GdkEventScroll *event, gpointer data)
{
    if (hover_node == NULL) {
        return FALSE;
    }

    if (event->direction == GDK_SCROLL_UP) {
        if (phy_node_get_tx_power(hover_node) + 0.1 <= 1.0) {
            phy_node_set_tx_power(hover_node, phy_node_get_tx_power(hover_node) + 0.1);
        }
        else {
            phy_node_set_tx_power(hover_node, 1.0);
        }
    }
    else /* (event->direction == GDK_SCROLL_DOWN) */{
        if (phy_node_get_tx_power(hover_node) - 0.1 >= 0.0) {
            phy_node_set_tx_power(hover_node, phy_node_get_tx_power(hover_node) - 0.1);
        }
        else {
            phy_node_set_tx_power(hover_node, 0.0);
        }
    }

    main_win_node_to_gui(hover_node);
    sim_field_redraw();

    return TRUE;
}

static void draw_arrow_line(cairo_t *cr, double start_x, double start_y, double end_x, double end_y, double radius, uint32 color)
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

    // todo use color

    cairo_move_to(cr, xb, yb);
    cairo_line_to(cr, xa, ya);
    cairo_line_to(cr, xc, yc);

    cairo_fill(cr);

    cairo_move_to(cr, xa, ya);
    cairo_line_to(cr, xd, yd);

    cairo_stroke(cr);
}

static void draw_text(cairo_t *cr, char *text, double x, double y, uint32 fg_color, uint32 bg_color)
{
    double r, g, b, a;
    cairo_text_extents_t te;
    double tx, ty, tw, th;

    cairo_select_font_face(cr, TEXT_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, TEXT_SIZE);
    cairo_text_extents(cr, text, &te);
    tx = x - te.width / 2;
    ty = y - te.height / 2;
    tw = te.width;
    th = te.height;

    if (bg_color != 0) {
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

static void draw_node(node_t *node, cairo_t *cr, float scale_x, float scale_y)
{
    percent_t tx_power = phy_node_get_tx_power(node);
    coord_t x = phy_node_get_x(node);
    coord_t y = phy_node_get_y(node);

    int pixel_x = x * scale_x;
    int pixel_y = y * scale_y;

    uint32 bg_color;
    cairo_surface_t **images;

    if (node == main_win_get_selected_node()) {
        if (node == hover_node) {
            bg_color = TEXT_HOVER_SELECTED_BG_COLOR;
            images = hover_selected_node_images;
        }
        else {
            bg_color = TEXT_SELECTED_BG_COLOR;
            images = selected_node_images;
        }
    }
    else {
        if (node == hover_node) {
            bg_color = TEXT_HOVER_BG_COLOR;
            images = hover_node_images;
        }
        else {
            bg_color = TEXT_NORMAL_BG_COLOR;
            images = normal_node_images;
        }
    }

    /* node itself */
    if (!main_win_get_display_params()->show_node_tx_power) {
        tx_power = 0.0;
    }

    cairo_surface_t *image = images[(int) round(tx_power * 10)];

    int image_width = cairo_image_surface_get_width(image);
    int image_height = cairo_image_surface_get_height(image);

    cairo_set_source_surface(cr, image, pixel_x - image_width / 2, pixel_y - image_height / 2);
    cairo_paint(cr);

    /* node name */
    if (main_win_get_display_params()->show_node_names) {
        draw_text(cr, phy_node_get_name(node), pixel_x, pixel_y, TEXT_NAME_FG_COLOR,
                strlen(phy_node_get_name(node)) > 2 ? bg_color : 0);
    }

    /* node address */
    if (main_win_get_display_params()->show_node_addresses) {
        draw_text(cr, ip_node_get_address(node), pixel_x, pixel_y + NODE_RADIUS * 2, TEXT_ADDRESS_FG_COLOR, bg_color);
    }

    //cairo_set_line_width(cr, 1);
    //cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
    //draw_arrow_line(cr, 200, 200, x, y, NODE_RADIUS);
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
    cairo_set_source_rgb(cr, 30 / 255.0, 30 / 255.0, 30 / 255.0);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    /* nodes */
    uint16 node_count, index;
    node_t **node_list;
    node_list = rs_system_get_node_list(&node_count);

    float scale_x = pixel_width / rs_system_get_width();
    float scale_y = pixel_height / rs_system_get_height();

    for (index = 0; index < node_count; index++) {
        node_t *node = node_list[index];
        draw_node(node, cr, scale_x, scale_y);
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

static node_t *find_node_under_coords(gint x, gint y, gint pixel_width, gint pixel_height, float scale_x, float scale_y)
{
    uint16 node_count;
    int32 index;
    node_t **node_list;
    node_list = rs_system_get_node_list(&node_count);

    uint16 pixel_x, pixel_y;

    for (index = node_count - 1; index >= 0; index--) {
        node_t *node = node_list[index];

        pixel_x = phy_node_get_x(node) * scale_x;
        pixel_y = phy_node_get_y(node) * scale_y;

        if ((x > pixel_x - NODE_RADIUS) &&
            (x < pixel_x + NODE_RADIUS) &&
            (y > pixel_y - NODE_RADIUS) &&
            (y < pixel_y + NODE_RADIUS)) {

            return node;
        }
    }

    return NULL;
}
