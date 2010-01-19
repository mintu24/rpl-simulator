#ifndef SIMFIELD_H_
#define SIMFIELD_H_

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

#include "../base.h"
#include "../node.h"

#define TX_POWER_STEP_COUNT     11
#define NODE_OVER_THRESH        20


GtkWidget *     sim_field_create();
void            sim_field_redraw();
//void            sim_field_node_coords_assure_non_intersection(node_t *subject_node, coord_t *x, coord_t *y);


#endif /* SIMFIELD_H_ */
