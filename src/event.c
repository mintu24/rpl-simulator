
#include "event.h"
#include "system.h"

#define EVENT_PARAM(node, data1, data2, arg)  ((arg >= 0 ? (arg > 0 ? (arg > 1 ? data2 : data1) : node) : NULL))


    /**** global variables ****/

static event_t *            event_list = NULL;
static uint16               event_count = 0;

static uint8                level = 0; /* not thread safe !!! */


    /**** local functions prototypes ****/

void                        event_log(uint16 event_id, node_t *node, void *data1, void *data2);


    /**** exported functions ****/

uint16 event_register(char *name, char *layer, event_handler_t handler, event_arg_str_t str_func)
{
    event_list = realloc(event_list, (event_count + 1) * sizeof(event_t));

    event_list[event_count].name = strdup(name);
    event_list[event_count].layer = strdup(layer);
    event_list[event_count].handler = handler;
    event_list[event_count].str_func = str_func;

    return event_count++;
}

bool event_execute(uint16 event_id, node_t *node, void *data1, void *data2)
{
    events_lock();

    rs_assert(event_id < event_count);

    bool all_ok = TRUE;

    event_t *event = &event_list[event_id];

    rs_debug(DEBUG_EVENT, "node '%s': executing event '%s.%s' @%d ms", node->phy_info->name, event->layer, event->name, rs_system->now);

    bool write_log = FALSE;

    write_log = write_log || ((strcmp(event->layer, "sys") == 0) && (DEBUG & DEBUG_SYSTEM));
    write_log = write_log || ((strcmp(event->layer, "phy") == 0) && (DEBUG & DEBUG_PHY));
    write_log = write_log || ((strcmp(event->layer, "mac") == 0) && (DEBUG & DEBUG_MAC));
    write_log = write_log || ((strcmp(event->layer, "ip") == 0) && (DEBUG & DEBUG_IP));
    write_log = write_log || ((strcmp(event->layer, "icmp") == 0) && (DEBUG & DEBUG_ICMP));
    write_log = write_log || ((strcmp(event->layer, "rpl") == 0) && (DEBUG & DEBUG_RPL));

    if (write_log) {
        event_log(event_id, node, data1, data2);
        level++;
    }

    if (!event->handler(node, data1, data2)) {
        all_ok = FALSE;
    }

    if (write_log) {
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


    /**** local functions ****/

void event_log(uint16 event_id, node_t *node, void *data1, void *data2)
{
    event_t *event = &event_list[event_id];

    char text[256];

    char *str_time = rs_system_sim_time_to_string(rs_system->now);

    char str1[256]; str1[0] = '\0';
    char str2[256]; str2[0] = '\0';

    char *node_name = node->phy_info != NULL ? node->phy_info->name : "<<removed>>";

    char indent[256]; indent[0] = '\0';
    uint16 i;
    for (i = 0; i < level; i++) {
        strcat(indent, "    ");
    }

    if (event->str_func != NULL) {
        event->str_func(data1, data2, str1, str2, 256);
    }

    if (strlen(str1) > 0) {
        if (strlen(str2) > 0) {
            snprintf(text, 256, "%s.%s.%s(%s, %s)", node_name, event->layer, event->name, str1, str2);
        }
        else {
            snprintf(text, 256, "%s.%s.%s(%s)", node_name, event->layer, event->name, str1);
        }
    }
    else {
        snprintf(text, 256, "%s.%s.%s()", node_name, event->layer, event->name);
    }

    fprintf(stderr, "%s : %s%s\n", str_time, indent, text);

    free(str_time);
}
