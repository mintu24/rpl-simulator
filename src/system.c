
#include "system.h"


rs_system_t *rs_system_create()
{
    rs_system_t *system = (rs_system_t *) malloc(sizeof(rs_system_t));

    system->node_list = NULL;
    system->node_count = 0;

    return system;
}

bool rs_system_destroy(rs_system_t *system)
{
    if (system == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    free(system->node_list);

    free(system);

    return TRUE;
}

bool rs_system_add_node(rs_system_t *system, node_t *node)
{
    if (node == NULL || system == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

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
    if (node == NULL || system == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

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
    if (system == NULL || name == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    int i;
    for (i = 0; i < system->node_count; i++) {
        if (!strcmp(system->node_list[i]->name, name)) {
            return system->node_list[i];
        }
    }

    return NULL;
}

double rs_system_get_link_quality(rs_system_t *system, node_t *node1, node_t *node2)
{
    // TODO: implement this
}

bool rs_system_send_frame(rs_system_t *system, node_t *src_node, node_t *dst_node, mac_pdu_t *frame)
{
    // TODO: implement this
}
