
#include "icmp.h"
#include "../system.h"


    /**** global variables ****/

uint16                  icmp_event_node_wake;
uint16                  icmp_event_node_kill;

uint16                  icmp_event_pdu_send;
uint16                  icmp_event_pdu_receive;

uint16                  icmp_event_ping_request;
uint16                  icmp_event_ping_reply;
uint16                  icmp_event_ping_timeout;


    /**** local function prototypes ****/

static bool             event_handler_node_wake(node_t *node);
static bool             event_handler_node_kill(node_t *node);

static bool             event_handler_pdu_send(node_t *node, char *dst_ip_address, icmp_pdu_t *pdu);
static bool             event_handler_pdu_receive(node_t *node, node_t *incoming_node, ip_pdu_t *pdu); /* yes, it's ip_pdu_t, since icmp and ip work closely together */

static bool             event_handler_ping_request(node_t *node);
static bool             event_handler_ping_timeout(node_t *node, char *dst_ip_address);

static void             event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len);


    /**** exported functions ****/

bool icmp_init()
{
    icmp_event_node_wake = event_register("node_wake", "icmp", (event_handler_t) event_handler_node_wake, NULL);
    icmp_event_node_kill = event_register("node_kill", "icmp", (event_handler_t) event_handler_node_kill, NULL);

    icmp_event_pdu_send = event_register("pdu_send", "icmp", (event_handler_t) event_handler_pdu_send, event_arg_str);
    icmp_event_pdu_receive = event_register("pdu_receive", "icmp", (event_handler_t) event_handler_pdu_receive, event_arg_str);

    icmp_event_ping_request = event_register("ping_request", "icmp", (event_handler_t) event_handler_ping_request, event_arg_str);
    icmp_event_ping_reply = event_register("ping_reply", "icmp", NULL, event_arg_str);
    icmp_event_ping_timeout = event_register("ping_timeout", "icmp", (event_handler_t) event_handler_ping_timeout, event_arg_str);

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

    if (pdu->sdu != NULL) {

        switch (pdu->type) {

            case ICMP_TYPE_RPL:

                switch (pdu->code) {

                    case ICMP_RPL_CODE_DIO:
                        rpl_dio_pdu_destroy(pdu->sdu);

                        break;

                    case ICMP_RPL_CODE_DAO:
                        rpl_dao_pdu_destroy(pdu->sdu);

                        break;

                }

                break;

        }

    }

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

void icmp_pdu_set_sdu(icmp_pdu_t *pdu, uint8 type, uint8 code, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->type = type;
    pdu->code = code;
    pdu->sdu = sdu;
}

void icmp_node_init(node_t *node)
{
    rs_assert(node != NULL);

    node->icmp_info = malloc(sizeof(icmp_node_info_t));
    node->icmp_info->ping_ip_address = NULL;
    node->icmp_info->ping_interval = ICMP_DEFAULT_PING_INTERVAL;
    node->icmp_info->ping_timeout = ICMP_DEFAULT_PING_TIMEOUT;
    node->icmp_info->ping_busy = FALSE;
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

bool icmp_node_send(node_t *node, char *dst_ip_address, uint8 type, uint8 code, void *sdu)
{
    rs_assert(node != NULL);

    icmp_pdu_t *icmp_pdu = icmp_pdu_create();
    icmp_pdu_set_sdu(icmp_pdu, type, code, sdu);

    if (!event_execute(icmp_event_pdu_send, node, dst_ip_address, icmp_pdu)) {
        icmp_pdu->sdu = NULL;
        icmp_pdu_destroy(icmp_pdu);
        return FALSE;
    }

    return TRUE;
}

bool icmp_node_receive(node_t *node, node_t *incoming_node, ip_pdu_t *ip_pdu)
{
    rs_assert(node != NULL);
    rs_assert(ip_pdu != NULL);

    bool all_ok = event_execute(icmp_event_pdu_receive, node, incoming_node, ip_pdu);

    icmp_pdu_destroy(ip_pdu->sdu);

    return all_ok;
}


    /**** local functions ****/

static bool event_handler_node_wake(node_t *node)
{
    if (node->icmp_info->ping_ip_address != NULL) {
        rs_system_schedule_event(node, icmp_event_ping_request,
                node->ip_info->address, node->icmp_info->ping_ip_address, node->icmp_info->ping_interval);
    }

    return TRUE;
}

static bool event_handler_node_kill(node_t *node)
{
    rs_system_cancel_event(node, icmp_event_ping_request, NULL, NULL, 0);
    rs_system_cancel_event(node, icmp_event_ping_timeout, NULL, NULL, 0);

    node->icmp_info->ping_busy = FALSE;

    return TRUE;
}

static bool event_handler_pdu_send(node_t *node, char *dst_ip_address, icmp_pdu_t *pdu)
{
    return ip_node_send(node, dst_ip_address, IP_NEXT_HEADER_ICMP, pdu);
}

static bool event_handler_pdu_receive(node_t *node, node_t *incoming_node, ip_pdu_t *ip_pdu)
{
    bool all_ok = TRUE;

    rs_assert(ip_pdu->sdu != NULL);
    icmp_pdu_t *pdu = ip_pdu->sdu;

    switch (pdu->type) {

        case ICMP_TYPE_ECHO_REQUEST: {
            rs_debug(DEBUG_ICMP, "node '%s': received a ping request from '%s'", node->phy_info->name, ip_pdu->src_address);
            rs_debug(DEBUG_ICMP, "node '%s': sending a ping reply to '%s'", node->phy_info->name, ip_pdu->src_address);

            event_execute(icmp_event_ping_reply, node, node->ip_info->address, ip_pdu->src_address);

            icmp_node_send(node, ip_pdu->src_address, ICMP_TYPE_ECHO_REPLY, 0, NULL);

            break;
        }

        case ICMP_TYPE_ECHO_REPLY: {
            if (node->icmp_info->ping_busy) {
                rs_debug(DEBUG_ICMP, "node '%s': received a ping reply from '%s'", node->phy_info->name, ip_pdu->src_address);
                measure_node_add_ping(node, TRUE);
                rs_system_cancel_event(node, icmp_event_ping_timeout, NULL, NULL, 0);
                node->icmp_info->ping_busy = FALSE;
            }

            break;
        }

        case ICMP_TYPE_RPL:
            switch (pdu->code) {

                case ICMP_RPL_CODE_DIS: {
                    if (!rpl_node_receive_dis(node, incoming_node)) {
                        rs_error("node '%s': failed to receive RPL DIS from node '%s'", node->phy_info->name, incoming_node->phy_info->name);
                        all_ok = FALSE;
                    }

                    break;
                }

                case ICMP_RPL_CODE_DIO: {
                    rpl_dio_pdu_t *rpl_dio_pdu = pdu->sdu;
                    if (!rpl_node_receive_dio(node, incoming_node, rpl_dio_pdu)) {
                        rs_error("node '%s': failed to receive RPL DIO from node '%s'", node->phy_info->name, incoming_node->phy_info->name);
                        all_ok = FALSE;
                    }

                    break;
                }

                case ICMP_RPL_CODE_DAO: {
                    rpl_dao_pdu_t *rpl_dao_pdu = pdu->sdu;
                    if (!rpl_node_receive_dao(node, incoming_node, rpl_dao_pdu)) {
                        rs_error("node '%s': failed to receive RPL DAO from node '%s'", node->phy_info->name, incoming_node->phy_info->name);
                        all_ok = FALSE;
                    }

                    break;
                }

                default:
                    rs_error("node '%': unknown ICMP code '0x%02X'", node->phy_info->name, pdu->code);
                    all_ok = FALSE;
            }

            pdu->sdu = NULL;

            break;

        default:
            rs_error("node '%s': unknown ICMP type '0x%02X'", node->phy_info->name, pdu->type);
            all_ok = FALSE;
    }

    return all_ok;
}

static bool event_handler_ping_request(node_t *node)
{
    if (node->icmp_info->ping_ip_address == NULL) {
        return TRUE;
    }

    if (!node->icmp_info->ping_busy) {
        rs_debug(DEBUG_ICMP, "node '%s': sending a ping request to '%s'", node->phy_info->name, node->icmp_info->ping_ip_address);
        icmp_node_send(node, node->icmp_info->ping_ip_address, ICMP_TYPE_ECHO_REQUEST, 0, NULL);

        rs_system_schedule_event(node, icmp_event_ping_timeout,
                node->ip_info->address, node->icmp_info->ping_ip_address, node->icmp_info->ping_timeout);

        node->icmp_info->ping_busy = TRUE;
    }

    rs_system_schedule_event(node, icmp_event_ping_request,
            node->ip_info->address, node->icmp_info->ping_ip_address, node->icmp_info->ping_interval);

    return TRUE;
}

static bool event_handler_ping_timeout(node_t *node, char *dst_ip_address)
{
    if (node->icmp_info->ping_ip_address == NULL) {
        return TRUE;
    }

    if (!node->icmp_info->ping_busy) {
        return TRUE;
    }

    rs_debug(DEBUG_ICMP, "node '%s': ping request to '%s' timeout", node->phy_info->name, dst_ip_address);

    measure_node_add_ping(node, FALSE);

    node->icmp_info->ping_busy = FALSE;

    return TRUE;
}

static void event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len)
{

    str1[0] = '\0';
    str2[0] = '\0';

    if (event_id == icmp_event_pdu_send) {
        char *dst_ip_address = data1;

        snprintf(str1, len, "dst = '%s'", dst_ip_address != NULL ? dst_ip_address : "<<broadcast>>");
    }
    else if (event_id == icmp_event_pdu_receive) {
        node_t *node = data1;
        ip_pdu_t *pdu = data2;

        snprintf(str1, len, "incoming_node = '%s'", (node != NULL ? node->phy_info->name : "<<unknown>>"));
        snprintf(str2, len, "ip_pdu = {src = '%s', dst = '%s'}", pdu->src_address, pdu->dst_address);
    }
    else if (event_id == icmp_event_ping_request) {
        char *src_ip_address = data1;
        char *dst_ip_address = data2;

        snprintf(str1, len, "src = '%s'", src_ip_address != NULL ? src_ip_address : "<<unknown>>");
        snprintf(str2, len, "dst = '%s'", dst_ip_address != NULL ? dst_ip_address : "<<broadcast>>");
    }
    else if (event_id == icmp_event_ping_reply) {
        char *src_ip_address = data1;
        char *dst_ip_address = data2;

        snprintf(str1, len, "src = '%s'", src_ip_address != NULL ? src_ip_address : "<<unknown>>");
        snprintf(str2, len, "dst = '%s'", dst_ip_address != NULL ? dst_ip_address : "<<broadcast>>");
    }
    else if (event_id == icmp_event_ping_timeout) {
        char *src_ip_address = data1;
        char *dst_ip_address = data2;

        snprintf(str1, len, "src = '%s'", src_ip_address != NULL ? src_ip_address : "<<unknown>>");
        snprintf(str2, len, "dst = '%s'", dst_ip_address != NULL ? dst_ip_address : "<<broadcast>>");
    }
}
