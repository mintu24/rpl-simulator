
#include "mac.h"
#include "phy.h"
#include "ip.h"
#include "../system.h"


    /**** local function prototypes ****/

static void             mac_event_before_pdu_sent(node_t *src_node, node_t *dst_node, mac_pdu_t *pdu);
static void             mac_event_after_pdu_received(node_t *src_node, node_t *dst_node, mac_pdu_t *pdu);


    /**** exported functions ****/

mac_pdu_t *mac_pdu_create(char *dst_address, char *src_address)
{
    rs_assert(dst_address != NULL);
    rs_assert(src_address != NULL);

    mac_pdu_t *pdu = malloc(sizeof(mac_pdu_t));

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

mac_node_info_t *mac_node_info_create(char *address)
{
    rs_assert(address != NULL);

    mac_node_info_t *node_info = malloc(sizeof(mac_node_info_t));

    node_info->address = strdup(address);

    return node_info;
}

bool mac_node_info_destroy(mac_node_info_t *node_info)
{
    rs_assert(node_info != NULL);

    if (node_info->address != NULL)
        free(node_info->address);

    free(node_info);

    return TRUE;
}

bool mac_init_node(node_t *node, mac_node_info_t *node_info)
{
    rs_assert(node!= NULL);
    rs_assert(node_info != NULL);

    node->mac_info = node_info;

    return TRUE;
}

char *mac_node_get_address(node_t *node)
{
    rs_assert(node != NULL);

    return node->mac_info->address;
}

void mac_node_set_address(node_t *node, const char *address)
{
    rs_assert(node != NULL);

    if (node->mac_info->address != NULL)
        free(node->mac_info->address);

    node->mac_info->address = strdup(address);
}

bool mac_send(node_t *src_node, node_t *dst_node, uint16 type, void *sdu)
{
    rs_assert(src_node != NULL);

    mac_pdu_t *mac_pdu = mac_pdu_create(mac_node_get_address(dst_node), mac_node_get_address(src_node));
    mac_pdu_set_sdu(mac_pdu, type, sdu);

    node_execute_src_dst(
            src_node,
            "mac_event_before_pdu_sent",
            (node_event_src_dst_t) mac_event_before_pdu_sent,
            src_node, dst_node, mac_pdu,
            TRUE);

    if (!phy_send(src_node, dst_node, mac_pdu)) {
        rs_error("failed to send PHY message");
        return FALSE;
    }

    return TRUE;
}

bool mac_receive(node_t *dst_node, mac_pdu_t *pdu)
{
    rs_assert(pdu!= NULL);
    rs_assert(dst_node != NULL);

    node_t *src_node = rs_system_find_node_by_mac_address(pdu->src_address);

    node_execute_src_dst(
            dst_node,
            "mac_event_after_pdu_received",
            (node_event_src_dst_t) mac_event_after_pdu_received,
            src_node, dst_node, pdu,
            TRUE);

    switch (pdu->type) {

        case MAC_TYPE_IP : {
            ip_pdu_t *ip_pdu = pdu->sdu;
            return ip_receive(src_node, dst_node, ip_pdu);

            break;
        }

        default:
            rs_error("unknown MAC type '0x%04X'", pdu->type);

            return FALSE;
    }

    return TRUE;
}


    /**** local functions ****/

static void mac_event_before_pdu_sent(node_t *src_node, node_t *dst_node, mac_pdu_t *pdu)
{
    rs_debug("'%s' -> '%s'", pdu->src_address, pdu->dst_address);
}

static void mac_event_after_pdu_received(node_t *src_node, node_t *dst_node, mac_pdu_t *pdu)
{
    rs_debug("'%s' -> '%s'", pdu->src_address, pdu->dst_address);
}
