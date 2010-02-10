
#ifndef ICMP_H_
#define ICMP_H_

#include "../base.h"
#include "../node.h"

#define IP_NEXT_HEADER_ICMP         0x0058


    /* info that a node supporting ICMP should store */
typedef struct icmp_node_info_t {

} icmp_node_info_t;

    /* fields contained in a ICMP message */
typedef struct icmp_pdu_t {

    uint8                   type;
    uint8                   code;
    void *                  sdu;

} icmp_pdu_t;


extern uint16       icmp_event_id_after_node_wake;
extern uint16       icmp_event_id_before_node_kill;
extern uint16       icmp_event_id_after_pdu_sent;
extern uint16       icmp_event_id_after_pdu_received;


bool                icmp_init();
bool                icmp_done();

icmp_pdu_t *        icmp_pdu_create();
void                icmp_pdu_destroy(icmp_pdu_t *pdu);
icmp_pdu_t *        icmp_pdu_duplicate(icmp_pdu_t *pdu);
bool                icmp_pdu_set_sdu(icmp_pdu_t *pdu, uint8 type, uint8 code, void *sdu);

void                icmp_node_init(node_t *node);
void                icmp_node_done(node_t *node);

bool                icmp_send(node_t *node, node_t *dst_node, uint8 type, uint8 code, void *sdu);
bool                icmp_receive(node_t *node, node_t *src_node, icmp_pdu_t *pdu);

    /* events */
bool                icmp_event_after_node_wake(node_t *node);
bool                icmp_event_before_node_kill(node_t *node);

bool                icmp_event_after_pdu_sent(node_t *node, node_t *dst_node, icmp_pdu_t *pdu);
bool                icmp_event_after_pdu_received(node_t *node, node_t *src_node, icmp_pdu_t *pdu);


#endif /* ICMP_H_ */
