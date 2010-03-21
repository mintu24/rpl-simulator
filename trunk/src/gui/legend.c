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

#include "legend.h"

#include "mainwin.h"
#include "simfield.h"

#include "../system.h"

    /**** global variables ****/

static GtkWidget *              legend_drawing_area1 = NULL;
static GtkWidget *              legend_drawing_area2 = NULL;
static GtkWidget *              legend_drawing_area3 = NULL;
static GtkWidget *              legend_drawing_area4 = NULL;
static GtkWidget *              legend_drawing_area5 = NULL;
static GtkWidget *              legend_drawing_area6 = NULL;
static GtkWidget *              legend_drawing_area7 = NULL;
static GtkWidget *              legend_drawing_area8 = NULL;
static GtkWidget *              legend_drawing_area9 = NULL;
static GtkWidget *              legend_drawing_area10 = NULL;
static GtkWidget *              legend_drawing_area11 = NULL;


    /**** local function prototypes ****/

gboolean                        cb_legend_drawing_area_expose(GtkWidget *drawing_area, GdkEventExpose *event, gpointer data);

static void                     draw_legend1(GdkWindow *window, GdkGC *gc);
static void                     draw_legend2(GdkWindow *window, GdkGC *gc);
static void                     draw_legend3(GdkWindow *window, GdkGC *gc);
static void                     draw_legend4(GdkWindow *window, GdkGC *gc);
static void                     draw_legend5(GdkWindow *window, GdkGC *gc);
static void                     draw_legend6(GdkWindow *window, GdkGC *gc);
static void                     draw_legend7(GdkWindow *window, GdkGC *gc);
static void                     draw_legend8(GdkWindow *window, GdkGC *gc);
static void                     draw_legend9(GdkWindow *window, GdkGC *gc);
static void                     draw_legend10(GdkWindow *window, GdkGC *gc);
static void                     draw_legend11(GdkWindow *window, GdkGC *gc);


    /**** exported functions ****/

GtkWidget *legend_create()
{
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled_window, 300, 0);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    GtkWidget *legend_table = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_table");
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), legend_table);

    legend_drawing_area1 = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_drawing_area1");
    legend_drawing_area2 = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_drawing_area2");
    legend_drawing_area3 = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_drawing_area3");
    legend_drawing_area4 = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_drawing_area4");
    legend_drawing_area5 = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_drawing_area5");
    legend_drawing_area6 = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_drawing_area6");
    legend_drawing_area7 = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_drawing_area7");
    legend_drawing_area8 = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_drawing_area8");
    legend_drawing_area9 = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_drawing_area9");
    legend_drawing_area10 = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_drawing_area10");
    legend_drawing_area11 = (GtkWidget *) gtk_builder_get_object(gtk_builder, "legend_drawing_area11");

    return scrolled_window;
}


    /**** local functions ****/

gboolean cb_legend_drawing_area_expose(GtkWidget *drawing_area, GdkEventExpose *event, gpointer data)
{
    GdkWindow *window = drawing_area->window;
    GdkGC *gc = drawing_area->style->fg_gc[GTK_STATE_NORMAL];

    if (drawing_area == legend_drawing_area1)
        draw_legend1(window, gc);
    else if (drawing_area == legend_drawing_area2)
        draw_legend2(window, gc);
    else if (drawing_area == legend_drawing_area3)
        draw_legend3(window, gc);
    else if (drawing_area == legend_drawing_area4)
        draw_legend4(window, gc);
    else if (drawing_area == legend_drawing_area5)
        draw_legend5(window, gc);
    else if (drawing_area == legend_drawing_area6)
        draw_legend6(window, gc);
    else if (drawing_area == legend_drawing_area7)
        draw_legend7(window, gc);
    else if (drawing_area == legend_drawing_area8)
        draw_legend8(window, gc);
    else if (drawing_area == legend_drawing_area9)
        draw_legend9(window, gc);
    else if (drawing_area == legend_drawing_area10)
        draw_legend10(window, gc);
    else if (drawing_area == legend_drawing_area11)
        draw_legend11(window, gc);

    return TRUE;
}

