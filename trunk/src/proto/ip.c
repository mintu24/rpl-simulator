
#include <ctype.h>

#include "ip.h"
#include "../system.h"


    /**** global variables ****/

uint16              ip_event_id_after_node_wake;
uint16              ip_event_id_before_node_kill;

uint16              ip_event_id_after_pdu_sent;
uint16              ip_event_id_after_pdu_received;

uint16              ip_event_id_after_neighbor_cache_timeout;


    /**** local function prototypes ****/

static uint8 *      route_expand_to_bits(char *dst, uint8 prefix_len);

static void         event_arg_str_one_node_func(void *data1, void *data2, char *str1, char *str2, uint16 len);
static void         event_arg_str_one_neighbor_func(void *data1, void *data2, char *str1, char *str2, uint16 len);


    /**** exported functions ****/

bool ip_init()
{
    ip_event_id_after_node_wake = event_register("after_node_wake", "ip", (event_handler_t) ip_event_after_node_wake, NULL);
    ip_event_id_before_node_kill = event_register("before_node_kill", "ip", (event_handler_t) ip_event_before_node_kill, NULL);

    ip_event_id_after_pdu_sent = event_register("after_pdu_sent", "ip", (event_handler_t) ip_event_after_pdu_sent, event_arg_str_one_node_func);
    ip_event_id_after_pdu_received = event_register("after_pdu_received", "ip", (event_handler_t) ip_event_after_pdu_received, event_arg_str_one_node_func);

    ip_event_id_after_neighbor_cache_timeout = event_register("after_neighbor_cache_timeout", "ip", (event_handler_t) ip_event_after_neighbor_cache_timeout, event_arg_str_one_neighbor_func);

    return TRUE;
}

bool ip_done()
{
    return TRUE;
}

ip_pdu_t *ip_pdu_create(char *dst_address, char *src_address)
{
    rs_assert(dst_address != NULL);
    rs_assert(src_address != NULL);

    ip_pdu_t *pdu = malloc(sizeof(ip_pdu_t));

    pdu->dst_address = strdup(dst_address);
    pdu->src_address = strdup(src_address);

    pdu->next_header = -1;
    pdu->sdu = NULL;

    return pdu;
}

void ip_pdu_destroy(ip_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    if (pdu->dst_address != NULL)
        free(pdu->dst_address);
    if (pdu->src_address != NULL)
        free(pdu->src_address);

    free(pdu);
}

ip_pdu_t *ip_pdu_duplicate(ip_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    ip_pdu_t *new_pdu = malloc(sizeof(ip_pdu_t));

    new_pdu->dst_address = strdup(pdu->dst_address);
    new_pdu->src_address = strdup(pdu->src_address);

    new_pdu->next_header = pdu->next_header;

    switch (pdu->next_header) {
        case IP_NEXT_HEADER_ICMP :
            new_pdu->sdu = icmp_pdu_duplicate(pdu->sdu);

            break;
    }

    return new_pdu;
}

bool ip_pdu_set_sdu(ip_pdu_t *pdu, uint16 next_header, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->next_header = next_header;
    pdu->sdu = sdu;

    return TRUE;
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

                free(route->dst);
                free(route->dst_bit_expanded);
                free(route);

                node->ip_info->route_count--;
            }

            free(node->ip_info->route_list);
        }

        free(node->ip_info);
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

void ip_node_add_route(node_t *node, uint8 type, char *dst, uint8 prefix_len, node_t *next_hop)
{
    rs_assert(node != NULL);
    rs_assert(strlen(dst) * 4 >= prefix_len);

    ip_route_t *route = malloc(sizeof(ip_route_t));

    route->type = type;
    route->dst = strdup(dst);
    route->prefix_len = prefix_len;
    route->next_hop = next_hop;
    route->dst_bit_expanded = route_expand_to_bits(dst, prefix_len);

    node->ip_info->route_list = realloc(node->ip_info->route_list, (node->ip_info->route_count + 1) * sizeof(ip_route_t *));
    node->ip_info->route_list[node->ip_info->route_count++] = route;
}

