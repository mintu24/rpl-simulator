
#ifndef MAC_H_
#define MAC_H_

#include "../base.h"
#include "../node.h"


    /* info that a node supporting MAC layer should store */
typedef struct mac_node_info_t {

    char *              address;
    bool                busy;
    bool                error;

} mac_node_info_t;

    /* fields contained in a MAC frame */
typedef struct mac_pdu_t {

	char *             dst_address;
	char *             src_address;

	uint16             type;
	void *             sdu;

} mac_pdu_t;


extern uint16           mac_event_node_wake;
extern uint16           mac_event_node_kill;

extern uint16           mac_event_pdu_send;
extern uint16           mac_event_pdu_send_timeout_check;
extern uint16           mac_event_pdu_receive;


bool                    mac_init();
bool                    mac_done();

mac_pdu_t *             mac_pdu_create(char *src_address, char *dst_address);
void                    mac_pdu_destroy(mac_pdu_t *pdu);
mac_pdu_t *             mac_pdu_duplicate(mac_pdu_t *pdu);
void                    mac_pdu_set_sdu(mac_pdu_t *pdu, uint16 type, void *sdu);

void                    mac_node_init(node_t *node, char *address);
void                    mac_node_done(node_t *node);

void                    mac_node_set_address(node_t *node, const char *address);

bool                    mac_node_send(node_t *node, node_t *outgoing_node, uint16 type, void *sdu);
bool                    mac_node_receive(node_t *node, node_t *incoming_node, mac_pdu_t *pdu);


#endif /* MAC_H_ */
