
#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <glib.h>   /* for GThread */

#include "base.h"
#include "node.h"
#include "event.h"
#include "measure.h"

#include "proto/phy.h"
#include "proto/mac.h"
#include "proto/ip.h"
#include "proto/icmp.h"
#include "proto/rpl.h"

#define DEFAULT_NO_LINK_DIST_THRESH             30
#define DEFAULT_NO_LINK_QUALITY_THRESH          0.2
#define DEFAULT_SYS_WIDTH                       100
#define DEFAULT_SYS_HEIGHT                      100
#define DEFAULT_SIMULATION_SECOND               1000

#define DEFAULT_TRANSMISSION_TIME               20
#define DEFAULT_NEIGHBOR_TIMEOUT                2000

#define DEFAULT_NODE_NAME                       "A"
#define DEFAULT_NODE_MAC_ADDRESS                "0001"
#define DEFAULT_NODE_IP_ADDRESS                 "AA01"

#define DEFAULT_RPL_AUTO_SN_INC_INT             -1
#define DEFAULT_RPL_STARTUP_SILENT              FALSE
#define DEFAULT_RPL_POISON_COUNT                4

#define DEFAULT_RPL_DAO_SUPPORTED               FALSE
#define DEFAULT_RPL_DAO_TRIGGER                 FALSE

#define DEFAULT_RPL_DIO_INTERVAL_DOUBLINGS      9 /* 9 times */
#define DEFAULT_RPL_DIO_INTERVAL_MIN            2 /* 2^2 = 4ms */
#define DEFAULT_RPL_DIO_REDUNDANCY_CONSTANT     0xFF /* mechanism disabled */

#define DEFAULT_RPL_MAX_RANK_INC                4
#define DEFAULT_RPL_MIN_HOP_RANK_INC            1

#define DEFAULT_RPL_PREFER_FLOATING             FALSE

#define RANDOM_SEED_Z                           1234
#define RANDOM_SEED_W                           6789


#define SYS_CORE_SLEEP                  100

#define events_lock() { \
        rs_debug(DEBUG_EVENTS_MUTEX, "EVENTS(%d) mutex: locking", rs_system->events_mutex.depth); \
        g_static_rec_mutex_lock(&rs_system->events_mutex); \
        rs_debug(DEBUG_EVENTS_MUTEX, "EVENTS(%d) mutex: locked", rs_system->events_mutex.depth); \
}

#define events_unlock() { \
        if (rs_system->events_mutex.depth == 1) rs_system->events_mutex.depth--; \
        g_static_rec_mutex_unlock(&rs_system->events_mutex); \
        rs_debug(DEBUG_EVENTS_MUTEX, "EVENTS(%d) mutex: unlocked", rs_system->events_mutex.depth); \
}

#define schedules_lock() { \
        rs_debug(DEBUG_SCHEDULES_MUTEX, "SCHEDULES(%d) mutex: locking", rs_system->schedules_mutex.depth); \
        g_static_rec_mutex_lock(&rs_system->schedules_mutex); \
        rs_debug(DEBUG_SCHEDULES_MUTEX, "SCHEDULES(%d) mutex: locked", rs_system->schedules_mutex.depth); \
}

#define schedules_unlock() { \
        if (rs_system->schedules_mutex.depth == 1) rs_system->schedules_mutex.depth--; \
        g_static_rec_mutex_unlock(&rs_system->schedules_mutex); \
        rs_debug(DEBUG_SCHEDULES_MUTEX, "SCHEDULES(%d) mutex: unlocked", rs_system->schedules_mutex.depth); \
}

#define nodes_lock() { \
        rs_debug(DEBUG_NODES_MUTEX, "NODES(%d) mutex: locking", rs_system->nodes_mutex.depth); \
        g_static_rec_mutex_lock(&rs_system->nodes_mutex); \
        rs_debug(DEBUG_NODES_MUTEX, "NODES(%d) mutex: locked", rs_system->nodes_mutex.depth); \
}

#define nodes_unlock() { \
        if (rs_system->nodes_mutex.depth == 1) rs_system->nodes_mutex.depth--; \
        g_static_rec_mutex_unlock(&rs_system->nodes_mutex); \
        rs_debug(DEBUG_NODES_MUTEX, "NODES(%d) mutex: unlocked", rs_system->nodes_mutex.depth); \
}

