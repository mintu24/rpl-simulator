
#include <unistd.h>
#include <math.h>

#include "system.h"
#include "measure.h"
#include "gui/simfield.h"
#include "gui/mainwin.h"
#include "gui/measurement.h"


    /**** global variables ****/

rs_system_t *               rs_system = NULL;
uint16                      sys_event_id_after_node_wake;
uint16                      sys_event_id_before_node_kill;
uint16                      sys_event_id_after_message_transmitted;


    /**** local function prototypes ****/

static void *               system_core(void *data);

static event_schedule_t *   schedule_create(node_t *node, uint16 event_id, void *data1, void *data2, sim_time_t time);
static bool                 schedule_destroy(event_schedule_t *schedule);

static void                 event_arg_str_one_node_func(void *data1, void *data2, char *str1, char *str2, uint16 len);

#ifdef DEBUG_EVENTS
static void                 print_scheduled_events();
#endif /* DEBUG_EVENTS */


    /**** exported functions ****/

bool rs_system_create()
{
    rs_system = malloc(sizeof(rs_system_t));

    rs_system->node_list = NULL;
    rs_system->node_count = 0;

    rs_system->no_link_dist_thresh = DEFAULT_NO_LINK_DIST_THRESH;
    rs_system->no_link_quality_thresh = DEFAULT_NO_LINK_QUALITY_THRESH;
    rs_system->transmission_time = DEFAULT_TRANSMISSION_TIME;
    rs_system->neighbor_timeout = DEFAULT_NEIGHBOR_TIMEOUT;

    rs_system->width = DEFAULT_SYS_WIDTH;
    rs_system->height = DEFAULT_SYS_HEIGHT;

    rs_system->auto_wake_nodes = TRUE;

    rs_system->simulation_second = DEFAULT_SIMULATION_SECOND;

    rs_system->rpl_auto_sn_inc_interval = DEFAULT_RPL_AUTO_SN_INC_INT;
    rs_system->rpl_start_silent = DEFAULT_RPL_STARTUP_SILENT;
    rs_system->rpl_poison_count = DEFAULT_RPL_POISON_COUNT;

    rs_system->rpl_dao_supported = DEFAULT_RPL_DAO_SUPPORTED;
    rs_system->rpl_dao_trigger = DEFAULT_RPL_DAO_TRIGGER;
    rs_system->rpl_dio_interval_doublings = DEFAULT_RPL_DIO_INTERVAL_DOUBLINGS;
    rs_system->rpl_dio_interval_min = DEFAULT_RPL_DIO_INTERVAL_MIN;
    rs_system->rpl_dio_redundancy_constant = DEFAULT_RPL_DIO_REDUNDANCY_CONSTANT;
    rs_system->rpl_max_inc_rank = DEFAULT_RPL_MAX_RANK_INC;

    rs_system->rpl_prefer_floating = DEFAULT_RPL_PREFER_FLOATING;
//    rs_system->rpl_min_hop_rank_inc = DEFAULT_RPL_MIN_HOP_RANK_INC;

    rs_system->schedules = NULL;
    rs_system->schedule_count = 0;

    rs_system->started = FALSE;
    rs_system->paused = FALSE;
    rs_system->step = FALSE;
    rs_system->now = 0;
    rs_system->event_count = 0;

    sys_event_id_after_node_wake = event_register("after_node_wake", "sys", (event_handler_t) sys_event_after_node_wake, NULL);
    sys_event_id_before_node_kill = event_register("before_node_kill", "sys", (event_handler_t) sys_event_before_node_kill, NULL);
    sys_event_id_after_message_transmitted = event_register("after_message_transmitted", "sys", (event_handler_t) sys_event_after_message_transmitted, event_arg_str_one_node_func);

    if (!phy_init()) {
        rs_error("failed to initialize PHY layer");
        return FALSE;
    }
    if (!mac_init()) {
        rs_error("failed to initialize MAC layer");
        return FALSE;
    }
    if (!ip_init()) {
        rs_error("failed to initialize IP layer");
        return FALSE;
    }
    if (!icmp_init()) {
        rs_error("failed to initialize ICMP layer");
        return FALSE;
    }
    if (!rpl_init()) {
        rs_error("failed to initialize RPL layer");
        return FALSE;
    }

    g_static_rec_mutex_init(&rs_system->events_mutex);
    g_static_rec_mutex_init(&rs_system->schedules_mutex);
    g_static_rec_mutex_init(&rs_system->nodes_mutex);
    g_static_rec_mutex_init(&rs_system->measures_mutex);

    return TRUE;
}

