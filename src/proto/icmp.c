// todo fix the bug where two fast mutual ping sessions block one each other

#include "icmp.h"
#include "../system.h"


    /**** local function prototypes ****/

static void         icmp_add_ping_measure(node_t *node, node_t *dst_node, bool timedout);
static void         icmp_do_ping(node_t *node);
static void         icmp_ping_timeout(node_t *node, node_t *dst_node);


    /**** exported functions ****/

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

    node->icmp_info->ping_measure_count = 0;
    node->icmp_info->ping_measures_enabled = FALSE;
    node->icmp_info->ping_measure_list = NULL;
    node->icmp_info->ping_interval = ICMP_DEFAULT_PING_INTERVAL;
    node->icmp_info->ping_timeout = ICMP_DEFAULT_PING_TIMEOUT;
    node->icmp_info->ping_current_seq = 0;
    node->icmp_info->ping_node = NULL;

    g_static_rec_mutex_init(&node->icmp_info->mutex);
}

void icmp_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->icmp_info != NULL) {
        icmp_node_lock(node);

        uint32 i;
        for (i = 0; i < node->icmp_info->ping_measure_count; i++) {
            free(node->icmp_info->ping_measure_list[i]);
        }

        icmp_node_unlock(node);

        g_static_rec_mutex_free(&node->icmp_info->mutex);

        free(node->icmp_info);
   }
}

void icmp_node_set_enable_ping_measurement(node_t *node, bool enabled)
{
    rs_assert(node != NULL);

    node->icmp_info->ping_measures_enabled = enabled;
}

bool icmp_node_is_enabled_ping_measurement(node_t *node)
{
    rs_assert(node != NULL);

    return node->icmp_info->ping_measures_enabled;
}

void icmp_node_set_ping_interval(node_t *node, uint32 interval)
{
    rs_assert(node != NULL);

    node->icmp_info->ping_interval = interval;
}

uint32 icmp_node_get_ping_interval(node_t *node)
{
    rs_assert(node != NULL);

    return node->icmp_info->ping_interval;
}

void icmp_node_set_ping_timeout(node_t *node, uint32 timeout)
{
    rs_assert(node != NULL);

    node->icmp_info->ping_timeout = timeout;
}

uint32 icmp_node_get_ping_timeout(node_t *node)
{
    rs_assert(node != NULL);

    return node->icmp_info->ping_timeout;
}

void icmp_node_set_ping_node(node_t *node, node_t *ping_node)
{
    rs_assert(node != NULL);

    node->icmp_info->ping_node = ping_node;
}

node_t *icmp_node_get_ping_node(node_t *node)
{
    rs_assert(node != NULL);

    return node->icmp_info->ping_node;
}

icmp_ping_measure_t *icmp_node_get_ping_measure(node_t *node, node_t *dst_node)
{
    rs_assert(node != NULL);

    icmp_node_lock(node);

    icmp_ping_measure_t *ping_measure = NULL;
    uint32 i;
    for (i = 0; i < node->icmp_info->ping_measure_count; i++) {
        if (node->icmp_info->ping_measure_list[i]->dst_node == dst_node) {
            ping_measure = node->icmp_info->ping_measure_list[i];
            break;
        }
    }

    icmp_node_unlock(node);

    return ping_measure;
}

icmp_ping_measure_t **icmp_node_get_ping_measure_list(node_t *node, uint32 *ping_measure_count)
{
    rs_assert(node != NULL);

    icmp_node_lock(node);

    if (ping_measure_count != NULL)
        *ping_measure_count = node->icmp_info->ping_measure_count;

    icmp_ping_measure_t *measure = node->icmp_info->ping_measure_list;

    icmp_node_unlock(node);

    return measure;
}

