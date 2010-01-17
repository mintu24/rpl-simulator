
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

phy_node_info_t *phy_node_info_create(char *name, coord_t cx, coord_t cy)
{
    rs_assert(name != NULL);

    phy_node_info_t *node_info = (phy_node_info_t *) malloc(sizeof(phy_node_info_t));

    node_info->name = strdup(name);
    node_info->cx = cx;
    node_info->cy = cy;

    node_info->mains_powered = FALSE;
    node_info->tx_power = 0;
    node_info->battery_level = 0;

    return node_info;
}

bool phy_node_info_destroy(phy_node_info_t *node_info)
{
    rs_assert(node_info != NULL);

    if (node_info->name != NULL)
        free(node_info->name);

    free(node_info);

    return TRUE;
}

bool phy_init_node(node_t *node, phy_node_info_t *node_info)
{
    rs_assert(node != NULL);
    rs_assert(node_info != NULL);

    node->phy_info = node_info;

    return TRUE;
}

void phy_event_before_pdu_sent(node_t *node, phy_pdu_t *pdu)
{
    rs_debug(NULL);
}

void phy_event_after_pdu_received(node_t *node, phy_pdu_t *pdu)
{
    rs_debug(NULL);
}
