
// todo source routing

#include <ctype.h>

#include "ip.h"
#include "../system.h"


    /**** global variables ****/

uint16                      ip_event_node_wake;
uint16                      ip_event_node_kill;

uint16                      ip_event_pdu_send;
uint16                      ip_event_pdu_send_timeout_check;
uint16                      ip_event_pdu_receive;

uint16                      ip_event_neighbor_cache_timeout_check;


    /**** local function prototypes ****/

static bool                 event_handler_node_wake(node_t *node);
static bool                 event_handler_node_kill(node_t *node);

static bool                 event_handler_pdu_send(node_t *node, node_t *incoming_node, ip_pdu_t *pdu);
static bool                 event_handler_pdu_send_timeout_check(node_t *node, ip_send_info_t *ip_send_info, ip_pdu_t *pdu);
static bool                 event_handler_pdu_receive(node_t *node, node_t *incoming_node, ip_pdu_t *pdu);

static bool                 event_handler_neighbor_cache_timeout(node_t *node, ip_neighbor_t *neighbor);

static uint8 *              route_expand_to_bits(char *dst, uint8 prefix_len);

static void                 event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len);


    /**** exported functions ****/

bool ip_init()
{
    ip_event_node_wake = event_register("node_wake", "ip", (event_handler_t) event_handler_node_wake, NULL);
    ip_event_node_kill = event_register("node_kill", "ip", (event_handler_t) event_handler_node_kill, NULL);

    ip_event_pdu_send = event_register("pdu_send", "ip", (event_handler_t) event_handler_pdu_send, event_arg_str);
    ip_event_pdu_send_timeout_check = event_register("pdu_send_timeout_check", "ip", (event_handler_t) event_handler_pdu_send_timeout_check, event_arg_str);
    ip_event_pdu_receive = event_register("pdu_receive", "ip", (event_handler_t) event_handler_pdu_receive, event_arg_str);

    ip_event_neighbor_cache_timeout_check = event_register("neighbor_cache_timeout_check", "ip", (event_handler_t) event_handler_neighbor_cache_timeout, event_arg_str);

    return TRUE;
}

bool ip_done()
{
    return TRUE;
}

ip_send_info_t *ip_send_info_create(node_t *incoming_node, node_t *first_next_hop, node_t **next_hop_list, uint16 next_hop_count)
{
    ip_send_info_t *ip_send_info = malloc(sizeof(ip_send_info_t));

    ip_send_info->incoming_node = incoming_node;
    ip_send_info->next_hop_index = 0;

    uint16 i;
    for (i = 0; i < next_hop_count; i++) {
        if (next_hop_list[i] == first_next_hop) {
            first_next_hop = NULL;
            break;
        }
    }

    if (first_next_hop != NULL) {
        ip_send_info->next_hop_list = malloc((next_hop_count + 1) * sizeof(node_t *));
        ip_send_info->next_hop_list[0] = first_next_hop;
        ip_send_info->next_hop_count = 1;
    }
    else if (next_hop_count > 0) {
        ip_send_info->next_hop_list = malloc(next_hop_count * sizeof(node_t *));
        ip_send_info->next_hop_count = 0;
    }
    else {
        ip_send_info->next_hop_list = NULL;
        ip_send_info->next_hop_count = 0;
    }

    for (i = 0; i < next_hop_count; i++) {
        ip_send_info->next_hop_list[ip_send_info->next_hop_count++] = next_hop_list[i];
    }

    return ip_send_info;
}

void ip_send_info_destroy(ip_send_info_t *ip_send_info)
{
    rs_assert(ip_send_info != NULL);

    if (ip_send_info->next_hop_list != NULL) {
        free(ip_send_info->next_hop_list);
    }

    free(ip_send_info);
}

