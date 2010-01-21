
#ifndef NODE_H_
#define NODE_H_

#include <glib.h>

#include "base.h"

#define NODE_LIFE_CORE_SLEEP            10000

#define PHY_TRANSMIT_MODE_BLOCK         0
#define PHY_TRANSMIT_MODE_REJECT        1
#define PHY_TRANSMIT_MODE_QUEUE         2


    /* a node in the simulated network */
typedef struct node_t {

    struct phy_node_info_t *phy_info;
    struct mac_node_info_t *mac_info;
    struct ip_node_info_t * ip_info;
    struct rpl_node_info_t *rpl_info;

    GQueue *                pdu_queue;

    GThread *               life;
    bool                    alive;

    GHashTable *            schedules;
    GTimer *                schedule_timer;

    GMutex *                life_mutex;
    GMutex *                schedule_mutex;
    GMutex *                proto_info_mutex;
    GMutex *                pdu_mutex;

    GCond *                 pdu_cond;

} node_t;

    /* a callback type representing a node's scheduled action */
typedef void (* node_schedule_func_t) (node_t *node, void *data);
typedef void (* node_event_t) (node_t *node, void *data);
typedef void (* node_event_src_dst_t) (node_t *src_node, node_t *dst_node, void *data);

    /* structure used for scheduling actions to be executed at a certain moment */
typedef struct node_schedule_t {

    char *                  name;
    node_schedule_func_t    func;
    void *                  data;
    uint32                  usecs;
    uint32                  remaining_usecs;
    bool                    recurrent;

} node_schedule_t;


node_t *                    node_create();
bool                        node_destroy(node_t* node);

bool                        node_wake(node_t* node, bool wait);
bool                        node_kill(node_t* node);

bool                        node_schedule(node_t *node, char *name, node_schedule_func_t func, void *data, uint32 usecs, bool recurrent);
bool                        node_execute(node_t *node, char *name, node_schedule_func_t func, void *data, bool blocking);
void                        node_execute_src_dst(node_t *node, char *name, node_event_src_dst_t func, node_t *src_node, node_t *dst_node, void *data, bool blocking);

bool                        node_enqueue_pdu(node_t *node, void *pdu, uint8 phy_transmit_mode);


#endif /* NODE_H_ */