bool rs_system_destroy()
{
    rs_assert(rs_system != NULL);

    int i;
    for (i = 0; i < rs_system->node_count; i++) {
        node_t *node = rs_system->node_list[i];
        node_destroy(node);
    }

    if (rs_system->node_list != NULL)
        free(rs_system->node_list);

    while (rs_system->schedules != NULL) {
        event_schedule_t *schedule = rs_system->schedules;
        rs_system->schedules = rs_system->schedules->next;

        schedule_destroy(schedule);
    }
    rs_system->schedule_count = 0;

    if (!rpl_done()) {
        rs_error("failed to destroy RPL layer");
        return FALSE;
    }
    if (!icmp_done()) {
        rs_error("failed to destroy ICMP layer");
        return FALSE;
    }
    if (!ip_done()) {
        rs_error("failed to destroy IP layer");
        return FALSE;
    }
    if (!mac_done()) {
        rs_error("failed to destroy MAC layer");
        return FALSE;
    }
    if (!phy_done()) {
        rs_error("failed to destroy PHY layer");
        return FALSE;
    }

    g_static_rec_mutex_free(&rs_system->measures_mutex);
    g_static_rec_mutex_free(&rs_system->nodes_mutex);
    g_static_rec_mutex_free(&rs_system->schedules_mutex);
    g_static_rec_mutex_free(&rs_system->events_mutex);

    free(rs_system);
    rs_system = NULL;

    return TRUE;
}

bool rs_system_add_node(node_t *node)
{
    nodes_lock();

    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);

    rs_system->node_list = realloc(rs_system->node_list, (++rs_system->node_count) * sizeof(node_t *));
    rs_system->node_list[rs_system->node_count - 1] = node;

    nodes_unlock();

    return TRUE;
}

bool rs_system_remove_node(node_t *node)
{
    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);

    /* remove all schedules that concern this node */
    rs_system_cancel_event(node, -1, NULL, NULL, 0);

    nodes_lock();

    int i, pos = -1;
    for (i = 0; i < rs_system->node_count; i++) {
        if (rs_system->node_list[i] == node) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' not found", rs_system->node_list[i]);
        measures_unlock();
        events_unlock();
        nodes_unlock();

        return FALSE;
    }

    for (i = pos; i < rs_system->node_count - 1; i++) {
        rs_system->node_list[i] = rs_system->node_list[i + 1];
    }

    rs_system->node_count--;
    rs_system->node_list = realloc(rs_system->node_list, (rs_system->node_count) * sizeof(node_t *));
    if (rs_system->node_count == 0) {
        rs_system->node_list = NULL;
    }

    nodes_unlock();

    events_lock();

    uint16 node_count;
    node_t **node_list = rs_system_get_node_list_copy(&node_count);

    /* remove all the references to this node */
    for (i = 0; i < node_count; i++) {
        node_t *other_node = node_list[i];

        /* nullify ip route refs */
        ip_node_rem_routes(other_node, NULL, -1, node, -1);

        /* ip neighbors */
        ip_neighbor_t *ip_neighbor = ip_node_find_neighbor_by_node(other_node, node);
        if (ip_neighbor != NULL) {
            rs_system_cancel_event(other_node, ip_event_id_after_neighbor_cache_timeout, ip_neighbor, NULL, 0);
            event_execute(rpl_event_id_after_neighbor_detach, other_node, node, NULL);
            ip_node_rem_neighbor(other_node, ip_neighbor);
        }
    }

    if (node_list != NULL) {
        free(node_list);
    }

    measures_lock();

    /* remove measurement entries that refer to this node */
    for (i = 0; i < measure_connect_entry_get_count(); i++) {
        measure_connect_t *measure = measure_connect_entry_get(i);

        if (measure->src_node == node || measure->dst_node == node) {
            measure_connect_entry_remove(i);
        }
    }

    for (i = 0; i < measure_sp_comp_entry_get_count(); i++) {
        measure_sp_comp_t *measure = measure_sp_comp_entry_get(i);

        if (measure->src_node == node || measure->dst_node == node) {
            measure_sp_comp_entry_remove(i);
        }
    }

    for (i = 0; i < measure_stat_entry_get_count(); i++) {
        measure_stat_t *measure = measure_stat_entry_get(i);

        if (measure->node == node) {
            measure_stat_entry_remove(i);
        }
    }

    measurement_entries_to_gui();

    measures_unlock();

    events_unlock();

    return TRUE;
}