ip_pdu_t *ip_pdu_create(char *src_address, char *dst_address)
{
    rs_assert(dst_address != NULL);
    rs_assert(src_address != NULL);

    ip_pdu_t *pdu = malloc(sizeof(ip_pdu_t));

    pdu->src_address = strdup(src_address);
    pdu->dst_address = strdup(dst_address);

    pdu->flow_label = malloc(sizeof(ip_flow_label_t));
    pdu->flow_label->forward_error = FALSE;
    pdu->flow_label->from_sibling = FALSE;
    pdu->flow_label->going_down = FALSE;
    pdu->flow_label->rank_error = FALSE;
    pdu->flow_label->sender_rank = 0;

    pdu->next_header = -1;
    pdu->sdu = NULL;

    pdu->queued = FALSE;

    return pdu;
}

void ip_pdu_destroy(ip_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    if (pdu->dst_address != NULL)
        free(pdu->dst_address);
    if (pdu->src_address != NULL)
        free(pdu->src_address);
    if (pdu->flow_label != NULL)
        free(pdu->flow_label);

    if (pdu->sdu != NULL) {
        switch (pdu->next_header) {

            case IP_NEXT_HEADER_ICMP :
                icmp_pdu_destroy(pdu->sdu);

                break;

            case IP_NEXT_HEADER_MEASURE :
                measure_pdu_destroy(pdu->sdu);

                break;

        }
    }

    free(pdu);
}

ip_pdu_t *ip_pdu_duplicate(ip_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    ip_pdu_t *new_pdu = malloc(sizeof(ip_pdu_t));

    new_pdu->dst_address = strdup(pdu->dst_address);
    new_pdu->src_address = strdup(pdu->src_address);

    if (pdu->flow_label != NULL) {
        new_pdu->flow_label = malloc(sizeof(ip_flow_label_t));
        *new_pdu->flow_label = *pdu->flow_label;
    }

    new_pdu->next_header = pdu->next_header;

    switch (pdu->next_header) {
        case IP_NEXT_HEADER_ICMP :
            new_pdu->sdu = icmp_pdu_duplicate(pdu->sdu);

            break;

        case IP_NEXT_HEADER_MEASURE :
            new_pdu->sdu = measure_pdu_duplicate(pdu->sdu);

            break;

        default:
            rs_error("invalid ip next header '0x%04X'", pdu->next_header);
            new_pdu->sdu = NULL;
    }

    new_pdu->queued = pdu->queued;

    return new_pdu;
}

void ip_pdu_set_sdu(ip_pdu_t *pdu, uint16 next_header, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->next_header = next_header;
    pdu->sdu = sdu;
}

void ip_node_init(node_t *node, char *address)
{
    rs_assert(node != NULL);
    rs_assert(address != NULL);

    node->ip_info = malloc(sizeof(ip_node_info_t));

    node->ip_info->address = strdup(address);

    node->ip_info->route_list = NULL;
    node->ip_info->route_count = 0;

    node->ip_info->neighbor_list = NULL;
    node->ip_info->neighbor_count = 0;

    node->ip_info->enqueued_count = 0;
    node->ip_info->busy = FALSE;
}

void ip_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->ip_info != NULL) {
        if (node->ip_info->address != NULL)
            free(node->ip_info->address);

        if (node->ip_info->route_list != NULL) {
            while (node->ip_info->route_count > 0) {
                ip_route_t *route = node->ip_info->route_list[node->ip_info->route_count - 1];

                if (route->dst != NULL) {
                    free(route->dst);
                }
                if (route->dst_bit_expanded != NULL) {
                    free(route->dst_bit_expanded);
                }
                if (route->further_info != NULL) {
                    free(route->further_info);
                }

                free(route);

                node->ip_info->route_count--;
            }

            free(node->ip_info->route_list);
        }

        free(node->ip_info);
        node->ip_info = NULL;
    }
}

void ip_node_set_address(node_t *node, const char *address)
{
    rs_assert(node != NULL);
    rs_assert(address != NULL);

    if (node->ip_info->address != NULL)
        free(node->ip_info->address);

    node->ip_info->address = strdup(address);
}

