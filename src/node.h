
#ifndef NODE_H_
#define NODE_H_

#include "base.h"


typedef struct {

    char *name;
    coord_t cx, cy;

    struct phy_node_info_t *phy_info;
    struct mac_node_info_t *mac_info;
    struct ip_node_info_t *ip_info;
    struct rpl_node_info_t *rpl_info;

} node_t;


node_t                  *rs_node_create(char *name);


#endif /* NODE_H_ */
