
#include "mac.h"
#include "../system.h"


    /**** global variables ****/

uint16                  mac_event_node_wake;
uint16                  mac_event_node_kill;

uint16                  mac_event_pdu_send;
uint16                  mac_event_pdu_send_timeout_check;
uint16                  mac_event_pdu_receive;


    /**** local function prototypes ****/

static bool             event_handler_node_wake(node_t *node);
static bool             event_handler_node_kill(node_t *node);

static bool             event_handler_pdu_send(node_t *node, node_t *outgoing_node, mac_pdu_t *pdu);
static bool             event_handler_pdu_send_timeout_check(node_t *node, node_t *outgoing_node, mac_pdu_t *pdu);
static bool             event_handler_pdu_receive(node_t *node, node_t *incoming_node, mac_pdu_t *pdu);

static void             event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len);

    /**** exported functions ****/

bool mac_init()
{
    mac_event_node_wake = event_register("node_wake", "mac", (event_handler_t) event_handler_node_wake, NULL);
    mac_event_node_kill = event_register("node_kill", "mac", (event_handler_t) event_handler_node_kill, NULL);

    mac_event_pdu_send = event_register("pdu_send", "mac", (event_handler_t) event_handler_pdu_send, event_arg_str);
    mac_event_pdu_send_timeout_check = event_register("pdu_send_timeout_check", "mac", (event_handler_t) event_handler_pdu_send_timeout_check, event_arg_str);
    mac_event_pdu_receive = event_register("pdu_receive", "mac", (event_handler_t) event_handler_pdu_receive, event_arg_str);

    return TRUE;
}

bool mac_done()
{
    return TRUE;
}

mac_pdu_t *mac_pdu_create(char *src_address, char *dst_address)
{
    rs_assert(src_address != NULL);
    rs_assert(dst_address != NULL);

    mac_pdu_t *pdu = malloc(sizeof(mac_pdu_t));

    pdu->src_address = strdup(src_address);
    pdu->dst_address = strdup(dst_address);

    pdu->type = -1;
    pdu->sdu = NULL;

    return pdu;
}

void mac_pdu_destroy(mac_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    if (pdu->dst_address != NULL)
        free(pdu->dst_address);

    if (pdu->src_address != NULL)
        free(pdu->src_address);

    if (pdu->sdu != NULL) {
        switch (pdu->type) {

            case MAC_TYPE_IP :
                ip_pdu_destroy(pdu->sdu);
                break;

        }
    }

    free(pdu);
}

mac_pdu_t *mac_pdu_duplicate(mac_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    mac_pdu_t *new_pdu = malloc(sizeof(mac_pdu_t));

    new_pdu->src_address = strdup(pdu->src_address);
    new_pdu->dst_address = strdup(pdu->dst_address);

    new_pdu->type = pdu->type;

    switch (pdu->type) {
        case MAC_TYPE_IP:
            new_pdu->sdu = ip_pdu_duplicate(pdu->sdu);

            break;
    }

    return new_pdu;
}

void mac_pdu_set_sdu(mac_pdu_t *pdu, uint16 type, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->type = type;
    pdu->sdu = sdu;
}

void mac_node_init(node_t *node, char *address)
{
    rs_assert(node!= NULL);
    rs_assert(address != NULL);

    node->mac_info = malloc(sizeof(mac_node_info_t));

    node->mac_info->address = strdup(address);
    node->mac_info->busy = FALSE;
    node->mac_info->error = FALSE;
}