ip_route_t *ip_node_add_route(node_t *node, char *dst, uint8 prefix_len, node_t *next_hop, uint8 type, void *further_info)
{
    rs_assert(node != NULL);
    rs_assert(dst != NULL);
    rs_assert(next_hop != NULL);
    rs_assert(strlen(dst) * 4 >= prefix_len);

    rs_debug(DEBUG_IP, "node '%s': adding route '%s/%d' via '%s'",
            node->phy_info->name, dst, prefix_len, next_hop->phy_info->name);

    ip_route_t *route = malloc(sizeof(ip_route_t));

    route->dst = strdup(dst);
    route->prefix_len = prefix_len;
    route->next_hop = next_hop;
    route->dst_bit_expanded = route_expand_to_bits(dst, prefix_len);
    route->type = type;
    route->further_info = further_info;
    route->update_time = rs_system->now;

    node->ip_info->route_list = realloc(node->ip_info->route_list, (node->ip_info->route_count + 1) * sizeof(ip_route_t *));
    node->ip_info->route_list[node->ip_info->route_count++] = route;

    return route;
}

void ip_node_rem_route(node_t *node, ip_route_t *route)
{
    rs_assert(node != NULL);

    int32 i;
    for (i = node->ip_info->route_count - 1; i >= 0; i--) { /* going backwards to increase performance a little bit */
        if (route == node->ip_info->route_list[i]) {
            rs_debug(DEBUG_IP, "node '%s': removing route '%s/%d' via '%s'",
                    node->phy_info->name, route->dst, route->prefix_len, route->next_hop->phy_info->name);

            free(route->dst);
            if (route->further_info != NULL) {
                free(route->further_info);
            }
            free(route);

            uint16 j;
            for (j = i; j < node->ip_info->route_count - 1; j++) {
                node->ip_info->route_list[j] = node->ip_info->route_list[j + 1];
            }

            node->ip_info->route_count--;

            break;
        }
    }

    node->ip_info->route_list = realloc(node->ip_info->route_list, node->ip_info->route_count * sizeof(ip_route_t *));
    if (node->ip_info->route_count == 0) {
        node->ip_info->route_list = NULL;
    }
}

void ip_node_rem_routes(node_t *node, char *dst, int8 prefix_len, node_t *next_hop, int8 type)
{
    rs_assert(node != NULL);

    int32 i;
    for (i = node->ip_info->route_count - 1; i >= 0; i--) { /* going backwards to increase performance a little bit */
        ip_route_t *route = node->ip_info->route_list[i];

        if (dst != NULL && (strcmp(route->dst, dst) != 0)) {
            continue;
        }

        if (prefix_len >= 0 && (route->prefix_len != prefix_len)) {
            continue;
        }

        if (next_hop != NULL && (route->next_hop != next_hop)) {
            continue;
        }

        if (type >= 0 && (route->type != type)) {
            continue;
        }

        rs_debug(DEBUG_IP, "node '%s': removing route '%s/%d' via '%s'",
                node->phy_info->name, route->dst, route->prefix_len, route->next_hop->phy_info->name);

        free(route->dst);
        if (route->further_info != NULL) {
            free(route->further_info);
        }
        free(route);

        uint16 j;
        for (j = i; j < node->ip_info->route_count - 1; j++) {
            node->ip_info->route_list[j] = node->ip_info->route_list[j + 1];
        }

        node->ip_info->route_count--;
    }

    node->ip_info->route_list = realloc(node->ip_info->route_list, node->ip_info->route_count * sizeof(ip_route_t *));
    if (node->ip_info->route_count == 0) {
        node->ip_info->route_list = NULL;
    }
}

ip_route_t **ip_node_get_routes(node_t *node, uint16 *route_count, char *dst, int8 prefix_len, node_t *next_hop, int8 type)
{
    rs_assert(node != NULL);

    ip_route_t **route_list = NULL;
    *route_count = 0;

    uint16 i;
    for (i = 0; i < node->ip_info->route_count; i++) {
        ip_route_t *route = node->ip_info->route_list[i];

        if (dst != NULL && (strcmp(route->dst, dst) != 0)) {
            continue;
        }

        if (prefix_len >= 0 && (route->prefix_len != prefix_len)) {
            continue;
        }

        if (next_hop != NULL && (route->next_hop != next_hop)) {
            continue;
        }

        if (type >= 0 && (route->type != type)) {
            continue;
        }

        route_list = realloc(route_list, ((*route_count) + 1) * sizeof(ip_route_t *));
        route_list[(*route_count)++] = route;
    }

    return route_list;
}

