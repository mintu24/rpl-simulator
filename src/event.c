
#include "event.h"
#include "system.h"

#define EVENT_PARAM(node, data1, data2, arg)  ((arg >= 0 ? (arg > 0 ? (arg > 1 ? data2 : data1) : node) : NULL))


    /**** global variables ****/

static event_t *            event_list = NULL;
static uint16               event_count = 0;


    /**** local functions prototypes ****/


    /**** exported functions ****/

uint16 event_register(char *name, char *layer, event_handler_t handler)
{
    event_list = realloc(event_list, (event_count + 1) * sizeof(event_t));

    event_list[event_count].name = strdup(name);
    event_list[event_count].layer = strdup(layer);
    event_list[event_count].handler = handler;

    return event_count++;
}

bool event_execute(uint16 event_id, node_t *node, void *data1, void *data2)
{
    events_lock();

    rs_assert(event_id < event_count);

    bool all_ok = TRUE;

    event_t *event = event_list + event_id;
    rs_debug(DEBUG_EVENT, "node '%s': executing event '%s.%s' @%d ms", node->phy_info->name, event->layer, event->name, rs_system->now);
    if (!event->handler(node, data1, data2)) {
        all_ok = FALSE;
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
