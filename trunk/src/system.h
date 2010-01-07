
#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "base.h"
#include "node.h"
#include "proto/mac.h"


typedef struct {

    node_t **node_list;
    uint16 node_count;

} rs_system_t;

rs_system_t *               rs_system_create();
bool                        rs_system_destroy(rs_system_t *system);
bool                        rs_system_add_node(rs_system_t *system, node_t *node);
bool                        rs_system_remove_node(rs_system_t *system, node_t *node);
node_t *                    rs_system_find_node_by_name(rs_system_t *system, char *name);
double                      rs_system_get_link_quality(rs_system_t *system, node_t *node1, node_t *node2);
bool                        rs_system_send_frame(rs_system_t *system, node_t *src_node, node_t *dst_node, mac_pdu_t *frame);


#endif /* SYSTEM_H_ */
