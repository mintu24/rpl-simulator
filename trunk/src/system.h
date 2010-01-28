
#ifndef SYSTEM_H_
#define SYSTEM_H_

#include "base.h"
#include "node.h"

#include "proto/phy.h"
#include "proto/mac.h"
#include "proto/ip.h"
#include "proto/icmp.h"
#include "proto/rpl.h"

#define DEFAULT_NO_LINK_DIST_THRESH     50
#define DEFAULT_NO_LINK_QUALITY_THRESH  0.2
#define DEFAULT_PHY_TRANSMIT_MODE       PHY_TRANSMIT_MODE_QUEUE
#define DEFAULT_SYS_WIDTH               100
#define DEFAULT_SYS_HEIGHT              100
#define DEFAULT_NODE_CORE_SLEEP         10000

#define DEFAULT_NODE_NAME               "A"
#define DEFAULT_NODE_MAC_ADDRESS        "0001"
#define DEFAULT_NODE_IP_ADDRESS         "AA01"

#define GARBAGE_COLLECTOR_INTERVAL      2000000

#define system_lock()                   g_static_rec_mutex_lock(&rs_system->mutex)
#define system_unlock()                 g_static_rec_mutex_unlock(&rs_system->mutex)


/*
#define system_lock()                        \
    { printf("locking SYSTEM mutex\n");                 \
    g_static_rec_mutex_lock(&rs_system->mutex);                \
    printf("SYSTEM mutex locked (%d)\n", rs_system->mutex.depth); }

#define system_unlock()                      \
    { g_static_rec_mutex_unlock(&rs_system->mutex);            \
    printf("SYSTEM mutex unlocked (%d)\n", rs_system->mutex.depth); }
*/



#define rs_system_has_node(node)        (rs_system_get_node_pos(node) >= 0)


typedef struct rs_system_t {

    /* params */
    coord_t             no_link_dist_thresh;
    percent_t           no_link_quality_thresh;
    uint8               phy_transmit_mode;

    coord_t             width;
    coord_t             height;

    uint32              node_core_sleep;

    /* nodes */
    node_t **           node_list;
    uint16              node_count;

    /* garbage collector stuff */
    GThread *           gc_thread;
    GMutex *            gc_mutex;
    bool                gc_running;
    node_t **           gc_list;
    uint16              gc_count;

    GThread *           main_thread;
    GStaticRecMutex     mutex;

} rs_system_t;

extern rs_system_t *    rs_system;


bool                        rs_system_create();
bool                        rs_system_destroy();

    /* params */
coord_t                     rs_system_get_no_link_dist_thresh();
void                        rs_system_set_no_link_dist_thresh(coord_t thresh);

percent_t                   rs_system_get_no_link_quality_thresh();
void                        rs_system_set_no_link_quality_thresh(percent_t thresh);

uint8                       rs_system_get_transmit_mode();
void                        rs_system_set_transmit_mode(uint8 mode);

coord_t                     rs_system_get_width();
coord_t                     rs_system_get_height();
void                        rs_system_set_width_height(coord_t width, coord_t height);

uint32                      rs_system_get_node_core_sleep();
void                        rs_system_set_node_core_sleep(uint32 node_core_sleep);


    /* nodes */
bool                        rs_system_add_node(node_t *node);
bool                        rs_system_remove_node(node_t *node);
int32                       rs_system_get_node_pos(node_t *node);
node_t *                    rs_system_find_node_by_name(char *name);
node_t *                    rs_system_find_node_by_mac_address(char *address);
node_t *                    rs_system_find_node_by_ip_address(char *address);
node_t **                   rs_system_get_node_list(uint16 *node_count);
node_t **                   rs_system_get_node_list_copy(uint16 *node_count);

percent_t                   rs_system_get_link_quality(node_t *src_node, node_t *dst_node);


#endif /* SYSTEM_H_ */
