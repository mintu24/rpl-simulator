
#include <unistd.h>

#include "node.h"
#include "system.h"


typedef struct {

    node_schedule_func_t func;
    void *data;
    bool executed;

} execute_wrapper_data_t;


static void *                   node_dequeue_pdu(node_t *node);
static bool                     node_process_message(node_t *node, phy_pdu_t *message);

static void *                   node_life_core(node_t *node);

static node_schedule_t *        node_schedule_create(char *name, node_schedule_func_t func, void *data, uint32 usecs, bool recurrent);
static bool                     node_schedule_destroy(node_schedule_t *schedule);

static void                     execute_wrapper(node_t *node, void *data);


node_t *node_create()
{
    node_t *node = (node_t *) malloc(sizeof(node_t));

    node->phy_info = NULL;
    node->mac_info = NULL;
    node->ip_info = NULL;
    node->rpl_info = NULL;

    node->life = NULL;
    node->alive = FALSE;

    node->schedules = g_hash_table_new(g_str_hash, g_str_equal);
    node->schedule_timer = g_timer_new();

    node->pdu_queue = g_queue_new();
    node->pdu_cond = g_cond_new();

    node->life_mutex = g_mutex_new();
    node->schedule_mutex = g_mutex_new();
    node->proto_info_mutex = g_mutex_new();
    node->pdu_mutex = g_mutex_new();

    return node;
}

bool node_destroy(node_t *node)
{
    rs_assert(node != NULL);

    if (node->alive)
        node_kill(node);

    if (node->schedules != NULL)
        g_hash_table_destroy(node->schedules);
    if (node->schedule_timer != NULL)
        g_timer_destroy(node->schedule_timer);

    if (node->pdu_queue != NULL)
        g_queue_free(node->pdu_queue);

    if (node->pdu_cond)
        g_cond_free(node->pdu_cond);

    if (node->life_mutex)
        g_mutex_free(node->life_mutex);
    if (node->schedule_mutex)
        g_mutex_free(node->schedule_mutex);
    if (node->proto_info_mutex)
        g_mutex_free(node->proto_info_mutex);
    if (node->pdu_mutex)
        g_mutex_free(node->pdu_mutex);

    if (node->phy_info != NULL) {
        phy_node_info_destroy(node->phy_info);
    }

    if (node->mac_info != NULL) {
        mac_node_info_destroy(node->mac_info);
    }

    if (node->ip_info != NULL) {
        ip_node_info_destroy(node->ip_info);
    }

    if (node->rpl_info != NULL) {
        rpl_node_info_destroy(node->rpl_info);
    }

    free(node);

    return TRUE;
}

bool node_start(node_t* node)
{
    rs_assert(!node->alive);
    rs_assert(node->life == NULL);

    g_mutex_lock(node->life_mutex);
    g_mutex_lock(node->schedule_mutex);
    g_mutex_lock(node->pdu_mutex);

    bool all_ok = TRUE;

    GError *error;
    node->life = g_thread_create((void *(*) (void *)) node_life_core, node, TRUE, &error);
    if (node->life == NULL) {
        rs_error("g_thread_create() failed: %s", error->message);
        all_ok = FALSE;
    }

    g_mutex_unlock(node->pdu_mutex);
    g_mutex_unlock(node->schedule_mutex);
    g_mutex_unlock(node->life_mutex);

    return all_ok;
}

bool node_kill(node_t* node)
{
    rs_assert(node != NULL);
    rs_assert(node->alive);

    g_mutex_lock(node->life_mutex);
    g_mutex_lock(node->schedule_mutex);
    g_mutex_lock(node->pdu_mutex);

    node->alive = FALSE;
    node->life = NULL;

    GList *schedule_list = g_hash_table_get_values(node->schedules);
    GList *schedule_element = g_list_first(schedule_list);
    while (schedule_element != NULL) {
        node_schedule_destroy(schedule_element->data);
        schedule_element = g_list_next(schedule_element);
    }
    g_list_free(schedule_list);

    while (!g_queue_is_empty(node->pdu_queue)) {
        phy_pdu_t *pdu = g_queue_pop_head(node->pdu_queue);
        phy_pdu_destroy(pdu);
    }

    g_mutex_unlock(node->pdu_mutex);
    g_mutex_unlock(node->schedule_mutex);
    g_mutex_unlock(node->life_mutex);

    return TRUE;
}

