
#include <ctype.h>

#include "ip.h"
#include "../system.h"


    /**** local function prototypes ****/

static uint8 *      route_expand_to_bits(char *dst, uint8 prefix_len);


    /**** exported functions ****/

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

    g_static_rec_mutex_init(&node->ip_info->mutex);
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

        if (node->ip_info->neighbor_list != NULL)
            free(node->ip_info->neighbor_list);

        g_static_rec_mutex_free(&node->ip_info->mutex);

        free(node->ip_info);
    }
}

char *ip_node_get_address(node_t *node)
{
    rs_assert(node != NULL);

    return node->ip_info->address;
}

void ip_node_set_address(node_t *node, const char *address)
{
    rs_assert(node != NULL);

    ip_node_lock(node);

    if (node->ip_info->address != NULL)
        free(node->ip_info->address);

    node->ip_info->address = strdup(address);

    ip_node_unlock(node);
}

void ip_node_add_route(node_t *node, uint8 type, char *dst, uint8 prefix_len, node_t *next_hop, bool aggregate)
{
    rs_assert(node != NULL);
    rs_assert(strlen(dst) * 4 >= prefix_len);

    ip_route_t *route = malloc(sizeof(ip_route_t));

    route->type = type;
    route->dst = strdup(dst);
    route->prefix_len = prefix_len;
    route->next_hop = next_hop;
    route->dst_bit_expanded = route_expand_to_bits(dst, prefix_len);

    ip_node_lock(node);

    node->ip_info->route_list = realloc(node->ip_info->route_list, (node->ip_info->route_count + 1) * sizeof(ip_route_t *));
    node->ip_info->route_list[node->ip_info->route_count++] = route;

    ip_node_unlock(node);
}

bool ip_node_rem_route(node_t *node, char *dst, uint8 prefix_len)
{
    rs_assert(node != NULL);
    rs_assert(dst != NULL);

    ip_node_lock(node);

    int32 pos = -1, i;
    for (i = 0; i < node->ip_info->route_count; i++) {
        ip_route_t *route = node->ip_info->route_list[i];

        if ((strcmp(route->dst, dst) == 0) && (route->prefix_len == prefix_len)) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' does not have a route for '%s/%d'", phy_node_get_name(node), dst, prefix_len);
        ip_node_unlock(node);

        return FALSE;
    }

    free(node->ip_info->route_list[pos]->dst);
    free(node->ip_info->route_list[pos]);

    for (i = pos; i < node->ip_info->route_count - 1; i++) {
        node->ip_info->route_list[i] = node->ip_info->route_list[i + 1];
    }

    node->ip_info->route_count--;
    node->ip_info->route_list = realloc(node->ip_info->route_list, node->ip_info->route_count * sizeof(ip_route_t *));

    ip_node_unlock(node);

    return TRUE;
}

ip_route_t **ip_node_get_route_list(node_t *node, uint16 *route_count)
{
    rs_assert(node != NULL);

    if (route_count != NULL)
        *route_count = node->ip_info->route_count;

    return node->ip_info->route_list;
}

node_t *ip_node_best_match_route(node_t *node, char *dst_address)
{
    rs_assert(node != NULL);

    uint16 i, j;
    int16 best_prefix_len = -1;
    int16 best_type = 255;  /* a big number */
    ip_route_t *best_route = NULL;

    uint8 dst_len = strlen(dst_address) * 4;
    uint8 *dst_bits_expanded = route_expand_to_bits(dst_address, dst_len);

    ip_node_lock(node);

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

    ip_node_unlock(node);

    if (best_route == NULL) {
        return NULL;
    }
    else {
        return best_route->next_hop;
    }
}

bool ip_node_add_neighbor(node_t *node, node_t *neighbor)
{
    rs_assert(node != NULL);
    rs_assert(neighbor != NULL);

    ip_node_lock(node);

    node->ip_info->neighbor_list = realloc(node->ip_info->neighbor_list, (node->ip_info->neighbor_count + 1) * sizeof(node_t *));
    node->ip_info->neighbor_list[node->ip_info->neighbor_count++] = neighbor;

    ip_node_unlock(node);

    return TRUE;
}

