#ifndef SIMFIELD_H_
#define SIMFIELD_H_

#include <stdio.h>
#include <stdlib.h>

#include <gdk/gdk.h>
#include <cairo.h>

#include "../base.h"

#define NODE_RADIUS             10
#define MIN_NODE_DISTANCE       0


void    sim_field_init(GdkWindow *window, GdkGC *gc);
void    sim_field_redraw();
void    sim_field_node_coords_assure_non_intersection(coord_t *x, coord_t *y);


#endif /* SIMFIELD_H_ */