ip_route_t *ip_node_get_next_hop_route(node_t *node, char *dst_address)
{
    rs_assert(node != NULL);
    rs_assert(dst_address != NULL);

    if (strcmp(node->ip_info->address, dst_address) == 0) { /* sending to the node itself? */
        return NULL;
    }

    uint16 i, j;
    int16 best_prefix_len = -1;
    int16 best_type = 255;  /* a big number */
    ip_route_t *best_route = NULL;

    uint8 dst_len = strlen(dst_address) * 4;
    uint8 *dst_bits_expanded = route_expand_to_bits(dst_address, dst_len);

    for (i = 0; i < node->ip_info->route_count; i++) {
        ip_route_t *route = node->ip_info->route_list[i];

        if (route->type > best_type) { /* prefer the smallest type */
            continue;
        }
        else if (route->type == best_type) {
            if (route->prefix_len <= best_prefix_len) { /* prefer the longest prefix */
                continue;
            }
        }

        if (dst_len < route->prefix_len) {
            rs_warn("node '%s': invalid IP address '%s' for route '%s/%d'", node->phy_info->name, dst_address, route->dst, route->prefix_len);
            continue;
        }

        if (route->dst_bit_expanded == NULL) {
            route->dst_bit_expanded = route_expand_to_bits(route->dst, route->prefix_len);
        }

        bool match = TRUE;
        for (j = 0; j < route->prefix_len; j++) {
            if (route->dst_bit_expanded[j] != dst_bits_expanded[j]) {
                match = FALSE;
                break;
            }
        }

        if (match) {
            best_route = route;
            best_prefix_len = route->prefix_len;
            best_type = route->type;
        }
    }

    return best_route;
}

ip_neighbor_t *ip_node_add_neighbor(node_t *node, node_t *neighbor_node)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);

    rs_debug(DEBUG_IP, "node '%s': adding neighbor '%s'", node->phy_info->name, neighbor_node->phy_info->name);

    ip_neighbor_t *neighbor = malloc(sizeof(ip_neighbor_t));
    neighbor->node = neighbor_node;
    neighbor->last_packet_time = rs_system->now;

    node->ip_info->neighbor_list = realloc(node->ip_info->neighbor_list, (node->ip_info->neighbor_count + 1) * sizeof(ip_neighbor_t *));
    node->ip_info->neighbor_list[node->ip_info->neighbor_count] = neighbor;
    node->ip_info->neighbor_count++;

    /* install a route to this neighbor */
    ip_node_add_route(node, neighbor_node->ip_info->address, strlen(neighbor_node->ip_info->address) * 4,
            neighbor_node, IP_ROUTE_TYPE_CONNECTED, NULL);

    return neighbor;
}

bool ip_node_rem_neighbor(node_t *node, ip_neighbor_t *neighbor)
{
    rs_assert(node != NULL);
    rs_assert(neighbor != NULL);

    int32 i, pos = -1;
    for (i = 0; i < node->ip_info->neighbor_count; i++) {
        if (node->ip_info->neighbor_list[i] == neighbor) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        if (neighbor->node != NULL)
            rs_error("node '%s': doesn't have neighbor '%s'", node->phy_info->name, neighbor->node->phy_info->name);

        return FALSE;
    }

    rs_debug(DEBUG_IP, "node '%s': removing neighbor '%s'", node->phy_info->name, neighbor->node->phy_info->name);

    for (i = pos; i < node->ip_info->neighbor_count - 1; i++) {
        node->ip_info->neighbor_list[i] = node->ip_info->neighbor_list[i + 1];
    }

    node->ip_info->neighbor_list = realloc(node->ip_info->neighbor_list, (node->ip_info->neighbor_count - 1) * sizeof(ip_neighbor_t *));
    node->ip_info->neighbor_count--;
    if (node->ip_info->neighbor_count == 0) {
        node->ip_info->neighbor_list = NULL;
    }

    /* remove the route to this neighbor */
    if (neighbor->node != NULL) {
        ip_node_rem_routes(node, neighbor->node->ip_info->address, strlen(neighbor->node->ip_info->address) * 4, neighbor->node, IP_ROUTE_TYPE_CONNECTED);
    }

    free(neighbor);

    return TRUE;
}

