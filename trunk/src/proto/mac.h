
#ifndef MAC_H_
#define MAC_H_

#include "../base.h"
#include "../node.h"


typedef struct mac_node_info_t {

    char *address;

} mac_node_info_t;

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


#endif /* MAC_H_ */
