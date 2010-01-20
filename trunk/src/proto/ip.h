
#ifndef IP_H_
#define IP_H_

#include "../base.h"
#include "../node.h"

#define MAC_TYPE_IP                 0x86DD
#define IP_NEXT_HEADER_ICMP         0x0058


    /* info that a node supporting IP should store */
typedef struct ip_node_info_t {

    char *      address;

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

icmp_pdu_t *        icmp_pdu_create();
bool                icmp_pdu_destroy(icmp_pdu_t *pdu);
bool                icmp_pdu_set_sdu(icmp_pdu_t *pdu, uint8 type, uint8 code, void *sdu);

ip_node_info_t *    ip_node_info_create(char *address);
bool                ip_node_info_destroy(ip_node_info_t *node_info);

bool                ip_init_node(node_t *node, ip_node_info_t *node_info);

char *              ip_node_get_address(node_t *node);
void                ip_node_set_address(node_t *node, const char *address);

node_t **           ip_node_get_neighbor_list(node_t *node, uint16 *neighbor_count);
bool                ip_node_add_neighbor(node_t *node, node_t *neighbor);
bool                ip_node_remove_neighbor(node_t *node, node_t *neighbor);
bool                ip_node_has_neighbor(node_t *node, node_t *neighbor);

bool                ip_send(node_t *src_node, node_t *dst_node, void *sdu);
bool                icmp_send(node_t *src_node, node_t *dst_node, uint8 type, uint8 code, void *sdu);

    /* IP events */
void                ip_event_before_pdu_sent(node_t *node, ip_pdu_t *pdu);
void                ip_event_after_pdu_received(node_t *node, ip_pdu_t *pdu);

void                icmp_event_before_pdu_sent(node_t *node, icmp_pdu_t *pdu);
void                icmp_event_after_pdu_received(node_t *node, icmp_pdu_t *pdu);


#endif /* IP_H_ */