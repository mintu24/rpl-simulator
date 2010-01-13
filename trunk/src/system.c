
#include "system.h"
#include "math.h"


rs_system_t *rs_system_create()
{
    rs_system_t *system = (rs_system_t *) malloc(sizeof(rs_system_t));

    system->node_list = NULL;
    system->node_count = 0;

    system->no_link_dist_thresh = DEFAULT_NO_LINK_DIST_THRESH;
    system->phy_transmit_mode = DEFAULT_PHY_TRANSMIT_MODE;

    return system;
}

bool rs_system_destroy(rs_system_t *system)
{
    rs_assert(system != NULL);

    free(system->node_list);

    free(system);

    return TRUE;
}

bool rs_system_add_node(rs_system_t *system, node_t *node)
{
    rs_assert(system != NULL);
    rs_assert(node != NULL);

    if (rs_system_find_node_by_name(system, node->name)) {
        rs_error("a node with name '%s' already exists", node->name);
        return FALSE;
    }

    system->node_list = (node_t **) realloc(system->node_list, (++system->node_count) * sizeof(node_t *));
    system->node_list[system->node_count - 1] = node;

    return TRUE;
}

bool rs_system_remove_node(rs_system_t *system, node_t *node)
{
    rs_assert(system != NULL);
    rs_assert(node != NULL);

    int i, pos = -1;
    for (i = 0; i < system->node_count; i++) {
        if (system->node_list[i] == node) {
            pos = i;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' not found", node->name);
        return FALSE;
    }

    for (i = pos; i < system->node_count - 1; i++) {
        system->node_list[i] = system->node_list[i + 1];
    }

    system->node_count--;

    return TRUE;
}

node_t *rs_system_find_node_by_name(rs_system_t *system, char *name)
{
    rs_assert(system != NULL);
    rs_assert(name != NULL);

    int i;
    for (i = 0; i < system->node_count; i++) {
        if (!strcmp(system->node_list[i]->name, name)) {
            return system->node_list[i];
        }
    }

    return NULL;
}

percent_t rs_system_get_link_quality(rs_system_t *system, node_t *src_node, node_t *dst_node)
{
    rs_assert(system != NULL);
    rs_assert(src_node != NULL);
    rs_assert(dst_node != NULL);

    coord_t distance = sqrt(pow(src_node->cx - dst_node->cx, 2) + pow(src_node->cy - dst_node->cy, 2));
    if (distance > system->no_link_dist_thresh) {
        distance = system->no_link_dist_thresh;
    }

    percent_t dist_factor = (percent_t) (system->no_link_dist_thresh - distance) / system->no_link_dist_thresh;
    percent_t quality = src_node->phy_info->power_level * dist_factor;

    return quality;
}

bool rs_system_send_message(rs_system_t *system, node_t *src_node, node_t *dst_node, phy_pdu_t *message)
{
    rs_assert(system != NULL);
    rs_assert(src_node != NULL);
    rs_assert(dst_node != NULL);
    rs_assert(message != NULL);

    // todo node_execute(event...)
    //phy_event_before_pdu_sent(src_node, message);
    bool all_ok = node_receive_pdu(dst_node, message, system->phy_transmit_mode);

    return all_ok;
}
