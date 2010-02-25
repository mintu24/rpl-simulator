
#include "measure.h"
#include "../system.h"


    /**** global variables ****/

uint16                              measure_event_node_wake;
uint16                              measure_event_node_kill;

uint16                              measure_event_pdu_send;
uint16                              measure_event_pdu_receive;

uint16                              measure_event_connect_update;
uint16                              measure_event_connect_hop_passed;
uint16                              measure_event_connect_hop_failed;
uint16                              measure_event_connect_hop_timeout;
uint16                              measure_event_connect_established;
uint16                              measure_event_connect_lost;

static measure_sp_comp_t *          measure_sp_comp_list = NULL;
static uint16                       measure_sp_comp_count = 0;

static measure_converg_t            measure_converg;

static measure_stat_t *             measure_stat_list = NULL;
static uint16                       measure_stat_count = 0;


    /**** local function prototypes ****/

static void                         measure_sp_comp_compute_output(measure_sp_comp_t *measure);
static void                         measure_converg_compute_output(measure_converg_t *measure);
static void                         measure_stat_compute_output(measure_stat_t *measure);

static bool                         event_handler_node_wake(node_t *node);
static bool                         event_handler_node_kill(node_t *node);

static bool                         event_handler_pdu_send(node_t *node, char *dst_ip_address, measure_pdu_t *pdu);
static bool                         event_handler_pdu_receive(node_t *node, node_t *incoming_node, measure_pdu_t *pdu);

static bool                         event_handler_connect_update(node_t *node, node_t *dst_node);
static bool                         event_handler_connect_hop_passed(node_t *node, node_t *dst_node, node_t *hop);
static bool                         event_handler_connect_hop_failed(node_t *node, node_t *dst_node, node_t *hop);
static bool                         event_handler_connect_hop_timeout(node_t *node, node_t *dst_node, node_t *last_hop);
static bool                         event_handler_connect_established(node_t *node, node_t *dst_node);
static bool                         event_handler_connect_lost(node_t *node, node_t *dst_node, node_t *last_hop);

static void                         event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len);


    /**** exported functions ****/

bool measure_init()
{
    measure_sp_comp_reset_output();
    measure_converg_reset_output();
    measure_stat_reset_output();

    measure_event_node_wake = event_register("node_wake", "measure", (event_handler_t) event_handler_node_wake, NULL);
    measure_event_node_kill = event_register("node_kill", "measure", (event_handler_t) event_handler_node_kill, NULL);

    measure_event_pdu_send = event_register("pdu_send", "measure", (event_handler_t) event_handler_pdu_send, event_arg_str);
    measure_event_pdu_receive = event_register("pdu_receive", "measure", (event_handler_t) event_handler_pdu_receive, event_arg_str);

    measure_event_connect_update = event_register("connect_update", "measure", (event_handler_t) event_handler_connect_update, NULL);
    measure_event_connect_hop_passed = event_register("connect_hop_passed", "measure", (event_handler_t) event_handler_connect_hop_passed, event_arg_str);
    measure_event_connect_hop_failed = event_register("connect_hop_failed", "measure", (event_handler_t) event_handler_connect_hop_failed, event_arg_str);
    measure_event_connect_hop_timeout = event_register("connect_hop_timeout", "measure", (event_handler_t) event_handler_connect_hop_timeout, event_arg_str);
    measure_event_connect_established = event_register("connect_established", "measure", (event_handler_t) event_handler_connect_established, event_arg_str);
    measure_event_connect_lost = event_register("connect_lost", "measure", (event_handler_t) event_handler_connect_lost, event_arg_str);

    return TRUE;
}

bool measure_done()
{
    return TRUE;
}

measure_pdu_t *measure_pdu_create(node_t *node, node_t *dst_node, uint8 type)
{
    measure_pdu_t *pdu = malloc(sizeof(measure_pdu_t));

    pdu->measuring_node = node;
    pdu->dst_node = dst_node;
    pdu->type = type;

    return pdu;
}

void measure_pdu_destroy(measure_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    free(pdu);
}

