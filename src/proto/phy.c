
#include "phy.h"
#include "mac.h"
#include "../system.h"


    /**** global variables ****/

uint16               phy_event_id_after_node_wake;
uint16               phy_event_id_before_node_kill;
uint16               phy_event_id_after_pdu_sent;
uint16               phy_event_id_after_pdu_received;


    /**** local function prototypes ****/


    /**** exported functions ****/

bool phy_init()
{
    phy_event_id_after_node_wake = event_register("phy_after_node_wake", (event_handler_t) phy_event_after_node_wake);
    phy_event_id_before_node_kill = event_register("phy_before_node_kill", (event_handler_t) phy_event_before_node_kill);
    phy_event_id_after_pdu_sent = event_register("phy_after_pdu_sent", (event_handler_t) phy_event_after_pdu_sent);
    phy_event_id_after_pdu_received = event_register("phy_after_pdu_received", (event_handler_t) phy_event_after_pdu_received);

    return TRUE;
}

bool phy_done()
{
    return TRUE;
}

phy_pdu_t *phy_pdu_create()
{
    phy_pdu_t *pdu = malloc(sizeof(phy_pdu_t));

    pdu->sdu = NULL;

    return pdu;
}

bool phy_pdu_destroy(phy_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    free(pdu);

    return TRUE;
}

phy_pdu_t *phy_pdu_duplicate(phy_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    phy_pdu_t *new_pdu = malloc(sizeof(phy_pdu_t));

    new_pdu->sdu = mac_pdu_duplicate(pdu->sdu);

    return new_pdu;
}

bool phy_pdu_set_sdu(phy_pdu_t *pdu, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->sdu = sdu;

    return TRUE;
}

bool phy_node_init(node_t *node, char *name, coord_t cx, coord_t cy)
{
    rs_assert(node != NULL);
    rs_assert(name != NULL);

    node->phy_info = malloc(sizeof(phy_node_info_t));

    node->phy_info->name = strdup(name);
    node->phy_info->cx = cx;
    node->phy_info->cy = cy;

    node->phy_info->mains_powered = FALSE;
    node->phy_info->tx_power = 0.5;
    node->phy_info->battery_level = 0.5;

    return TRUE;
}

void phy_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->phy_info != NULL) {
        if (node->phy_info->name != NULL)
            free(node->phy_info->name);

        free(node->phy_info);
    }
}

void phy_node_set_name(node_t *node, const char *name)
{
    rs_assert(node != NULL);
    rs_assert(name != NULL);

    if (node->phy_info->name != NULL)
        free(node->phy_info->name);

    node->phy_info->name = strdup(name);
}

bool phy_send(node_t *node, node_t *dst_node, void *sdu)
{
    rs_assert(node != NULL);
    rs_assert(sdu != NULL);

    phy_pdu_t *phy_pdu = phy_pdu_create();
    phy_pdu_set_sdu(phy_pdu, sdu);

    return event_execute(phy_event_id_after_pdu_sent, node, dst_node, phy_pdu);
}

bool phy_receive(node_t *node, node_t *src_node, phy_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    bool all_ok = event_execute(phy_event_id_after_pdu_received, node, src_node, pdu);

    phy_pdu_destroy(pdu);

    return all_ok;
}


bool phy_event_after_node_wake(node_t *node)
{
    return TRUE;
}

bool phy_event_before_node_kill(node_t *node)
{
    return TRUE;
}

bool phy_event_after_pdu_sent(node_t *node, node_t *dst_node, phy_pdu_t *pdu)
{
    if (!rs_system_send(node, dst_node, pdu)) {
        rs_error("node '%s': failed to send SYS message", node->phy_info->name);
        return FALSE;
    }

    return TRUE;
}

bool phy_event_after_pdu_received(node_t *node, node_t *src_node, phy_pdu_t *pdu)
{
    mac_pdu_t *mac_pdu = pdu->sdu;

    bool all_ok = TRUE;

    if (!mac_receive(node, src_node, mac_pdu)) {
        rs_error("node '%s': failed to receive MAC pdu from node '%s'", node->phy_info->name, src_node->phy_info->name);
        all_ok = FALSE;
    }

    return all_ok;
}


/**** local functions ****/