bool node_schedule(node_t *node, char *name, node_schedule_func_t func, void *data, uint32 usecs, bool recurrent)
{
    rs_assert(node != NULL);
    rs_assert(name != NULL);
    rs_assert(node->life != NULL);

    /* a non-NULL "life" field assures a previous call to node_start(),
    while a true "alive" field assures that the core entered its main loop */

    while (!node->alive) {
        usleep(10);
    }

    g_mutex_lock(node->schedule_mutex);

    node_schedule_t *new_schedule = NULL;
    if (func != NULL) {
        new_schedule = node_schedule_create(name, func, data, usecs, recurrent);
    }

    node_schedule_t *old_schedule = g_hash_table_lookup(node->schedules, name);
    if (old_schedule == NULL) { /* the schedule does not exist yet */
        if (new_schedule != NULL) {
            g_hash_table_insert(node->schedules, new_schedule->name, new_schedule);

            if (new_schedule->usecs > 0)
                rs_debug("scheduled action '%s' for node '%s' in %d milliseconds", new_schedule->name, phy_node_get_name(node), usecs);
//            else  <- annoying
//                rs_debug("scheduled action '%s' for node '%s'", new_schedule->name, node->phy_info->name, usecs);
        }
        else {
            rs_warn("no schedule '%s' to cancel", name);
        }
    }
    else { /* the schedule already exist */
        if (new_schedule != NULL) {
            g_hash_table_insert(node->schedules, new_schedule->name, new_schedule);

            rs_debug("rescheduled action '%s' for node '%s' in %d milliseconds", new_schedule->name, node->phy_info->name, usecs);
        }
        else {
            g_hash_table_remove(node->schedules, name);
            rs_debug("canceled scheduled action '%s' for node '%s' in %d milliseconds", name, node->phy_info->name, usecs);
        }

        node_schedule_destroy(old_schedule);
    }

    g_mutex_unlock(node->schedule_mutex);

    return TRUE;
}

bool node_execute(node_t *node, char *name, node_schedule_func_t func, void *data, bool blocking)
{
    rs_assert(node != NULL);
    rs_assert(func != NULL);

    if (blocking) {

        if (g_thread_self() == node->life) { /* the call is being made from the node's thread */
            // rs_debug("executing action '%s' for node '%s'", name, node->phy_info->name); <- annoying
            func(node, data);
        }
        else {
            execute_wrapper_data_t *execute_data = (execute_wrapper_data_t *) malloc(sizeof(execute_wrapper_data_t));

            execute_data->func = func;
            execute_data->data = data;
            execute_data->executed = FALSE;

            node_schedule(node, name, execute_wrapper, execute_data, 0, FALSE);

            while (!execute_data->executed) {
                usleep(NODE_LIFE_CORE_SLEEP);
            }
        }

    }
    else { /* no blocking */

        execute_wrapper_data_t *execute_data = (execute_wrapper_data_t *) malloc(sizeof(execute_wrapper_data_t));

        execute_data->func = func;
        execute_data->data = data;
        execute_data->executed = FALSE;

        node_schedule(node, name, execute_wrapper, execute_data, 0, FALSE);

    }

    return TRUE;
}

