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
#define SIM_FIELD_TEXT_BG_COLOR                 0xFF303030
#define SIM_FIELD_TEXT_NAME_FG_COLOR            0xFFFFFF00
#define SIM_FIELD_TEXT_RANK_FG_COLOR            0xFFFFFF00
#define SIM_FIELD_TEXT_ADDRESS_FG_COLOR         0xFFFFFF00

#define SIM_FIELD_PARENT_ARROW_COLOR            0xFF72ADFF
#define SIM_FIELD_SIBLING_ARROW_COLOR           0xFF72ADFF
#define SIM_FIELD_DEAD_ARROW_COLOR              0xFF808080

#define SIM_FIELD_HOVER_COLOR                   0xFFFFFFFF
#define SIM_FIELD_SELECTED_COLOR                0xFFFFFFFF

GtkWidget *     sim_field_create();
void            sim_field_redraw();


#endif /* SIMFIELD_H_ */
