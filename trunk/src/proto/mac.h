
#ifndef MAC_H_
#define MAC_H_

#include "../base.h"
#include "../node.h"

#define mac_node_lock(node)        proto_node_lock("MAC", &(node)->mac_info->mutex)
#define mac_node_unlock(node)      proto_node_unlock("MAC", &(node)->mac_info->mutex)


    /* info that a node supporting MAC layer should store */
typedef struct mac_node_info_t {

    char *          address;

    GStaticRecMutex mutex;

} mac_node_info_t;

    /* fields contained in a MAC frame */
typedef struct mac_pdu_t {

	char *         dst_address;
	char *         src_address;

	uint16         type;
	void *         sdu;

} mac_pdu_t;


mac_pdu_t *             mac_pdu_create(char *dst_address, char *src_address);
bool                    mac_pdu_destroy(mac_pdu_t *pdu);
mac_pdu_t *             mac_pdu_duplicate(mac_pdu_t *pdu);
bool                    mac_pdu_set_sdu(mac_pdu_t *pdu, uint16 type, void *sdu);

bool                    mac_node_init(node_t *node, char *address);
void                    mac_node_done(node_t *node);

char *                  mac_node_get_address(node_t *node);
void                    mac_node_set_address(node_t *node, const char *address);

bool                    mac_send(node_t *node, node_t *dst_node, uint16 type, void *sdu);
bool                    mac_receive(node_t *node, node_t *src_node, mac_pdu_t *pdu);

    /* events */
void                    mac_event_after_node_wake(node_t *node);
void                    mac_event_before_node_kill(node_t *node);

void                    mac_event_before_pdu_sent(node_t *node, node_t *dst_node, mac_pdu_t *pdu);
void                    mac_event_after_pdu_received(node_t *node, node_t *src_node, mac_pdu_t *pdu);


#endif /* MAC_H_ */
