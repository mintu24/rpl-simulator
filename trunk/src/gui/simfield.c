
#include <math.h>

#include "simfield.h"
#include "../main.h"
#include "../system.h"


    /*** global variables ***/

static GdkWindow *          sim_field_window = NULL;
static GdkGC *              sim_field_gc = NULL;
static cairo_surface_t *    node_images[TX_POWER_STEP_COUNT];

static GtkWidget *          sim_field_drawing_area = NULL;
static GtkWidget *          sim_field_vruler = NULL;
static GtkWidget *          sim_field_hruler = NULL;

static node_t *             selected_node = NULL;


    /**** local function prototypes ****/

static gboolean             cb_sim_field_drawing_area_expose(GtkWidget *widget, GdkEventExpose *event, gpointer data);
static gboolean             cb_sim_field_drawing_area_button_press(GtkDrawingArea *widget, GdkEventButton *event, gpointer data);
static gboolean             cb_sim_field_drawing_area_button_release(GtkDrawingArea *widget, GdkEventButton *event, gpointer data);
static gboolean             cb_sim_field_drawing_area_motion_notify(GtkDrawingArea *widget, GdkEventMotion *event, gpointer data);
static gboolean             cb_sim_field_drawing_area_scroll(GtkDrawingArea *widget, GdkEventScroll *event, gpointer data);

static void                 draw_arrow_line(cairo_t *cr, double start_x, double start_y, double end_x, double end_y, double radius);
static void                 draw_node(node_t *node, cairo_t *cr, float scale_x, float scale_y);
static gboolean             draw_sim_field(void *data);


    /**** exported functions ****/

GtkWidget *sim_field_create(GdkWindow *window, GdkGC *gc)
{
    sim_field_window = window;
    sim_field_gc = gc;

    /* load node images */
    char filename[256];
    int power_step;

    for (power_step = 0; power_step < TX_POWER_STEP_COUNT; power_step++) {
        snprintf(filename, sizeof(filename), "%s/node-%d.png", RES_DIR, power_step * (100 / (TX_POWER_STEP_COUNT - 1)));

        cairo_surface_t *image = cairo_image_surface_create_from_png(filename);
        if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
            rs_error("failed to load png image '%s': %s", filename, cairo_status_to_string(cairo_surface_status(image)));
            return NULL;
        }

        node_images[power_step] = image;
    }

    GtkWidget *table = gtk_table_new(2, 2, FALSE);

    sim_field_hruler = gtk_hruler_new();
    gtk_ruler_set_metric(GTK_RULER(sim_field_hruler), GTK_PIXELS);
    gtk_ruler_set_range(GTK_RULER(sim_field_hruler), 0, 100, 0, 100);
    gtk_table_attach(GTK_TABLE(table), sim_field_hruler, 1, 2, 0, 1, GTK_FILL, GTK_FILL, 0, 1);

    sim_field_vruler = gtk_vruler_new();
    gtk_ruler_set_metric(GTK_RULER(sim_field_vruler), GTK_PIXELS);
    gtk_ruler_set_range(GTK_RULER(sim_field_vruler), 0, 100, 0, 100);
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

/*
void sim_field_node_coords_assure_non_intersection(node_t *subject_node, coord_t *x, coord_t *y)
{
    // fixme: baaad implementation

    if (x == NULL || y == NULL) {
        rs_error("invalid arguments");
        return;
    }

    node_t **node_list, *node;
    uint16 node_count, index;
    coord_t xlim, ylim, node_x, node_y;
    double angle;

    node_list = rs_system_get_nodes(&node_count);

    for (index = 0; index < node_count; index++) {
        node = node_list[index];

        if (node == subject_node) {
            continue;
        }

        node_x = phy_node_get_x(node);
        node_y = phy_node_get_y(node);

        angle = atan2(node_y - *y, node_x - *x) + M_PI;
        xlim = node_x + MIN_NODE_DISTANCE * cos(angle);
        ylim = node_y + MIN_NODE_DISTANCE * sin(angle);

        if ((*x > node_x && *x < xlim) || (*x < node_x && *x > xlim)) {
            *x = xlim;
        }

        if ((*y > node_y && *y < ylim) || (*y < node_y && *y > ylim)) {
            *y = ylim;
        }
    }

}
*/

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
    rs_assert(selected_node == NULL);

    gint pixel_width, pixel_height;
    gdk_drawable_get_size(sim_field_window, &pixel_width, &pixel_height);

    float scale_x = pixel_width / rs_system_get_width();
    float scale_y = pixel_height / rs_system_get_height();

    coord_t current_x = event->x / scale_x;
    coord_t current_y = event->y / scale_y;

    selected_node = rs_system_find_node_by_name("B");
    if (selected_node == NULL) {
        return FALSE;
    }

    phy_node_set_xy(selected_node, current_x, current_y);

    sim_field_redraw();

    return TRUE;
}

static gboolean cb_sim_field_drawing_area_button_release(GtkDrawingArea *widget, GdkEventButton *event, gpointer data)
{
    selected_node = NULL;

    return TRUE;
}

static gboolean cb_sim_field_drawing_area_motion_notify(GtkDrawingArea *widget, GdkEventMotion *event, gpointer data)
{
    if (selected_node == NULL) {
        return FALSE;
    }

    // todo update h,v ruler positions

    gint pixel_width, pixel_height;
    gdk_drawable_get_size(sim_field_window, &pixel_width, &pixel_height);

    float scale_x = pixel_width / rs_system_get_width();
    float scale_y = pixel_height / rs_system_get_height();

    coord_t current_x = event->x / scale_x;
    coord_t current_y = event->y / scale_y;

    phy_node_set_xy(selected_node, current_x, current_y);

    sim_field_redraw();

    return TRUE;
}

static gboolean cb_sim_field_drawing_area_scroll(GtkDrawingArea *widget, GdkEventScroll *event, gpointer data)
{
    node_t *node = rs_system_find_node_by_name("B");
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

    sim_field_redraw();

    return TRUE;
}

static void draw_arrow_line(cairo_t *cr, double start_x, double start_y, double end_x, double end_y, double radius)
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

    cairo_move_to(cr, xb, yb);
    cairo_line_to(cr, xa, ya);
    cairo_line_to(cr, xc, yc);

    cairo_fill(cr);

    cairo_move_to(cr, xa, ya);
    cairo_line_to(cr, xd, yd);

    cairo_stroke(cr);
}

static void draw_node(node_t *node, cairo_t *cr, float scale_x, float scale_y)
{
    // todo: return bool

    percent_t tx_power = phy_node_get_tx_power(node);
    coord_t x = phy_node_get_x(node);
    coord_t y = phy_node_get_y(node);

    int pixel_x = x * scale_x;
    int pixel_y = y * scale_y;

    cairo_surface_t *image = node_images[(int) round(tx_power * 10)];

    int w = cairo_image_surface_get_width(image);
    int h = cairo_image_surface_get_height(image);

    cairo_set_source_surface(cr, image, pixel_x - w / 2, pixel_y - h / 2);
    cairo_paint(cr);

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
    node_list = rs_system_get_nodes(&node_count);

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