bool ip_node_remove_neighbor(node_t *node, node_t *neighbor)
{
    rs_assert(node != NULL);
    rs_assert(neighbor != NULL);

    ip_node_lock(node);

    int pos = -1, i;
    for (i = 0; i < node->ip_info->neighbor_count; i++) {
        if (node->ip_info->neighbor_list[i] == neighbor) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' does not have node '%s' as a neighbor", phy_node_get_name(node), phy_node_get_name(neighbor));
        ip_node_unlock(node);

        return FALSE;
    }

    for (i = pos; i < node->ip_info->neighbor_count - 1; i++) {
        node->ip_info->neighbor_list[i] = node->ip_info->neighbor_list[i + 1];
    }

    node->ip_info->neighbor_count--;
    node->ip_info->neighbor_list = realloc(node->ip_info->neighbor_list, node->ip_info->neighbor_count * sizeof(node_t *));

    ip_node_unlock(node);

    return TRUE;
}

node_t **ip_node_get_neighbor_list(node_t *node, uint16 *neighbor_count)
{
    rs_assert(node != NULL);

    if (neighbor_count != NULL)
        *neighbor_count = node->ip_info->neighbor_count;

    return node->ip_info->neighbor_list;
}

bool ip_node_has_neighbor(node_t *node, node_t *neighbor)
{
    rs_assert(node != NULL);
    rs_assert(neighbor != NULL);

    ip_node_lock(node);

    int i;
    for (i = 0; i < node->ip_info->neighbor_count; i++) {
        if (node->ip_info->neighbor_list[i] == neighbor) {
            ip_node_unlock(node);

            return TRUE;
        }
    }

    ip_node_unlock(node);

    return FALSE;
}

bool ip_send(node_t *node, node_t *dst_node, uint16 next_header, void *sdu)
{
    rs_assert(node != NULL);

    if (dst_node == NULL) {
        rs_error("IP broadcast not supported");
        return FALSE;
    }
    else {
        ip_pdu_t *ip_pdu = ip_pdu_create(ip_node_get_address(dst_node), ip_node_get_address(node));
        ip_pdu_set_sdu(ip_pdu, next_header, sdu);

        node_execute_event(
                node,
                "ip_event_before_pdu_sent",
                (node_event_t) ip_event_before_pdu_sent,
                dst_node, ip_pdu,
                TRUE);

        /* route the packet */
        node_t *next_hop = ip_node_best_match_route(node, ip_node_get_address(dst_node));

        if (next_hop == NULL) {
            rs_warn("destination '%s' not reachable by '%s'", ip_node_get_address(dst_node), phy_node_get_name(node));
            return FALSE;
        }

        if (!mac_send(node, next_hop, MAC_TYPE_IP, ip_pdu)) {
            rs_error("failed to send MAC frame");
            return FALSE;
        }
    }

    return TRUE;
}

bool ip_forward(node_t *node, ip_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    node_execute_event(
            node,
            "ip_event_before_pdu_sent",
            (node_event_t) ip_event_before_pdu_sent,
            NULL, pdu,
            TRUE);

    /* route the packet */
    node_t *next_hop = ip_node_best_match_route(node, pdu->dst_address);

    if (next_hop == NULL) {
        rs_warn("destination '%s' not reachable by '%s'", pdu->dst_address, phy_node_get_name(node));
        return FALSE;
    }

    if (!mac_send(node, next_hop, MAC_TYPE_IP, pdu)) {
        rs_error("failed to send MAC frame");
        return FALSE;
    }

    return TRUE;
}

bool ip_receive(node_t *node, node_t *src_node, ip_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    node_execute_event(
            node,
            "ip_event_after_pdu_received",
            (node_event_t) ip_event_after_pdu_received,
            src_node, pdu,
            TRUE);

    /* if the packet is not intended for us, we forward it */
    if (strcmp(ip_node_get_address(node), pdu->dst_address) != 0) {
        return ip_forward(node, pdu);
    }

    bool all_ok = TRUE;

    switch (pdu->next_header) {

        case IP_NEXT_HEADER_ICMP: {
            icmp_pdu_t *icmp_pdu = pdu->sdu;
            if (!icmp_receive(node, src_node, icmp_pdu)) {
                rs_error("failed to recevie ICMP pdu from node '%s'", phy_node_get_name(src_node));
                all_ok = FALSE;
            }

            break;
        }

        default:
            rs_error("unknown IP next header '0x%04X'", pdu->next_header);
            all_ok = FALSE;
    }

    ip_pdu_destroy(pdu);

    return all_ok;
}

void ip_event_after_node_wake(node_t *node)
{
}

void ip_event_before_node_kill(node_t *node)
{
}

void ip_event_before_pdu_sent(node_t *node, node_t *dst_node, ip_pdu_t *pdu)
{
}

void ip_event_after_pdu_received(node_t *node, node_t *src_node, ip_pdu_t *pdu)
{
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