ip_neighbor_t* ip_node_find_neighbor_by_node(node_t *node, node_t *neighbor_node)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);

    uint16 i;
    for (i = 0; i < node->ip_info->neighbor_count; i++) {
        if (node->ip_info->neighbor_list[i]->node == neighbor_node) {
            return node->ip_info->neighbor_list[i];
        }
    }

    return NULL;
}

bool ip_node_send(node_t *node, char *dst_ip_address, uint16 next_header, void *sdu)
{
    rs_assert(node != NULL);

    ip_pdu_t *ip_pdu = ip_pdu_create(node->ip_info->address, dst_ip_address != NULL ? dst_ip_address : "");
    ip_pdu_set_sdu(ip_pdu, next_header, sdu);

    if (!event_execute(ip_event_pdu_send, node, NULL, ip_pdu)) {
        ip_pdu->sdu = NULL;
        ip_pdu_destroy(ip_pdu);

        return FALSE;
    }

    measure_node_add_ip_packet(node, TRUE);

    return TRUE;
}

bool ip_node_forward(node_t *node, node_t *incoming_node, ip_pdu_t *pdu) {
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    pdu = ip_pdu_duplicate(pdu);

    if (!event_execute(ip_event_pdu_send, node, incoming_node, pdu)) {
        ip_pdu_destroy(pdu);

        return FALSE;
    }

    measure_node_add_ip_packet(node, FALSE);

    return TRUE;
}

bool ip_node_receive(node_t *node, node_t *incoming_node, ip_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    bool all_ok = event_execute(ip_event_pdu_receive, node, incoming_node, pdu);

    ip_pdu_destroy(pdu);

    return all_ok;
}


    /**** local functions ****/

static bool event_handler_node_wake(node_t *node)
{
    return TRUE;
}

static bool event_handler_node_kill(node_t *node)
{
    while (node->ip_info->neighbor_count > 0) {
        ip_neighbor_t *neighbor = node->ip_info->neighbor_list[node->ip_info->neighbor_count - 1];

        event_execute(rpl_event_neighbor_detach, node, neighbor->node, NULL);
        ip_node_rem_neighbor(node, neighbor);
    }

    node->ip_info->enqueued_count = 0;
    node->ip_info->busy = FALSE;

    ip_node_rem_routes(node, NULL, -1, NULL, IP_ROUTE_TYPE_CONNECTED);
    ip_node_rem_routes(node, NULL, -1, NULL, IP_ROUTE_TYPE_RPL_DAO);
    ip_node_rem_routes(node, NULL, -1, NULL, IP_ROUTE_TYPE_RPL_DIO);

    return TRUE;
}

