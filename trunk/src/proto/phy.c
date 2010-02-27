
#include <math.h>

#include "phy.h"
#include "../system.h"


    /**** global variables ****/

uint16                  phy_event_node_wake;
uint16                  phy_event_node_kill;

uint16                  phy_event_pdu_send;
uint16                  phy_event_pdu_receive;

uint16                  phy_event_neighbor_attach;
uint16                  phy_event_neighbor_detach;

uint16                  phy_event_change_mobility;


    /**** local function prototypes ****/

static bool             event_handler_node_wake(node_t *node);
static bool             event_handler_node_kill(node_t *node);

static bool             event_handler_pdu_send(node_t *node, node_t *outgoing_node, phy_pdu_t *pdu);
static bool             event_handler_pdu_receive(node_t *node, node_t *incoming_node, phy_pdu_t *pdu);

static bool             event_handler_neighbor_attach(node_t *node, node_t *neighbor_node);
static bool             event_handler_neighbor_detach(node_t *node, node_t *neighbor_node);

static bool             event_handler_change_mobility(node_t *node, phy_mobility_t *mobility);

static void             update_neighbors(node_t *node);

static void             event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len);


    /**** exported functions ****/

bool phy_init()
{
    phy_event_node_wake = event_register("node_wake", "phy", (event_handler_t) event_handler_node_wake, NULL);
    phy_event_node_kill = event_register("node_kill", "phy", (event_handler_t) event_handler_node_kill, NULL);

    phy_event_pdu_send = event_register("pdu_send", "phy", (event_handler_t) event_handler_pdu_send, event_arg_str);
    phy_event_pdu_receive = event_register("pdu_receive", "phy", (event_handler_t) event_handler_pdu_receive, event_arg_str);
    
    phy_event_neighbor_attach = event_register("neighbor_attach", "phy", (event_handler_t) event_handler_neighbor_attach, event_arg_str);
    phy_event_neighbor_detach = event_register("neighbor_detach", "phy", (event_handler_t) event_handler_neighbor_detach, event_arg_str);

    phy_event_change_mobility = event_register("change_mobility", "phy", (event_handler_t) event_handler_change_mobility, event_arg_str);

    return TRUE;
}

bool phy_done()
{
    return TRUE;
}

phy_pdu_t *phy_pdu_create()
{
    phy_pdu_t *pdu = malloc(sizeof(phy_pdu_t));

    pdu->sdu = NULL;

    return pdu;
}

void phy_pdu_destroy(phy_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    if (pdu->sdu != NULL) {
        mac_pdu_destroy(pdu->sdu);
    }

    free(pdu);
}

phy_pdu_t *phy_pdu_duplicate(phy_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    phy_pdu_t *new_pdu = malloc(sizeof(phy_pdu_t));

    new_pdu->sdu = mac_pdu_duplicate(pdu->sdu);

    return new_pdu;
}

void phy_pdu_set_sdu(phy_pdu_t *pdu, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->sdu = sdu;
}

void phy_node_init(node_t *node, char *name, coord_t cx, coord_t cy)
{
    rs_assert(node != NULL);
    rs_assert(name != NULL);

    node->phy_info = malloc(sizeof(phy_node_info_t));

    node->phy_info->name = strdup(name);
    node->phy_info->cx = cx;
    node->phy_info->cy = cy;

    node->phy_info->mains_powered = FALSE;
    node->phy_info->tx_power = 0.5;
    node->phy_info->battery_level = 0.5;

    node->phy_info->mobility_start_x = 0;
    node->phy_info->mobility_start_y = 0;
    node->phy_info->mobility_start_time = 0;
    node->phy_info->mobility_stop_time = 0;
    node->phy_info->mobility_cos_alpha = 0;
    node->phy_info->mobility_sin_alpha = 0;
    node->phy_info->mobility_speed = 0;

    node->phy_info->neighbor_list = NULL;
    node->phy_info->neighbor_count = 0;

    node->phy_info->mobility_list = NULL;
    node->phy_info->mobility_count = 0;
}

