
#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "base.h"
#include "node.h"
#include "proto/phy.h"
#include "proto/mac.h"
#include "proto/ip.h"
#include "proto/rpl.h"


typedef struct {

    node_t **node_list;
    uint16 node_count;

    coord_t no_link_dist_thresh;

} rs_system_t;

rs_system_t *               rs_system_create();
bool                        rs_system_destroy(rs_system_t *system);
bool                        rs_system_add_node(rs_system_t *system, node_t *node);
bool                        rs_system_remove_node(rs_system_t *system, node_t *node);
node_t *                    rs_system_find_node_by_name(rs_system_t *system, char *name);
percent_t                   rs_system_get_link_quality(rs_system_t *system, node_t *node1, node_t *node2);
bool                        rs_system_send_message(rs_system_t *system, node_t *src_node, node_t *dst_node, phy_pdu_t *message);


#endif /* SYSTEM_H_ */
