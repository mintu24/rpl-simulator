
#include "event.h"
#include "system.h"
#include "gui/mainwin.h"

#define EVENT_PARAM(node, data1, data2, arg)  ((arg >= 0 ? (arg > 0 ? (arg > 1 ? data2 : data1) : node) : NULL))


    /**** global variables ****/

static event_t *            event_list = NULL;
static uint16               event_count = 0;

static uint8                level = 0; /* not thread safe !!! */

static uint32               log_event_count = 0;
static FILE *				log_file = NULL;


    /**** local functions prototypes ****/

void                        event_log(uint16 event_id, node_t *node, void *data1, void *data2);


    /**** exported functions ****/

void event_init()
{
}

void event_done()
{
	if (log_file != NULL) {
        rs_debug(DEBUG_EVENT, "closing event log");

        fclose(log_file);
		log_file = NULL;
	}
}

uint16 event_register(char *name, char *layer, event_handler_t handler, event_arg_str_t str_func)
{
    event_list = realloc(event_list, (event_count + 1) * sizeof(event_t));

    event_list[event_count].name = strdup(name);
    event_list[event_count].layer = strdup(layer);
    event_list[event_count].handler = handler;
    event_list[event_count].str_func = str_func;
    event_list[event_count].loggable = FALSE;

    return event_count++;
}

bool event_execute(uint16 event_id, node_t *node, void *data1, void *data2)
{
    events_lock();

    rs_assert(event_id < event_count);

    event_t *event = &event_list[event_id];

    if (node != NULL) {
        rs_debug(DEBUG_EVENT, "node '%s': executing event '%s.%s' @%d ms", node->phy_info->name, event->layer, event->name, rs_system->now);
    }
    else {
        rs_debug(DEBUG_EVENT, "executing event '%s.%s' @%d ms", event->layer, event->name, rs_system->now);
    }


    if (event->loggable) {
        event_log(event_id, node, data1, data2);
        level++;
    }

    bool all_ok = TRUE;

    if (event->handler != NULL) {
        if (!event->handler(node, data1, data2)) {
            all_ok = FALSE;
        }
    }

    if (event->loggable) {
        level--;
    }

    rs_system->event_count++;

    events_unlock();

    return all_ok;
}

event_t event_find_by_id(uint16 event_id)
{
    rs_assert(event_id < event_count);

    return event_list[event_id];
}

uint16 event_get_count()
{
    return event_count;
}

void event_set_logging(uint16 event_id, bool loggable)
{
    rs_assert(event_id < event_count);

    event_list[event_id].loggable = loggable;
}

bool event_get_logging(uint16 event_id)
{
    rs_assert(event_id < event_count);

    return event_list[event_id].loggable;
}

void event_set_log_file(char *filename)
{
	if (log_file != NULL) {
        rs_debug(DEBUG_EVENT, "closing event log");

        fclose(log_file);
        log_file = NULL;

    }

	if (filename != NULL) {
        rs_debug(DEBUG_EVENT, "opening event log '%s'", filename);

        log_file = fopen(filename, "w");
        if (log_file == NULL) {
            rs_error("failed to open event log '%s' for writing: ", filename, strerror(errno));
        }
    }

	log_event_count = 0;
	main_win_clear_log();
}


    /**** local functions ****/

void event_log(uint16 event_id, node_t *node, void *data1, void *data2)
{
    event_t *event = &event_list[event_id];

    char text[4 * 256];

    char *str_time = rs_system_sim_time_to_string(rs_system->now, TRUE);

    char str1[4 * 256]; str1[0] = '\0';
    char str2[4 * 256]; str2[0] = '\0';
    char info[4 * 256]; info[0] = '\0';
    char stats[4 * 256]; stats[0] = '\0';

    char *node_name;
    if (node != NULL) {
        if (node->phy_info != NULL) {
            node_name = node->phy_info->name;
        }
        else {
            node_name = "<<removed>>";
        }
    }
    else {
        node_name = "system";
    }

    char indent[4 * 256]; indent[0] = '\0';
    uint16 i;
    for (i = 0; i < level; i++) {
        strcat(indent, "    ");
    }

    if (event->str_func != NULL) {
        event->str_func(event_id, data1, data2, str1, str2, 4 * 256);
    }

    if (node != NULL) {
        snprintf(info, 4 * 256, "info = {'%s' %.02f %.02f '%s' '%s' %d %d %d %d}",
                node->phy_info->name,
                node->phy_info->cx,
                node->phy_info->cy,
                node->mac_info->address,
                node->ip_info->address,
                node->ip_info->enqueued_count,
                rpl_node_is_joined(node) ? node->rpl_info->joined_dodag->rank : rpl_node_is_root(node) ? RPL_RANK_ROOT : RPL_RANK_INFINITY,
                rpl_node_is_joined(node) ? node->rpl_info->joined_dodag->parent_count : 0,
                rpl_node_is_joined(node) ? node->rpl_info->joined_dodag->sibling_count : 0
                );

        snprintf(stats, 4 * 256, "stats = {%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d}",
                node->measure_info->forward_inconsistency_count,
                node->measure_info->forward_failure_count,
                node->measure_info->rpl_r_dis_message_count,
                node->measure_info->rpl_r_dio_message_count,
                node->measure_info->rpl_r_dao_message_count,
                node->measure_info->rpl_s_dis_message_count,
                node->measure_info->rpl_s_dio_message_count,
                node->measure_info->rpl_s_dao_message_count,
                node->measure_info->ping_successful_count,
                node->measure_info->ping_timeout_count,
                node->measure_info->gen_ip_packet_count,
                node->measure_info->fwd_ip_packet_count,
                measure_converg_get()->stable_node_count,
                measure_converg_get()->floating_node_count,
                measure_converg_get()->total_node_count
                );
    }

    if (strlen(str1) > 0) {
        if (strlen(str2) > 0) {
            if (strlen(stats) > 0) {
                snprintf(text, 4 * 256, "%s.%s.%s(%s, %s, %s, %s)", node_name, event->layer, event->name, info, stats, str1, str2);
            }
            else {
                snprintf(text, 4 * 256, "%s.%s.%s(%s, %s)", node_name, event->layer, event->name, str1, str2);
            }
        }
        else {
            if (strlen(stats)) {
                snprintf(text, 4 * 256, "%s.%s.%s(%s, %s, %s)", node_name, event->layer, event->name, info, stats, str1);
            }
            else {
                snprintf(text, 4 * 256, "%s.%s.%s(%s)", node_name, event->layer, event->name, str1);
            }
        }
    }
    else {
        if (strlen(stats)) {
            snprintf(text, 4 * 256, "%s.%s.%s(%s, %s)", node_name, event->layer, event->name, info, stats);
        }
        else {
            snprintf(text, 4 * 256, "%s.%s.%s()", node_name, event->layer, event->name);
        }
    }

    if (log_file != NULL) {
        fprintf(log_file, "%s : %s%s\n", str_time, indent, text);
        fflush(log_file);
    }
    else {
        fprintf(stderr, "%s : %s%s\n", str_time, indent, text);
    }

    log_event_count++;

    main_win_add_log_line(log_event_count, str_time, node_name, event->layer, event->name, str1, str2);

    free(str_time);
}