bool icmp_send(node_t *node, node_t *dst_node, uint8 type, uint8 code, void *sdu)
{
    rs_assert(node != NULL);

    icmp_pdu_t *icmp_pdu = icmp_pdu_create();
    icmp_pdu_set_sdu(icmp_pdu, type, code, sdu);

    node_execute_event(
            node,
            "icmp_event_before_pdu_sent",
            (node_event_t) icmp_event_before_pdu_sent,
            dst_node, icmp_pdu,
            TRUE);

    if (!ip_send(node, dst_node, IP_NEXT_HEADER_ICMP, icmp_pdu)) {
        rs_error("failed to send IP packet");
        return FALSE;
    }

    return TRUE;
}

bool icmp_receive(node_t *node, node_t *src_node, icmp_pdu_t *pdu)
{
    rs_assert(pdu != NULL);
    rs_assert(node != NULL);

    node_execute_event(
            node,
            "icmp_event_after_pdu_received",
            (node_event_t) icmp_event_after_pdu_received,
            src_node, pdu,
            TRUE);

    bool all_ok = TRUE;

    switch (pdu->type) {

        case ICMP_TYPE_ECHO_REQUEST:
            break;

        case ICMP_TYPE_ECHO_REPLY:
            break;

        case ICMP_TYPE_RPL:
            switch (pdu->code) {

                case ICMP_RPL_CODE_DIS: {
                    if (!rpl_receive_dis(node, src_node)) {
                        rs_error("failed to receive RPL DIS from node '%s'", phy_node_get_name(src_node));
                        all_ok = FALSE;
                    }

                    break;
                }

                case ICMP_RPL_CODE_DIO: {
                    rpl_dio_pdu_t *rpl_dio_pdu = pdu->sdu;
                    if (!rpl_receive_dio(node, src_node, rpl_dio_pdu)) {
                        rs_error("failed to receive RPL DIO from node '%s'", phy_node_get_name(src_node));
                        all_ok = FALSE;
                    }

                    break;
                }

                case ICMP_RPL_CODE_DAO: {
                    rpl_dao_pdu_t *rpl_dao_pdu = pdu->sdu;
                    if (!rpl_receive_dao(node, src_node, rpl_dao_pdu)) {
                        rs_error("failed to receive RPL DAO from node '%s'", phy_node_get_name(src_node));
                        all_ok = FALSE;
                    }

                    break;
                }

                default:
                    rs_error("unknown ICMP code '0x%02X'", pdu->code);
                    all_ok = FALSE;
            }

            break;

        default:
            rs_error("unknown ICMP type '0x%02X'", pdu->type);
            all_ok = FALSE;
    }

    icmp_pdu_destroy(pdu);

    return all_ok;
}

void icmp_event_after_node_wake(node_t *node)
{
    rs_debug(DEBUG_ICMP, "scheduling ping measurements routine");
    node_schedule(node, "ping", (node_schedule_func_t) icmp_do_ping, NULL, node->icmp_info->ping_interval, TRUE);
}

void icmp_event_before_node_kill(node_t *node)
{
}

void icmp_event_before_pdu_sent(node_t *node, node_t *dst_node, icmp_pdu_t *pdu)
{
}

void icmp_event_after_pdu_received(node_t *node, node_t *src_node, icmp_pdu_t *pdu)
{
    switch (pdu->type) {
        case ICMP_TYPE_RPL:
            break;

        case ICMP_TYPE_ECHO_REQUEST: {
            uint32 *seq = pdu->sdu;
            rs_debug(DEBUG_ICMP, "ICMP Echo Request received from '%s' with seq = %d", phy_node_get_name(src_node), *seq);

            rs_debug(DEBUG_ICMP, "sending ICMP Echo Reply to '%s' with seq = %d", phy_node_get_name(src_node), *seq);
            icmp_send(node, src_node, ICMP_TYPE_ECHO_REPLY, 0, seq);
            break;
        }

        case ICMP_TYPE_ECHO_REPLY: {
            uint32 *seq = pdu->sdu;
            rs_debug(DEBUG_ICMP, "ICMP Echo Reply received from '%s' with seq = %d", phy_node_get_name(src_node), *seq);

            char temp[256];
            snprintf(temp, sizeof(temp), "ping %s/%d timeout", phy_node_get_name(src_node), *seq);

            /* cancel the timeout handler */
            node_schedule(node, temp, NULL, NULL, 0, FALSE);

            /* mark down the measurement */
            icmp_add_ping_measure(node, src_node, FALSE);

            break;
        }

        default:
            rs_error("unknown ICMP type '0x%02X'", pdu->type);
            break;
    }
}


    /**** local functions ****/