bool node_enqueue_pdu(node_t *node, void *pdu, uint8 phy_transmit_mode)
{
    rs_assert(pdu != NULL);
    rs_assert(node != NULL);

    g_mutex_lock(node->pdu_mutex);

    bool all_ok = TRUE;

    switch (phy_transmit_mode) {

        case PHY_TRANSMIT_MODE_BLOCK:
            while (!g_queue_is_empty(node->pdu_queue)) {
                g_cond_wait(node->pdu_cond, node->pdu_mutex);
            }

            g_queue_push_tail(node->pdu_queue, pdu);

            break;

        case PHY_TRANSMIT_MODE_REJECT:
            if (g_queue_is_empty(node->pdu_queue)) {
                g_queue_push_tail(node->pdu_queue, pdu);
            }
            else {
                all_ok = FALSE;
            }

            break;

        case PHY_TRANSMIT_MODE_QUEUE:
            g_queue_push_tail(node->pdu_queue, pdu);

            break;

        default:
            rs_error("invalid phy transmit mode: %d", phy_transmit_mode);
            all_ok = FALSE;
    }

    g_mutex_unlock(node->pdu_mutex);

    rs_debug("enqueued message for node '%s'", phy_node_get_name(node));

    return all_ok;
}


static void *node_dequeue_pdu(node_t *node)
{
    rs_assert(node != NULL);

    g_mutex_lock(node->pdu_mutex);

    void *pdu = g_queue_pop_head(node->pdu_queue);
    if (pdu != NULL) {
        rs_debug("dequeued message by node '%s'", phy_node_get_name(node));
    }

    g_cond_signal(node->pdu_cond);

    g_mutex_unlock(node->pdu_mutex);

    return pdu;
}

static bool node_process_message(node_t *node, phy_pdu_t *message)
{
    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);
    rs_assert(message != NULL);

    phy_pdu_t *phy_pdu = message;
    if (!node_execute(node, "phy_event_after_pdu_received", (node_schedule_func_t) phy_event_after_pdu_received, phy_pdu, TRUE)) {
        rs_error("failed to execute phy_event_after_pdu_received()");
        return FALSE;
    }

    mac_pdu_t *mac_pdu = phy_pdu->sdu;
    rs_assert(mac_pdu != NULL);
    if (!node_execute(node, "mac_event_after_pdu_received", (node_schedule_func_t) mac_event_after_pdu_received, mac_pdu, TRUE)) {
        rs_error("failed to execute mac_event_after_pdu_received()");
        return FALSE;
    }

    switch (mac_pdu->type) {

        case MAC_TYPE_IP : {
            ip_pdu_t *ip_pdu = mac_pdu->sdu;
            rs_assert(ip_pdu != NULL);
            if (!node_execute(node, "ip_event_after_pdu_received", (node_schedule_func_t) ip_event_after_pdu_received, ip_pdu, TRUE)) {
                rs_error("failed to execute ip_event_after_pdu_received()");
                return FALSE;
            }

            switch (ip_pdu->next_header) {

                case IP_NEXT_HEADER_ICMP: {
                    icmp_pdu_t *icmp_pdu = ip_pdu->sdu;
                    rs_assert(icmp_pdu != NULL);
                    if (!node_execute(node, "icmp_event_after_pdu_received", (node_schedule_func_t) icmp_event_after_pdu_received, icmp_pdu, TRUE)) {
                        rs_error("failed to execute icmp_event_after_pdu_received()");
                        return FALSE;
                    }

                    switch (icmp_pdu->type) {

                        case ICMP_TYPE_RPL:
                            switch (icmp_pdu->code) {

                                case ICMP_CODE_DIS: {
                                    rs_assert(icmp_pdu->sdu == NULL);
                                    if (!node_execute(node, "rpl_event_after_dis_pdu_received", (node_schedule_func_t) rpl_event_after_dis_pdu_received, NULL, TRUE)) {
                                        rs_error("failed to execute rpl_event_after_dis_pdu_received()");
                                        return FALSE;
                                    }

                                    break;
                                }

                                case ICMP_CODE_DIO: {
                                    rpl_dio_pdu_t *rpl_dio_pdu = icmp_pdu->sdu;
                                    rs_assert(rpl_dio_pdu != NULL);
                                    if (!node_execute(node, "rpl_event_after_dio_pdu_received", (node_schedule_func_t) rpl_event_after_dio_pdu_received, rpl_dio_pdu, TRUE)) {
                                        rs_error("failed to execute rpl_event_after_dio_pdu_received()");
                                        return FALSE;
                                    }

                                    break;
                                }

                                case ICMP_CODE_DAO: {
                                    rpl_dao_pdu_t *rpl_dao_pdu = icmp_pdu->sdu;
                                    rs_assert(rpl_dao_pdu != NULL);
                                    if (!node_execute(node, "rpl_event_after_dao_pdu_received", (node_schedule_func_t) rpl_event_after_dao_pdu_received, rpl_dao_pdu, TRUE)) {
                                        rs_error("failed to execute rpl_event_after_dao_pdu_received()");
                                        return FALSE;
                                    }

                                    break;
                                }

                                default:
                                    rs_error("unknown icmp code '0x%02X'", icmp_pdu->code);
                                    return FALSE;
                            }

                            break;

                        default:
                            rs_error("unknown icmp type '0x%02X'", icmp_pdu->type);
                            return FALSE;
                    }

                    break;
                }

                default:
                    rs_error("unknown ip next header '0x%04X'");
                    return FALSE;
            }

            break;
        }

        default:
            rs_error("unknown mac type '0x%04X'", mac_pdu->type);
            return FALSE;
    }

    return TRUE;
}

