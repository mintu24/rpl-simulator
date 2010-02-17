
#include "mac.h"
#include "phy.h"
#include "ip.h"
#include "../system.h"


    /**** global variables ****/

uint16               mac_event_id_after_node_wake;
uint16               mac_event_id_before_node_kill;
uint16               mac_event_id_after_pdu_sent;
uint16               mac_event_id_after_pdu_received;


    /**** local function prototypes ****/


    /**** exported functions ****/

bool mac_init()
{
    mac_event_id_after_node_wake = event_register("after_node_wake", "mac", (event_handler_t) mac_event_after_node_wake, NULL);
    mac_event_id_before_node_kill = event_register("before_node_kill", "mac", (event_handler_t) mac_event_before_node_kill, NULL);
    mac_event_id_after_pdu_sent = event_register("after_pdu_sent", "mac", (event_handler_t) mac_event_after_pdu_sent, NULL);
    mac_event_id_after_pdu_received = event_register("after_pdu_received", "mac", (event_handler_t) mac_event_after_pdu_received, NULL);

    return TRUE;
}

bool mac_done()
{
    return TRUE;
}

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

    free(pdu);

    return TRUE;
}

mac_pdu_t *mac_pdu_duplicate(mac_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    mac_pdu_t *new_pdu = malloc(sizeof(mac_pdu_t));

    new_pdu->dst_address = strdup(pdu->dst_address);
    new_pdu->src_address = strdup(pdu->src_address);

    new_pdu->type = pdu->type;

    switch (pdu->type) {
        case MAC_TYPE_IP:
            new_pdu->sdu = ip_pdu_duplicate(pdu->sdu);

            break;
    }

    return new_pdu;
}

bool mac_pdu_set_sdu(mac_pdu_t *pdu, uint16 type, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->type = type;
    pdu->sdu = sdu;

    return TRUE;
}

bool mac_node_init(node_t *node, char *address)
{
    rs_assert(node!= NULL);
    rs_assert(address != NULL);

    node->mac_info = malloc(sizeof(mac_node_info_t));

    node->mac_info->address = strdup(address);

    return TRUE;
}

void mac_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->mac_info != NULL) {
        if (node->mac_info->address != NULL)
            free(node->mac_info->address);

        free(node->mac_info);
    }
}

void mac_node_set_address(node_t *node, const char *address)
{
    rs_assert(node != NULL);
    rs_assert(address != NULL);

    if (node->mac_info->address != NULL)
        free(node->mac_info->address);

    node->mac_info->address = strdup(address);
}

bool mac_send(node_t *node, node_t *dst_node, uint16 type, void *sdu)
{
    rs_assert(node != NULL);

    mac_pdu_t *mac_pdu = mac_pdu_create(dst_node != NULL ? dst_node->mac_info->address : "", node->mac_info->address);
    mac_pdu_set_sdu(mac_pdu, type, sdu);

    return event_execute(mac_event_id_after_pdu_sent, node, dst_node, mac_pdu);
}

bool mac_receive(node_t *node, node_t *src_node, mac_pdu_t *pdu)
{
    rs_assert(pdu!= NULL);
    rs_assert(node != NULL);

    bool all_ok = event_execute(mac_event_id_after_pdu_received, node, src_node, pdu);

    mac_pdu_destroy(pdu);

    return all_ok;
}


bool mac_event_after_node_wake(node_t *node)
{
    return TRUE;
}

bool mac_event_before_node_kill(node_t *node)
{
    return TRUE;
}

bool mac_event_after_pdu_sent(node_t *node, node_t *dst_node, mac_pdu_t *pdu)
{
    if (!phy_send(node, dst_node, pdu)) {
        rs_error("node '%s': failed to send PHY message", node->phy_info->name);
        return FALSE;
    }

    return TRUE;
}

bool mac_event_after_pdu_received(node_t *node, node_t *src_node, mac_pdu_t *pdu)
{
    bool all_ok = TRUE;

    switch (pdu->type) {

        case MAC_TYPE_IP : {
            ip_pdu_t *ip_pdu = pdu->sdu;

            if (!ip_receive(node, src_node, ip_pdu)) {
                rs_error("node '%s': failed to receive IP pdu from node '%s'", node->phy_info->name, src_node->phy_info->name);
                all_ok = FALSE;
            }

            break;
        }

        default:
            rs_error("node '%s': unknown MAC type '0x%04X'", node->phy_info->name, pdu->type);
            all_ok = FALSE;
    }

    return TRUE;
}

    /**** local functions ****/