void phy_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->phy_info != NULL) {
        if (node->phy_info->name != NULL)
            free(node->phy_info->name);

        if (node->phy_info->neighbor_list != NULL)
            free(node->phy_info->neighbor_list);

        while (node->phy_info->mobility_count > 0) {
            phy_node_rem_mobility(node, node->phy_info->mobility_count - 1);
        }

        free(node->phy_info);
        node->phy_info = NULL;
    }
}

void phy_node_set_name(node_t *node, const char *name)
{
    rs_assert(node != NULL);
    rs_assert(name != NULL);

    if (node->phy_info->name != NULL)
        free(node->phy_info->name);

    node->phy_info->name = strdup(name);
}

void phy_node_set_coords(node_t* node, coord_t cx, coord_t cy)
{
    rs_assert(node != NULL);

    node->phy_info->cx = cx;
    node->phy_info->cy = cy;

    if (node->alive) {
        update_neighbors(node);
    }
}

void phy_node_set_tx_power(node_t* node, percent_t tx_power)
{
    rs_assert(node != NULL);

    node->phy_info->tx_power = tx_power;

    if (node->alive) {
        update_neighbors(node);
    }
}

void phy_node_add_mobility(node_t *node, sim_time_t trigger_time, sim_time_t duration, coord_t dest_x, coord_t dest_y)
{
    rs_assert(node != NULL);

    phy_mobility_t *mobility = malloc(sizeof(phy_mobility_t));
    mobility->trigger_time = trigger_time;
    mobility->duration = duration;
    mobility->dest_x = dest_x;
    mobility->dest_y = dest_y;

    node->phy_info->mobility_list = realloc(node->phy_info->mobility_list, (node->phy_info->mobility_count + 1) * sizeof(phy_mobility_t *));
    node->phy_info->mobility_list[node->phy_info->mobility_count++] = mobility;

    if (trigger_time >= rs_system->now) {
        rs_system_schedule_event(node, phy_event_change_mobility, mobility, NULL, trigger_time);
    }
}

void phy_node_rem_mobility(node_t *node, uint16 index)
{
    rs_assert(node != NULL);
    rs_assert(index < node->phy_info->mobility_count);

    uint16 i;
    for (i = index; i < node->phy_info->mobility_count - 1; i++) {
        node->phy_info->mobility_list[i] = node->phy_info->mobility_list[i + 1];
    }

    rs_system_cancel_event(node, phy_event_change_mobility, node->phy_info->mobility_list[index], NULL, node->phy_info->mobility_list[index]->trigger_time);
    free(node->phy_info->mobility_list[index]);

    node->phy_info->mobility_list = realloc(node->phy_info->mobility_list, (--node->phy_info->mobility_count) * sizeof(phy_mobility_t *));

    if (node->phy_info->mobility_count == 0) {
        node->phy_info->mobility_list = NULL;
    }
}

void phy_node_update_mobility_coords(node_t *node)
{
    rs_assert(node != NULL);

    if (node->phy_info->mobility_speed == 0) {
        return;
    }

    if (node->phy_info->mobility_stop_time <= rs_system->now) {
        node->phy_info->mobility_speed = 0;
        return;
    }

    node->phy_info->cx = node->phy_info->mobility_start_x +
            node->phy_info->mobility_speed * (rs_system->now - node->phy_info->mobility_start_time) * node->phy_info->mobility_cos_alpha;
    node->phy_info->cy  = node->phy_info->mobility_start_y +
            node->phy_info->mobility_speed * (rs_system->now - node->phy_info->mobility_start_time) * node->phy_info->mobility_sin_alpha;

    if (node->alive) {
        update_neighbors(node);
    }
}

bool phy_node_send(node_t *node, node_t *outgoing_node, void *sdu)
{
    rs_assert(node != NULL);
    rs_assert(sdu != NULL);

    phy_pdu_t *phy_pdu = phy_pdu_create();
    phy_pdu_set_sdu(phy_pdu, sdu);

    if (!event_execute(phy_event_pdu_send, node, outgoing_node, phy_pdu)) {
        phy_pdu->sdu = NULL;
        phy_pdu_destroy(phy_pdu);
        return FALSE;
    }

    return TRUE;
}