static void *node_life_core(node_t *node)
{
    /* wait until the creation routine exists */
    g_mutex_lock(node->life_mutex);
    g_mutex_unlock(node->life_mutex);

    rs_assert(node != NULL);
    rs_assert(node->alive == FALSE);

    rs_debug("node '%s' life started", phy_node_get_name(node));

    node->alive = TRUE;

    while (node->alive) {
        g_timer_reset(node->schedule_timer);

        usleep(NODE_LIFE_CORE_SLEEP);

        g_mutex_lock(node->schedule_mutex);

        /* what if the node was killed while usleep()-ing? */
        if (!node->alive) {
            break;
        }

        uint32 elapsed_usecs = 1000000 * g_timer_elapsed(node->schedule_timer, NULL);

        /* update all the schedules' remaining time, and execute if necessary */
        GList *schedule_list = g_hash_table_get_values(node->schedules);
        GList *schedule_element = g_list_first(schedule_list);
        while (schedule_element != NULL) {
            node_schedule_t *schedule = schedule_element->data;

            int32 new_remaining_usecs = schedule->remaining_usecs - elapsed_usecs;
            if (new_remaining_usecs > 0) {
                schedule->remaining_usecs = new_remaining_usecs;
            }
            else {
                schedule->remaining_usecs = 0;
                schedule->func(node, schedule->data);

                /* remove the finished and non-recurrent schedules */
                if (schedule->recurrent) {
                    schedule->remaining_usecs = schedule->usecs;
                }
                else {
                    if (!g_hash_table_remove(node->schedules, schedule->name)) {
                        rs_error("failed to remove dead schedule '%s'", schedule->name);
                    }
                }
            }

            schedule_element = g_list_next(schedule_element);
        }

        g_list_free(schedule_list);

        g_mutex_unlock(node->schedule_mutex);


        /* process a possibly received message */

        phy_pdu_t *message = node_dequeue_pdu(node);
        if (message != NULL) {
            node_process_message(node, message);
        }
    }

    rs_debug("node '%s' life stopped", node->phy_info->name);

    g_thread_exit(NULL);

    return NULL;
}


static node_schedule_t *node_schedule_create(char *name, node_schedule_func_t func, void *data, uint32 usecs, bool recurrent)
{
    node_schedule_t *schedule = (node_schedule_t *) malloc(sizeof(node_schedule_t));

    schedule->name = strdup(name);
    schedule->func = func;
    schedule->data = data;
    schedule->usecs = usecs;
    schedule->remaining_usecs = usecs;
    schedule->recurrent = recurrent;

    return schedule;
}


static bool node_schedule_destroy(node_schedule_t *schedule)
{
    rs_assert(schedule != NULL);

    if (schedule->name != NULL)
        free(schedule->name);

    free(schedule);

    return TRUE;
}

static void execute_wrapper(node_t *node, void *data)
{
    rs_assert(node != NULL);

    execute_wrapper_data_t *execute_data = data;

    execute_data->func(node, execute_data->data);
    execute_data->executed = TRUE;

    free(execute_data);
}