measure_pdu_t *measure_pdu_duplicate(measure_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    measure_pdu_t *new_pdu = malloc(sizeof(measure_pdu_t));

    new_pdu->measuring_node = pdu->measuring_node;
    new_pdu->dst_node = pdu->dst_node;
    new_pdu->type = pdu->type;

    return new_pdu;
}

void measure_node_init(node_t *node)
{
    rs_assert(node != NULL);

    node->measure_info = malloc(sizeof(measure_node_info_t));

    /* connectivity */
    node->measure_info->connect_busy = FALSE;
    node->measure_info->connect_dst_node = NULL;
    node->measure_info->connect_dst_reachable = FALSE;

    /* statistics */
    node->measure_info->forward_inconsistency_count = 0;
    node->measure_info->forward_failure_count = 0;
    node->measure_info->rpl_event_count = 0;
    node->measure_info->rpl_r_dis_message_count = 0;
    node->measure_info->rpl_r_dio_message_count = 0;
    node->measure_info->rpl_r_dao_message_count = 0;
    node->measure_info->rpl_s_dis_message_count = 0;
    node->measure_info->rpl_s_dio_message_count = 0;
    node->measure_info->rpl_s_dao_message_count = 0;
    node->measure_info->ping_successful_count = 0;
    node->measure_info->ping_timeout_count = 0;
}

void measure_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->measure_info != NULL) {
        free(node->measure_info);
        node->measure_info = NULL;
    }
}

bool measure_send(node_t *node, node_t *dst_node, uint8 type)
{
    rs_assert(node != NULL);
    rs_assert(dst_node != NULL);

    measure_pdu_t *measure_pdu = measure_pdu_create(node, dst_node, type);

    if (!event_execute(measure_event_pdu_send, node, dst_node->ip_info->address, measure_pdu)) {
        measure_pdu_destroy(measure_pdu);
        return FALSE;
    }

    return TRUE;
}

bool measure_receive(node_t *node, node_t *incoming_node, measure_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    bool all_ok = event_execute(measure_event_pdu_receive, node, incoming_node, pdu);

    measure_pdu_destroy(pdu);

    return all_ok;
}

void measure_node_add_forward_inconsistency(node_t *node)
{
    rs_assert(node != NULL);

    node->measure_info->forward_inconsistency_count++;
}

void measure_node_add_forward_failure(node_t *node)
{
    rs_assert(node != NULL);

    node->measure_info->forward_failure_count++;
}

void measure_node_add_rpl_event(node_t *node)
{
    rs_assert(node != NULL);

    node->measure_info->rpl_event_count++;
}

void measure_node_add_rpl_dis_message(node_t *node, bool sent)
{
    rs_assert(node != NULL);

    if (sent)
        node->measure_info->rpl_s_dis_message_count++;
    else
        node->measure_info->rpl_r_dis_message_count++;
}

void measure_node_add_rpl_dio_message(node_t *node, bool sent)
{
    rs_assert(node != NULL);

    if (sent)
        node->measure_info->rpl_s_dio_message_count++;
    else
        node->measure_info->rpl_r_dio_message_count++;
}

void measure_node_add_rpl_dao_message(node_t *node, bool sent)
{
    rs_assert(node != NULL);

    if (sent)
        node->measure_info->rpl_s_dao_message_count++;
    else
        node->measure_info->rpl_r_dao_message_count++;
}


void measure_node_add_ping(node_t *node, bool successful)
{
    rs_assert(node != NULL);

    if (successful)
        node->measure_info->ping_successful_count++;
    else
        node->measure_info->ping_timeout_count++;

}

