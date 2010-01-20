
#include "phy.h"
#include "mac.h"
#include "../system.h"


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

char *phy_node_get_name(node_t *node)
{
    rs_assert(node != NULL);

    return node->phy_info->name;
}

void phy_node_set_name(node_t *node, const char *name)
{
    rs_assert(node != NULL);

    if (node->phy_info->name != NULL)
        free(node->phy_info->name);

    node->phy_info->name = strdup(name);
}

coord_t phy_node_get_x(node_t *node)
{
    rs_assert(node != NULL);

    return node->phy_info->cx;
}

coord_t phy_node_get_y(node_t *node)
{
    rs_assert(node != NULL);

    return node->phy_info->cy;
}

void phy_node_set_xy(node_t *node, coord_t x, coord_t y)
{
    rs_assert(node != NULL);

    node->phy_info->cx = x;
    node->phy_info->cy = y;
}

percent_t phy_node_get_battery_level(node_t *node)
{
    rs_assert(node != NULL);

    return node->phy_info->battery_level;
}

void phy_node_set_battery_level(node_t *node, percent_t level)
{
    rs_assert(node != NULL);

    node->phy_info->battery_level = level;
}

percent_t phy_node_get_tx_power(node_t *node)
{
    rs_assert(node != NULL);

    return node->phy_info->tx_power;
}

void phy_node_set_tx_power(node_t *node, percent_t tx_power)
{
    rs_assert(node != NULL);

    node->phy_info->tx_power = tx_power;
}

bool phy_node_is_mains_powered(node_t *node)
{
    rs_assert(node != NULL);

    return node->phy_info->mains_powered;
}

void phy_node_set_mains_powered(node_t *node, bool value)
{
    rs_assert(node != NULL);

    node->phy_info->mains_powered = value;
}

bool phy_send(node_t *src_node, node_t *dst_node, void *sdu)
{
    phy_pdu_t *phy_pdu = phy_pdu_create();
    phy_pdu_set_sdu(phy_pdu, sdu);
    node_execute(src_node, "phy_event_before_pdu_sent", (node_schedule_func_t) phy_event_before_pdu_sent, phy_pdu, TRUE);

    if (dst_node == NULL) { /* broadcast */
        node_t **node_list;
        uint16 index, node_count;

        node_list = rs_system_get_node_list(&node_count);

        // fixme this gives temporal priority to nodes at the begining of the node list
        for (index = 0; index < node_count; index++) {
            node_t *node = node_list[index];

            if (!phy_send(src_node, node, sdu)) {
                return FALSE;
            }
        }
    }
    else {
        if (!node_enqueue_pdu(dst_node, phy_pdu, rs_system_get_transmit_mode())) {
            rs_error("failed to enqueue pdu");
            return FALSE;
        }
    }

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
