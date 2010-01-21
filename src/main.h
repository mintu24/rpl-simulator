
#ifndef MAIN_H_
#define MAIN_H_

#include "base.h"
#include "node.h"

#define                 AUTOINC_ADDRESS_PART        16


extern                  GThread *rs_main_thread;


void                    rs_open();
void                    rs_save();
void                    rs_quit();

void                    rs_start();
void                    rs_stop();

node_t *                rs_add_node();
void                    rs_rem_node(node_t *node);
void                    rs_wake_node(node_t *node);
void                    rs_kill_node(node_t *node);

void                    rs_add_more_nodes();
void                    rs_rem_all_nodes();
void                    rs_wake_all_nodes();
void                    rs_kill_all_nodes();

#endif /* MAIN_H_ */
