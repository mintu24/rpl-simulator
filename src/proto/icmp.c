
#include "icmp.h"
#include "../system.h"


    /**** global variables ****/

uint16              icmp_event_id_after_node_wake;
uint16              icmp_event_id_before_node_kill;

uint16              icmp_event_id_after_pdu_sent;
uint16              icmp_event_id_after_pdu_received;

uint16              icmp_event_id_after_ping_timer_timeout;
uint16              icmp_event_id_after_ping_timeout;


    /**** local function prototypes ****/


    /**** exported functions ****/

bool icmp_init()
{
    icmp_event_id_after_node_wake = event_register("after_node_wake", "icmp", (event_handler_t) icmp_event_after_node_wake, NULL);
    icmp_event_id_before_node_kill = event_register("before_node_kill", "icmp", (event_handler_t) icmp_event_before_node_kill, NULL);

    icmp_event_id_after_pdu_sent = event_register("after_pdu_sent", "icmp", (event_handler_t) icmp_event_after_pdu_sent, NULL);
    icmp_event_id_after_pdu_received = event_register("after_pdu_received", "icmp", (event_handler_t) icmp_event_after_pdu_received, NULL);

    icmp_event_id_after_ping_timer_timeout = event_register("after_ping_timer_timeout", "icmp", (event_handler_t) icmp_event_after_ping_timer_timeout, NULL);
    icmp_event_id_after_ping_timeout = event_register("after_ping_timeout", "icmp", (event_handler_t) icmp_event_after_ping_timeout, NULL);

    return TRUE;
}

bool icmp_done()
{
    return TRUE;
}

icmp_pdu_t *icmp_pdu_create()
{
    icmp_pdu_t *pdu = malloc(sizeof(icmp_pdu_t));

    pdu->type = -1;
    pdu->code = -1;
    pdu->sdu = NULL;

    return pdu;
}

void icmp_pdu_destroy(icmp_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    free(pdu);
}

icmp_pdu_t *icmp_pdu_duplicate(icmp_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    icmp_pdu_t *new_pdu = malloc(sizeof(icmp_pdu_t));

    new_pdu->type = pdu->type;
    new_pdu->code = pdu->code;

    switch (pdu->type) {
        case ICMP_TYPE_RPL:

            switch (pdu->code) {
                case ICMP_RPL_CODE_DIS:
                    new_pdu->sdu = NULL;
                    break;

                case ICMP_RPL_CODE_DIO:
                    new_pdu->sdu = rpl_dio_pdu_duplicate(pdu->sdu);
                    break;

                case ICMP_RPL_CODE_DAO:
                    new_pdu->sdu = rpl_dao_pdu_duplicate(pdu->sdu);
                    break;
            }

            break;
    }

    return new_pdu;
}

bool icmp_pdu_set_sdu(icmp_pdu_t *pdu, uint8 type, uint8 code, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->type = type;
    pdu->code = code;
    pdu->sdu = sdu;

    return TRUE;
}

void icmp_node_init(node_t *node)
{
    rs_assert(node != NULL);

    node->icmp_info = malloc(sizeof(icmp_node_info_t));
    node->icmp_info->ping_ip_address = NULL;
    node->icmp_info->ping_interval = ICMP_DEFAULT_PING_INTERVAL;
    node->icmp_info->ping_timeout = ICMP_DEFAULT_PING_TIMEOUT;
}

void icmp_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->icmp_info != NULL) {
        if (node->icmp_info->ping_ip_address != NULL) {
            free(node->icmp_info->ping_ip_address);
        }

        free(node->icmp_info);
        node->icmp_info = NULL;
    }
}

bool icmp_send(node_t *node, char *dst_ip_address, uint8 type, uint8 code, void *sdu)
{
    rs_assert(node != NULL);

    icmp_pdu_t *icmp_pdu = icmp_pdu_create();
    icmp_pdu_set_sdu(icmp_pdu, type, code, sdu);

    if (!event_execute(icmp_event_id_after_pdu_sent, node, dst_ip_address, icmp_pdu)) {
        icmp_pdu_destroy(icmp_pdu);
        return FALSE;
    }

    return TRUE;
}

bool icmp_receive(node_t *node, node_t *incoming_node, ip_pdu_t *ip_pdu)
{
    rs_assert(node != NULL);
    rs_assert(ip_pdu != NULL);

    bool all_ok = event_execute(icmp_event_id_after_pdu_received, node, incoming_node, ip_pdu);

    icmp_pdu_destroy(ip_pdu->sdu);

    return all_ok;
}

bool icmp_event_after_node_wake(node_t *node)
{
    if (node->icmp_info->ping_ip_address != NULL) {
        rs_system_schedule_event(node, icmp_event_id_after_ping_timer_timeout, NULL, NULL, node->icmp_info->ping_interval);
    }

    return TRUE;
}

