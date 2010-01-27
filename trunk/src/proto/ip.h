
#ifndef IP_H_
#define IP_H_

#include "../base.h"
#include "../node.h"

#define MAC_TYPE_IP                 0x86DD

#define IP_ROUTE_TYPE_MANUAL        0
#define IP_ROUTE_TYPE_RPL_DAO_MCAS  1
#define IP_ROUTE_TYPE_RPL_DAO_UCAST 2
#define IP_ROUTE_TYPE_RPL_DIO       3

#define ip_node_lock(node)          g_static_rec_mutex_lock(&node->ip_info->mutex)
#define ip_node_unlock(node)        g_static_rec_mutex_unlock(&node->ip_info->mutex)


    /* a struct defining a route record */
typedef struct ip_route_t {

    uint8                   type;
    char *                  dst;
    uint8                   prefix_len;
    node_t *                next_hop;

        /* destination expressed as a bit-array, a performance workaround */
    uint8 *                 dst_bit_expanded;

} ip_route_t;

    /* info that a node supporting IP should store */
typedef struct ip_node_info_t {

    char *                  address;

    ip_route_t **
                            route_list;
    uint16                  route_count;

    GStaticRecMutex         mutex;

} ip_node_info_t;

    /* fields contained in a IP packet */
typedef struct ip_pdu_t {

    char *                  dst_address;
    char *                  src_address;

    uint16                  next_header;
    void *                  sdu;

} ip_pdu_t;


ip_pdu_t *          ip_pdu_create(char *dst_address, char *src_address);
void                ip_pdu_destroy(ip_pdu_t *pdu);
bool                ip_pdu_set_sdu(ip_pdu_t *pdu, uint16 next_header, void *sdu);

void                ip_node_init(node_t *node, char *address);
void                ip_node_done(node_t *node);

char *              ip_node_get_address(node_t *node);
void                ip_node_set_address(node_t *node, const char *address);

void                ip_node_add_route(node_t *node, uint8 type, char *dst, uint8 prefix_len, node_t *next_hop, bool aggregate);
bool                ip_node_rem_route(node_t *node, char *dst, uint8 prefix_len);
ip_route_t **       ip_node_get_route_list(node_t *node, uint16 *route_count);
node_t *            ip_node_best_match_route(node_t *node, char *dst_address);

bool                ip_send(node_t *node, node_t *dst_node, uint16 next_header, void *sdu);
bool                ip_forward(node_t *node, ip_pdu_t *pdu);
bool                ip_receive(node_t *node, node_t *src_node, ip_pdu_t *pdu);


    /* events */
void                ip_event_after_node_wake(node_t *node);
void                ip_event_before_node_kill(node_t *node);

void                ip_event_before_pdu_sent(node_t *node, node_t *dst_node, ip_pdu_t *pdu);
void                ip_event_after_pdu_received(node_t *node, node_t *src_node, ip_pdu_t *pdu);


#endif /* IP_H_ */
