
#include <ctype.h>

#include "ip.h"
#include "phy.h"
#include "mac.h"
#include "rpl.h"


    /**** local function prototypes ****/

static void         ip_event_before_pdu_sent(node_t *node, node_t *dst_node, ip_pdu_t *pdu);
static void         ip_event_after_pdu_received(node_t *node, node_t *src_node, ip_pdu_t *pdu);

static void         icmp_event_before_pdu_sent(node_t *node, node_t *dst_node, icmp_pdu_t *pdu);
static void         icmp_event_after_pdu_received(node_t *node, node_t *src_node, icmp_pdu_t *pdu);

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

bool ip_pdu_destroy(ip_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    bool all_ok = TRUE;

    if (pdu->sdu != NULL) {
        switch (pdu->next_header) {
            case IP_NEXT_HEADER_ICMP:
                icmp_pdu_destroy(pdu->sdu);
                break;

            default:
                rs_error("unknown ip next header '0x%04X'", pdu->next_header);
                all_ok = FALSE;
        }
    }

    free(pdu);

    return all_ok;
}

bool ip_pdu_set_sdu(ip_pdu_t *pdu, uint16 next_header, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->next_header = next_header;
    pdu->sdu = sdu;

    return TRUE;
}

bool ip_node_init(node_t *node, char *address)
{
    rs_assert(node != NULL);
    rs_assert(address != NULL);

    node->ip_info = malloc(sizeof(ip_node_info_t));

    node->ip_info->address = strdup(address);

    node->ip_info->route_list = NULL;
    node->ip_info->route_count = 0;

    node->ip_info->neighbor_list = NULL;
    node->ip_info->neighbor_count = 0;

    return TRUE;
}

void ip_done_node(node_t *node)
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

    g_mutex_lock(node->proto_info_mutex);

    if (node->ip_info->address != NULL)
        free(node->ip_info->address);

    node->ip_info->address = strdup(address);

    g_mutex_unlock(node->proto_info_mutex);
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

    g_mutex_lock(node->proto_info_mutex);

    node->ip_info->route_list = realloc(node->ip_info->route_list, (node->ip_info->route_count + 1) * sizeof(ip_route_t *));
    node->ip_info->route_list[node->ip_info->route_count++] = route;

    g_mutex_unlock(node->proto_info_mutex);
}

bool ip_node_rem_route(node_t *node, char *dst, uint8 prefix_len)
{
    rs_assert(node != NULL);
    rs_assert(dst != NULL);

    g_mutex_lock(node->proto_info_mutex);

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
        g_mutex_unlock(node->proto_info_mutex);

        return FALSE;
    }

    free(node->ip_info->route_list[i]->dst);
    free(node->ip_info->route_list[i]);

    for (i = pos; i < node->ip_info->route_count - 1; i++) {
        node->ip_info->route_list[i] = node->ip_info->route_list[i + 1];
    }

    node->ip_info->route_count--;
    node->ip_info->route_list = realloc(node->ip_info->route_list, node->ip_info->route_count * sizeof(ip_route_t *));

    g_mutex_unlock(node->proto_info_mutex);

    return TRUE;
}

ip_route_t **ip_node_get_route_list(node_t *node, uint16 *route_count)
{
    rs_assert(node != NULL);

    g_mutex_lock(node->proto_info_mutex);

    ip_route_t **list = node->ip_info->route_list;
    *route_count = node->ip_info->route_count;

    g_mutex_unlock(node->proto_info_mutex);

    return list;
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
        return NULL;
    }
    else {
        return best_route->next_hop;
    }
}

node_t **ip_node_get_neighbor_list(node_t *node, uint16 *neighbor_count)
{
    rs_assert(node != NULL);

    g_mutex_lock(node->proto_info_mutex);

    node_t **list = node->ip_info->neighbor_list;
    *neighbor_count = node->ip_info->neighbor_count;

    g_mutex_unlock(node->proto_info_mutex);

    return list;
}

