#ifndef SIMFIELD_H_
#define SIMFIELD_H_

#include <stdio.h>
#include <stdlib.h>

#include <gdk/gdk.h>
#include <cairo.h>

#include "../base.h"

#define NODE_RADIUS             10
#define MIN_NODE_DISTANCE       0


void draw_simfield(GdkDrawable *drawable, GdkGC *gc, coord_t current_x, coord_t current_y, double power);
void snap_node_coords(coord_t *x, coord_t *y);


#endif /* SIMFIELD_H_ */