static bool event_handler_pdu_send(node_t *node, node_t *incoming_node, ip_pdu_t *pdu)
{
    if (node->ip_info->busy) { /* IP layer busy */
        if (node->ip_info->enqueued_count < rs_system->ip_queue_size) { /* still enough room in IP queue */
            rs_debug(DEBUG_IP, "node '%s': IP layer is busy (%d), retrying later", node->phy_info->name, node->ip_info->enqueued_count);
            rs_system_schedule_event(node, ip_event_pdu_send, incoming_node, pdu, rs_system->ip_pdu_timeout);

            if (!pdu->queued) { /* "add" the packet to "queue" only once */
                node->ip_info->enqueued_count++;
                pdu->queued = TRUE;
            }

            return TRUE;
        }
        else { /* IP queue full */
            rs_debug(DEBUG_IP, "node '%s': IP layer queue is full, dropping packet", node->phy_info->name, node->ip_info->enqueued_count);

            if (pdu->next_header == IP_NEXT_HEADER_MEASURE) {
                measure_pdu_t *measure_pdu = pdu->sdu;
                rs_assert(measure_pdu != NULL);

                event_execute(measure_event_connect_hop_failed, measure_pdu->measuring_node, measure_pdu->dst_node, node);
            }

            return FALSE;
        }
    }
    else { /* IP layer idle */
        pdu->queued = FALSE;

        if (strlen(pdu->dst_address) == 0) { /* broadcast */
            if (mac_node_send(node, NULL, MAC_TYPE_IP, pdu)) {
                rs_system_schedule_event(node, ip_event_pdu_send_timeout_check, NULL, pdu, rs_system->ip_pdu_timeout);
                node->ip_info->busy = TRUE;
            }
            else {
                rs_error("node '%s': MAC layer should never be busy, please increase IP PDU timeout", node->phy_info->name);
            }

            return TRUE;
        }
        else { /* unicast, must route the packet */
            ip_route_t *route = ip_node_get_next_hop_route(node, pdu->dst_address);

            node_t *next_hop = NULL;

            if (route == NULL) {
                rs_debug(DEBUG_IP, "node '%s': destination '%s' not reachable, querying RPL for next hops", node->phy_info->name, pdu->dst_address);
            }
            else {
                next_hop = route->next_hop;
            }

            node_t *redir_next_hop = rpl_node_process_outgoing_flow_label(node, incoming_node, next_hop, pdu);
            if (redir_next_hop != next_hop) { /* RPL redirected us, don't try all the parents & stuff in case of failure */
                return mac_node_send(node, redir_next_hop, MAC_TYPE_IP, pdu);
            }

            uint16 next_hop_count;
            node_t **next_hop_list = rpl_node_get_next_hop_list(node, &next_hop_count);

            ip_send_info_t *ip_send_info = ip_send_info_create(incoming_node, next_hop, next_hop_list, next_hop_count);

            if (next_hop_list != NULL) {
                free(next_hop_list);
            }

            if (ip_send_info->next_hop_count > 0) {
                if (mac_node_send(node, ip_send_info->next_hop_list[0], MAC_TYPE_IP, pdu)) {
                    rs_system_schedule_event(node, ip_event_pdu_send_timeout_check, ip_send_info, pdu, rs_system->ip_pdu_timeout);
                    node->ip_info->busy = TRUE;
                }
                else {
                    rs_error("node '%s': MAC layer should never be busy, please increase IP PDU timeout", node->phy_info->name);
                }

                return TRUE;
            }
            else {
                rs_debug(DEBUG_IP, "node '%s': destination '%s' not reachable at all", node->phy_info->name, pdu->dst_address);

                if (pdu->next_header == IP_NEXT_HEADER_MEASURE) {
                    measure_pdu_t *measure_pdu = pdu->sdu;
                    rs_assert(measure_pdu != NULL);

                    event_execute(measure_event_connect_hop_failed, measure_pdu->measuring_node, measure_pdu->dst_node, node);
                }

                return FALSE;
            }
        }
    }
}

