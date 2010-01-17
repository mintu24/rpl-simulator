
#include <math.h>

#include "simfield.h"
#include "../system.h"


    /*** global variables ***/

static GdkWindow *sim_field_window = NULL;
static GdkGC *sim_field_gc = NULL;
static cairo_t *sim_field_cr = NULL;


static void draw_arrow_line(double start_x, double start_y, double end_x, double end_y, double radius);
static void draw_node(node_t *node, gint width, gint height);


void sim_field_init(GdkWindow *window, GdkGC *gc)
{
    sim_field_window = window;
    sim_field_gc = gc;

    sim_field_cr = gdk_cairo_create(window);
}

void sim_field_redraw()
{
    gint width, height;
    gdk_drawable_get_size(sim_field_window, &width, &height);

    /* paint the back frame */
    cairo_set_source_rgb(sim_field_cr, 60 / 255.0, 90 / 255.0, 160 / 255.0);
    cairo_rectangle(sim_field_cr, 0, 0, width, height);
    cairo_stroke(sim_field_cr);

    uint16 node_count, index;
    node_t **node_list;
    node_list = rs_system_get_nodes(&node_count);

    for (index = 0; index < node_count; index++) {
        node_t *node = node_list[index];
        draw_node(node, width, height);
    }
}

void sim_field_node_coords_assure_non_intersection(coord_t *x, coord_t *y)
{
    if (x == NULL || y == NULL) {
        rs_error("invalid arguments");
        return;
    }

    double angle = atan2(200 - *y, 200 - *x) + M_PI;

    int xlim = 200 + MIN_NODE_DISTANCE * cos(angle);
    int ylim = 200 + MIN_NODE_DISTANCE * sin(angle);

    if ((*x > 200 && *x < xlim) || (*x < 200 && *x > xlim)) {
        *x = xlim;
    }

    if ((*y > 200 && *y < ylim) || (*y < 200 && *y > ylim)) {
        *y = ylim;
    }
}


static void draw_arrow_line(double start_x, double start_y, double end_x, double end_y, double radius)
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

    cairo_move_to(sim_field_cr, xb, yb);
    cairo_line_to(sim_field_cr, xa, ya);
    cairo_line_to(sim_field_cr, xc, yc);

    cairo_fill(sim_field_cr);

    cairo_move_to(sim_field_cr, xa, ya);
    cairo_line_to(sim_field_cr, xd, yd);

    cairo_stroke(sim_field_cr);
}

static void draw_node(node_t *node, gint width, gint height)
{
    percent_t tx_power = node->phy_info->tx_power;
    coord_t x = node->phy_info->cx;
    coord_t y = node->phy_info->cy;

    char filename[256];
    snprintf(filename, sizeof(filename), "%s/node-%d.png", RES_DIR, (int) round(tx_power * 10) * 10);

    cairo_surface_t *image = cairo_image_surface_create_from_png(filename);
    if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
        rs_error("failed to load png image '%s': %s", filename, cairo_status_to_string(cairo_surface_status(image)));
        return;
    }

    int w = cairo_image_surface_get_width(image);
    int h = cairo_image_surface_get_height(image);

    //cairo_scale(cr, 50 / (double) w, 50 / (double) h);

    cairo_set_source_surface(sim_field_cr, image, 200 - w / 2, 200 - h / 2);
    cairo_paint(sim_field_cr);
    cairo_set_source_surface(sim_field_cr, image, x - w / 2, y - h / 2);
    cairo_paint(sim_field_cr);
    cairo_surface_destroy(image);

    cairo_set_line_width(sim_field_cr, 1);
    cairo_set_source_rgba(sim_field_cr, 0, 0, 0, 0.5);

    draw_arrow_line(200, 200, x, y, NODE_RADIUS);

    //cairo_destroy(cr); todo: move this call to a sim_field_done() function
}
