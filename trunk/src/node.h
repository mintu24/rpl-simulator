
#ifndef NODE_H_
#define NODE_H_

#include "base.h"


typedef struct {

    char *name;
    coord_t cx, cy;

    char *mac_address;
    char *ip_address;

} node_t;


node_t *rs_node_create(char *name);


#endif /* NODE_H_ */
