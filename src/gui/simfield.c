#include <math.h>

#include "simfield.h"

void draw_arrow_line(cairo_t *cr, double start_x, double start_y, double end_x, double end_y, double radius)
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

void draw_simfield(GdkDrawable *drawable, GdkGC *gc, coord_t current_x, coord_t current_y, double power)
{
    gint width, height;

    gdk_drawable_get_size(drawable, &width, &height);

    cairo_t *cr = gdk_cairo_create(drawable);

    cairo_set_source_rgb(cr, 60 / 255.0, 90 / 255.0, 160 / 255.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_stroke(cr);

    char filename[256];
    snprintf(filename, sizeof(filename), "%s/node-%d.png", RES_DIR, (int) round(power * 10) * 10);

    cairo_surface_t *image = cairo_image_surface_create_from_png(filename);
    if (cairo_surface_status(image) != CAIRO_STATUS_SUCCESS) {
        rs_error("failed to load png image '%s': %s", filename, cairo_status_to_string(cairo_surface_status(image)));
        return;
    }

    int w = cairo_image_surface_get_width(image);
    int h = cairo_image_surface_get_height(image);

    //cairo_scale(cr, 50 / (double) w, 50 / (double) h);

    cairo_set_source_surface(cr, image, 200 - w / 2, 200 - h / 2);
    cairo_paint(cr);
    cairo_set_source_surface(cr, image, current_x - w / 2, current_y - h / 2);
    cairo_paint(cr);
    cairo_surface_destroy(image);

    cairo_set_line_width(cr, 1);
    cairo_set_source_rgba(cr, 0, 0, 0, 0.5);

    draw_arrow_line(cr, 200, 200, current_x, current_y, NODE_RADIUS);

    //  cairo_set_source_surface(cr, image, 0, 0);
    //  cairo_paint(cr);

    cairo_destroy(cr);
}

void snap_node_coords(coord_t *x, coord_t *y)
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

