
#ifndef MAIN_H_
#define MAIN_H_

#include "base.h"
#include "node.h"

#define                 AUTOINC_ADDRESS_PART        16

void                    rs_new();
char *                  rs_open();
char *                  rs_save();
void                    rs_quit();

void                    rs_start();
void                    rs_pause();
void                    rs_step();
void                    rs_stop();

node_t *                rs_add_node(coord_t x, coord_t y);
void                    rs_rem_node(node_t *node);
void                    rs_wake_node(node_t *node);
void                    rs_kill_node(node_t *node);

void                    rs_add_more_nodes(uint16 node_number, uint8 pattern, coord_t horiz_dist, coord_t vert_dist, uint16 row_length);
void                    rs_rem_all_nodes();
void                    rs_wake_all_nodes();
void                    rs_kill_all_nodes();

#endif /* MAIN_H_ */
