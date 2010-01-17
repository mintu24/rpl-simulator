
#include <math.h>

#include "simfield.h"
#include "../system.h"


    /*** global variables ***/

static GdkWindow *sim_field_window = NULL;
static GdkGC *sim_field_gc = NULL;


static void draw_arrow_line(cairo_t *cr, double start_x, double start_y, double end_x, double end_y, double radius);
static void draw_node(node_t *node, cairo_t *cr, gint width, gint height);


void sim_field_init(GdkWindow *window, GdkGC *gc)
{
    sim_field_window = window;
    sim_field_gc = gc;
}

void sim_field_redraw()
{
    gint width, height;
    gdk_drawable_get_size(sim_field_window, &width, &height);

    cairo_t *cr = gdk_cairo_create(sim_field_window);

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

    cairo_destroy(cr);
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

    char filename[256];
    snprintf(filename, sizeof(filename), "%s/node-%d.png", RES_DIR, (int) round(tx_power * 10) * 10);

    cairo_surface_t *image = cairo_image_surface_create_from_png(filename);
    if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
        rs_error("failed to load png image '%s': %s", filename, cairo_status_to_string(cairo_surface_status(image)));
        return;
    }

    int w = cairo_image_surface_get_width(image);
    int h = cairo_image_surface_get_height(image);

    cairo_set_source_surface(cr, image, x - w / 2, y - h / 2);
    cairo_paint(cr);
    cairo_surface_destroy(image);   // todo: do not destroy surface, reuse it!

    //cairo_set_line_width(cr, 1);
    //cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
    //draw_arrow_line(cr, 200, 200, x, y, NODE_RADIUS);
}
