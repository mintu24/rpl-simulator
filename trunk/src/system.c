
#include <math.h>

#include "system.h"


    /**** global variables ****/

rs_system_t *           rs_system = NULL;


    /**** local function prototypes ****/


    /**** exported functions ****/

bool rs_system_create()
{
    rs_system = malloc(sizeof(rs_system_t));

    rs_system->node_list = NULL;
    rs_system->node_count = 0;

    rs_system->no_link_dist_thresh = DEFAULT_NO_LINK_DIST_THRESH;
    rs_system->phy_transmit_mode = DEFAULT_PHY_TRANSMIT_MODE;

    rs_system->width = DEFAULT_SYS_WIDTH;
    rs_system->height = DEFAULT_SYS_HEIGHT;

    rs_system->nodes_mutex = g_mutex_new();

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

    g_mutex_free(rs_system->nodes_mutex);

    free(rs_system);

    return TRUE;
}

coord_t rs_system_get_no_link_dist_thresh()
{
    rs_assert(rs_system != NULL);

    return rs_system->no_link_dist_thresh;
}

void rs_system_set_no_link_dist_thresh(coord_t thresh)
{
    rs_assert(rs_system != NULL);

}

uint8 rs_system_get_transmit_mode()
{
    rs_assert(rs_system != NULL);

    return rs_system->phy_transmit_mode;
}

void rs_system_set_transmit_mode(uint8 mode)
{
    rs_assert(rs_system != NULL);

    rs_system->phy_transmit_mode = mode;
}

coord_t rs_system_get_width()
{
    rs_assert(rs_system != NULL);

    return rs_system->width;
}

coord_t rs_system_get_height()
{
    rs_assert(rs_system != NULL);

    return rs_system->height;
}

void rs_system_set_width_height(coord_t width, coord_t height)
{
    rs_assert(rs_system != NULL);

    rs_system->width = width;
    rs_system->height = height;
}

bool rs_system_add_node(node_t *node)
{
    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    rs_system->node_list = realloc(rs_system->node_list, (++rs_system->node_count) * sizeof(node_t *));
    rs_system->node_list[rs_system->node_count - 1] = node;

    g_mutex_unlock(rs_system->nodes_mutex);

    return TRUE;
}

bool rs_system_remove_node(node_t *node)
{
    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    int i, pos = -1;
    for (i = 0; i < rs_system->node_count; i++) {
        if (rs_system->node_list[i] == node) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' not found", rs_system->node_list[i]);
        g_mutex_unlock(rs_system->nodes_mutex);
        return FALSE;
    }

    for (i = pos; i < rs_system->node_count - 1; i++) {
        rs_system->node_list[i] = rs_system->node_list[i + 1];
    }

    rs_system->node_count--;
    rs_system->node_list = realloc(rs_system->node_list, (rs_system->node_count) * sizeof(node_t *));

    g_mutex_unlock(rs_system->nodes_mutex);

    return TRUE;
}

node_t *rs_system_find_node_by_name(char *name)
{
    rs_assert(rs_system != NULL);
    rs_assert(name != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    int i;
    node_t *node = NULL;
    for (i = 0; i < rs_system->node_count; i++) {
        if (!strcmp(phy_node_get_name(rs_system->node_list[i]), name)) {
            node = rs_system->node_list[i];
            break;
        }
    }

    g_mutex_unlock(rs_system->nodes_mutex);

    return node;
}

node_t *rs_system_find_node_by_mac_address(char *address)
{
    rs_assert(rs_system != NULL);
    rs_assert(address != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    int i;
    node_t *node = NULL;
    for (i = 0; i < rs_system->node_count; i++) {
        if (!strcmp(mac_node_get_address(rs_system->node_list[i]), address)) {
            node = rs_system->node_list[i];
            break;
        }
    }

    g_mutex_unlock(rs_system->nodes_mutex);

    return node;
}

node_t *rs_system_find_node_by_ip_address(char *address)
{
    rs_assert(rs_system != NULL);
    rs_assert(address != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    int i;
    node_t *node = NULL;
    for (i = 0; i < rs_system->node_count; i++) {
        if (!strcmp(ip_node_get_address(rs_system->node_list[i]), address)) {
            node = rs_system->node_list[i];
            break;
        }
    }

    g_mutex_unlock(rs_system->nodes_mutex);

    return node;
}

node_t **rs_system_get_node_list(uint16 *node_count)
{
    rs_assert(rs_system != NULL);

    g_mutex_lock(rs_system->nodes_mutex);

    if (node_count != NULL) {
        *node_count = rs_system->node_count;
    }

    node_t **list = rs_system->node_list;

    g_mutex_unlock(rs_system->nodes_mutex);

    return list;
}

percent_t rs_system_get_link_quality(node_t *src_node, node_t *dst_node)
{
    rs_assert(rs_system != NULL);
    rs_assert(src_node != NULL);
    rs_assert(dst_node != NULL);

    coord_t distance = sqrt(pow(phy_node_get_x(src_node) - phy_node_get_x(dst_node), 2) + pow(phy_node_get_y(src_node) - phy_node_get_x(dst_node), 2));
    if (distance > rs_system_get_no_link_dist_thresh()) {
        distance = rs_system_get_no_link_dist_thresh();
    }

    percent_t dist_factor = (percent_t) (rs_system->no_link_dist_thresh - distance) / rs_system->no_link_dist_thresh;
    percent_t quality = phy_node_get_tx_power(src_node) * dist_factor;

    return quality;
}


    /**** local functions ****/

