
#include <unistd.h>

#include "node.h"
#include "system.h"
#include "measure.h"
#include "gui/mainwin.h"


    /**** local function prototypes ****/

    /**** exported functions ****/

node_t *node_create()
{
    node_t *node = malloc(sizeof(node_t));

    node->measure_info = NULL;
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

    rpl_node_done(node);
    icmp_node_done(node);
    ip_node_done(node);
    mac_node_done(node);
    phy_node_done(node);
    measure_node_done(node);

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

    event_execute(sys_event_id_after_node_wake, node, NULL, NULL);

    return TRUE;
}

bool node_kill(node_t* node)
{
    rs_assert(node != NULL);

    if (!node->alive) {
        rs_error("node '%s': already dead", node->phy_info->name);
        return FALSE;
    }

    event_execute(sys_event_id_before_node_kill, node, NULL, NULL);

    node->alive = FALSE;

    return TRUE;
}


/**** local functions ****/

