
#include <unistd.h>

#include "node.h"
#include "system.h"
#include "gui/mainwin.h"


    /**** local function prototypes ****/

    /**** exported functions ****/

node_t *node_create()
{
    node_t *node = malloc(sizeof(node_t));

    node->phy_info = NULL;
    node->mac_info = NULL;
    node->ip_info = NULL;
    node->icmp_info = NULL;
    node->rpl_info = NULL;

    node->alive = FALSE;
    return node;
}

bool node_destroy(node_t *node)
{
    rs_assert(node != NULL);

    if (node->alive) {
        if (!node_kill(node)) {
            rs_error("failed to kill node '%s'", node->phy_info->name);
            return FALSE;
        }
    }

    free(node);

    return TRUE;
}

bool node_wake(node_t* node)
{
    rs_assert(node != NULL);

    if (node->alive) {
        rs_error("node '%s': already alive", node->phy_info->name);
        return FALSE;
    }

    node->alive = TRUE;

    rs_system_schedule_event(node, sys_event_id_after_node_wake, NULL, NULL, 0);

    return TRUE;
}

bool node_kill(node_t* node)
{
    rs_assert(node != NULL);

    if (!node->alive) {
        rs_error("node '%s': already dead", node->phy_info->name);
        return FALSE;
    }

    rs_system_schedule_event(node, sys_event_id_before_node_kill, NULL, NULL, 0);

    node->alive = FALSE;

    return TRUE;
}


/**** local functions ****/

