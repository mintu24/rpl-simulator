
#ifndef NODE_H_
#define NODE_H_

#include "base.h"


    /* a node in the simulated network */
typedef struct node_t {

    struct phy_node_info_t *    phy_info;
    struct mac_node_info_t *    mac_info;
    struct ip_node_info_t *     ip_info;
    struct icmp_node_info_t *   icmp_info;
    struct rpl_node_info_t *    rpl_info;

    bool                        alive;

} node_t;


node_t *                    node_create();
bool                        node_destroy(node_t* node);

bool                        node_wake(node_t* node);
bool                        node_kill(node_t* node);


#endif /* NODE_H_ */
