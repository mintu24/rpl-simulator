#ifndef SIMFIELD_H_
#define SIMFIELD_H_

#include <stdio.h>
#include <stdlib.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>

#include "../base.h"
#include "../node.h"

#define TX_POWER_STEP_COUNT             11
#define NODE_RADIUS                     10
#define TEXT_FONT                       "Verdana"
#define TEXT_SIZE                       12.0
#define TEXT_NORMAL_BG_COLOR            0xFF404040
#define TEXT_HOVER_BG_COLOR             0xFF808080
#define TEXT_SELECTED_BG_COLOR          0xFF255FB4
#define TEXT_HOVER_SELECTED_BG_COLOR    0xFF5F8FF4
#define TEXT_NAME_FG_COLOR              0xFFFFFFFF
#define TEXT_ADDRESS_FG_COLOR           0xFFFFFF00


GtkWidget *     sim_field_create();
void            sim_field_redraw();
//void            sim_field_node_coords_assure_non_intersection(node_t *subject_node, coord_t *x, coord_t *y);


#endif /* SIMFIELD_H_ */