static bool event_handler_pdu_send_timeout_check(node_t *node, ip_send_info_t *ip_send_info, ip_pdu_t *pdu)
{
    if (node->mac_info->error) {
        if (ip_send_info != NULL) { /* unicast */
            if (ip_send_info->next_hop_index < ip_send_info->next_hop_count - 1) { /* still some next_hops to try */
                rs_debug(DEBUG_IP, "node '%s': hop '%s' failed, trying hop '%s'",
                        node->phy_info->name,
                        ip_send_info->next_hop_list[ip_send_info->next_hop_index]->phy_info->name,
                        ip_send_info->next_hop_list[ip_send_info->next_hop_index + 1]->phy_info->name);

                if (mac_node_send(node, ip_send_info->next_hop_list[++ip_send_info->next_hop_index], MAC_TYPE_IP, pdu)) {
                    rs_system_schedule_event(node, ip_event_pdu_send_timeout_check, ip_send_info, pdu, rs_system->ip_pdu_timeout);
                }
                else {
                    rs_error("node '%s': MAC layer should never be busy, please increase IP PDU timeout", node->phy_info->name);
                }
            }
            else { /* no more next_hops to try */
                rs_debug(DEBUG_IP, "node '%s': all next hops failed, dropping packet", node->phy_info->name);

                rs_debug(DEBUG_RPL, "node '%s': a forwarding failure occurred, with src = '%s' and dst = '%s'",
                        node->phy_info->name,
                        ip_send_info->incoming_node != NULL ? ip_send_info->incoming_node->phy_info->name : "<<local>>",
                        pdu->src_address, pdu->dst_address);

                event_execute(rpl_event_forward_failure, node, ip_send_info->incoming_node, pdu);

                if (pdu->next_header == IP_NEXT_HEADER_MEASURE) {
                    measure_pdu_t *measure_pdu = pdu->sdu;
                    rs_assert(measure_pdu != NULL);

                    event_execute(measure_event_connect_hop_failed, measure_pdu->measuring_node, measure_pdu->dst_node, node);
                }

                ip_pdu_destroy(pdu);
                ip_send_info_destroy(ip_send_info);

                node->ip_info->busy = FALSE;
                if (node->ip_info->enqueued_count > 0) {
                    node->ip_info->enqueued_count--;
                }
            }
        }
        else { /* broadcast */
            rs_debug(DEBUG_IP, "node '%s': no neighbor to receive broadcast", node->phy_info->name);

            ip_pdu_destroy(pdu);

            node->ip_info->busy = FALSE;
            if (node->ip_info->enqueued_count > 0) {
                node->ip_info->enqueued_count--;
            }
        }
    }
    else { /* at least one node received the message */
        if (ip_send_info != NULL) {
            ip_send_info_destroy(ip_send_info);
        }

        node->ip_info->busy = FALSE;
        if (node->ip_info->enqueued_count > 0) {
            node->ip_info->enqueued_count--;
        }
    }

    return TRUE;
}

static bool event_handler_pdu_receive(node_t *node, node_t *incoming_node, ip_pdu_t *pdu)
{
    /* refresh the neighbor cache */
    ip_neighbor_t *neighbor = ip_node_find_neighbor_by_node(node, incoming_node);
    if (neighbor != NULL) {
        neighbor->last_packet_time = rs_system->now;
    }
    else {
        neighbor = ip_node_add_neighbor(node, incoming_node);
        event_execute(rpl_event_neighbor_attach, node, incoming_node, NULL);

        rs_system_schedule_event(node, ip_event_neighbor_cache_timeout_check, neighbor, NULL, rs_system->ip_neighbor_timeout);
    }

    rs_debug(DEBUG_IP, "node '%s': received packet from '%s', with src = '%s' and dst = '%s'",
            node->phy_info->name, incoming_node != NULL ? incoming_node->phy_info->name : "<<unknown>>",
            pdu->src_address, pdu->dst_address);

    if (!rpl_node_process_incoming_flow_label(node, incoming_node, pdu)) { /* drop the packet if RPL says so */
        rs_debug(DEBUG_IP, "node '%s': dropped packet from '%s', with src = '%s' and dst = '%s'",
                node->phy_info->name, incoming_node != NULL ? incoming_node->phy_info->name : "<<unknown>>",
                pdu->src_address, pdu->dst_address);

        return TRUE;
    }

    /* if the packet is not intended for us, neither broadcasted, we forward it */
    if (strcmp(node->ip_info->address, pdu->dst_address) != 0 && (strlen(pdu->dst_address) > 0)) {
        /* give a special treatment to measure messages */
        if (pdu->next_header == IP_NEXT_HEADER_MEASURE) {
            measure_pdu_t *measure_pdu = pdu->sdu;
            rs_assert(measure_pdu != NULL);

            /* a workaround for confirming the connectivity when reaching a member of a virtual dodag */
            if (rpl_node_is_root(node) &&
                    rpl_node_is_root(measure_pdu->dst_node) &&
                    strcmp(measure_pdu->dst_node->rpl_info->root_info->dodag_id, node->rpl_info->root_info->dodag_id) == 0) {

                pdu->sdu = NULL;
                return measure_node_receive(node, incoming_node, measure_pdu);
            }
            else {
                event_execute(measure_event_connect_hop_passed, measure_pdu->measuring_node, measure_pdu->dst_node, node);
            }
        }

        ip_node_forward(node, incoming_node, pdu);

        return TRUE;
    }
    else {
        bool all_ok = TRUE;

        switch (pdu->next_header) {

            case IP_NEXT_HEADER_ICMP: {
                if (!icmp_node_receive(node, incoming_node, pdu)) { /* yes, we directly pass the IP layer pdu to ICMP */
                    all_ok = FALSE;
                }
                pdu->sdu = NULL;

                break;
            }

            case IP_NEXT_HEADER_MEASURE: {
                measure_pdu_t *measure_pdu = pdu->sdu;
                pdu->sdu = NULL;

                rs_assert(measure_pdu != NULL);
                if (!measure_node_receive(node, incoming_node, measure_pdu)) {
                    all_ok = FALSE;
                }

                break;
            }

            default:
                rs_error("node '%s': unknown IP next header '0x%04X'", node->phy_info->name, pdu->next_header);
                all_ok = FALSE;
        }

        return all_ok;
    }
}

