
#ifndef IP_H_
#define IP_H_

#include "../base.h"
#include "../node.h"

#define MAC_TYPE_IP                 0x86DD
#define IP_NEXT_HEADER_ICMP         0x0058

    /* a struct defining a route record */
typedef struct ip_route_t {

    uint8       type;
    char *      dst;
    uint8       prefix_len;
    node_t *    next_hop;

        /* destination expressed as a bit-array, a performance workaround */
    uint8 *     dst_bit_expanded;

} ip_route_t;

    /* info that a node supporting IP should store */
typedef struct ip_node_info_t {

    char *      address;

    ip_route_t **
                route_list;
    uint16      route_count;

    node_t **   neighbor_list;
    uint16      neighbor_count;

} ip_node_info_t;

    /* fields contained in a IP packet */
typedef struct ip_pdu_t {

    char *      dst_address;
    char *      src_address;

    uint16      next_header;
    void *      sdu;

} ip_pdu_t;

    /* fields contained in a ICMP message */
typedef struct icmp_pdu_t {

    uint8       type;
    uint8       code;
    void *      sdu;

} icmp_pdu_t;


ip_pdu_t *          ip_pdu_create(char *dst_address, char *src_address);
bool                ip_pdu_destroy(ip_pdu_t *pdu);
bool                ip_pdu_set_sdu(ip_pdu_t *pdu, uint16 next_header, void *sdu);

bool                ip_node_init(node_t *node, char *address);
void                ip_done_node(node_t *node);

char *              ip_node_get_address(node_t *node);
void                ip_node_set_address(node_t *node, const char *address);

void                ip_node_add_route(node_t *node, uint8 type, char *dst, uint8 prefix_len, node_t *next_hop, bool aggregate);
bool                ip_node_rem_route(node_t *node, char *dst, uint8 prefix_len);
node_t *            ip_node_longest_prefix_match_route(node_t *node, char *dst_address);

node_t **           ip_node_get_neighbor_list(node_t *node, uint16 *neighbor_count);
bool                ip_node_add_neighbor(node_t *node, node_t *neighbor);
bool                ip_node_remove_neighbor(node_t *node, node_t *neighbor);
bool                ip_node_has_neighbor(node_t *node, node_t *neighbor);

bool                ip_send(node_t *node, node_t *dst_node, uint16 next_header, void *sdu);
bool                ip_forward(node_t *node, ip_pdu_t *pdu);
bool                ip_receive(node_t *node, node_t *src_node, ip_pdu_t **pdu);

icmp_pdu_t *        icmp_pdu_create();
bool                icmp_pdu_destroy(icmp_pdu_t *pdu);
bool                icmp_pdu_set_sdu(icmp_pdu_t *pdu, uint8 type, uint8 code, void *sdu);

bool                icmp_send(node_t *node, node_t *dst_node, uint8 type, uint8 code, void *sdu);
bool                icmp_receive(node_t *node, node_t *src_node, icmp_pdu_t *pdu);


#endif /* IP_H_ */
