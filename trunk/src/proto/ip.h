
#ifndef IP_H_
#define IP_H_

#include "../base.h"
#include "../node.h"

#define MAC_TYPE_IP                 0x86DD
#define IP_NEXT_HEADER_ICMP         0x0058


    /* info that a node supporting IP should store */
typedef struct ip_node_info_t {

    char *address;

} ip_node_info_t;

    /* fields contained in a IP packet */
typedef struct {

    char *dst_address;
    char *src_address;

    uint16 next_header;
    void *sdu;

} ip_pdu_t;

    /* fields contained in a ICMP message */
typedef struct {

    uint8 type;
    uint8 code;
    void *sdu;

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

    /* IP events */
void                ip_event_before_pdu_sent(node_t *node, ip_pdu_t *pdu);
void                ip_event_after_pdu_received(node_t *node, ip_pdu_t *pdu);

void                icmp_event_before_pdu_sent(node_t *node, icmp_pdu_t *pdu);
void                icmp_event_after_pdu_received(node_t *node, icmp_pdu_t *pdu);


#endif /* IP_H_ */