int32 rs_system_get_node_pos(node_t *node)
{
    nodes_lock();

    rs_assert(rs_system != NULL);

    uint16 i;
    for (i = 0; i < rs_system->node_count; i++) {
        if (rs_system->node_list[i] == node) {
            nodes_unlock();

            return i;
        }
    }

    nodes_unlock();

    return -1;
}

node_t *rs_system_find_node_by_name(char *name)
{
    nodes_lock();

    rs_assert(rs_system != NULL);
    rs_assert(name != NULL);

    int i;
    node_t *node = NULL;
    for (i = 0; i < rs_system->node_count; i++) {
        if (!strcmp(rs_system->node_list[i]->phy_info->name, name)) {
            node = rs_system->node_list[i];
            break;
        }
    }

    nodes_unlock();

    return node;
}

node_t *rs_system_find_node_by_mac_address(char *address)
{
    nodes_lock();

    rs_assert(rs_system != NULL);
    rs_assert(address != NULL);

    int i;
    node_t *node = NULL;
    for (i = 0; i < rs_system->node_count; i++) {
        if (!strcmp(rs_system->node_list[i]->mac_info->address, address)) {
            node = rs_system->node_list[i];
            break;
        }
    }

    nodes_unlock();

    return node;
}

node_t *rs_system_find_node_by_ip_address(char *address)
{
    nodes_lock();

    rs_assert(rs_system != NULL);
    rs_assert(address != NULL);

    int i;
    node_t *node = NULL;
    for (i = 0; i < rs_system->node_count; i++) {
        if (!strcmp(rs_system->node_list[i]->ip_info->address, address)) {
            node = rs_system->node_list[i];
            break;
        }
    }

    nodes_unlock();

    return node;
}

node_t **rs_system_get_node_list_copy(uint16 *node_count)
{
    rs_assert(rs_system != NULL);
    rs_assert(node_count != NULL);

    nodes_lock();

    *node_count = rs_system->node_count;
    node_t **node_list = malloc(sizeof(node_t *) * rs_system->node_count);
    uint16 i;
    for (i = 0; i < rs_system->node_count; i++) {
        node_list[i] = rs_system->node_list[i];
    }

    nodes_unlock();

    return node_list;
}

void rs_system_schedule_event(node_t *node, uint16 event_id, void *data1, void *data2, sim_time_t time)
{
    rs_assert(rs_system != NULL);

    if (!rs_system->started || (node != NULL && !node->alive)) { /* don't schedule anything if not started or node dead */
        return;
    }

    schedules_lock();

    time += rs_system->now; /* make the time absolute */

    event_schedule_t *new_schedule = schedule_create(node, event_id, data1, data2, time);

    event_schedule_t *schedule = rs_system->schedules;
    event_schedule_t *prev_schedule = NULL;

    while ((schedule != NULL) && (schedule->time <= time)) {
        prev_schedule = schedule;
        schedule = schedule->next;
    }

    new_schedule->next = schedule;
    if (prev_schedule == NULL) {
        rs_system->schedules = new_schedule;
    }
    else {
        prev_schedule->next = new_schedule;
    }

    rs_system->schedule_count++;

    schedules_unlock();
}

void rs_system_cancel_event(node_t *node, int32 event_id, void *data1, void *data2, int32 time)
{
    rs_assert(rs_system != NULL);

    schedules_lock();

    event_schedule_t *schedule = rs_system->schedules;
    event_schedule_t *prev_schedule = NULL;

    if (time < 0) {
        time = rs_system->now - time;
    }

    while (schedule != NULL) {
        if ((event_id ==-1 || schedule->event_id == event_id) &&
                (node == NULL || node == schedule->node) &&
                (data1 == NULL || data1 == schedule->data1) &&
                (data2 == NULL || data2 == schedule->data2) &&
                (time == 0 || time == schedule->time)) {

            event_schedule_t *schedule_to_destroy = schedule;

            if (prev_schedule == NULL) {
                rs_system->schedules = schedule->next;
            }
            else {
                prev_schedule->next = schedule->next;
            }

            schedule = schedule->next;
            schedule_destroy(schedule_to_destroy);
            rs_system->schedule_count--;
        }
        else {
            prev_schedule = schedule;
            schedule = schedule->next;
        }
    }

    schedules_unlock();
}