bool ip_node_add_neighbor(node_t *node, node_t *neighbor)
{
    rs_assert(node != NULL);
    rs_assert(neighbor != NULL);

    g_mutex_lock(node->proto_info_mutex);

    node->ip_info->neighbor_list = realloc(node->ip_info->neighbor_list, (node->ip_info->neighbor_count + 1) * sizeof(node_t *));
    node->ip_info->neighbor_list[node->ip_info->neighbor_count++] = neighbor;

    g_mutex_unlock(node->proto_info_mutex);

    return TRUE;
}

bool ip_node_remove_neighbor(node_t *node, node_t *neighbor)
{
    rs_assert(node != NULL);
    rs_assert(neighbor != NULL);

    g_mutex_lock(node->proto_info_mutex);

    int pos = -1, i;
    for (i = 0; i < node->ip_info->neighbor_count; i++) {
        if (node->ip_info->neighbor_list[i] == neighbor) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' does not have node '%s' as a neighbor", phy_node_get_name(node), phy_node_get_name(neighbor));
        g_mutex_unlock(node->proto_info_mutex);

        return FALSE;
    }

    for (i = pos; i < node->ip_info->neighbor_count - 1; i++) {
        node->ip_info->neighbor_list[i] = node->ip_info->neighbor_list[i + 1];
    }

    node->ip_info->neighbor_count--;
    node->ip_info->neighbor_list = realloc(node->ip_info->neighbor_list, node->ip_info->neighbor_count * sizeof(node_t *));

    g_mutex_unlock(node->proto_info_mutex);

    return TRUE;
}

