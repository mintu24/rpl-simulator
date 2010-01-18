#ifndef SIMFIELD_H_
#define SIMFIELD_H_

#include <stdio.h>
#include <stdlib.h>

#include <gdk/gdk.h>
#include <cairo.h>

#include "../base.h"
#include "../node.h"

#define NODE_RADIUS             10
#define MIN_NODE_DISTANCE       20
#define TX_POWER_STEP_COUNT     11


void    sim_field_init(GdkWindow *window, GdkGC *gc);
void    sim_field_redraw();
void    sim_field_node_coords_assure_non_intersection(node_t *subject_node, coord_t *x, coord_t *y);


#endif /* SIMFIELD_H_ */