bool icmp_event_before_node_kill(node_t *node)
{
    rs_system_cancel_event(node, icmp_event_id_after_ping_timer_timeout, NULL, NULL, 0);
    rs_system_cancel_event(node, icmp_event_id_after_ping_timeout, NULL, NULL, 0);

    return TRUE;
}

bool icmp_event_after_pdu_sent(node_t *node, char *dst_ip_address, icmp_pdu_t *pdu)
{
    return ip_send(node, dst_ip_address, IP_NEXT_HEADER_ICMP, pdu);
}

bool icmp_event_after_pdu_received(node_t *node, node_t *incoming_node, ip_pdu_t *ip_pdu)
{
    bool all_ok = TRUE;

    rs_assert(ip_pdu->sdu != NULL);
    icmp_pdu_t *pdu = ip_pdu->sdu;

    switch (pdu->type) {

        case ICMP_TYPE_ECHO_REQUEST: {
            rs_debug(DEBUG_ICMP, "node '%s': received a ping request from '%s'", node->phy_info->name, ip_pdu->src_address);
            rs_debug(DEBUG_ICMP, "node '%s': sending a ping reply to '%s'", node->phy_info->name, ip_pdu->src_address);
            icmp_send(node, ip_pdu->src_address, ICMP_TYPE_ECHO_REPLY, 0, NULL);

            break;
        }

        case ICMP_TYPE_ECHO_REPLY: {
            rs_debug(DEBUG_ICMP, "node '%s': received a ping reply from '%s'", node->phy_info->name, ip_pdu->src_address);
            rs_system_cancel_event(node, icmp_event_id_after_ping_timeout, NULL, NULL, 0);

            break;
        }

        case ICMP_TYPE_RPL:
            switch (pdu->code) {

                case ICMP_RPL_CODE_DIS: {
                    if (!rpl_receive_dis(node, incoming_node)) {
                        rs_error("node '%s': failed to receive RPL DIS from node '%s'", node->phy_info->name, incoming_node->phy_info->name);
                        all_ok = FALSE;
                    }

                    break;
                }

                case ICMP_RPL_CODE_DIO: {
                    rpl_dio_pdu_t *rpl_dio_pdu = pdu->sdu;
                    if (!rpl_receive_dio(node, incoming_node, rpl_dio_pdu)) {
                        rs_error("node '%s': failed to receive RPL DIO from node '%s'", node->phy_info->name, incoming_node->phy_info->name);
                        all_ok = FALSE;
                    }

                    break;
                }

                case ICMP_RPL_CODE_DAO: {
                    rpl_dao_pdu_t *rpl_dao_pdu = pdu->sdu;
                    if (!rpl_receive_dao(node, incoming_node, rpl_dao_pdu)) {
                        rs_error("node '%s': failed to receive RPL DAO from node '%s'", node->phy_info->name, incoming_node->phy_info->name);
                        all_ok = FALSE;
                    }

                    break;
                }

                default:
                    rs_error("node '%': unknown ICMP code '0x%02X'", node->phy_info->name, pdu->code);
                    all_ok = FALSE;
            }

            break;

        default:
            rs_error("node '%s': unknown ICMP type '0x%02X'", node->phy_info->name, pdu->type);
            all_ok = FALSE;
    }

    return all_ok;
}

bool icmp_event_after_ping_timer_timeout(node_t *node)
{
    if (node->icmp_info->ping_ip_address == NULL) {
        return TRUE;
    }

    // todo apparently a successful ping is registered when changing the destination address to ping...

    rs_debug(DEBUG_ICMP, "node '%s': sending a ping request to '%s'", node->phy_info->name, node->icmp_info->ping_ip_address);
    icmp_send(node, node->icmp_info->ping_ip_address, ICMP_TYPE_ECHO_REQUEST, 0, NULL);

    measure_node_add_ping(node, FALSE);

    rs_system_schedule_event(node, icmp_event_id_after_ping_timeout, node->icmp_info->ping_ip_address, NULL, node->icmp_info->ping_timeout);
    rs_system_schedule_event(node, icmp_event_id_after_ping_timer_timeout, NULL, NULL, node->icmp_info->ping_interval);

    return TRUE;
}

bool icmp_event_after_ping_timeout(node_t *node, char *dst_ip_address)
{
    if (node->icmp_info->ping_ip_address == NULL) {
        return TRUE;
    }

    rs_debug(DEBUG_ICMP, "node '%s': ping request to '%s' timeout", node->phy_info->name, dst_ip_address);

    measure_node_add_ping(node, TRUE);

    return TRUE;
}


    /**** local functions ****/

