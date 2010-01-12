
#ifndef MAC_H_
#define MAC_H_

#include "../base.h"
#include "../node.h"


    /* info that a node supporting MAC layer should store */
typedef struct mac_node_info_t {

    char *address;

} mac_node_info_t;

    /* fields contained in a MAC frame */
typedef struct {

	char *dst_address;
	char *src_address;

	uint16 type;
	void *sdu;

} mac_pdu_t;


mac_pdu_t *             mac_pdu_create(char *dst_address, char *src_address);
bool                    mac_pdu_destroy(mac_pdu_t *pdu);
bool                    mac_pdu_set_sdu(mac_pdu_t *pdu, uint16 type, void *sdu);

bool                    mac_init_node(node_t *node, mac_node_info_t *node_info);

    /* MAC events */
void                    mac_event_before_pdu_sent(node_t *node, mac_pdu_t *pdu);
void                    mac_event_after_pdu_received(node_t *node, mac_pdu_t *pdu);


#endif /* MAC_H_ */
