
#include "mac.h"
#include "ip.h"


mac_pdu_t *mac_pdu_create(char *dst_address, char *src_address)
{
    mac_pdu_t *pdu = (mac_pdu_t *) malloc(sizeof(mac_pdu_t));

    if (dst_address == NULL || src_address == NULL) {
        rs_error("invalid argument");
        return NULL;
    }

    pdu->dst_address = strdup(dst_address);
    pdu->src_address = strdup(src_address);

    pdu->type = -1;
    pdu->sdu = NULL;

    return pdu;
}

bool mac_pdu_destroy(mac_pdu_t *pdu)
{
    if (pdu == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

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
    if (pdu == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    pdu->type = type;
    pdu->sdu = sdu;

    return TRUE;
}

bool mac_init_node(node_t *node, char *mac_address)
{
    if (node == NULL || mac_address == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    node->mac_address = strdup(mac_address);

    return TRUE;
}