void measure_node_reset(node_t *node)
{
    rs_assert(node != NULL);

    node->measure_info->connect_busy = FALSE;
    node->measure_info->connect_dst_reachable = FALSE;
    node->measure_info->connect_start_time = -1;
    node->measure_info->connect_connected_time = 0;

    node->measure_info->forward_inconsistency_count = 0;
    node->measure_info->forward_failure_count = 0;
    node->measure_info->rpl_event_count = 0;
    node->measure_info->rpl_r_dis_message_count = 0;
    node->measure_info->rpl_r_dio_message_count = 0;
    node->measure_info->rpl_r_dao_message_count = 0;
    node->measure_info->rpl_s_dis_message_count = 0;
    node->measure_info->rpl_s_dio_message_count = 0;
    node->measure_info->rpl_s_dao_message_count = 0;
    node->measure_info->ping_successful_count = 0;
    node->measure_info->ping_timeout_count = 0;
}
/*
void measure_connect_entry_add(node_t *src_node, node_t *dst_node)
{
    measures_lock();

    measure_connect_list = realloc(measure_connect_list, (measure_connect_count + 1) * sizeof(measure_connect_t));

    measure_connect_list[measure_connect_count].src_node = src_node;
    measure_connect_list[measure_connect_count].dst_node = dst_node;
    measure_connect_list[measure_connect_count].last_connected_time = -1;
    measure_connect_list[measure_connect_count].start_time = -1;
    measure_connect_list[measure_connect_count].output.connected_time = 0;
    measure_connect_list[measure_connect_count].output.total_time = 0;
    measure_connect_list[measure_connect_count].output.measure_time = 0;

    measure_connect_count++;

    measures_unlock();
}


void measure_connect_entry_remove(uint16 index)
{
    measures_lock();

    rs_assert(index < measure_connect_count);

    uint16 i;
    for (i = index; i < measure_connect_count - 1; i++) {
        measure_connect_list[i] = measure_connect_list[i + 1];
    }

    measure_connect_count--;
    measure_connect_list = realloc(measure_connect_list, measure_connect_count * sizeof(measure_connect_t));
    if (measure_connect_count == 0) {
        measure_connect_list = NULL;
    }

    measures_unlock();
}


void measure_connect_entry_remove_all()
{
    measures_lock();

    if (measure_connect_list != NULL) {
        free(measure_connect_list);
        measure_connect_list = NULL;
    }

    measure_connect_count = 0;

    measures_unlock();
}


uint16 measure_connect_entry_get_count()
{
    return measure_connect_count;
}


measure_connect_t *measure_connect_entry_get(uint16 index)
{
    rs_assert(index < measure_connect_count);

    return &measure_connect_list[index];
}


void measure_connect_reset_all()
{
    measures_lock();

    uint16 i;
    for (i = 0; i < measure_connect_count; i++) {
        measure_connect_t *measure = &measure_connect_list[i];

        measure->connected_time = 0;
        measure->start_time = -1;
    }

    measures_unlock();
}
*/
void measure_node_connect_update(node_t *node)
{
    rs_assert(node != NULL);

    if (node->measure_info->connect_busy) {
        return;
    }

    if (node->measure_info->connect_dst_node == NULL) {
        return;
    }

    if (node->measure_info->connect_dst_node == node) {
        return;
    }

    rs_system_schedule_event(node, measure_event_connect_update, node->measure_info->connect_dst_node, NULL, 0);
}


void measure_sp_comp_entry_add(node_t *src_node, node_t *dst_node)
{
    measures_lock();

    measure_sp_comp_list = realloc(measure_sp_comp_list, (measure_sp_comp_count + 1) * sizeof(measure_sp_comp_t));

    measure_sp_comp_list[measure_sp_comp_count].src_node = src_node;
    measure_sp_comp_list[measure_sp_comp_count].dst_node = dst_node;
    measure_sp_comp_list[measure_sp_comp_count].output.rpl_cost = 0;
    measure_sp_comp_list[measure_sp_comp_count].output.sp_cost = 0;
    measure_sp_comp_list[measure_sp_comp_count].output.measure_time = 0;

    measure_sp_comp_count++;

    measures_unlock();
}


void measure_sp_comp_entry_remove(uint16 index)
{
    measures_lock();

    rs_assert(index < measure_sp_comp_count);

    uint16 i;
    for (i = index; i < measure_sp_comp_count - 1; i++) {
        measure_sp_comp_list[i] = measure_sp_comp_list[i + 1];
    }

    measure_sp_comp_count--;
    measure_sp_comp_list = realloc(measure_sp_comp_list, measure_sp_comp_count * sizeof(measure_sp_comp_t));
    if (measure_sp_comp_count == 0) {
        measure_sp_comp_list = NULL;
    }

    measures_unlock();
}


