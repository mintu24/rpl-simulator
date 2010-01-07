
#ifndef IP_H_
#define IP_H_

#include "../base.h"
#include "../node.h"

#define MAC_TYPE_IP                 0x86DD
#define IP_NEXT_HEADER_ICMP         0x0058


typedef struct {

    char *dst_address;
    char *src_address;

    uint16 next_header;
    void *sdu;

} ip_pdu_t;

typedef struct {

    uint8 type;
    uint8 code;
    void *sdu;

} icmp_pdu_t;


ip_pdu_t *          ip_pdu_create(char *dst_address, char *src_address, void *sdu);
bool                ip_pdu_destroy(ip_pdu_t *pdu);
bool                ip_pdu_set_sdu(ip_pdu_t *pdu, uint16 next_header, void *sdu);

icmp_pdu_t *        icmp_pdu_create();
bool                icmp_pdu_destroy(icmp_pdu_t *pdu);
bool                icmp_pdu_set_sdu(icmp_pdu_t *pdu, uint8 type, uint8 code, void *sdu);

bool                ip_init_node(node_t *node, char *ip_address);


#endif /* IP_H_ */
