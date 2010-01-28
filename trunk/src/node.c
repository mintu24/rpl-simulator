
#include <unistd.h>

#include "node.h"
#include "system.h"
#include "gui/mainwin.h"


typedef struct execute_event_wrapper_data_t {

    node_event_t            func;
    node_t *                node;
    void *                  data1;
    void *                  data2;

} execute_event_wrapper_data_t;


static phy_pdu_t *              node_dequeue_pdu(node_t *node);
static bool                     node_process_message(node_t *node, node_t *src_node, phy_pdu_t *message);

static void *                   node_life_core(node_t *node);

static node_schedule_t *        node_schedule_create(char *name, node_schedule_func_t func, void *data, uint32 usecs, bool recurrent);
static bool                     node_schedule_destroy(node_schedule_t *schedule);

static void                     execute_event_wrapper(node_t *node, void *data);


node_t *node_create()
{
    node_t *node = malloc(sizeof(node_t));

    node->phy_info = NULL;
    node->mac_info = NULL;
    node->ip_info = NULL;
    node->icmp_info = NULL;
    node->rpl_info = NULL;

    node->life = NULL;
    node->alive = FALSE;

    node->schedules = g_hash_table_new(g_str_hash, g_str_equal);
    node->schedule_timer = g_timer_new();

    node->pdu_queue = g_queue_new();
    node->pdu_cond = g_cond_new();

    node->life_mutex = g_mutex_new();
    node->pdu_mutex = g_mutex_new();

    return node;
}

bool node_destroy(node_t *node)
{
    rs_assert(node != NULL);

    if (node->alive) {
        node_kill(node);
    }

    /* wait until the node is really dead */
    g_mutex_lock(node->life_mutex);
    g_mutex_unlock(node->life_mutex);

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
    if (node->pdu_mutex)
        g_mutex_free(node->pdu_mutex);

    free(node);

    return TRUE;
}

bool node_wake(node_t* node)
{
    rs_assert(node != NULL);

    if (node->alive) {
        rs_error("node '%s' is already alive", phy_node_get_name(node));
        return FALSE;
    }

    bool all_ok = TRUE;

    GError *error = NULL;
    node->life = g_thread_create((void *(*) (void *)) node_life_core, node, TRUE, &error);
    if (node->life == NULL) {
        rs_error("g_thread_create() failed: %s", error->message);
        all_ok = FALSE;
    }

    return all_ok;
}

bool node_kill(node_t* node)
{
    rs_assert(node != NULL);

    if (!node->alive) {
        rs_error("node '%s' is already dead", phy_node_get_name(node));
        return FALSE;
    }

    node->alive = FALSE;

    return TRUE;
}

bool node_schedule(node_t *node, char *name, node_schedule_func_t func, void *data, uint32 usecs, bool recurrent)
{
    rs_assert(node != NULL);
    rs_assert(name != NULL);
    rs_assert(node->life == g_thread_self());

    node_schedule_t *old_schedule = g_hash_table_lookup(node->schedules, name);

    if (old_schedule == NULL) { /* the schedule does not exist yet */
        if (func == NULL) {
            rs_warn("no schedule '%s' to cancel", name);
        }
        else {
            node_schedule_t *new_schedule = node_schedule_create(name, func, data, usecs, recurrent);

            g_hash_table_insert(node->schedules, new_schedule->name, new_schedule);

            if (new_schedule->usecs > 0)
                rs_debug(DEBUG_NODE, "scheduled action '%s' for node '%s' in %d microseconds", new_schedule->name, phy_node_get_name(node), usecs);
        }
    }
    else { /* the schedule already exist */
        if (func != NULL) {
            old_schedule->func = func;
            old_schedule->data = data;
            old_schedule->usecs = usecs;
            old_schedule->remaining_usecs = usecs;
            old_schedule->recurrent = recurrent;

            rs_debug(DEBUG_NODE, "rescheduled action '%s' for node '%s' in %d microseconds", old_schedule->name, phy_node_get_name(node), usecs);
        }
        else {
            g_hash_table_remove(node->schedules, name);
            rs_debug(DEBUG_NODE, "canceled scheduled action '%s' for node '%s'", name, phy_node_get_name(node));
        }
    }

    return TRUE;
}