bool phy_node_receive(node_t *node, node_t *incoming_node, phy_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    bool all_ok = event_execute(phy_event_pdu_receive, node, incoming_node, pdu);

    phy_pdu_destroy(pdu);

    return all_ok;
}

bool phy_node_add_neighbor(node_t* node, node_t* neighbor_node)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);

    if (phy_node_has_neighbor(node, neighbor_node)) {
        return FALSE;
    }

    node->phy_info->neighbor_list = realloc(node->phy_info->neighbor_list, (node->phy_info->neighbor_count + 1) * sizeof(node_t *));
    node->phy_info->neighbor_list[node->phy_info->neighbor_count++] = neighbor_node;

    return TRUE;
}

bool phy_node_rem_neighbor(node_t* node, node_t *neighbor_node)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);

    int i, pos = -1;
    for (i = 0; i < node->phy_info->neighbor_count; i++) {
        if (node->phy_info->neighbor_list[i] == neighbor_node) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        return FALSE;
    }

    for (i = pos; i < node->phy_info->neighbor_count - 1; i++) {
        node->phy_info->neighbor_list[i] = node->phy_info->neighbor_list[i + 1];
    }

    node->phy_info->neighbor_count--;
    node->phy_info->neighbor_list = realloc(node->phy_info->neighbor_list, (node->phy_info->neighbor_count) * sizeof(node_t *));
    if (node->phy_info->neighbor_count == 0) {
        node->phy_info->neighbor_list = NULL;
    }

    return TRUE;
}

bool phy_node_has_neighbor(node_t* node, node_t *neighbor_node)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);

    int i;
    for (i = 0; i < node->phy_info->neighbor_count; i++) {
        if (node->phy_info->neighbor_list[i] == neighbor_node) {
            return TRUE;
        }
    }

    return FALSE;
}


    /**** local functions ****/

static bool event_handler_node_wake(node_t *node)
{
    update_neighbors(node);

    uint16 i; /* schedule all the mobility changes that were programmed */
    for (i = 0; i < node->phy_info->mobility_count; i++) {
        phy_mobility_t *mobility = node->phy_info->mobility_list[i];
        rs_system_schedule_event(node, phy_event_change_mobility, mobility, NULL, mobility->trigger_time);
    }

    return TRUE;
}

static bool event_handler_node_kill(node_t *node)
{
    rs_assert(node != NULL);

    /* removing all the phy neighboring reference to this node */
    uint16 node_count;
    node_t** node_list = rs_system_get_node_list_copy(&node_count);

    uint16 i;
    for(i = 0; i < node->phy_info->neighbor_count; i++) {
        node_t* other_node = node->phy_info->neighbor_list[i];

        phy_node_rem_neighbor(other_node, node);
    }

    if (node_list != NULL) {
        free(node_list);
    }

    if (node->phy_info->neighbor_list != NULL) {
        free(node->phy_info->neighbor_list);
    }

    node->phy_info->neighbor_list = NULL;
    node->phy_info->neighbor_count = 0;

    return TRUE;
}

static bool event_handler_pdu_send(node_t *node, node_t *outgoing_node, phy_pdu_t *pdu)
{
    return rs_system_send(node, outgoing_node, pdu);
}

static bool event_handler_pdu_receive(node_t *node, node_t *incoming_node, phy_pdu_t *pdu)
{
    mac_pdu_t *mac_pdu = pdu->sdu;
    pdu->sdu = NULL;

    return mac_node_receive(node, incoming_node, mac_pdu);
}

static bool event_handler_neighbor_attach(node_t *node, node_t *neighbor_node)
{
    measure_connect_update();

    return TRUE;
}

static bool event_handler_neighbor_detach(node_t *node, node_t *neighbor_node)
{
    measure_connect_update();

    return TRUE;
}

