#ifndef SIMFIELD_H_
#define SIMFIELD_H_

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

#include "../base.h"
#include "../node.h"

#define SIM_FIELD_TX_POWER_STEP_COUNT           11
#define SIM_FIELD_NODE_COLOR_COUNT              6
#define SIM_FIELD_NODE_RADIUS                   10

#define SIM_FIELD_BG_COLOR                      0xFF151515

#define SIM_FIELD_TEXT_FONT                     "sans"
#define SIM_FIELD_TEXT_SIZE                     12.0
#define SIM_FIELD_TEXT_BG_COLOR                 0x80303030
#define SIM_FIELD_TEXT_NAME_FG_COLOR            0xFFFFFF00
#define SIM_FIELD_TEXT_RANK_FG_COLOR            0xFFFFFF00
#define SIM_FIELD_TEXT_ADDRESS_FG_COLOR         0xFFFFFF00

#define SIM_FIELD_PREF_PARENT_ARROW_COLOR       0xFFFF8E50
#define SIM_FIELD_PARENT_ARROW_COLOR            0xFF72ADFF
#define SIM_FIELD_SIBLING_ARROW_COLOR           0xFF72ADFF
#define SIM_FIELD_DEAD_ARROW_COLOR              0xFF808080

#define SIM_FIELD_HOVER_COLOR                   0xFFFFFFFF
#define SIM_FIELD_SELECTED_COLOR                0xFFFFFFFF

#define SIM_FIELD_REDRAW_INTERVAL               200


#define EXPAND_COLOR(color, r, g, b, a) { \
        r = (((color) & 0x00FF0000) >> 16) / 255.0; \
        g = (((color) & 0x0000FF00) >> 8) / 255.0; \
        b = (((color) & 0x000000FF)) / 255.0; \
        a = (((color) & 0xFF000000) >> 24) / 255.0; \
}



GtkWidget *     sim_field_create();
void            sim_field_redraw();

void            sim_field_draw_node(node_t *node, cairo_t *cr, double pixel_x, double pixel_y);
void            sim_field_draw_parent_arrow(cairo_t *cr, double start_pixel_x, double start_pixel_y, double stop_pixel_x, double stop_pixel_y, uint32 color, bool packet);
void            sim_field_draw_sibling_arrow(cairo_t *cr, double start_pixel_x, double start_pixel_y, double stop_pixel_x, double stop_pixel_y, uint32 color, bool packet, bool draw_line);
void            sim_field_draw_arrow(cairo_t *cr, double start_x, double start_y, double end_x, double end_y, double radius, uint32 color, bool draw_line, double thickness, bool dashed);
void            sim_field_draw_text(cairo_t *cr, char *text, double x, double y, uint32 fg_color, uint32 bg_color);


#endif /* SIMFIELD_H_ */