bool rs_system_send(node_t *src_node, node_t* dst_node, phy_pdu_t *message)
{
    rs_assert(rs_system != NULL);
    rs_assert(src_node != NULL);

    if (dst_node == NULL) { /* broadcast */

        uint16 node_count;
        node_t **node_list = rs_system_get_node_list_copy(&node_count);

        uint16 i;
        for (i = 0; i < node_count; i++) {
            dst_node = node_list[i];

            if (dst_node == src_node) { /* don't process our own broadcast */
                continue;
            }

            if (!dst_node->alive) { /* dont't send messages to dead nodes */
                continue;
            }

            percent_t quality = rs_system_get_link_quality(src_node, dst_node);
            if (quality < rs_system->no_link_quality_thresh) { /* no physical link */
                continue;
            }

            if (i < rs_system->node_count - 1) {
                rs_system_send(src_node, dst_node, phy_pdu_duplicate(message));
            }
            else {
                rs_system_send(src_node, dst_node, message);
            }
        }

        if (node_list != NULL) {
            free(node_list);
        }
    }
    else {
        // todo implement collisions

        percent_t quality = rs_system_get_link_quality(src_node, dst_node);
        if (quality < rs_system->no_link_quality_thresh) { /* no physical link */
            return FALSE;
        }

        rs_system_schedule_event(src_node, sys_event_id_after_message_transmitted, dst_node, message, rs_system->transmission_time);
    }

    return TRUE;
}

percent_t rs_system_get_link_quality(node_t *src_node, node_t *dst_node)
{
    rs_assert(rs_system != NULL);
    rs_assert(src_node != NULL);
    rs_assert(dst_node != NULL);

    if (src_node == dst_node) { /* the quality between a node and itself is considered perfect */
        return 1.0;
    }

    coord_t distance = sqrt(
            pow(src_node->phy_info->cx - dst_node->phy_info->cx, 2) +
            pow(src_node->phy_info->cy - dst_node->phy_info->cy, 2));

    if (distance > rs_system->no_link_dist_thresh) {
        distance = rs_system->no_link_dist_thresh;
    }

    percent_t dist_factor = (percent_t) (rs_system->no_link_dist_thresh - distance) / rs_system->no_link_dist_thresh;
    percent_t quality = src_node->phy_info->tx_power * dist_factor;

    return quality;
}

void rs_system_start(bool start_paused)
{
    rs_assert(rs_system != NULL);

    if (rs_system->paused) {
        rs_system->paused = FALSE;
        rs_debug(DEBUG_SYSTEM, "system core resumed");
    }
    else {
        rs_system->paused = start_paused;
        rs_system->step = FALSE;

        measure_connect_reset_output();
        measure_sp_comp_reset_output();
        measure_converg_reset_output();
        measure_stat_reset_output();

        measurement_output_to_gui();

        rs_system->now = 0;
        rs_system->event_count = 0;

        rs_system->random_z = RANDOM_SEED_Z;
        rs_system->random_w = RANDOM_SEED_W;

        GError *error;
        rs_system->sys_thread = g_thread_create(system_core, NULL, TRUE, &error);
        if (rs_system->sys_thread == NULL) {
            rs_error("g_thread_create() failed: %s", error->message);
        }

        /* wait till started */
        while (!rs_system->started) {
            usleep(SYS_CORE_SLEEP);
        }

        /* schedule the auto incrementing of seq num mechanism */
        if (rs_system->rpl_auto_sn_inc_interval > 0 ) { // todo this blocks the startup for 10 seconds!
            rs_system_schedule_event(NULL, rpl_event_id_after_seq_num_timer_timeout, NULL, NULL, rs_system->rpl_auto_sn_inc_interval);
        }


        /* wake all nodes, if that's the chosen option */
        if (rs_system->auto_wake_nodes) {
            uint16 node_count;
            node_t **node_list = rs_system_get_node_list_copy(&node_count);

            uint16 i;
            for (i = 0; i < node_count; i++) {
                node_t *node = node_list[i];
                if (!node->alive && !node_wake(node)) {
                    rs_error("failed to wake node '%s'", node->phy_info->name);
                }
            }

            if (node_list != NULL) {
                free(node_list);
            }

            main_win_update_nodes_status();
        }
    }

    main_win_update_nodes_status();
    main_win_update_sim_time_status();
}

