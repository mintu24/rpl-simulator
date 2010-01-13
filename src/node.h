
#ifndef NODE_H_
#define NODE_H_

#include <glib.h>

#include "base.h"

#define NODE_LIFE_CORE_SLEEP            10

#define PHY_TRANSMIT_MODE_BLOCK         0
#define PHY_TRANSMIT_MODE_REJECT        1
#define PHY_TRANSMIT_MODE_QUEUE         2


    /* a node in the simulated network */
typedef struct {

    char *name;
    coord_t cx, cy;

    struct phy_node_info_t *phy_info;
    struct mac_node_info_t *mac_info;
    struct ip_node_info_t *ip_info;
    struct rpl_node_info_t *rpl_info;

    GThread *life;
    bool alive;

    GHashTable *schedules;
    GTimer *schedule_timer;

    GMutex *life_mutex;
    GMutex *schedule_mutex;
    GMutex *pdu_mutex;

    GQueue *pdu_queue;
    GCond *pdu_cond;

} node_t;

    /* a callback type representing a node's scheduled action */
typedef void (* node_schedule_func_t) (node_t *node, void *data);

    /* structure used for scheduling actions to be executed at a certain moment */
typedef struct {

    char *name;
    node_schedule_func_t func;
    void *data;
    uint32 usecs;
    uint32 remaining_usecs;
    bool recurrent;

} node_schedule_t;


node_t *                    node_create(char *name, coord_t cx, coord_t cy);
bool                        node_destroy(node_t* node);

bool                        node_start(node_t* node);
bool                        node_kill(node_t* node);

bool                        node_schedule(node_t *node, char *name, node_schedule_func_t func, void *data, uint32 usecs, bool recurrent);
bool                        node_execute(node_t *node, char *name, node_schedule_func_t func, void *data, bool blocking);

bool                        node_receive_pdu(node_t *node, void *pdu, uint8 phy_transmit_mode);
void *                      node_process_pdu(node_t *node);


#endif /* NODE_H_ */