void node_execute_event(node_t *node, char *name, node_event_t func, void *data1, void *data2, bool blocking)
{
    rs_assert(node != NULL);
    rs_assert(node->life == g_thread_self());

    if (blocking) {
        func(node, data1, data2);
    }
    else {
        execute_event_wrapper_data_t *execute_data = malloc(sizeof(execute_event_wrapper_data_t));

        execute_data->func = func;
        execute_data->node = node;
        execute_data->data1 = data1;
        execute_data->data2 = data2;

        node_schedule(node, name, execute_event_wrapper, execute_data, 0, FALSE);
    }
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

    rs_debug(DEBUG_NODE, "enqueued message for node '%s'", phy_node_get_name(node));

    return all_ok;
}

bool node_has_pdu_from(node_t *node, node_t *src_node)
{
    rs_assert(node != NULL);

    g_mutex_lock(node->pdu_mutex);

    uint16 i;
    for (i = 0; i < g_queue_get_length(node->pdu_queue); i++) {
        phy_pdu_t *pdu = g_queue_peek_nth(node->pdu_queue, i);

        if (pdu->src_node == src_node) {
            g_mutex_unlock(node->pdu_mutex);
            return TRUE;
        }
    }

    g_mutex_unlock(node->pdu_mutex);

    return FALSE;
}

static phy_pdu_t *node_dequeue_pdu(node_t *node)
{
    rs_assert(node != NULL);

    g_mutex_lock(node->pdu_mutex);

    phy_pdu_t *pdu = g_queue_pop_head(node->pdu_queue);
    if (pdu != NULL) {
        rs_debug(DEBUG_NODE, "dequeued message from '%s'", phy_node_get_name(pdu->src_node));
    }

    g_cond_signal(node->pdu_cond);

    g_mutex_unlock(node->pdu_mutex);

    return pdu;
}

static bool node_process_message(node_t *node, node_t *src_node, phy_pdu_t *message)
{
    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);
    rs_assert(message != NULL);

    if (!phy_receive(node, src_node, message)) {
        rs_error("failed to process message");
    }

    return TRUE;
}