static void draw_legend1(GdkWindow *window, GdkGC *gc)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    node_t *node = node_create();
    measure_node_init(node);
    phy_node_init(node, "A", 0, 0);
    mac_node_init(node, "mac address");
    ip_node_init(node, "ip address");
    icmp_node_init(node);
    rpl_node_init(node);
    node->phy_info->tx_power = 0.5;
    node->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node->rpl_info->joined_dodag->dodag_id = NULL;
    node->rpl_info->joined_dodag->parent_list = NULL;
    node->rpl_info->joined_dodag->sibling_list = NULL;
    node->rpl_info->joined_dodag->pref_parent = NULL;
    node->rpl_info->joined_dodag->seq_num = 0;
    node->rpl_info->joined_dodag->rank = 3;

    sim_field_draw_node(node, cr, pixel_width / 2, pixel_height / 2);

    node->alive = FALSE;

    node_destroy(node);

    /* do the actual double-buffered paint */
    cairo_t *window_cr = gdk_cairo_create(window);
    cairo_set_source_surface(window_cr, surface, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void draw_legend2(GdkWindow *window, GdkGC *gc)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    node_t *node = node_create();
    measure_node_init(node);
    phy_node_init(node, "A", 0, 0);
    mac_node_init(node, "mac address");
    ip_node_init(node, "ip address");
    icmp_node_init(node);
    rpl_node_init(node);
    node->phy_info->tx_power = 0.5;
    node->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node->rpl_info->joined_dodag->dodag_id = NULL;
    node->rpl_info->joined_dodag->parent_list = NULL;
    node->rpl_info->joined_dodag->sibling_list = NULL;
    node->rpl_info->joined_dodag->pref_parent = NULL;
    node->rpl_info->joined_dodag->rank = 3;
    node->rpl_info->joined_dodag->seq_num = 0;
    node->alive = TRUE;

    sim_field_draw_node(node, cr, pixel_width / 2, pixel_height / 2);

    node->alive = FALSE;
    node_destroy(node);

    /* do the actual double-buffered paint */
    cairo_t *window_cr = gdk_cairo_create(window);
    cairo_set_source_surface(window_cr, surface, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void draw_legend3(GdkWindow *window, GdkGC *gc)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    uint32 pixel_x = pixel_width / 2;
    uint32 pixel_y = pixel_height / 2;

    node_t *node = node_create();
    measure_node_init(node);
    phy_node_init(node, "A", 0, 0);
    mac_node_init(node, "mac address");
    ip_node_init(node, "ip address");
    icmp_node_init(node);
    rpl_node_init(node);
    node->phy_info->tx_power = 0.5;
    node->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node->rpl_info->joined_dodag->dodag_id = NULL;
    node->rpl_info->joined_dodag->parent_list = NULL;
    node->rpl_info->joined_dodag->sibling_list = NULL;
    node->rpl_info->joined_dodag->pref_parent = NULL;
    node->rpl_info->joined_dodag->rank = 3;
    node->rpl_info->joined_dodag->seq_num = 0;
    node->alive = TRUE;

    sim_field_draw_node(node, cr, pixel_x, pixel_y);

    node->alive = FALSE;
    node_destroy(node);

    EXPAND_COLOR(SIM_FIELD_SELECTED_COLOR, r, g, b, a);

    static double dashes[] = {5.0, 5.0};
    static int len = sizeof(dashes) / sizeof(dashes[0]);

    cairo_set_dash(cr, dashes, len, 0.0);
    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_set_line_width(cr, 2.0);

    cairo_move_to(cr, pixel_x + SIM_FIELD_NODE_RADIUS * 2, pixel_y);
    cairo_arc(cr, pixel_x, pixel_y, SIM_FIELD_NODE_RADIUS * 2, 0, M_PI * 2);
    cairo_stroke(cr);

    /* do the actual double-buffered paint */
    cairo_t *window_cr = gdk_cairo_create(window);
    cairo_set_source_surface(window_cr, surface, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void draw_legend4(GdkWindow *window, GdkGC *gc)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    uint32 pixel_x = pixel_width / 2;
    uint32 pixel_y = pixel_height / 2;

    node_t *node = node_create();
    measure_node_init(node);
    phy_node_init(node, "A", 0, 0);
    mac_node_init(node, "mac address");
    ip_node_init(node, "ip address");
    icmp_node_init(node);
    rpl_node_init(node);
    node->phy_info->tx_power = 0.5;
    node->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node->rpl_info->joined_dodag->dodag_id = NULL;
    node->rpl_info->joined_dodag->parent_list = NULL;
    node->rpl_info->joined_dodag->sibling_list = NULL;
    node->rpl_info->joined_dodag->pref_parent = NULL;
    node->rpl_info->joined_dodag->rank = 3;
    node->rpl_info->joined_dodag->seq_num = 0;
    node->alive = TRUE;

    sim_field_draw_node(node, cr, pixel_x, pixel_y);

    node->alive = FALSE;
    node_destroy(node);

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

    /* do the actual double-buffered paint */
    cairo_t *window_cr = gdk_cairo_create(window);
    cairo_set_source_surface(window_cr, surface, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void draw_legend5(GdkWindow *window, GdkGC *gc)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    uint32 pixel_x = pixel_width / 2;
    uint32 pixel_y = pixel_height / 2;

    node_t *node = node_create();
    measure_node_init(node);
    phy_node_init(node, "BR", 0, 0);
    mac_node_init(node, "mac address");
    ip_node_init(node, "ip address");
    icmp_node_init(node);
    rpl_node_init(node);
    node->phy_info->tx_power = 0.5;
    node->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node->rpl_info->joined_dodag->dodag_id = NULL;
    node->rpl_info->joined_dodag->parent_list = NULL;
    node->rpl_info->joined_dodag->sibling_list = NULL;
    node->rpl_info->joined_dodag->pref_parent = NULL;
    node->rpl_info->joined_dodag->rank = 1;
    node->rpl_info->joined_dodag->seq_num = 0;
    node->alive = TRUE;

    sim_field_draw_node(node, cr, pixel_x, pixel_y);

    node->alive = FALSE;
    node_destroy(node);

    /* do the actual double-buffered paint */
    cairo_t *window_cr = gdk_cairo_create(window);
    cairo_set_source_surface(window_cr, surface, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void draw_legend6(GdkWindow *window, GdkGC *gc)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    uint32 pixel_y = pixel_height / 2;

    node_t *node = node_create();
    measure_node_init(node);
    phy_node_init(node, "A", 0, 0);
    mac_node_init(node, "");
    ip_node_init(node, "");
    icmp_node_init(node);
    rpl_node_init(node);
    node->phy_info->tx_power = 0;
    node->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node->rpl_info->joined_dodag->dodag_id = NULL;
    node->rpl_info->joined_dodag->parent_list = NULL;
    node->rpl_info->joined_dodag->sibling_list = NULL;
    node->rpl_info->joined_dodag->pref_parent = NULL;
    node->rpl_info->joined_dodag->rank = 0;
    node->rpl_info->joined_dodag->seq_num = 0;
    node->alive = TRUE;

    uint8 seq_num;
    for (seq_num = 0; seq_num < SIM_FIELD_NODE_COLOR_COUNT - 1; seq_num++) {
        node->rpl_info->joined_dodag->seq_num = seq_num;
        uint32 pixel_x = seq_num * pixel_width / (SIM_FIELD_NODE_COLOR_COUNT - 1) + pixel_width / ((SIM_FIELD_NODE_COLOR_COUNT - 1) * 2);

        sim_field_draw_node(node, cr, pixel_x, pixel_y);
    }

    node->alive = FALSE;
    node_destroy(node);

    /* do the actual double-buffered paint */
    cairo_t *window_cr = gdk_cairo_create(window);
    cairo_set_source_surface(window_cr, surface, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void draw_legend7(GdkWindow *window, GdkGC *gc)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    uint32 pixel_x1 = pixel_width / 4;
    uint32 pixel_x2 = 3 * pixel_width / 4;
    uint32 pixel_y = pixel_height / 2;

    node_t *node1 = node_create();
    measure_node_init(node1);
    phy_node_init(node1, "C", 0, 0);
    mac_node_init(node1, "");
    ip_node_init(node1, "");
    icmp_node_init(node1);
    rpl_node_init(node1);
    node1->phy_info->tx_power = 0.5;
    node1->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node1->rpl_info->joined_dodag->dodag_id = NULL;
    node1->rpl_info->joined_dodag->parent_list = NULL;
    node1->rpl_info->joined_dodag->sibling_list = NULL;
    node1->rpl_info->joined_dodag->pref_parent = NULL;
    node1->rpl_info->joined_dodag->rank = 3;
    node1->rpl_info->joined_dodag->seq_num = 0;
    node1->alive = TRUE;

    node_t *node2 = node_create();
    measure_node_init(node2);
    phy_node_init(node2, "P", 0, 0);
    mac_node_init(node2, "");
    ip_node_init(node2, "");
    icmp_node_init(node2);
    rpl_node_init(node2);
    node2->phy_info->tx_power = 0.5;
    node2->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node2->rpl_info->joined_dodag->dodag_id = NULL;
    node2->rpl_info->joined_dodag->parent_list = NULL;
    node2->rpl_info->joined_dodag->sibling_list = NULL;
    node2->rpl_info->joined_dodag->pref_parent = NULL;
    node2->rpl_info->joined_dodag->rank = 2;
    node2->rpl_info->joined_dodag->seq_num = 0;
    node2->alive = TRUE;

    sim_field_draw_node(node1, cr, pixel_x1, pixel_y);
    sim_field_draw_node(node2, cr, pixel_x2, pixel_y);

    sim_field_draw_parent_arrow(cr, pixel_x1, pixel_y, pixel_x2, pixel_y, SIM_FIELD_PARENT_ARROW_COLOR, FALSE);

    node1->alive = FALSE;
    node2->alive = FALSE;
    node_destroy(node1);
    node_destroy(node2);

    /* do the actual double-buffered paint */
    cairo_t *window_cr = gdk_cairo_create(window);
    cairo_set_source_surface(window_cr, surface, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void draw_legend8(GdkWindow *window, GdkGC *gc)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    uint32 pixel_x1 = pixel_width / 4;
    uint32 pixel_x2 = 3 * pixel_width / 4;
    uint32 pixel_y = pixel_height / 2;

    node_t *node1 = node_create();
    measure_node_init(node1);
    phy_node_init(node1, "C", 0, 0);
    mac_node_init(node1, "");
    ip_node_init(node1, "");
    icmp_node_init(node1);
    rpl_node_init(node1);
    node1->phy_info->tx_power = 0.5;
    node1->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node1->rpl_info->joined_dodag->dodag_id = NULL;
    node1->rpl_info->joined_dodag->parent_list = NULL;
    node1->rpl_info->joined_dodag->sibling_list = NULL;
    node1->rpl_info->joined_dodag->pref_parent = NULL;
    node1->rpl_info->joined_dodag->rank = 3;
    node1->rpl_info->joined_dodag->seq_num = 0;
    node1->alive = TRUE;

    node_t *node2 = node_create();
    measure_node_init(node2);
    phy_node_init(node2, "P", 0, 0);
    mac_node_init(node2, "");
    ip_node_init(node2, "");
    icmp_node_init(node2);
    rpl_node_init(node2);
    node2->phy_info->tx_power = 0.5;
    node2->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node2->rpl_info->joined_dodag->dodag_id = NULL;
    node2->rpl_info->joined_dodag->parent_list = NULL;
    node2->rpl_info->joined_dodag->sibling_list = NULL;
    node2->rpl_info->joined_dodag->pref_parent = NULL;
    node2->rpl_info->joined_dodag->rank = 2;
    node2->rpl_info->joined_dodag->seq_num = 0;
    node2->alive = TRUE;

    sim_field_draw_node(node1, cr, pixel_x1, pixel_y);
    sim_field_draw_node(node2, cr, pixel_x2, pixel_y);

    sim_field_draw_parent_arrow(cr, pixel_x1, pixel_y, pixel_x2, pixel_y, SIM_FIELD_PREF_PARENT_ARROW_COLOR, FALSE);

    node1->alive = FALSE;
    node2->alive = FALSE;
    node_destroy(node1);
    node_destroy(node2);

    /* do the actual double-buffered paint */
    cairo_t *window_cr = gdk_cairo_create(window);
    cairo_set_source_surface(window_cr, surface, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void draw_legend9(GdkWindow *window, GdkGC *gc)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    uint32 pixel_x1 = pixel_width / 4;
    uint32 pixel_x2 = 3 * pixel_width / 4;
    uint32 pixel_y = pixel_height / 2;

    node_t *node1 = node_create();
    measure_node_init(node1);
    phy_node_init(node1, "S", 0, 0);
    mac_node_init(node1, "");
    ip_node_init(node1, "");
    icmp_node_init(node1);
    rpl_node_init(node1);
    node1->phy_info->tx_power = 0.5;
    node1->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node1->rpl_info->joined_dodag->dodag_id = NULL;
    node1->rpl_info->joined_dodag->parent_list = NULL;
    node1->rpl_info->joined_dodag->sibling_list = NULL;
    node1->rpl_info->joined_dodag->pref_parent = NULL;
    node1->rpl_info->joined_dodag->rank = 3;
    node1->rpl_info->joined_dodag->seq_num = 0;
    node1->alive = TRUE;

    node_t *node2 = node_create();
    measure_node_init(node2);
    phy_node_init(node2, "S", 0, 0);
    mac_node_init(node2, "");
    ip_node_init(node2, "");
    icmp_node_init(node2);
    rpl_node_init(node2);
    node2->phy_info->tx_power = 0.5;
    node2->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node2->rpl_info->joined_dodag->dodag_id = NULL;
    node2->rpl_info->joined_dodag->parent_list = NULL;
    node2->rpl_info->joined_dodag->sibling_list = NULL;
    node2->rpl_info->joined_dodag->pref_parent = NULL;
    node2->rpl_info->joined_dodag->rank = 3;
    node2->rpl_info->joined_dodag->seq_num = 0;
    node2->alive = TRUE;

    sim_field_draw_node(node1, cr, pixel_x1, pixel_y);
    sim_field_draw_node(node2, cr, pixel_x2, pixel_y);

    sim_field_draw_sibling_arrow(cr, pixel_x1, pixel_y, pixel_x2, pixel_y, SIM_FIELD_SIBLING_ARROW_COLOR, FALSE, TRUE);

    node1->alive = FALSE;
    node2->alive = FALSE;
    node_destroy(node1);
    node_destroy(node2);

    /* do the actual double-buffered paint */
    cairo_t *window_cr = gdk_cairo_create(window);
    cairo_set_source_surface(window_cr, surface, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void draw_legend10(GdkWindow *window, GdkGC *gc)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    uint32 pixel_x1 = pixel_width / 4;
    uint32 pixel_x2 = 3 * pixel_width / 4;
    uint32 pixel_y = pixel_height / 2;

    node_t *node1 = node_create();
    measure_node_init(node1);
    phy_node_init(node1, "C", 0, 0);
    mac_node_init(node1, "");
    ip_node_init(node1, "");
    icmp_node_init(node1);
    rpl_node_init(node1);
    node1->phy_info->tx_power = 0.5;
    node1->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node1->rpl_info->joined_dodag->dodag_id = NULL;
    node1->rpl_info->joined_dodag->parent_list = NULL;
    node1->rpl_info->joined_dodag->sibling_list = NULL;
    node1->rpl_info->joined_dodag->pref_parent = NULL;
    node1->rpl_info->joined_dodag->rank = 3;
    node1->rpl_info->joined_dodag->seq_num = 0;
    node1->alive = TRUE;

    node_t *node2 = node_create();
    measure_node_init(node2);
    phy_node_init(node2, "P", 0, 0);
    mac_node_init(node2, "");
    ip_node_init(node2, "");
    icmp_node_init(node2);
    rpl_node_init(node2);
    node2->phy_info->tx_power = 0.5;
    node2->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node2->rpl_info->joined_dodag->dodag_id = NULL;
    node2->rpl_info->joined_dodag->parent_list = NULL;
    node2->rpl_info->joined_dodag->sibling_list = NULL;
    node2->rpl_info->joined_dodag->pref_parent = NULL;
    node2->rpl_info->joined_dodag->rank = 2;
    node2->rpl_info->joined_dodag->seq_num = 0;
    node2->alive = FALSE;

    sim_field_draw_node(node1, cr, pixel_x1, pixel_y);
    sim_field_draw_node(node2, cr, pixel_x2, pixel_y);

    sim_field_draw_parent_arrow(cr, pixel_x1, pixel_y, pixel_x2, pixel_y, SIM_FIELD_DEAD_ARROW_COLOR, FALSE);

    node1->alive = FALSE;
    node2->alive = FALSE;
    node_destroy(node1);
    node_destroy(node2);

    /* do the actual double-buffered paint */
    cairo_t *window_cr = gdk_cairo_create(window);
    cairo_set_source_surface(window_cr, surface, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

static void draw_legend11(GdkWindow *window, GdkGC *gc)
{
    gint pixel_width, pixel_height;
    gdk_drawable_get_size(window, &pixel_width, &pixel_height);

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pixel_width, pixel_height);
    cairo_t *cr = cairo_create(surface);

    /* background */
    double r, g, b, a;
    EXPAND_COLOR(SIM_FIELD_BG_COLOR, r, g, b, a);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_rectangle(cr, 0, 0, pixel_width, pixel_height);
    cairo_fill(cr);

    uint32 pixel_x1 = pixel_width / 4;
    uint32 pixel_x2 = 3 * pixel_width / 4;
    uint32 pixel_y = pixel_height / 2;

    node_t *node1 = node_create();
    measure_node_init(node1);
    phy_node_init(node1, "S", 0, 0);
    mac_node_init(node1, "");
    ip_node_init(node1, "");
    icmp_node_init(node1);
    rpl_node_init(node1);
    node1->phy_info->tx_power = 0.5;
    node1->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node1->rpl_info->joined_dodag->dodag_id = NULL;
    node1->rpl_info->joined_dodag->parent_list = NULL;
    node1->rpl_info->joined_dodag->sibling_list = NULL;
    node1->rpl_info->joined_dodag->pref_parent = NULL;
    node1->rpl_info->joined_dodag->rank = 3;
    node1->rpl_info->joined_dodag->seq_num = 0;
    node1->alive = TRUE;

    node_t *node2 = node_create();
    measure_node_init(node2);
    phy_node_init(node2, "S", 0, 0);
    mac_node_init(node2, "");
    ip_node_init(node2, "");
    icmp_node_init(node2);
    rpl_node_init(node2);
    node2->phy_info->tx_power = 0.5;
    node2->rpl_info->joined_dodag = malloc(sizeof(rpl_dodag_t));
    node2->rpl_info->joined_dodag->dodag_id = NULL;
    node2->rpl_info->joined_dodag->parent_list = NULL;
    node2->rpl_info->joined_dodag->sibling_list = NULL;
    node2->rpl_info->joined_dodag->pref_parent = NULL;
    node2->rpl_info->joined_dodag->rank = 3;
    node2->rpl_info->joined_dodag->seq_num = 0;
    node2->alive = FALSE;

    sim_field_draw_node(node1, cr, pixel_x1, pixel_y);
    sim_field_draw_node(node2, cr, pixel_x2, pixel_y);

    sim_field_draw_sibling_arrow(cr, pixel_x1, pixel_y, pixel_x2, pixel_y, SIM_FIELD_DEAD_ARROW_COLOR, FALSE, TRUE);

    node1->alive = FALSE;
    node2->alive = FALSE;
    node_destroy(node1);
    node_destroy(node2);

    /* do the actual double-buffered paint */
    cairo_t *window_cr = gdk_cairo_create(window);
    cairo_set_source_surface(window_cr, surface, 0, 0);
    cairo_paint(window_cr);
    cairo_destroy(window_cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