void measure_sp_comp_entry_remove_all()
{
    measures_lock();

    if (measure_sp_comp_list != NULL) {
        free(measure_sp_comp_list);
        measure_sp_comp_list = NULL;
    }

    measure_sp_comp_count = 0;

    measures_unlock();
}


uint16 measure_sp_comp_entry_get_count()
{
    return measure_sp_comp_count;
}


measure_sp_comp_t *measure_sp_comp_entry_get(uint16 index)
{
    rs_assert(index < measure_sp_comp_count);

    return &measure_sp_comp_list[index];
}


void measure_sp_comp_reset_output()
{
    measures_lock();

    uint16 i;
    for (i = 0; i < measure_sp_comp_count; i++) {
        measure_sp_comp_t *measure = &measure_sp_comp_list[i];
        measure->output.rpl_cost = 0;
        measure->output.sp_cost = 0;
        measure->output.measure_time = 0;
    }

    measures_unlock();
}


void measure_sp_comp_update_output()
{
    measures_lock();

    uint16 i;
    for (i = 0; i < measure_sp_comp_count; i++) {
        measure_sp_comp_t *measure = &measure_sp_comp_list[i];
        measure_sp_comp_compute_output(measure);
    }

    measures_unlock();
}


measure_converg_t *measure_converg_entry_get()
{
    return &measure_converg;
}


void measure_converg_reset_output()
{
    measures_lock();

    measure_converg.output.total_node_count = 0;
    measure_converg.output.connected_node_count = 0;
    measure_converg.output.stable_node_count = 0;
    measure_converg.output.floating_node_count = 0;
    measure_converg.output.measure_time = 0;

    measures_unlock();
}


void measure_converg_update_output()
{
    measures_lock();

    measure_converg_compute_output(&measure_converg);

    measures_unlock();
}


void measure_stat_entry_add(node_t *node, uint8 type)
{
    measures_lock();

    measure_stat_list = realloc(measure_stat_list, (measure_stat_count + 1) * sizeof(measure_stat_t));

    measure_stat_list[measure_stat_count].node = node;
    measure_stat_list[measure_stat_count].type = type;
    measure_stat_list[measure_stat_count].output.forward_failure_count = 0;
    measure_stat_list[measure_stat_count].output.forward_error_inconsistency = 0;
    measure_stat_list[measure_stat_count].output.rpl_event_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_r_dis_message_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_r_dio_message_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_r_dao_message_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_s_dis_message_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_s_dio_message_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_s_dao_message_count = 0;
    measure_stat_list[measure_stat_count].output.ping_successful_count = 0;
    measure_stat_list[measure_stat_count].output.ping_timeout_count = 0;
    measure_stat_list[measure_stat_count].output.measure_time = 0;

    measure_stat_count++;

    measures_unlock();
}


void measure_stat_entry_remove(uint16 index)
{
    measures_lock();

    rs_assert(index < measure_stat_count);

    uint16 i;
    for (i = index; i < measure_stat_count - 1; i++) {
        measure_stat_list[i] = measure_stat_list[i + 1];
    }

    measure_stat_count--;
    measure_stat_list = realloc(measure_stat_list, measure_stat_count * sizeof(measure_stat_t));
    if (measure_stat_count == 0) {
        measure_stat_list = NULL;
    }

    measures_unlock();
}


void measure_stat_entry_remove_all()
{
    measures_lock();

    if (measure_stat_list != NULL) {
        free(measure_stat_list);
        measure_stat_list = NULL;
    }

    measure_stat_count = 0;

    measures_unlock();
}


uint16 measure_stat_entry_get_count()
{
    return measure_stat_count;
}

measure_stat_t *measure_stat_entry_get(uint16 index)
{
    rs_assert(index < measure_stat_count);

    return &measure_stat_list[index];
}

