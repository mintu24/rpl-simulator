
#include "ip.h"
#include "phy.h"
#include "mac.h"
#include "rpl.h"


ip_pdu_t *ip_pdu_create(char *dst_address, char *src_address)
{
    rs_assert(dst_address != NULL);
    rs_assert(src_address != NULL);

    ip_pdu_t *pdu = (ip_pdu_t *) malloc(sizeof(ip_pdu_t*));

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

ip_node_info_t *ip_node_info_create(char *address)
{
    rs_assert(address != NULL);

    ip_node_info_t *node_info = (ip_node_info_t *) malloc(sizeof(ip_node_info_t));

    node_info->address = strdup(address);
    node_info->neighbor_list = NULL;
    node_info->neighbor_count = 0;

    return node_info;
}

bool ip_node_info_destroy(ip_node_info_t *node_info)
{
    rs_assert(node_info != NULL);

    if (node_info->address != NULL)
        free(node_info->address);

    if (node_info->neighbor_list != NULL)
        free(node_info->neighbor_list);

    free(node_info);

    return TRUE;
}

icmp_pdu_t *icmp_pdu_create()
{
    icmp_pdu_t *pdu = (icmp_pdu_t *) malloc(sizeof(icmp_pdu_t));

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
                // TODO: implement this
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

bool ip_init_node(node_t *node, ip_node_info_t *node_info)
{
    rs_assert(node != NULL);
    rs_assert(node_info != NULL);

    node->ip_info = node_info;

    return TRUE;
}

char *ip_node_get_address(node_t *node)
{
    rs_assert(node != NULL);

    return node->ip_info->address;
}

void ip_node_set_address(node_t *node, const char *address)
{
    rs_assert(node != NULL);

    if (node->ip_info->address != NULL)
        free(node->ip_info->address);

    node->ip_info->address = strdup(address);
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

bool ip_send(node_t *src_node, node_t *dst_node, void *sdu)
{
    rs_assert(src_node != NULL);

    ip_pdu_t *ip_pdu = ip_pdu_create(ip_node_get_address(dst_node), ip_node_get_address(src_node));
    ip_pdu_set_sdu(ip_pdu, IP_NEXT_HEADER_ICMP, sdu);

    node_execute(src_node, "ip_event_before_pdu_sent", (node_schedule_func_t) ip_event_before_pdu_sent, ip_pdu, TRUE);

    if (!mac_send(src_node, dst_node, MAC_TYPE_IP, ip_pdu)) {
        rs_error("failed to send MAC frame");
        return FALSE;
    }

    return TRUE;
}

bool icmp_send(node_t *src_node, node_t *dst_node, uint8 type, uint8 code, void *sdu)
{
    rs_assert(src_node != NULL);

    icmp_pdu_t *icmp_pdu = icmp_pdu_create();
    icmp_pdu_set_sdu(icmp_pdu, type, code, sdu);

    node_execute(src_node, "icmp_event_before_pdu_sent", (node_schedule_func_t) icmp_event_before_pdu_sent, icmp_pdu, TRUE);

    if (!ip_send(src_node, dst_node, icmp_pdu)) {
        rs_error("failed to send IP packet");
        return FALSE;
    }

    return TRUE;
}

void ip_event_before_pdu_sent(node_t *node, ip_pdu_t *pdu)
{
    rs_debug("src = '%s', dst = '%s'", pdu->src_address, pdu->dst_address);
}

void ip_event_after_pdu_received(node_t *node, ip_pdu_t *pdu)
{
    rs_debug("src = '%s', dst = '%s'", pdu->src_address, pdu->dst_address);
}

void icmp_event_before_pdu_sent(node_t *node, icmp_pdu_t *pdu)
{
    rs_debug(NULL);
}

void icmp_event_after_pdu_received(node_t *node, icmp_pdu_t *pdu)
{
    rs_debug(NULL);
}
