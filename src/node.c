
#include <unistd.h>

#include "node.h"


static void *                   node_life_core(node_t *node);

static node_schedule_t *        node_schedule_create(char *name, node_schedule_func_t func, void *data, uint32 usecs, bool recurrent);
static bool                     node_schedule_destroy(node_schedule_t *schedule);


node_t *node_create(char *name, coord_t cx, coord_t cy)
{
    rs_assert(name != NULL);

    node_t *node = (node_t *) malloc(sizeof(node_t));

    node->name = strdup(name);
    node->cx = cx;
    node->cy = cy;

    node->phy_info = NULL;
    node->mac_info = NULL;
    node->ip_info = NULL;
    node->rpl_info = NULL;

    node->life = NULL;
    node->life_mutex = NULL;
    node->alive = FALSE;

    node->schedules = NULL;
    node->schedule_mutex = NULL;
    node->schedule_timer = NULL;

    return node;
}

bool node_destroy(node_t *node)
{
    rs_assert(node != NULL);

    if (node->alive)
        node_kill(node);

    if (node->name != NULL)
        free(node->name);

    if (node->phy_info != NULL)
        free(node->phy_info);
    if (node->mac_info != NULL)
        free(node->mac_info);
    if (node->ip_info != NULL)
        free(node->ip_info);
    if (node->rpl_info != NULL)
        free(node->rpl_info);

    return TRUE;
}

bool node_start(node_t* node)
{
    rs_assert(!node->alive);
    rs_assert(node->life == NULL);

    node->life_mutex = g_mutex_new();
    node->schedule_mutex = g_mutex_new();

    g_mutex_lock(node->life_mutex);

    node->schedules = g_hash_table_new(g_str_hash, g_str_equal);
    node->schedule_timer = g_timer_new();

    GError *error;
    node->life = g_thread_create((void *(*) (void *)) node_life_core, node, TRUE, &error);
    if (node->life == NULL) {
        rs_error("g_thread_create() failed: %s", error->message);

        g_mutex_unlock(node->life_mutex);

        return FALSE;
    }

    g_mutex_unlock(node->life_mutex);

    return TRUE;
}

bool node_kill(node_t* node)
{
    rs_assert(node != NULL);
    rs_assert(node->alive);

    g_mutex_lock(node->life_mutex);

    node->alive = FALSE;
    node->life = NULL;

    g_mutex_lock(node->schedule_mutex);

    GList *schedule_list = g_hash_table_get_values(node->schedules);
    GList *schedule_element = g_list_first(schedule_list);
    while (schedule_element != NULL) {
        node_schedule_destroy(schedule_element->data);
        schedule_element = g_list_next(schedule_element);
    }
    g_list_free(schedule_list);
    g_hash_table_destroy(node->schedules);
    node->schedules = NULL;

    g_timer_destroy(node->schedule_timer);
    node->schedule_timer = NULL;

    g_mutex_unlock(node->schedule_mutex);

    g_mutex_unlock(node->life_mutex);

    g_mutex_free(node->life_mutex);
    node->life_mutex = NULL;
    g_mutex_free(node->schedule_mutex);
    node->schedule_mutex = NULL;

    return TRUE;
}

bool node_schedule(node_t *node, char *name, node_schedule_func_t func, void *data, uint32 usecs, bool recurrent)
{
    while (!node->alive) {
        usleep(10);
    }

    g_mutex_lock(node->schedule_mutex);

    rs_assert(node != NULL);
    rs_assert(name != NULL);
    rs_assert(node->alive);

    node_schedule_t *new_schedule = NULL;
    if (func != NULL) {
        new_schedule = node_schedule_create(name, func, data, usecs, recurrent);
    }

    node_schedule_t *old_schedule = g_hash_table_lookup(node->schedules, name);
    if (old_schedule == NULL) { /* the schedule does not exist yet */
        if (new_schedule != NULL) {
            g_hash_table_insert(node->schedules, new_schedule->name, new_schedule);
            rs_debug("scheduled action '%s' for node '%s' in %d milliseconds", new_schedule->name, node->name, usecs);
        }
        else {
            rs_warn("no schedule '%s' to cancel", name);
        }
    }
    else { /* the schedule already exist */
        if (new_schedule != NULL) {
            g_hash_table_insert(node->schedules, new_schedule->name, new_schedule);
            rs_debug("rescheduled action '%s' for node '%s' in %d milliseconds", new_schedule->name, node->name, usecs);
        }
        else {
            g_hash_table_remove(node->schedules, name);
            rs_debug("canceled scheduled action '%s' for node '%s' in %d milliseconds", name, node->name, usecs);
        }

        node_schedule_destroy(old_schedule);
    }

    g_mutex_unlock(node->schedule_mutex);

    return TRUE;
}


static void *node_life_core(node_t *node)
{
    /* wait until the creation routine exists */
    g_mutex_lock(node->life_mutex);
    g_mutex_unlock(node->life_mutex);

    rs_assert(node != NULL);
    rs_assert(node->alive == FALSE);

    rs_debug("node '%s' life started", node->name);

    node->alive = TRUE;

    while (node->alive) {
        g_timer_reset(node->schedule_timer);

        usleep(NODE_LIFE_CORE_SLEEP);

        uint32 elapsed_usecs = 1000000 * g_timer_elapsed(node->schedule_timer, NULL);

        g_mutex_lock(node->schedule_mutex);

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
                schedule->func(node, schedule->data);   // todo: synchronize this call?

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
    }

    rs_debug("node '%s' life stopped", node->name);

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