bool ip_node_has_neighbor(node_t *node, node_t *neighbor)
{
    rs_assert(node != NULL);
    rs_assert(neighbor != NULL);

    g_mutex_lock(node->proto_info_mutex);

    int i;
    for (i = 0; i < node->ip_info->neighbor_count; i++) {
        if (node->ip_info->neighbor_list[i] == neighbor) {
            g_mutex_unlock(node->proto_info_mutex);

            return TRUE;
        }
    }

    g_mutex_unlock(node->proto_info_mutex);

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

        node_execute_pdu_event(
                node,
                "ip_event_before_pdu_sent",
                (node_pdu_event_t) ip_event_before_pdu_sent,
                node, dst_node, ip_pdu,
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

    node_execute_pdu_event(
            node,
            "ip_event_before_pdu_sent",
            (node_pdu_event_t) ip_event_before_pdu_sent,
            node, NULL, pdu,
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

bool ip_receive(node_t *node, node_t *src_node, ip_pdu_t **pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);
    rs_assert(*pdu != NULL);

    node_execute_pdu_event(
            node,
            "ip_event_after_pdu_received",
            (node_pdu_event_t) ip_event_after_pdu_received,
            node, src_node, *pdu,
            TRUE);

    /* if the packet is not intended for us, we forward it */
    if (strcmp(ip_node_get_address(node), (*pdu)->dst_address) != 0) {
        ip_pdu_t *fwd_pdu = (*pdu);
        *pdu = NULL;
        return ip_forward(node, fwd_pdu);
    }

    switch ((*pdu)->next_header) {

        case IP_NEXT_HEADER_ICMP: {
            icmp_pdu_t *icmp_pdu = (*pdu)->sdu;
            if (!icmp_receive(node, src_node, icmp_pdu)) {
                rs_error("failed to recevie ICMP pdu from node '%s'", phy_node_get_name(src_node));
                return FALSE;
            }

            break;
        }

        default:
            rs_error("unknown IP next header '0x%04X'", (*pdu)->next_header);

            return FALSE;
    }

    return TRUE;
}

icmp_pdu_t *icmp_pdu_create()
{
    icmp_pdu_t *pdu = malloc(sizeof(icmp_pdu_t));

    pdu->type = -1;
    pdu->code = -1;
    pdu->sdu = NULL;

    return pdu;
}

bool icmp_pdu_destroy(icmp_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    bool all_ok = TRUE;

    if (pdu->sdu != NULL) {

        switch (pdu->type) {

            case ICMP_TYPE_RPL :
                switch (pdu->code) {

                    case ICMP_RPL_CODE_DIS :
                        break;

                    case ICMP_RPL_CODE_DIO:
                        rpl_dio_pdu_destroy(pdu->sdu);
                        break;

                    case ICMP_RPL_CODE_DAO:
                        rpl_dao_pdu_destroy(pdu->sdu);
                        break;

                    default:
                        rs_error("unknown icmp code '0x%02X'", pdu->code);
                        all_ok = FALSE;
                }

                break;

            default:
                rs_error("unknown icmp type '0x%02X'", pdu->type);
                all_ok = FALSE;
        }

    }

    free(pdu);

    return all_ok;
}

bool icmp_pdu_set_sdu(icmp_pdu_t *pdu, uint8 type, uint8 code, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->type = type;
    pdu->code = code;
    pdu->sdu = sdu;

    return TRUE;
}

bool icmp_send(node_t *node, node_t *dst_node, uint8 type, uint8 code, void *sdu)
{
    rs_assert(node != NULL);
    rs_assert(dst_node != NULL);    /* for the moment we don't want ICMP/IP broadcast */

    icmp_pdu_t *icmp_pdu = icmp_pdu_create();
    icmp_pdu_set_sdu(icmp_pdu, type, code, sdu);

    node_execute_pdu_event(
            node,
            "icmp_event_before_pdu_sent",
            (node_pdu_event_t) icmp_event_before_pdu_sent,
            node, dst_node, icmp_pdu,
            TRUE);

    if (!ip_send(node, dst_node, IP_NEXT_HEADER_ICMP, icmp_pdu)) {
        rs_error("failed to send IP packet");
        return FALSE;
    }

    return TRUE;
}

bool icmp_receive(node_t *node, node_t *src_node, icmp_pdu_t *pdu)
{
    rs_assert(pdu != NULL);
    rs_assert(node != NULL);

    node_execute_pdu_event(
            node,
            "icmp_event_after_pdu_received",
            (node_pdu_event_t) icmp_event_after_pdu_received,
            src_node, node, pdu,
            TRUE);

    switch (pdu->type) {

        case ICMP_TYPE_RPL:
            switch (pdu->code) {

                case ICMP_RPL_CODE_DIS: {
                    if (!rpl_receive_dis(node, src_node)) {
                        rs_error("failed to receive RPL DIS from node '%s'", phy_node_get_name(src_node));
                        return FALSE;
                    }

                    break;
                }

                case ICMP_RPL_CODE_DIO: {
                    rpl_dio_pdu_t *rpl_dio_pdu = pdu->sdu;
                    if (!rpl_receive_dio(node, src_node, rpl_dio_pdu)) {
                        rs_error("failed to receive RPL DIO from node '%s'", phy_node_get_name(src_node));
                        return FALSE;
                    }

                    break;
                }

                case ICMP_RPL_CODE_DAO: {
                    rpl_dao_pdu_t *rpl_dao_pdu = pdu->sdu;
                    if (!rpl_receive_dao(node, src_node, rpl_dao_pdu)) {
                        rs_error("failed to receive RPL DAO from node '%s'", phy_node_get_name(src_node));
                        return FALSE;
                    }

                    break;
                }

                default:
                    rs_error("unknown ICMP code '0x%02X'", pdu->code);

                    return FALSE;
            }

            break;

        default:
            rs_error("unknown ICMP type '0x%02X'", pdu->type);

            return FALSE;
    }

    return TRUE;
}


    /**** local functions ****/

static void ip_event_before_pdu_sent(node_t *node, node_t *dst_node, ip_pdu_t *pdu)
{
    rs_debug("'%s' -> '%s'", pdu->src_address, pdu->dst_address);
}

static void ip_event_after_pdu_received(node_t *node, node_t *src_node, ip_pdu_t *pdu)
{
    rs_debug("'%s' -> '%s'", pdu->src_address, pdu->dst_address);
}

static void icmp_event_before_pdu_sent(node_t *node, node_t *dst_node, icmp_pdu_t *pdu)
{
    rs_debug(NULL);
}

static void icmp_event_after_pdu_received(node_t *node, node_t *src_node, icmp_pdu_t *pdu)
{
    rs_debug(NULL);
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
