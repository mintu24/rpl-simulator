
#include <math.h>

#include "simfield.h"
#include "../system.h"


    /*** global variables ***/

static GdkWindow *sim_field_window = NULL;
static GdkGC *sim_field_gc = NULL;
static cairo_surface_t *node_images[TX_POWER_STEP_COUNT];


static void draw_arrow_line(cairo_t *cr, double start_x, double start_y, double end_x, double end_y, double radius);
static void draw_node(node_t *node, cairo_t *cr, gint width, gint height);


void sim_field_init(GdkWindow *window, GdkGC *gc)
{
    // todo: return bool
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
            return;
        }

        node_images[power_step] = image;
    }

}

void sim_field_redraw()
{
    gint width, height;
    gdk_drawable_get_size(sim_field_window, &width, &height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    cairo_set_source_rgb(cr, 30 / 255.0, 30 / 255.0, 30 / 255.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

//    /* frame */
//    cairo_set_source_rgb(cr, 255 / 255.0, 255 / 255.0, 255 / 255.0);
//    cairo_rectangle(cr, 0, 0, width, height);
//    cairo_stroke(cr);

    uint16 node_count, index;
    node_t **node_list;
    node_list = rs_system_get_nodes(&node_count);

    for (index = 0; index < node_count; index++) {
        node_t *node = node_list[index];
        draw_node(node, cr, width, height);
    }

    /* do the actual double-buffered paint */
    cairo_t *sim_field_cr = gdk_cairo_create(sim_field_window);
    cairo_set_source_surface(sim_field_cr, surface, 0, 0);
    cairo_paint(sim_field_cr);
    cairo_destroy(sim_field_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

void sim_field_node_coords_assure_non_intersection(node_t *subject_node, coord_t *x, coord_t *y)
{
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

        node_x = node->phy_info->cx;
        node_y = node->phy_info->cy;

        angle = atan2(node_y - *y, node_x - *x) + M_PI;
        xlim = node_x + MIN_NODE_DISTANCE * cos(angle);
        ylim = node_y + MIN_NODE_DISTANCE * sin(angle);

        rs_debug("xlim = %d, ylim = %d", xlim, ylim);

        if ((*x > node_x && *x < xlim) || (*x < node_x && *x > xlim)) {
            *x = xlim;
        }

        if ((*y > node_y && *y < ylim) || (*y < node_y && *y > ylim)) {
            *y = ylim;
        }
    }

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

static void draw_node(node_t *node, cairo_t *cr, gint width, gint height)
{
    // todo: return bool

    percent_t tx_power = node->phy_info->tx_power;
    coord_t x = node->phy_info->cx;
    coord_t y = node->phy_info->cy;

    cairo_surface_t *image = node_images[(int) round(tx_power * 10)];

    int w = cairo_image_surface_get_width(image);
    int h = cairo_image_surface_get_height(image);

    cairo_set_source_surface(cr, image, x - w / 2, y - h / 2);
    cairo_paint(cr);

    //cairo_set_line_width(cr, 1);
    //cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
    //draw_arrow_line(cr, 200, 200, x, y, NODE_RADIUS);
}