void measure_stat_reset_output()
{
    measures_lock();

    uint16 i;
    for (i = 0; i < measure_stat_count; i++) {
        measure_stat_t *measure = &measure_stat_list[i];

        measure->output.forward_failure_count = 0;
        measure->output.forward_error_inconsistency = 0;
        measure->output.rpl_event_count = 0;
        measure->output.rpl_r_dis_message_count = 0;
        measure->output.rpl_r_dio_message_count = 0;
        measure->output.rpl_r_dao_message_count = 0;
        measure->output.rpl_s_dis_message_count = 0;
        measure->output.rpl_s_dio_message_count = 0;
        measure->output.rpl_s_dao_message_count = 0;
        measure->output.ping_successful_count = 0;
        measure->output.ping_timeout_count = 0;
        measure->output.measure_time = 0;
    }

    measures_unlock();
}

void measure_stat_update_output(node_t *node)
{
    measures_lock();

    uint16 i;
    for (i = 0; i < measure_stat_count; i++) {
        measure_stat_t *measure = &measure_stat_list[i];

        if (measure->type == MEASURE_STAT_TYPE_NODE && node != NULL && measure->node != node) {
            continue;
        }

        measure_stat_compute_output(measure);
    }

    measures_unlock();
}


    /**** local functions ****/

static void measure_sp_comp_compute_output(measure_sp_comp_t *measure)
{
    measure->output.rpl_cost = 0;
    measure->output.sp_cost = 0;
    measure->output.measure_time = rs_system->now;
}

static void measure_converg_compute_output(measure_converg_t *measure)
{
    uint16 node_count;
    node_t **node_list = rs_system_get_node_list_copy(&node_count);

    events_lock();

    measure->output.total_node_count = rs_system->node_count;
    measure->output.connected_node_count = 0;

    uint16 i;
    measure->output.stable_node_count = 0;
    measure->output.floating_node_count = 0;
    for (i = 0; i < node_count; i++) {
        node_t *node = node_list[i];

        if (!node->alive) {
            continue;
        }

        if (rpl_node_is_root(node)) {
            if (node->rpl_info->trickle_i_doublings_so_far == node->rpl_info->root_info->dio_interval_doublings) {
                measure->output.stable_node_count++;
            }
            if (!node->rpl_info->root_info->grounded) {
                measure->output.floating_node_count++;
            }
        }
        else if (rpl_node_is_joined(node)) {
            if (node->rpl_info->trickle_i_doublings_so_far == node->rpl_info->joined_dodag->dio_interval_doublings) {
                measure->output.stable_node_count++;
            }
        }
        else { /* node is isolated, thus considered stable */
            measure->output.stable_node_count++;
        }
    }

    if (node_list != NULL) {
        free(node_list);
    }

    measure->output.measure_time = rs_system->now;

    events_unlock();
}