void rs_system_stop()
{
    rs_assert(rs_system != NULL);

    rs_system->started = FALSE;
    rs_system->paused = FALSE;

    /* this schedules_lock() waits for the system core to terminate */

    schedules_lock();
    events_lock();

    uint16 node_count;
    node_t **node_list = rs_system_get_node_list_copy(&node_count);

    uint16 i;
    for (i = 0; i < node_count; i++) {
        node_t *node = node_list[i];
        if (node->alive && !node_kill(node)) {
            rs_error("failed to kill node '%s'", node->phy_info->name);
        }
    }

    if (node_list != NULL) {
        free(node_list);
    }

    while (rs_system->schedules != NULL) {
        event_schedule_t *schedule = rs_system->schedules;
        rs_system->schedules = rs_system->schedules->next;

        schedule_destroy(schedule);
    }

    rs_system->schedule_count = 0;

    events_unlock();
    schedules_unlock();
}

void rs_system_pause()
{
    rs_assert(rs_system != NULL);
    rs_assert(!rs_system->paused);

    rs_system->paused = TRUE;
    rs_system->step = FALSE;

    rs_debug(DEBUG_SYSTEM, "system core paused");
}

void rs_system_step()
{
    rs_assert(rs_system != NULL);
    rs_assert(rs_system->paused);

    rs_system->step = TRUE;

    rs_debug(DEBUG_SYSTEM, "system core stepped");
}

char *rs_system_sim_time_to_string(sim_time_t time)
{
    char *text = malloc(256);

    if (time < 1000) {
        snprintf(text, 256, "%d ms", time);
    }
    else if (time < 60000) {
        snprintf(text, 256, "%.3f s", time / 1000.0);
    }
    else if (time < 3600000) {
        uint32 m = time / 60000;
        uint32 s = (time - m * 60000) / 1000;
        snprintf(text, 256, "%02d:%02d", m, s);
    }
    else {
        uint32 h = time / 3600000;
        uint32 m = (time - h * 3600000) / 60000;
        uint32 s = (time - h * 3600000 - m * 60000) / 1000;
        snprintf(text, 256, "%02d:%02d:%02d", h, m, s);
    }

    return text;
}

uint32 rs_system_random()
{
    rs_system->random_z = 36969 * (rs_system->random_z & 0xFFFF) + (rs_system->random_z >> 16);
    rs_system->random_w = 18000 * (rs_system->random_w & 0xFFFF) + (rs_system->random_w >> 16);

    return (rs_system->random_z << 16) + rs_system->random_w;
}

bool sys_event_after_node_wake(node_t *node)
{
    measure_node_reset(node);

    if (!event_execute(phy_event_id_after_node_wake, node, NULL, NULL))
        return FALSE;
    if (!event_execute(mac_event_id_after_node_wake, node, NULL, NULL))
        return FALSE;
    if (!event_execute(ip_event_id_after_node_wake, node, NULL, NULL))
        return FALSE;
    if (!event_execute(icmp_event_id_after_node_wake, node, NULL, NULL))
        return FALSE;
    if (!event_execute(rpl_event_id_after_node_wake, node, NULL, NULL))
        return FALSE;

    return TRUE;
}

bool sys_event_before_node_kill(node_t *node)
{
    if (!event_execute(rpl_event_id_before_node_kill, node, NULL, NULL))
        return FALSE;
    if (!event_execute(icmp_event_id_before_node_kill, node, NULL, NULL))
        return FALSE;
    if (!event_execute(ip_event_id_before_node_kill, node, NULL, NULL))
        return FALSE;
    if (!event_execute(mac_event_id_before_node_kill, node, NULL, NULL))
        return FALSE;
    if (!event_execute(phy_event_id_before_node_kill, node, NULL, NULL))
        return FALSE;

    return TRUE;
}

