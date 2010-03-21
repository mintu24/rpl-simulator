/*
   RPL Simulator.

   Copyright (c) Calin Crisan 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

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

#ifdef USE_DOCUMENT_COLORS_AND_SIZES

#define SIM_FIELD_NODE_RADIUS                   20

#define SIM_FIELD_BG_COLOR                      0xFFFFFFFF

#define SIM_FIELD_TEXT_FONT                     "sans"
#define SIM_FIELD_TEXT_SIZE                     24.0
#define SIM_FIELD_TEXT_BG_COLOR                 0xBB303030
#define SIM_FIELD_TEXT_NAME_FG_COLOR            0xFFFFFFFF
#define SIM_FIELD_TEXT_RANK_FG_COLOR            0xFFFFFF00
#define SIM_FIELD_TEXT_ADDRESS_FG_COLOR         0xFFFFFF00

#define SIM_FIELD_PREF_PARENT_ARROW_COLOR       0xFF911D17
#define SIM_FIELD_PARENT_ARROW_COLOR            0xFF172A91
#define SIM_FIELD_SIBLING_ARROW_COLOR           0xFF172A91
#define SIM_FIELD_DEAD_ARROW_COLOR              0xFF808080
#define SIM_FIELD_REPRESENT_LINK_QUALITY        FALSE

#define SIM_FIELD_HOVER_COLOR                   0xFF000000
#define SIM_FIELD_SELECTED_COLOR                0xFF000000

#else /* USE_DOCUMENT_COLORS_AND_SIZES */

#define SIM_FIELD_NODE_RADIUS                   10

#define SIM_FIELD_BG_COLOR                      0xFF151515

#define SIM_FIELD_TEXT_FONT                     "sans"
#define SIM_FIELD_TEXT_SIZE                     12.0
#define SIM_FIELD_TEXT_BG_COLOR                 0x80303030
#define SIM_FIELD_TEXT_NAME_FG_COLOR            0xFFFFFFFF
#define SIM_FIELD_TEXT_RANK_FG_COLOR            0xFFFFFF00
#define SIM_FIELD_TEXT_ADDRESS_FG_COLOR         0xFFFFFF00

#define SIM_FIELD_PREF_PARENT_ARROW_COLOR       0xFFFF8E50
#define SIM_FIELD_PARENT_ARROW_COLOR            0xFF72ADFF
#define SIM_FIELD_SIBLING_ARROW_COLOR           0xFF72ADFF
#define SIM_FIELD_DEAD_ARROW_COLOR              0xFF808080
#define SIM_FIELD_REPRESENT_LINK_QUALITY        TRUE

#define SIM_FIELD_HOVER_COLOR                   0xFFFFFFFF
#define SIM_FIELD_SELECTED_COLOR                0xFFFFFFFF

#endif /* USE_DOCUMENT_COLORS_AND_SIZES */

#define SIM_FIELD_REDRAW_INTERVAL               100


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