static void *node_life_core(node_t *node)
{
    rs_assert(node != NULL);
    rs_assert(node->alive == FALSE);

    /* wait until the creation routine exists */
    /* life mutex will be locked during all the life of the node */
    g_mutex_lock(node->life_mutex);

    node->alive = TRUE;

    rs_debug(DEBUG_NODE, "node '%s' life started", phy_node_get_name(node));

    node_execute_event(node, "phy_event_after_node_wake", (node_event_t) phy_event_after_node_wake, NULL, NULL, TRUE);
    node_execute_event(node, "mac_event_after_node_wake", (node_event_t) mac_event_after_node_wake, NULL, NULL, TRUE);
    node_execute_event(node, "ip_event_after_node_wake", (node_event_t) ip_event_after_node_wake, NULL, NULL, TRUE);
    node_execute_event(node, "icmp_event_after_node_wake", (node_event_t) icmp_event_after_node_wake, NULL, NULL, TRUE);
    node_execute_event(node, "rpl_event_after_node_wake", (node_event_t) rpl_event_after_node_wake, NULL, NULL, TRUE);

    node_execute_event(node, "main_win_event_after_node_wake", (node_event_t) main_win_event_after_node_wake, NULL, NULL, TRUE);

    node_schedule_t *schedules_to_exec[NODE_MAX_SCHEDULES_TO_EXEC];
    uint16 schedules_to_exec_count, schedule_index;

    while (node->alive) {
        g_timer_reset(node->schedule_timer);

        usleep(rs_system_get_node_core_sleep());


        /* what if the node was killed while usleep()-ing? */
        if (!node->alive) {
            break;
        }

        uint32 elapsed_usecs = 1000000 * g_timer_elapsed(node->schedule_timer, NULL);

        /* update all the schedules' remaining time, and execute if necessary */
        GList *schedule_list = g_hash_table_get_values(node->schedules);
        GList *schedule_element = g_list_first(schedule_list);
        schedules_to_exec_count = 0;
        while (schedule_element != NULL) {
            node_schedule_t *schedule = schedule_element->data;

            int32 new_remaining_usecs = schedule->remaining_usecs - elapsed_usecs;
            if (new_remaining_usecs > 0) {
                schedule->remaining_usecs = new_remaining_usecs;
            }
            else {
                schedule->remaining_usecs = 0;
                schedules_to_exec[schedules_to_exec_count++] = schedule;
            }

            schedule_element = g_list_next(schedule_element);
        }

        g_list_free(schedule_list);

        /* execute all the schedules that were planed for this loop iteration,
         * remove the finished and non-recurrent ones */
        for (schedule_index = 0; schedule_index < schedules_to_exec_count; schedule_index++) {
            node_schedule_t *schedule = schedules_to_exec[schedule_index];

            schedule->func(node, schedule->data);

            if (schedule->recurrent) {
                schedule->remaining_usecs = schedule->usecs;
            }
            else {
                if (!g_hash_table_remove(node->schedules, schedule->name)) {
                    rs_error("failed to remove dead schedule '%s'", schedule->name);
                }

                node_schedule_destroy(schedule);
            }
        }

        /* process a possibly received message */
        phy_pdu_t *message = node_dequeue_pdu(node);
        if (message != NULL) {
            node_process_message(node, message->src_node, message);
        }
    }

    node_execute_event(node, "main_win_event_before_node_kill", (node_event_t) main_win_event_before_node_kill, NULL, NULL, TRUE);
    node_execute_event(node, "rpl_event_before_node_kill", (node_event_t) rpl_event_before_node_kill, NULL, NULL, TRUE);
    node_execute_event(node, "icmp_event_before_node_kill", (node_event_t) icmp_event_before_node_kill, NULL, NULL, TRUE);
    node_execute_event(node, "ip_event_before_node_kill", (node_event_t) ip_event_before_node_kill, NULL, NULL, TRUE);
    node_execute_event(node, "mac_event_before_node_kill", (node_event_t) mac_event_before_node_kill, NULL, NULL, TRUE);
    node_execute_event(node, "phy_event_before_node_kill", (node_event_t) phy_event_before_node_kill, NULL, NULL, TRUE);

    rs_debug(DEBUG_NODE, "node '%s' life stopped", node->phy_info->name);

    /* destroy the remaining schedules */
    GList *schedule_list = g_hash_table_get_values(node->schedules);
    GList *schedule_element = g_list_first(schedule_list);
    while (schedule_element != NULL) {
        node_schedule_t *schedule = schedule_element->data;
        node_schedule_destroy(schedule);
        schedule_element = g_list_next(schedule_element);
    }
    g_list_free(schedule_list);
    g_hash_table_remove_all(node->schedules);

    /* destroy the remaining pdus */
    g_mutex_lock(node->pdu_mutex);
    while (!g_queue_is_empty(node->pdu_queue)) {
        phy_pdu_t *pdu = g_queue_pop_head(node->pdu_queue);
        phy_pdu_destroy(pdu);
    }
    g_mutex_unlock(node->pdu_mutex);

    g_mutex_unlock(node->life_mutex);

    g_thread_exit(NULL);

    return NULL;
}


static node_schedule_t *node_schedule_create(char *name, node_schedule_func_t func, void *data, uint32 usecs, bool recurrent)
{
    node_schedule_t *schedule = malloc(sizeof(node_schedule_t));

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

static void execute_event_wrapper(node_t *node, void *data)
{
    execute_event_wrapper_data_t *execute_data = data;

    execute_data->func(execute_data->node, execute_data->data1, execute_data->data2);

    free(execute_data);
}