static bool event_handler_change_mobility(node_t *node, phy_mobility_t *mobility)
{
    if (mobility->duration == 0) { /* instant position change */
        node->phy_info->mobility_speed = 0;
        node->phy_info->cx = mobility->dest_x;
        node->phy_info->cy = mobility->dest_y;

        if (node->alive) {
            update_neighbors(node);
        }
    }
    else {
        coord_t dist = sqrt(pow(mobility->dest_x - node->phy_info->cx, 2) + pow(mobility->dest_y - node->phy_info->cy, 2));

        node->phy_info->mobility_start_x = node->phy_info->cx;
        node->phy_info->mobility_start_y = node->phy_info->cy;
        node->phy_info->mobility_start_time = rs_system->now;
        node->phy_info->mobility_stop_time = rs_system->now + mobility->duration;
        node->phy_info->mobility_sin_alpha = sin(atan2(mobility->dest_y - node->phy_info->cy, mobility->dest_x - node->phy_info->cx));
        node->phy_info->mobility_cos_alpha = cos(atan2(mobility->dest_y - node->phy_info->cy, mobility->dest_x - node->phy_info->cx));
        node->phy_info->mobility_speed = dist / mobility->duration;
    }

    return TRUE;
}

static void update_neighbors(node_t *node)
{
    rs_assert(node != NULL);

    uint16 i, node_count;
    node_t **node_list = rs_system_get_node_list_copy(&node_count);

    for (i = 0; i < node_count; i++) {
        node_t *other_node = node_list[i];

        if (other_node == node) { /* we're not a neighbor of ourselves */
            continue;
        }

        if (!other_node->alive) { /* ignore dead neighbors */
            continue;
        }

        /* node to other_node link quality */
        if (rs_system_link_quality_enough(node, other_node)) {
            if (phy_node_add_neighbor(other_node, node)) { /* returns true if the neighbor wasn't present before */
                rs_system_schedule_event(other_node, phy_event_neighbor_attach, node, NULL, 0);
            }
        }
        else {
            if (phy_node_rem_neighbor(other_node, node)) { /* returns true if the neighbor was present before */
                rs_system_schedule_event(other_node, phy_event_neighbor_detach, node, NULL, 0);
            }
        }

        /* other_node to node link quality */
        if (rs_system_link_quality_enough(other_node, node)) {
            if (phy_node_add_neighbor(node, other_node)) { /* returns true if the neighbor wasn't present before */
                rs_system_schedule_event(node, phy_event_neighbor_attach, other_node, NULL, 0);
            }
        }
        else {
            if (phy_node_rem_neighbor(node, other_node)) { /* returns true if the neighbor was present before */
                rs_system_schedule_event(node, phy_event_neighbor_detach, other_node, NULL, 0);
            }
        }
    }
}

static void event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len)
{
    str1[0] = '\0';
    str2[0] = '\0';

    if (event_id == phy_event_pdu_send) {
        node_t *node = data1;

        snprintf(str1, len, "outgoing_node = '%s'", (node != NULL ? node->phy_info->name : "<<unknown>>"));
    }
    else if (event_id == phy_event_pdu_receive) {
        node_t *node = data1;

        snprintf(str1, len, "incoming_node = '%s'", (node != NULL ? node->phy_info->name : "<<unknown>>"));
    }
    else if (event_id == phy_event_neighbor_attach) {
        node_t *neighbor_node = data1;

        snprintf(str1, len, "neighbor = '%s'", (neighbor_node != NULL ? neighbor_node->phy_info->name : "<<unknown>>"));
    }
    else if (event_id == phy_event_neighbor_detach) {
        node_t *neighbor_node = data1;

        snprintf(str1, len, "neighbor = '%s'", (neighbor_node != NULL ? neighbor_node->phy_info->name : "<<unknown>>"));
    }
    else if (event_id == phy_event_change_mobility) {
        phy_mobility_t *mobility = data1;

        snprintf(str1, len, "mobility = {duration = %d, dest_x = %.02f, dest_y = %.02f}",
                mobility->duration, mobility->dest_x, mobility->dest_y);
    }
}