bool sys_event_after_message_transmitted(node_t *src_node, node_t *dst_node, phy_pdu_t *message)
{
    bool all_ok = TRUE;

    if (!phy_receive(dst_node, src_node, message)) {
        rs_error("node '%s': failed to receive PHY pdu from node '%s'", dst_node->phy_info->name, src_node->phy_info->name);
        all_ok = FALSE;
    }

    return all_ok;
}


    /**** local functions ****/

static void *system_core(void *data)
{
    rs_debug(DEBUG_SYSTEM, "system core started");

    rs_system->started = TRUE;

    while (rs_system->started) {
        usleep(SYS_CORE_SLEEP);

        if (rs_system->paused && !rs_system->step) {
            continue;
        }

        schedules_lock();

        if (rs_system->schedules != NULL) {

#ifdef DEBUG_EVENTS
            print_scheduled_events();
#endif /* DEBUG_EVENTS */

            if (rs_system->simulation_second > 0 && !rs_system->step) {
                sim_time_t diff = rs_system->schedules->time - rs_system->now;

                if (diff * rs_system->simulation_second > SYS_CORE_SLEEP) {
                    schedules_unlock();
                    usleep(diff * rs_system->simulation_second - SYS_CORE_SLEEP);
                    schedules_lock();
                }

                /* if system was paused or stopped or all the schedules removed while sleeping... */
                if (rs_system->paused || !rs_system->started || rs_system->schedules == NULL) {
                    schedules_unlock();
                    continue;
                }
            }

            rs_system->now = rs_system->schedules->time;
            rs_debug(DEBUG_SYSTEM, "time is now %d", rs_system->now);
            main_win_update_sim_time_status();

            while ((rs_system->schedules != NULL) && (rs_system->schedules->time == rs_system->now)) {
                event_schedule_t *schedule = rs_system->schedules;
                rs_system->schedules = rs_system->schedules->next;

                /* test to see if the concerned node still exists and is alive */

                if (schedule->node == NULL || schedule->node->alive) {
                    event_execute(schedule->event_id, schedule->node, schedule->data1, schedule->data2);
                }
                else {
                    // todo check to see why do some orphan schedules remain in the system
                    event_t event = event_find_by_id(schedule->event_id);
                    rs_warn("a '%s.%s' event for a dead/inexistent node was left out in the system", event.layer, event.name);
                }

                schedule_destroy(schedule);
                rs_system->schedule_count--;
            }
        }

        schedules_unlock();

        if (rs_system->paused) {
            rs_system->step = FALSE;
        }
    }

    rs_system->paused = FALSE;

    rs_debug(DEBUG_SYSTEM, "system core stopped");

    g_thread_exit(NULL);

    return NULL;
}

static event_schedule_t *schedule_create(node_t *node, uint16 event_id, void *data1, void *data2, sim_time_t time)
{
    event_schedule_t *schedule = malloc(sizeof(event_schedule_t));

    schedule->node = node;
    schedule->event_id = event_id;
    schedule->data1 = data1;
    schedule->data2 = data2;
    schedule->time = time;

    schedule->next = NULL;

    return schedule;
}


static bool schedule_destroy(event_schedule_t *schedule)
{
    rs_assert(schedule != NULL);

    free(schedule);

    return TRUE;
}

static void event_arg_str_one_node_func(void *data1, void *data2, char *str1, char *str2, uint16 len)
{
    node_t *node = data1;

    snprintf(str1, len, "%s", (node != NULL ? node->phy_info->name : "broadcast"));
    str2[0] = '\0';
}

#ifdef DEBUG_EVENTS

static void print_scheduled_events()
{
    event_schedule_t *schedule = rs_system->schedules;

    fprintf(stderr, "######## event list @%d ########\n", rs_system->now);

    while (schedule != NULL) {
        event_t event = event_find_by_id(schedule->event_id);

        fprintf(stderr, "event: name = '%s', node = '%s', data1 = %p, data2 = %p, time = @%d\n",
                event.name,
                schedule->node->phy_info->name,
                schedule->data1,
                schedule->data2,
                schedule->time);

        schedule = schedule->next;
    }

    fprintf(stderr, "######## event list end ########\n");
}

#endif /* DEBUG_EVENTS */