#define measures_lock() { \
        rs_debug(DEBUG_MEASURES_MUTEX, "MEASURES(%d) mutex: locking", rs_system->measures_mutex.depth); \
        g_static_rec_mutex_lock(&rs_system->measures_mutex); \
        rs_debug(DEBUG_MEASURES_MUTEX, "MEASURES(%d) mutex: locked", rs_system->measures_mutex.depth); \
}

#define measures_unlock() { \
        if (rs_system->measures_mutex.depth == 1) rs_system->measures_mutex.depth--; \
        g_static_rec_mutex_unlock(&rs_system->measures_mutex); \
        rs_debug(DEBUG_MEASURES_MUTEX, "MEASURES(%d) mutex: unlocked", rs_system->measures_mutex.depth); \
}



#define rs_system_has_node(node)        (rs_system_get_node_pos(node) >= 0)


    /* structure used for scheduling events to be executed at a certain moment */
typedef struct event_schedule_t {

    node_t *                    node;
    uint16                      event_id;
    void *                      data1;
    void *                      data2;
    sim_time_t                  time;

    struct event_schedule_t *   next;

} event_schedule_t;

typedef struct rs_system_t {

    /* params */
    coord_t                     no_link_dist_thresh;
    percent_t                   no_link_quality_thresh;
    sim_time_t                  transmission_time;
    sim_time_t                  neighbor_timeout;

    coord_t                     width;
    coord_t                     height;

    int32                       simulation_second;

    bool                        auto_wake_nodes;

    int32                       rpl_auto_sn_inc_interval;
    bool                        rpl_start_silent;
    uint8                       rpl_poison_count;

    bool                        rpl_dao_supported;
    bool                        rpl_dao_trigger;
    uint8                       rpl_dio_interval_doublings;
    uint8                       rpl_dio_interval_min;
    uint8                       rpl_dio_redundancy_constant;
    uint8                       rpl_max_inc_rank;
    // uint8                       rpl_min_hop_rank_inc;

    bool                        rpl_prefer_floating;

    /* nodes */
    node_t **                   node_list;
    uint16                      node_count;

    sim_time_t                  now;
    uint32                      event_count;

    GThread *                   sys_thread;
    bool                        started;
    bool                        paused;
    bool                        step;

    uint32                      random_z;
    uint32                      random_w;

    GStaticRecMutex             events_mutex;
    GStaticRecMutex             schedules_mutex;
    GStaticRecMutex             nodes_mutex;
    GStaticRecMutex             measures_mutex;

    event_schedule_t *          schedules;
    uint32                      schedule_count; /* redundant size counter */

} rs_system_t;


extern rs_system_t *    rs_system;

extern uint16           sys_event_id_after_node_wake;
extern uint16           sys_event_id_before_node_kill;
extern uint16           sys_event_id_after_message_transmitted;


bool                    rs_system_create();
bool                    rs_system_destroy();

bool                    rs_system_add_node(node_t *node);
bool                    rs_system_remove_node(node_t *node);
int32                   rs_system_get_node_pos(node_t *node);
node_t *                rs_system_find_node_by_name(char *name);
node_t *                rs_system_find_node_by_mac_address(char *address);
node_t *                rs_system_find_node_by_ip_address(char *address);
node_t **               rs_system_get_node_list_copy(uint16 *node_count);

void                    rs_system_schedule_event(node_t *node, uint16 event_id, void *data1, void *data2, sim_time_t time);
void                    rs_system_cancel_event(node_t *node, int32 event_id, void *data1, void *data2, int32 time);
bool                    rs_system_send(node_t *src_node, node_t* dst_node, phy_pdu_t *message);
percent_t               rs_system_get_link_quality(node_t *src_node, node_t *dst_node);

void                    rs_system_start(bool start_paused);
void                    rs_system_stop();
void                    rs_system_pause();
void                    rs_system_step();

char *                  rs_system_sim_time_to_string(sim_time_t time);
uint32                  rs_system_random();

    /* events */
bool                    sys_event_after_node_wake(node_t *node);
bool                    sys_event_before_node_kill(node_t *node);

bool                    sys_event_after_message_transmitted(node_t *src_node, node_t *dst_node, phy_pdu_t *message);


#endif /* SYSTEM_H_ */