bool ip_node_rem_route(node_t *node, char *dst, uint8 prefix_len)
{
    rs_assert(node != NULL);
    rs_assert(dst != NULL);

    int32 pos = -1, i;
    for (i = 0; i < node->ip_info->route_count; i++) {
        ip_route_t *route = node->ip_info->route_list[i];

        if ((strcmp(route->dst, dst) == 0) && (route->prefix_len == prefix_len)) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' does not have a route for '%s/%d'", node->phy_info->name, dst, prefix_len);

        return FALSE;
    }

    free(node->ip_info->route_list[pos]->dst);
    free(node->ip_info->route_list[pos]);

    for (i = pos; i < node->ip_info->route_count - 1; i++) {
        node->ip_info->route_list[i] = node->ip_info->route_list[i + 1];
    }

    node->ip_info->route_count--;
    node->ip_info->route_list = realloc(node->ip_info->route_list, node->ip_info->route_count * sizeof(ip_route_t *));
    if (node->ip_info->route_count == 0) {
        node->ip_info->route_list = NULL;
    }

    return TRUE;
}

node_t *ip_node_get_next_hop(node_t *node, char *dst_address)
{
    rs_assert(node != NULL);
    rs_assert(dst_address != NULL);

    if (strcmp(node->ip_info->address, dst_address) == 0) { /* sending to this node itself? */
        return node;
    }

    uint16 i, j;
    int16 best_prefix_len = -1;
    int16 best_type = 255;  /* a big number */
    ip_route_t *best_route = NULL;

    uint8 dst_len = strlen(dst_address) * 4;
    uint8 *dst_bits_expanded = route_expand_to_bits(dst_address, dst_len);

    for (i = 0; i < node->ip_info->route_count; i++) {
        ip_route_t *route = node->ip_info->route_list[i];

        if (route->type > best_type) {
            continue;
        }
        else if (route->type == best_type) {
            if (route->prefix_len <= best_prefix_len) {
                continue;
            }
        }

        if (dst_len < route->prefix_len) {
            rs_warn("invalid IP address '%s' for route '%s/%d'", dst_address, route->dst, route->prefix_len);
            continue;
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

    if (best_route == NULL) {
        /* if IP didn't suggest any specific route, we proceed as RPL says */
        return rpl_node_get_next_hop(node, dst_address);
    }
    else {
        return best_route->next_hop;
    }
}

ip_neighbor_t *ip_node_add_neighbor(node_t *node, node_t *neighbor_node)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);

    ip_neighbor_t *neighbor = malloc(sizeof(ip_neighbor_t));
    neighbor->node = neighbor_node;
    neighbor->last_packet_time = rs_system->now;

    node->ip_info->neighbor_list = realloc(node->ip_info->neighbor_list, (node->ip_info->neighbor_count + 1) * sizeof(ip_neighbor_t *));
    node->ip_info->neighbor_list[node->ip_info->neighbor_count] = neighbor;
    node->ip_info->neighbor_count++;

    /* install a route to this neighbor */
    // todo this type shouldn't be MANUAL
    ip_node_add_route(node, IP_ROUTE_TYPE_MANUAL, neighbor_node->ip_info->address,
            strlen(neighbor_node->ip_info->address) * 4, neighbor_node);

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
        ip_node_rem_route(node, neighbor->node->ip_info->address,
                strlen(neighbor->node->ip_info->address) * 4);
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

bool ip_send(node_t *node, node_t *dst_node, uint16 next_header, void *sdu)
{
    rs_assert(node != NULL);

    ip_pdu_t *ip_pdu = ip_pdu_create(dst_node != NULL ? dst_node->ip_info->address : "", node->ip_info->address);
    ip_pdu_set_sdu(ip_pdu, next_header, sdu);

    return event_execute(ip_event_id_after_pdu_sent, node, dst_node, ip_pdu);
}

bool ip_forward(node_t *node, ip_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    pdu = ip_pdu_duplicate(pdu);

    return event_execute(ip_event_id_after_pdu_sent, node, NULL, pdu);
}

bool ip_receive(node_t *node, node_t *src_node, ip_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    bool all_ok = event_execute(ip_event_id_after_pdu_received, node, src_node, pdu);

    ip_pdu_destroy(pdu);

    return all_ok;
}


bool ip_event_after_node_wake(node_t *node)
{
    return TRUE;
}

bool ip_event_before_node_kill(node_t *node)
{
    while (node->ip_info->neighbor_count > 0) {
        ip_neighbor_t *neighbor = node->ip_info->neighbor_list[node->ip_info->neighbor_count - 1];

        event_execute(rpl_event_id_after_neighbor_detach, node, neighbor->node, NULL);
        ip_node_rem_neighbor(node, neighbor);
    }

    return TRUE;
}

bool ip_event_after_pdu_sent(node_t *node, node_t *dst_node, ip_pdu_t *pdu)
{
    if (dst_node == NULL) { /* broadcast */
        if (!mac_send(node, NULL, MAC_TYPE_IP, pdu)) {
            rs_error("node '%s': failed to send MAC frame", node->phy_info->name);
            return FALSE;
        }
    }
    else {
        /* route the packet */
        node_t *next_hop = ip_node_get_next_hop(node, dst_node->ip_info->address);

        if (next_hop == NULL) {
            rs_warn("node '%s': destination '%s' not reachable", node->phy_info->name, dst_node->ip_info->address);
            return FALSE;
        }

        if (!mac_send(node, next_hop, MAC_TYPE_IP, pdu)) {
            rs_error("node '%s': failed to send MAC frame", node->phy_info->name);
            return FALSE;
        }
    }

    return TRUE;
}

bool ip_event_after_pdu_received(node_t *node, node_t *src_node, ip_pdu_t *pdu)
{
    /* refresh the neighbor cache */
    ip_neighbor_t *neighbor = ip_node_find_neighbor_by_node(node, src_node);
    if (neighbor != NULL) {
        neighbor->last_packet_time = rs_system->now;
    }
    else {
        neighbor = ip_node_add_neighbor(node, src_node);
        event_execute(rpl_event_id_after_neighbor_attach, node, src_node, NULL);

        rs_system_schedule_event(node, ip_event_id_after_neighbor_cache_timeout, neighbor, NULL, IP_NEIGHBOR_CACHE_TIMEOUT);
    }


    /* if the packet is not intended for us, neither broadcasted, we forward it */
    if (strcmp(node->ip_info->address, pdu->dst_address) != 0 && (strlen(pdu->dst_address) > 0)) {
        return ip_forward(node, pdu);
    }
    else {
        bool all_ok = TRUE;

        switch (pdu->next_header) {

            case IP_NEXT_HEADER_ICMP: {
                icmp_pdu_t *icmp_pdu = pdu->sdu;
                if (!icmp_receive(node, src_node, icmp_pdu)) {
                    rs_error("node '%s': failed to receive ICMP pdu from node '%s'", node->phy_info->name, src_node->phy_info->name);
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

bool ip_event_after_neighbor_cache_timeout(node_t *node, ip_neighbor_t *neighbor)
{
    sim_time_t diff = rs_system->now - neighbor->last_packet_time;

    if (diff >= IP_NEIGHBOR_CACHE_TIMEOUT) {
        event_execute(rpl_event_id_after_neighbor_detach, node, neighbor->node, NULL);
        ip_node_rem_neighbor(node, neighbor);
    }
    else {
        rs_system_schedule_event(node, ip_event_id_after_neighbor_cache_timeout, neighbor, NULL, IP_NEIGHBOR_CACHE_TIMEOUT - diff);
    }

    return TRUE;
}


    /**** local functions ****/

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

static void event_arg_str_one_node_func(void *data1, void *data2, char *str1, char *str2, uint16 len)
{
    node_t *node = data1;

    snprintf(str1, len, "%s", (node != NULL ? node->phy_info->name : "broadcast"));
    str2[0] = '\0';
}

static void event_arg_str_one_neighbor_func(void *data1, void *data2, char *str1, char *str2, uint16 len)
{
    ip_neighbor_t *neighbor = data1;

    snprintf(str1, len, "%s", (neighbor != NULL ? neighbor->node->phy_info->name : "broadcast"));
    str2[0] = '\0';
}
