
#include "measure.h"
#include "system.h"


    /**** global variables ****/

static measure_connect_t *          measure_connect_list = NULL;
static uint16                       measure_connect_count = 0;

static measure_sp_comp_t *          measure_sp_comp_list = NULL;
static uint16                       measure_sp_comp_count = 0;

static measure_converg_t            measure_converg;

static measure_stat_t *             measure_stat_list = NULL;
static uint16                       measure_stat_count = 0;


    /**** local function prototypes ****/

static measure_connect_output_t     measure_connect_compute_output(measure_connect_t *measure);
static measure_sp_comp_output_t     measure_sp_comp_compute_output(measure_sp_comp_t *measure);
static measure_converg_output_t     measure_converg_compute_output();
static measure_stat_output_t        measure_stat_compute_output(measure_stat_t *measure);


    /**** exported functios ****/

bool measure_init()
{
    measure_connect_reset_output();
    measure_sp_comp_reset_output();
    measure_converg_reset_output();
    measure_stat_reset_output();

    return TRUE;
}

bool measure_done()
{
    return TRUE;
}

void measure_node_init(node_t *node)
{
    rs_assert(node != NULL);

    /* statistics */
    node->measure_info = malloc(sizeof(measure_node_info_t));

    node->measure_info->forward_error_count = 0;
    node->measure_info->forward_failure_count = 0;
    node->measure_info->rpl_event_count = 0;
    node->measure_info->rpl_r_dis_message_count = 0;
    node->measure_info->rpl_r_dio_message_count = 0;
    node->measure_info->rpl_r_dao_message_count = 0;
    node->measure_info->rpl_s_dis_message_count = 0;
    node->measure_info->rpl_s_dio_message_count = 0;
    node->measure_info->rpl_s_dao_message_count = 0;
    node->measure_info->ping_total_count = 0;
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

void measure_node_add_forward_error(node_t *node)
{
    rs_assert(node != NULL);

    node->measure_info->forward_error_count++;
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

void measure_node_add_ping(node_t *node, bool timeout)
{
    rs_assert(node != NULL);

    if (timeout)
        node->measure_info->ping_timeout_count++;
    else
        node->measure_info->ping_total_count++;

}

void measure_node_reset(node_t *node)
{
    rs_assert(node != NULL);

    node->measure_info->forward_error_count = 0;
    node->measure_info->forward_failure_count = 0;
    node->measure_info->rpl_event_count = 0;
    node->measure_info->rpl_r_dis_message_count = 0;
    node->measure_info->rpl_r_dio_message_count = 0;
    node->measure_info->rpl_r_dao_message_count = 0;
    node->measure_info->rpl_s_dis_message_count = 0;
    node->measure_info->rpl_s_dio_message_count = 0;
    node->measure_info->rpl_s_dao_message_count = 0;
    node->measure_info->ping_total_count = 0;
    node->measure_info->ping_timeout_count = 0;
}

void measure_connect_entry_add(node_t *src_node, node_t *dst_node)
{
    measures_lock();

    measure_connect_list = realloc(measure_connect_list, (measure_connect_count + 1) * sizeof(measure_connect_t));

    measure_connect_list[measure_connect_count].src_node = src_node;
    measure_connect_list[measure_connect_count].dst_node = dst_node;
    measure_connect_list[measure_connect_count].last_connected_event_time = -1;
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


void measure_connect_reset_output()
{
    measures_lock();

    uint16 i;
    for (i = 0; i < measure_connect_count; i++) {
        measure_connect_t *measure = &measure_connect_list[i];

        measure->output.connected_time = 0;
        measure->output.total_time = 0;
        measure->output.measure_time = 0;
    }

    measures_unlock();
}

void measure_connect_update_output()
{
    measures_lock();

    uint16 i;
    for (i = 0; i < measure_connect_count; i++) {
        measure_connect_t *measure = &measure_connect_list[i];

        measure->output = measure_connect_compute_output(measure);
    }

    measures_unlock();
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
        measure->output = measure_sp_comp_compute_output(measure);
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

    measure_converg.output = measure_converg_compute_output();

    measures_unlock();
}


void measure_stat_entry_add(node_t *node, uint8 type)
{
    measures_lock();

    measure_stat_list = realloc(measure_stat_list, (measure_stat_count + 1) * sizeof(measure_stat_t));

    measure_stat_list[measure_stat_count].node = node;
    measure_stat_list[measure_stat_count].type = type;
    measure_stat_list[measure_stat_count].output.forward_error_count = 0;
    measure_stat_list[measure_stat_count].output.forward_failure_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_event_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_r_dis_message_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_r_dio_message_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_r_dao_message_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_s_dis_message_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_s_dio_message_count = 0;
    measure_stat_list[measure_stat_count].output.rpl_s_dao_message_count = 0;
    measure_stat_list[measure_stat_count].output.ping_total_count = 0;
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

        measure->output.forward_error_count = 0;
        measure->output.forward_failure_count = 0;
        measure->output.rpl_event_count = 0;
        measure->output.rpl_r_dis_message_count = 0;
        measure->output.rpl_r_dio_message_count = 0;
        measure->output.rpl_r_dao_message_count = 0;
        measure->output.rpl_s_dis_message_count = 0;
        measure->output.rpl_s_dio_message_count = 0;
        measure->output.rpl_s_dao_message_count = 0;
        measure->output.ping_total_count = 0;
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

        measure->output = measure_stat_compute_output(measure);
    }

    measures_unlock();
}


    /**** local functions ****/

static measure_connect_output_t measure_connect_compute_output(measure_connect_t *measure)
{
    measure_connect_output_t output;

    output.connected_time = 0;
    output.total_time = rs_system->now;
    output.measure_time = rs_system->now;

    return output;
}

static measure_sp_comp_output_t measure_sp_comp_compute_output(measure_sp_comp_t *measure)
{
    measure_sp_comp_output_t output;

    output.rpl_cost = 0;
    output.sp_cost = 0;
    output.measure_time = rs_system->now;

    return output;
}

static measure_converg_output_t measure_converg_compute_output()
{
    measure_converg_output_t output;

    nodes_lock();
    events_lock();

    output.total_node_count = rs_system->node_count;

    output.connected_node_count = 0;

    uint16 i;
    output.stable_node_count = 0;
    output.floating_node_count = 0;
    for (i = 0; i < rs_system->node_count; i++) {
        node_t *node = rs_system->node_list[i];

        if (!node->alive) {
            continue;
        }

        if (rpl_node_is_root(node)) {
            if (node->rpl_info->trickle_i_doublings_so_far == node->rpl_info->root_info->dio_interval_doublings) {
                output.stable_node_count++;
            }
            if (!node->rpl_info->root_info->grounded) {
                output.floating_node_count++;
            }
        }
        else if (rpl_node_is_joined(node)) {
            if (node->rpl_info->trickle_i_doublings_so_far == node->rpl_info->joined_dodag->dio_interval_doublings) {
                output.stable_node_count++;
            }
        }
        else { /* node is isolated, thus considered stable */
            output.stable_node_count++;
        }
    }

    output.measure_time = rs_system->now;

    events_unlock();
    nodes_unlock();

    return output;
}

static measure_stat_output_t measure_stat_compute_output(measure_stat_t *measure)
{
    measure_stat_output_t output;

    if (measure->type == MEASURE_STAT_TYPE_NODE) {
        output.forward_error_count = measure->node->measure_info->forward_error_count;
        output.forward_failure_count = measure->node->measure_info->forward_failure_count;
        output.rpl_event_count = measure->node->measure_info->rpl_event_count;
        output.rpl_r_dis_message_count = measure->node->measure_info->rpl_r_dis_message_count;
        output.rpl_r_dio_message_count = measure->node->measure_info->rpl_r_dio_message_count;
        output.rpl_r_dao_message_count = measure->node->measure_info->rpl_r_dao_message_count;
        output.rpl_s_dis_message_count = measure->node->measure_info->rpl_s_dis_message_count;
        output.rpl_s_dio_message_count = measure->node->measure_info->rpl_s_dio_message_count;
        output.rpl_s_dao_message_count = measure->node->measure_info->rpl_s_dao_message_count;
        output.ping_total_count = measure->node->measure_info->ping_total_count;
        output.ping_timeout_count = measure->node->measure_info->ping_timeout_count;
    }
    else {
        output.forward_error_count = 0;
        output.forward_failure_count = 0;
        output.rpl_event_count = 0;
        output.rpl_r_dis_message_count = 0;
        output.rpl_r_dio_message_count = 0;
        output.rpl_r_dao_message_count = 0;
        output.rpl_s_dis_message_count = 0;
        output.rpl_s_dio_message_count = 0;
        output.rpl_s_dao_message_count = 0;
        output.ping_total_count = 0;
        output.ping_timeout_count = 0;

        nodes_lock();

        uint16 i;
        for (i = 0; i < rs_system->node_count; i++) {
            node_t *node = rs_system->node_list[i];

            output.forward_error_count += node->measure_info->forward_error_count;
            output.forward_failure_count += node->measure_info->forward_failure_count;
            output.rpl_event_count += node->measure_info->rpl_event_count;
            output.rpl_r_dis_message_count += node->measure_info->rpl_r_dis_message_count;
            output.rpl_r_dio_message_count += node->measure_info->rpl_r_dio_message_count;
            output.rpl_r_dao_message_count += node->measure_info->rpl_r_dao_message_count;
            output.rpl_s_dis_message_count += node->measure_info->rpl_s_dis_message_count;
            output.rpl_s_dio_message_count += node->measure_info->rpl_s_dio_message_count;
            output.rpl_s_dao_message_count += node->measure_info->rpl_s_dao_message_count;
            output.ping_total_count += node->measure_info->ping_total_count;
            output.ping_timeout_count += node->measure_info->ping_timeout_count;
        }

        nodes_unlock();

        if (measure->type == MEASURE_STAT_TYPE_AVG) {
            output.forward_error_count /= rs_system->node_count;
            output.forward_failure_count /= rs_system->node_count;
            output.rpl_event_count /= rs_system->node_count;
            output.rpl_r_dis_message_count /= rs_system->node_count;
            output.rpl_r_dio_message_count /= rs_system->node_count;
            output.rpl_r_dao_message_count /= rs_system->node_count;
            output.rpl_s_dis_message_count /= rs_system->node_count;
            output.rpl_s_dio_message_count /= rs_system->node_count;
            output.rpl_s_dao_message_count /= rs_system->node_count;
            output.ping_total_count /= rs_system->node_count;
            output.ping_timeout_count /= rs_system->node_count;
        }
    }

    output.measure_time = rs_system->now;

    return output;
}
