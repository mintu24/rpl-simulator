
#ifndef MAC_H_
#define MAC_H_

#include "../base.h"
#include "../node.h"


    /* info that a node supporting MAC layer should store */
typedef struct mac_node_info_t {

    char *address;

} mac_node_info_t;

    /* fields contained in a MAC frame */
typedef struct mac_pdu_t {

	char *dst_address;
	char *src_address;

	uint16 type;
	void *sdu;

} mac_pdu_t;


mac_pdu_t *             mac_pdu_create(char *dst_address, char *src_address);
bool                    mac_pdu_destroy(mac_pdu_t *pdu);
bool                    mac_pdu_set_sdu(mac_pdu_t *pdu, uint16 type, void *sdu);

mac_node_info_t *       mac_node_info_create(char *address);
bool                    mac_node_info_destroy(mac_node_info_t *node_info);

bool                    mac_init_node(node_t *node, mac_node_info_t *node_info);
void                    mac_done_node(node_t *node);

char *                  mac_node_get_address(node_t *node);
void                    mac_node_set_address(node_t *node, const char *address);

bool                    mac_send(node_t *src_node, node_t *dst_node, uint16 type, void *sdu);
bool                    mac_receive(node_t *dst_node, mac_pdu_t *pdu);


#endif /* MAC_H_ */