static void icmp_add_ping_measure(node_t *node, node_t *dst_node, bool timedout)
{
    rs_assert(node != NULL);
    rs_assert(dst_node != NULL);

    icmp_node_lock(node);

    icmp_ping_measure_t *ping_measure = NULL;
    uint32 i;
    for (i = 0; i < node->icmp_info->ping_measure_count; i++) {
        if (node->icmp_info->ping_measure_list[i]->dst_node == dst_node) {
            ping_measure = node->icmp_info->ping_measure_list[i];
            break;
        }
    }

    if (ping_measure == NULL) {
        ping_measure = malloc(sizeof(icmp_ping_measure_t));
        ping_measure->dst_node = dst_node;
        ping_measure->failed_count = 0;
        ping_measure->total_count = 0;

        node->icmp_info->ping_measure_list = realloc(node->icmp_info->ping_measure_list, sizeof(icmp_ping_measure_t *) * (node->icmp_info->ping_measure_count + 1));
        node->icmp_info->ping_measure_list[node->icmp_info->ping_measure_count++] = ping_measure;
    }

    if (timedout) {
        ping_measure->failed_count++;
    }

    ping_measure->total_count++;

    icmp_node_unlock(node);
}

static void icmp_do_ping(node_t *node)
{
    rs_assert(node != NULL);

    /* reschedule the ping routine, according to the current node's ICMP/ping parameters */
    node_schedule(node, "ping", (node_schedule_func_t) icmp_do_ping, NULL, node->icmp_info->ping_interval, TRUE);

    if (!icmp_node_is_enabled_ping_measurement(node))
        return;

    icmp_node_lock(node);

    node_t *dst_node = NULL;
    if (node->icmp_info->ping_node == NULL) {
        uint16 node_count;
        node_t **node_list = rs_system_get_node_list_copy(&node_count);

        /* nothing to ping if 0 or 1 node in the system */
        if (node_count < 2) {
            free(node_list);
            icmp_node_unlock(node);

            return;
        }

        uint16 pos = rand() % node_count;
        dst_node = node_list[pos];

        free(node_list);
    }
    else {
        dst_node = node->icmp_info->ping_node;
    }

    if (dst_node == node || !dst_node->alive) {
        icmp_node_unlock(node);

        return;
    }

    uint32 *seq = malloc(sizeof(uint32));
    *seq = node->icmp_info->ping_current_seq++;

    char temp[256];
    snprintf(temp, sizeof(temp), "ping %s/%d timeout", phy_node_get_name(dst_node), *seq);
    node_schedule(
            node,
            temp,
            (node_schedule_func_t) icmp_ping_timeout,
            dst_node,
            node->icmp_info->ping_timeout,
            FALSE);

    icmp_node_unlock(node);

    rs_debug(DEBUG_ICMP, "sending ICMP Echo Request to '%s' with seq = %d", phy_node_get_name(dst_node), *seq);
    icmp_send(node, dst_node, ICMP_TYPE_ECHO_REQUEST, 0, seq);
}

static void icmp_ping_timeout(node_t *node, node_t *dst_node)
{
    rs_debug(DEBUG_ICMP, "ICMP Echo Request for '%s' timed out", phy_node_get_name(dst_node));

    icmp_add_ping_measure(node, dst_node, TRUE);
}
