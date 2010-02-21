
#ifndef IP_H_
#define IP_H_

#include "../base.h"
#include "../node.h"

#define MAC_TYPE_IP                 0x86DD

#define IP_ROUTE_TYPE_CONNECTED     0
#define IP_ROUTE_TYPE_MANUAL        1
#define IP_ROUTE_TYPE_RPL_DAO       2
#define IP_ROUTE_TYPE_RPL_DIO       3


typedef struct ip_neighbor_t {

    node_t *                node;
    sim_time_t              last_packet_time;

} ip_neighbor_t;

typedef struct ip_flow_label_t {

    bool                    going_down;
    bool                    from_sibling;
    bool                    rank_error;
    bool                    forward_error;
    uint16                  sender_rank;

} ip_flow_label_t;

    /* a struct defining a route record */
typedef struct ip_route_t {

    char *                  dst;
    uint8                   prefix_len;
    node_t *                next_hop;

    uint8                   type;
    void *                  further_info;

        /* destination expressed as a bit-array, a performance workaround */
    uint8 *                 dst_bit_expanded;

} ip_route_t;

    /* info that a node supporting IP should store */
typedef struct ip_node_info_t {

    char *                  address;

    ip_route_t **           route_list;
    uint16                  route_count;

    ip_neighbor_t **        neighbor_list;
    uint16                  neighbor_count;

} ip_node_info_t;

    /* fields contained in a IP packet */
typedef struct ip_pdu_t {

    char *                  dst_address;
    char *                  src_address;

    ip_flow_label_t *       flow_label;

    uint16                  next_header;
    void *                  sdu;

} ip_pdu_t;


extern uint16       ip_event_id_after_node_wake;
extern uint16       ip_event_id_before_node_kill;

extern uint16       ip_event_id_after_pdu_sent;
extern uint16       ip_event_id_after_pdu_received;

extern uint16       ip_event_id_after_neighbor_cache_timeout;


bool                ip_init();
bool                ip_done();

ip_pdu_t *          ip_pdu_create(char *dst_address, char *src_address);
void                ip_pdu_destroy(ip_pdu_t *pdu);
ip_pdu_t *          ip_pdu_duplicate(ip_pdu_t *pdu);
bool                ip_pdu_set_sdu(ip_pdu_t *pdu, uint16 next_header, void *sdu);

void                ip_node_init(node_t *node, char *address);
void                ip_node_done(node_t *node);

void                ip_node_set_address(node_t *node, const char *address);

void                ip_node_add_route(node_t *node, char *dst, uint8 prefix_len, node_t *next_hop, uint8 type, void *further_info);
void                ip_node_rem_routes(node_t *node, char *dst, int8 prefix_len, node_t *next_hop, int8 type);
ip_route_t *        ip_node_get_next_hop_route(node_t *node, char *dst_address);

ip_neighbor_t *     ip_node_add_neighbor(node_t *node, node_t *neighbor_node);
bool                ip_node_rem_neighbor(node_t *node, ip_neighbor_t *neighbor);
ip_neighbor_t *     ip_node_find_neighbor_by_node(node_t *node, node_t *neighbor_node);

bool                ip_send(node_t *node, char *dst_ip_address, uint16 next_header, void *sdu);
bool                ip_forward(node_t *node, node_t *incoming_node, ip_pdu_t *pdu);
bool                ip_receive(node_t *node, node_t *incoming_node, ip_pdu_t *pdu);


    /* events */
bool                ip_event_after_node_wake(node_t *node);
bool                ip_event_before_node_kill(node_t *node);

bool                ip_event_after_pdu_sent(node_t *node, node_t *incoming_node, ip_pdu_t *pdu);
bool                ip_event_after_pdu_received(node_t *node, node_t *incoming_node, ip_pdu_t *pdu);

bool                ip_event_after_neighbor_cache_timeout(node_t *node, ip_neighbor_t *neighbor);


#endif /* IP_H_ */
