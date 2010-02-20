
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

    pdu->flow_label = malloc(sizeof(ip_flow_label_t));
    pdu->flow_label->forward_error = FALSE;
    pdu->flow_label->from_sibling = FALSE;
    pdu->flow_label->going_down = FALSE;
    pdu->flow_label->rank_error = FALSE;
    pdu->flow_label->sender_rank = 0;

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
    if (pdu->flow_label != NULL)
        free(pdu->flow_label);

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

void ip_node_add_route(node_t *node, char *dst, uint8 prefix_len, node_t *next_hop, uint8 type, void *further_info)
{
    rs_assert(node != NULL);
    rs_assert(dst != NULL);
    rs_assert(next_hop != NULL);
    rs_assert(strlen(dst) * 4 >= prefix_len);

    ip_route_t *route = malloc(sizeof(ip_route_t));

    route->dst = strdup(dst);
    route->prefix_len = prefix_len;
    route->next_hop = next_hop;
    route->dst_bit_expanded = route_expand_to_bits(dst, prefix_len);
    route->type = type;
    route->further_info = further_info;

    node->ip_info->route_list = realloc(node->ip_info->route_list, (node->ip_info->route_count + 1) * sizeof(ip_route_t *));
    node->ip_info->route_list[node->ip_info->route_count++] = route;
}

void ip_node_rem_routes(node_t *node, char *dst, int8 prefix_len, node_t *next_hop, int8 type)
{
    rs_assert(node != NULL);

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

        free(node->ip_info->route_list[i]->dst);
        free(node->ip_info->route_list[i]);

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

bool ip_send(node_t *node, char *dst_ip_address, uint16 next_header, void *sdu)
{
    rs_assert(node != NULL);

    ip_pdu_t *ip_pdu = ip_pdu_create(dst_ip_address != NULL ? dst_ip_address : "", node->ip_info->address);
    ip_pdu_set_sdu(ip_pdu, next_header, sdu);

    if (!event_execute(ip_event_id_after_pdu_sent, node, NULL, ip_pdu)) {
        ip_pdu_destroy(ip_pdu);
        return FALSE;
    }

    return TRUE;
}

bool ip_forward(node_t *node, node_t *incoming_node, ip_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    pdu = ip_pdu_duplicate(pdu);

    if (!event_execute(ip_event_id_after_pdu_sent, node, incoming_node, pdu)) {
        ip_pdu_destroy(pdu);
        return FALSE;
    }

    return TRUE;
}

bool ip_receive(node_t *node, node_t *incoming_node, ip_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    bool all_ok = event_execute(ip_event_id_after_pdu_received, node, incoming_node, pdu);

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

bool ip_event_after_pdu_sent(node_t *node, node_t *incoming_node, ip_pdu_t *pdu)
{

    if (strlen(pdu->dst_address) == 0) { /* broadcast */
        node_t *dst_node = rpl_node_process_outgoing_flow_label(node, incoming_node, NULL, pdu);

        if (!mac_send(node, dst_node, MAC_TYPE_IP, pdu)) {
            return FALSE;
        }
    }
    else {
        /* route the packet */
        ip_route_t *route = ip_node_get_next_hop_route(node, pdu->dst_address);

        if (route == NULL) {
            rs_warn("node '%s': destination '%s' not reachable", node->phy_info->name, pdu->dst_address);
            return FALSE;
        }

        node_t *next_hop = route->next_hop;
        node_t *redir_next_hop = rpl_node_process_outgoing_flow_label(node, incoming_node, next_hop, pdu);
        if (redir_next_hop != next_hop) { /* RPL redirected us, don't try all the parents & stuff in case immediate of failure */
            return mac_send(node, redir_next_hop, MAC_TYPE_IP, pdu);
        }

        if (!mac_send(node, next_hop, MAC_TYPE_IP, pdu)) { /* probably no link, passing the forwarding task to the RPL layer */
            uint16 node_count, i;
            node_t **node_list = rpl_node_get_next_hop_list(node, &node_count);
            bool sent = FALSE;

            for (i = 0; i < node_count; i++) {
                node_t *next_node = node_list[i];

                if (mac_send(node, next_node, MAC_TYPE_IP, pdu)) {
                    sent = TRUE;
                    break;
                }
            }

            if (node_list != NULL) {
                free(node_list);
            }

            if (!sent) {
                event_execute(rpl_event_id_after_forward_failure, node, NULL, NULL);
            }

            return sent;
        }
    }

    return TRUE;
}

bool ip_event_after_pdu_received(node_t *node, node_t *incoming_node, ip_pdu_t *pdu)
{
    /* refresh the neighbor cache */
    ip_neighbor_t *neighbor = ip_node_find_neighbor_by_node(node, incoming_node);
    if (neighbor != NULL) {
        neighbor->last_packet_time = rs_system->now;
    }
    else {
        neighbor = ip_node_add_neighbor(node, incoming_node);
        event_execute(rpl_event_id_after_neighbor_attach, node, incoming_node, NULL);

        rs_system_schedule_event(node, ip_event_id_after_neighbor_cache_timeout, neighbor, NULL, IP_NEIGHBOR_CACHE_TIMEOUT);
    }

    if (!rpl_node_process_incoming_flow_label(node, incoming_node, pdu)) { /* drop the packet if RPL says so */
        return FALSE;
    }

    /* if the packet is not intended for us, neither broadcasted, we forward it */
    if (strcmp(node->ip_info->address, pdu->dst_address) != 0 && (strlen(pdu->dst_address) > 0)) {
        return ip_forward(node, incoming_node, pdu);
    }
    else {
        bool all_ok = TRUE;

        switch (pdu->next_header) {

            case IP_NEXT_HEADER_ICMP: {
                if (!icmp_receive(node, incoming_node, pdu)) { /* yes, we directly pass the IP layer pdu to ICMP */
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

        if (!ip_node_rem_neighbor(node, neighbor)) {
            if (neighbor->node != NULL)
                rs_error("node '%s': no longer has neighbor '%s'", node->phy_info->name, neighbor->node->phy_info->name);

            return FALSE;
        }
    }
    else {
        if (neighbor->node != NULL) {
            rs_system_schedule_event(node, ip_event_id_after_neighbor_cache_timeout, neighbor, NULL, IP_NEIGHBOR_CACHE_TIMEOUT - diff);
        }
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

    if (neighbor == NULL) {
        snprintf(str1, len, "%s", "broadcast");
    }
    else {
        snprintf(str1, len, "%s", (neighbor->node != NULL ? neighbor->node->phy_info->name : "<<removed>>"));
    }

    str2[0] = '\0';
}