void mac_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->mac_info != NULL) {
        if (node->mac_info->address != NULL)
            free(node->mac_info->address);

        free(node->mac_info);
        node->mac_info = NULL;
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

bool mac_node_send(node_t *node, node_t *outgoing_node, uint16 type, void *sdu)
{
    rs_assert(node != NULL);

    mac_pdu_t *mac_pdu = mac_pdu_create(node->mac_info->address, outgoing_node != NULL ? outgoing_node->mac_info->address : "");
    mac_pdu_set_sdu(mac_pdu, type, sdu);

    if (!event_execute(mac_event_pdu_send, node, outgoing_node, mac_pdu)) {
        mac_pdu->sdu = NULL;
        mac_pdu_destroy(mac_pdu);

        return FALSE;
    }

    return TRUE;
}

bool mac_node_receive(node_t *node, node_t *incoming_node, mac_pdu_t *pdu)
{
    rs_assert(pdu!= NULL);
    rs_assert(node != NULL);

    bool all_ok = event_execute(mac_event_pdu_receive, node, incoming_node, pdu);

    mac_pdu_destroy(pdu);

    return all_ok;
}


    /**** local functions ****/

bool event_handler_node_wake(node_t *node)
{
    return TRUE;
}

bool event_handler_node_kill(node_t *node)
{
    return TRUE;
}

bool event_handler_pdu_send(node_t *node, node_t *outgoing_node, mac_pdu_t *pdu)
{
    if (node->mac_info->busy) {
        rs_debug(DEBUG_MAC, "node '%s': MAC layer is busy, dropping frame", node->phy_info->name);

        return FALSE;
    }

    if (!phy_node_send(node, outgoing_node, pdu)) {
        return FALSE;
    }

    rs_system_schedule_event(node, mac_event_pdu_send_timeout_check, outgoing_node, pdu, rs_system->mac_pdu_timeout);

    node->mac_info->error = TRUE;
    node->mac_info->busy = TRUE;

    return TRUE;
}

bool event_handler_pdu_send_timeout_check(node_t *node, node_t *outgoing_node, mac_pdu_t *pdu)
{
    if (node->mac_info->error) { /* the message wasn't received */
        rs_debug(DEBUG_MAC, "node '%s': a frame wasn't correctly received, destroying it", node->phy_info->name);
        pdu->sdu = NULL;
        mac_pdu_destroy(pdu);
    }

    node->mac_info->busy = FALSE;

    return TRUE;
}

bool event_handler_pdu_receive(node_t *node, node_t *incoming_node, mac_pdu_t *pdu)
{
    bool all_ok = TRUE;

    incoming_node->mac_info->error = FALSE; /* emulate a MAC acknowledgment */

    switch (pdu->type) {

        case MAC_TYPE_IP : {
            ip_pdu_t *ip_pdu = pdu->sdu;
            pdu->sdu = NULL;

            if (!ip_node_receive(node, incoming_node, ip_pdu)) {
                all_ok = FALSE;
            }

            break;
        }

        default:
            rs_error("node '%s': unknown MAC type '0x%04X'", node->phy_info->name, pdu->type);
            all_ok = FALSE;
    }

    return all_ok;
}

static void event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len)
{

    str1[0] = '\0';
    str2[0] = '\0';

    if (event_id == mac_event_pdu_send) {
        node_t *node = data1;
        mac_pdu_t *pdu = data2;

        snprintf(str1, len, "outgoing_node = '%s'", (node != NULL ? node->phy_info->name : "<<broadcast>>"));
        snprintf(str2, len, "mac_pdu = {src = '%s', dst = '%s'}", pdu->src_address, pdu->dst_address);
    }
    else if (event_id == mac_event_pdu_send_timeout_check) {
        node_t *node = data1;

        snprintf(str1, len, "outgoing_node = '%s'", (node != NULL ? node->phy_info->name : "<<broadcast>>"));
    }
    else if (event_id == mac_event_pdu_receive) {
        node_t *node = data1;
        mac_pdu_t *pdu = data2;

        snprintf(str1, len, "incoming_node = '%s'", (node != NULL ? node->phy_info->name : "<<unknown>>"));
        snprintf(str2, len, "mac_pdu = {src = '%s', dst = '%s'}", pdu->src_address, pdu->dst_address);
    }
}