static bool event_handler_neighbor_cache_timeout(node_t *node, ip_neighbor_t *neighbor)
{
    if (neighbor->node == NULL || !phy_node_has_neighbor(neighbor->node, node)) {
        event_execute(rpl_event_neighbor_detach, node, neighbor->node, NULL);

        if (!ip_node_rem_neighbor(node, neighbor)) {
            if (neighbor->node != NULL)
                rs_error("node '%s': no longer has neighbor '%s'", node->phy_info->name, neighbor->node->phy_info->name);

            return FALSE; /* this should never happen */
        }
    }
    else {
        if (neighbor->node != NULL) {
            rs_system_schedule_event(node, ip_event_neighbor_cache_timeout_check, neighbor, NULL, rs_system->ip_neighbor_timeout);
        }
    }

    return TRUE;
}

static uint8 *route_expand_to_bits(char *dst, uint8 prefix_len)
{
    /* expand the destination hex digits to bits, for the sake of performance */
    uint8* dst_bit_expanded = malloc(prefix_len);

    uint16 i;
    for (i = 0; i < prefix_len; i++) {
        char c = dst[i / 4];

        if (isdigit(c)) {
            dst_bit_expanded[i] = (((c - '0') & (8u >> (i % 4))) != 0);
        }
        else {
            dst_bit_expanded[i] = (((c - 'A' + 10) & (8u >> (i % 4))) != 0);
        }
    }

    return dst_bit_expanded;
}

static void event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len)
{

    str1[0] = '\0';
    str2[0] = '\0';

    if (event_id == ip_event_pdu_send) {
        node_t *node = data1;
        ip_pdu_t *pdu = data2;

        snprintf(str1, len, "incoming_node = '%s'", (node != NULL ? node->phy_info->name : "<<local>>"));
        snprintf(str2, len, "ip_pdu = {src = '%s', dst = '%s'}", pdu->src_address, pdu->dst_address);
    }
    else if (event_id == ip_event_pdu_send_timeout_check) {
        ip_send_info_t *send_info = data1;

        if (send_info != NULL) {
            snprintf(str1, len, "send_info = {incoming_node = '%s', next_hop_index = %d, next_hop_count = %d}",
                    (send_info->incoming_node != NULL ? send_info->incoming_node->phy_info->name : "<<local>>"),
                    send_info->next_hop_index, send_info->next_hop_count);
        }
    }
    else if (event_id == ip_event_pdu_receive) {
        node_t *node = data1;
        mac_pdu_t *pdu = data2;

        snprintf(str1, len, "incoming_node = '%s'", (node != NULL ? node->phy_info->name : "<<unknown>>"));
        snprintf(str2, len, "ip_pdu = {src = '%s', dst = '%s'}", pdu->src_address, pdu->dst_address);
    }
    else if (event_id == ip_event_neighbor_cache_timeout_check) {
        ip_neighbor_t *neighbor = data1;

        snprintf(str1, len, "neighbor = '%s'", (neighbor->node != NULL ? neighbor->node->phy_info->name : "<<unknown>>"));
    }
}
