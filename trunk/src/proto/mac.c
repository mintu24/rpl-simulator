
#include "mac.h"
#include "ip.h"


mac_pdu_t *mac_pdu_create(char *dst_address, char *src_address)
{
    mac_pdu_t *pdu = (mac_pdu_t *) malloc(sizeof(mac_pdu_t));

    rs_assert(dst_address != NULL);
    rs_assert(src_address != NULL);

    pdu->dst_address = strdup(dst_address);
    pdu->src_address = strdup(src_address);

    pdu->type = -1;
    pdu->sdu = NULL;

    return pdu;
}

bool mac_pdu_destroy(mac_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    if (pdu->dst_address != NULL)
        free(pdu->dst_address);

    if (pdu->src_address != NULL)
        free(pdu->src_address);

    if (pdu->sdu != NULL) {
        switch (pdu->type) {
            case MAC_TYPE_IP:
                ip_pdu_destroy(pdu->sdu);
                break;
        }
    }

    free(pdu);

    return TRUE;
}

bool mac_pdu_set_sdu(mac_pdu_t *pdu, uint16 type, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->type = type;
    pdu->sdu = sdu;

    return TRUE;
}

bool mac_init_node(node_t *node, mac_node_info_t *node_info)
{
    rs_assert(node!= NULL);
    rs_assert(node_info != NULL);
    rs_assert(node->mac_info == NULL);

    node->mac_info = (mac_node_info_t *) malloc(sizeof(mac_node_info_t));
    memcpy(node->mac_info, node_info, sizeof(mac_node_info_t));

    return TRUE;
}