static void measure_stat_compute_output(measure_stat_t *measure)
{
    if (measure->type == MEASURE_STAT_TYPE_NODE) {
        measure->output.forward_error_inconsistency = measure->node->measure_info->forward_inconsistency_count;
        measure->output.forward_failure_count = measure->node->measure_info->forward_failure_count;
        measure->output.rpl_event_count = measure->node->measure_info->rpl_event_count;
        measure->output.rpl_r_dis_message_count = measure->node->measure_info->rpl_r_dis_message_count;
        measure->output.rpl_r_dio_message_count = measure->node->measure_info->rpl_r_dio_message_count;
        measure->output.rpl_r_dao_message_count = measure->node->measure_info->rpl_r_dao_message_count;
        measure->output.rpl_s_dis_message_count = measure->node->measure_info->rpl_s_dis_message_count;
        measure->output.rpl_s_dio_message_count = measure->node->measure_info->rpl_s_dio_message_count;
        measure->output.rpl_s_dao_message_count = measure->node->measure_info->rpl_s_dao_message_count;
        measure->output.ping_successful_count = measure->node->measure_info->ping_successful_count;
        measure->output.ping_timeout_count = measure->node->measure_info->ping_timeout_count;
    }
    else {
        measure->output.forward_error_inconsistency = 0;
        measure->output.forward_failure_count = 0;
        measure->output.rpl_event_count = 0;
        measure->output.rpl_r_dis_message_count = 0;
        measure->output.rpl_r_dio_message_count = 0;
        measure->output.rpl_r_dao_message_count = 0;
        measure->output.rpl_s_dis_message_count = 0;
        measure->output.rpl_s_dio_message_count = 0;
        measure->output.rpl_s_dao_message_count = 0;
        measure->output.ping_successful_count = 0;
        measure->output.ping_timeout_count = 0;

        uint16 node_count;
        node_t **node_list = rs_system_get_node_list_copy(&node_count);

        uint16 i;
        for (i = 0; i < node_count; i++) {
            node_t *node = node_list[i];

            measure->output.forward_error_inconsistency += node->measure_info->forward_inconsistency_count;
            measure->output.forward_failure_count += node->measure_info->forward_failure_count;
            measure->output.rpl_event_count += node->measure_info->rpl_event_count;
            measure->output.rpl_r_dis_message_count += node->measure_info->rpl_r_dis_message_count;
            measure->output.rpl_r_dio_message_count += node->measure_info->rpl_r_dio_message_count;
            measure->output.rpl_r_dao_message_count += node->measure_info->rpl_r_dao_message_count;
            measure->output.rpl_s_dis_message_count += node->measure_info->rpl_s_dis_message_count;
            measure->output.rpl_s_dio_message_count += node->measure_info->rpl_s_dio_message_count;
            measure->output.rpl_s_dao_message_count += node->measure_info->rpl_s_dao_message_count;
            measure->output.ping_successful_count += node->measure_info->ping_successful_count;
            measure->output.ping_timeout_count += node->measure_info->ping_timeout_count;
        }

        if (measure->type == MEASURE_STAT_TYPE_AVG) {
            measure->output.forward_error_inconsistency /= rs_system->node_count;
            measure->output.forward_failure_count /= rs_system->node_count;
            measure->output.rpl_event_count /= rs_system->node_count;
            measure->output.rpl_r_dis_message_count /= rs_system->node_count;
            measure->output.rpl_r_dio_message_count /= rs_system->node_count;
            measure->output.rpl_r_dao_message_count /= rs_system->node_count;
            measure->output.rpl_s_dis_message_count /= rs_system->node_count;
            measure->output.rpl_s_dio_message_count /= rs_system->node_count;
            measure->output.rpl_s_dao_message_count /= rs_system->node_count;
            measure->output.ping_successful_count /= rs_system->node_count;
            measure->output.ping_timeout_count /= rs_system->node_count;
        }

        if (node_list != NULL) {
            free(node_list);
        }
    }

    measure->output.measure_time = rs_system->now;
}

static bool event_handler_node_wake(node_t *node)
{
    return TRUE;
}

static bool event_handler_node_kill(node_t *node)
{
    // rs_system_cancel_event(node, icmp_event_ping_timeout, NULL, NULL, 0); todo cancel measure events

    return TRUE;
}

static bool event_handler_pdu_send(node_t *node, char *dst_ip_address, measure_pdu_t *pdu)
{
    return ip_send(node, dst_ip_address, IP_NEXT_HEADER_MEASURE, pdu);
}

static bool event_handler_pdu_receive(node_t *node, node_t *incoming_node, measure_pdu_t *pdu)
{
    switch (pdu->type) {

        case MEASURE_TYPE_CONNECT:
            pdu->measuring_node->measure_info->connect_busy = FALSE;
            rs_system_cancel_event(pdu->measuring_node, measure_event_connect_hop_timeout, pdu->dst_node, NULL, 0);

            if (!pdu->measuring_node->measure_info->connect_dst_reachable) { /* wasn't reachable before */

                return event_execute(measure_event_connect_established, pdu->measuring_node, pdu->dst_node, node);
            }

            break;
    }

    return TRUE;
}

static bool event_handler_connect_update(node_t *node, node_t *dst_node)
{
    if (measure_send(node, node->measure_info->connect_dst_node, MEASURE_TYPE_CONNECT)) {
        node->measure_info->connect_busy = TRUE;

        rs_system_cancel_event(node, measure_event_connect_hop_timeout, dst_node, NULL, 0); /* cancel all possible previous hop timeouts */
        rs_system_schedule_event(node, measure_event_connect_hop_timeout, node->measure_info->connect_dst_node, node, 1000); // todo make this configurable

        return TRUE;
    }
    else {
        node->measure_info->connect_busy = FALSE;
        node->measure_info->connect_dst_reachable = FALSE;

        return FALSE;
    }
}

