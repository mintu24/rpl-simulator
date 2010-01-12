
#include "phy.h"
#include "mac.h"


phy_pdu_t *phy_pdu_create()
{
    phy_pdu_t *pdu = (phy_pdu_t *) malloc(sizeof(phy_pdu_t));

    pdu->sdu = NULL;

    return pdu;
}

bool phy_pdu_destroy(phy_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    if (pdu->sdu != NULL) {
        mac_pdu_destroy(pdu->sdu);
    }

    free(pdu);

    return TRUE;
}

bool phy_pdu_set_sdu(phy_pdu_t *pdu, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->sdu = sdu;

    return TRUE;
}

bool phy_init_node(node_t *node, phy_node_info_t *node_info)
{
    rs_assert(node != NULL);
    rs_assert(node_info != NULL);
    rs_assert(node->phy_info == NULL);

    node->phy_info = (phy_node_info_t *) malloc(sizeof(phy_node_info_t));
    memcpy(node->phy_info, node_info, sizeof(phy_node_info_t));

    return TRUE;
}

void phy_event_before_pdu_sent(node_t *node, phy_pdu_t *pdu)
{
    // todo
}

void phy_event_after_pdu_received(node_t *node, phy_pdu_t *pdu)
{
    // todo
}