static bool event_handler_connect_hop_passed(node_t *node, node_t *dst_node, node_t *hop)
{
    rs_system_cancel_event(node, measure_event_connect_hop_timeout, dst_node, NULL, 0);

    if (hop != dst_node) {
        rs_system_schedule_event(node, measure_event_connect_hop_timeout, dst_node, hop, 1000); // todo make this timeout configurable
    }

    return TRUE;
}

static bool event_handler_connect_hop_failed(node_t *node, node_t *dst_node, node_t *hop)
{
    rs_system_cancel_event(node, measure_event_connect_hop_timeout, dst_node, NULL, 0);

    node->measure_info->connect_busy = FALSE;

    if (node->measure_info->connect_dst_reachable) { /* was reachable before */
        return event_execute(measure_event_connect_lost, node, dst_node, hop);
    }

    return TRUE;
}

static bool event_handler_connect_hop_timeout(node_t *node, node_t *dst_node, node_t *last_hop)
{
    node->measure_info->connect_busy = FALSE;

    if (node->measure_info->connect_dst_reachable) { /* was reachable before */
        return event_execute(measure_event_connect_lost, node, dst_node, last_hop);
    }

    return TRUE;
}

static bool event_handler_connect_established(node_t *node, node_t *dst_node)
{
    node->measure_info->connect_dst_reachable = TRUE;

    return TRUE;
}

static bool event_handler_connect_lost(node_t *node, node_t *dst_node, node_t *last_hop)
{
    node->measure_info->connect_dst_reachable = FALSE;

    return FALSE;
}

static void event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len)
{
    str1[0] = '\0';
    str2[0] = '\0';

    if (event_id == measure_event_pdu_send) {
            char *dst_ip_address = data1;

            snprintf(str1, len, "dst = '%s'", dst_ip_address != NULL ? dst_ip_address : "<<broadcast>>");
    }
    else if (event_id == measure_event_pdu_receive) {
        node_t *node = data1;
        measure_pdu_t *measure_pdu = data2;

        snprintf(str1, len, "incoming_node = '%s'", (node != NULL ? node->phy_info->name : "<<unknown>>"));
        snprintf(str2, len, "measure_pdu = {measuring_node = '%s'}", measure_pdu->measuring_node->phy_info->name);
    }
    else if (event_id == measure_event_connect_hop_passed) {
        node_t *dst_node = data1;
        node_t *hop = data2;

        snprintf(str1, len, "dst = '%s'", dst_node != NULL ? dst_node->phy_info->name : "<<unknown>>");
        snprintf(str2, len, "hop = '%s'", hop != NULL ? hop->phy_info->name : "<<unknown>>");
    }
    else if (event_id == measure_event_connect_hop_failed) {
        node_t *dst_node = data1;
        node_t *hop = data2;

        snprintf(str1, len, "dst = '%s'", dst_node != NULL ? dst_node->phy_info->name : "<<unknown>>");
        snprintf(str2, len, "hop = '%s'", hop != NULL ? hop->phy_info->name : "<<unknown>>");
    }
    else if (event_id == measure_event_connect_hop_timeout) {
        node_t *dst_node = data1;
        node_t *last_hop = data2;

        snprintf(str1, len, "dst = '%s'", dst_node != NULL ? dst_node->phy_info->name : "<<unknown>>");
        snprintf(str2, len, "last_hop = '%s'", last_hop != NULL ? last_hop->phy_info->name : "<<unknown>>");
    }
    else if (event_id == measure_event_connect_established) {
        node_t *dst_node = data1;

        snprintf(str1, len, "dst = '%s'", dst_node != NULL ? dst_node->phy_info->name : "<<unknown>>");
    }
    else if (event_id == measure_event_connect_lost) {
        node_t *dst_node = data1;
        node_t *last_hop = data2;

        snprintf(str1, len, "dst = '%s'", dst_node != NULL ? dst_node->phy_info->name : "<<unknown>>");
        snprintf(str2, len, "last_hop = '%s'", last_hop != NULL ? last_hop->phy_info->name : "<<unknown>>");
    }
}
