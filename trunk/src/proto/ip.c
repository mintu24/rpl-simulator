
#include "ip.h"
#include "phy.h"
#include "mac.h"
#include "rpl.h"


    /**** local function prototypes ****/

static void         ip_event_before_pdu_sent(node_t *src_node, node_t *dst_node, ip_pdu_t *pdu);
static void         ip_event_after_pdu_received(node_t *src_node, node_t *dst_node, ip_pdu_t *pdu);

static void         icmp_event_before_pdu_sent(node_t *src_node, node_t *dst_node, icmp_pdu_t *pdu);
static void         icmp_event_after_pdu_received(node_t *src_node, node_t *dst_node, icmp_pdu_t *pdu);


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

ip_node_info_t *ip_node_info_create(char *address)
{
    rs_assert(address != NULL);

    ip_node_info_t *node_info = malloc(sizeof(ip_node_info_t));

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

bool ip_init_node(node_t *node, ip_node_info_t *node_info)
{
    rs_assert(node != NULL);
    rs_assert(node_info != NULL);

    node->ip_info = node_info;

    return TRUE;
}

void ip_done_node(node_t *node)
{
    rs_assert(node != NULL);

    if (node->ip_info != NULL) {
        ip_node_info_destroy(node->ip_info);
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

bool ip_send(node_t *src_node, node_t *dst_node, uint16 next_header, void *sdu)
{
    rs_assert(src_node != NULL);

    ip_pdu_t *ip_pdu = ip_pdu_create(ip_node_get_address(dst_node), ip_node_get_address(src_node));
    ip_pdu_set_sdu(ip_pdu, next_header, sdu);

    node_execute_src_dst(
            src_node,
            "ip_event_before_pdu_sent",
            (node_event_src_dst_t) ip_event_before_pdu_sent,
            src_node, dst_node, ip_pdu,
            TRUE);

    if (!mac_send(src_node, dst_node, MAC_TYPE_IP, ip_pdu)) {
        rs_error("failed to send MAC frame");
        return FALSE;
    }

    return TRUE;
}

bool ip_receive(node_t *src_node, node_t *dst_node, ip_pdu_t *pdu)
{
    rs_assert(pdu != NULL);
    rs_assert(dst_node != NULL);

    node_execute_src_dst(
            dst_node,
            "ip_event_after_pdu_received",
            (node_event_src_dst_t) ip_event_after_pdu_received,
            src_node, dst_node, pdu,
            TRUE);

    switch (pdu->next_header) {

        case IP_NEXT_HEADER_ICMP: {
            icmp_pdu_t *icmp_pdu = pdu->sdu;
            return icmp_receive(src_node, dst_node, icmp_pdu);

            break;
        }

        default:
            rs_error("unknown IP next header '0x%04X'", pdu->next_header);

            return FALSE;
    }

    return TRUE;
}

bool icmp_send(node_t *src_node, node_t *dst_node, uint8 type, uint8 code, void *sdu)
{
    rs_assert(src_node != NULL);

    icmp_pdu_t *icmp_pdu = icmp_pdu_create();
    icmp_pdu_set_sdu(icmp_pdu, type, code, sdu);

    node_execute_src_dst(
            src_node,
            "icmp_event_before_pdu_sent",
            (node_event_src_dst_t) icmp_event_before_pdu_sent,
            src_node, dst_node, icmp_pdu,
            TRUE);

    if (!ip_send(src_node, dst_node, IP_NEXT_HEADER_ICMP, icmp_pdu)) {
        rs_error("failed to send IP packet");
        return FALSE;
    }

    return TRUE;
}

bool icmp_receive(node_t *src_node, node_t *dst_node, icmp_pdu_t *pdu)
{
    rs_assert(pdu != NULL);
    rs_assert(dst_node != NULL);

    node_execute_src_dst(
            dst_node,
            "icmp_event_after_pdu_received",
            (node_event_src_dst_t) icmp_event_after_pdu_received,
            src_node, dst_node, pdu,
            TRUE);

    switch (pdu->type) {

        case ICMP_TYPE_RPL:
            switch (pdu->code) {

                case ICMP_RPL_CODE_DIS: {
                    return rpl_receive_dis(src_node, dst_node);

                    break;
                }

                case ICMP_RPL_CODE_DIO: {
                    rpl_dio_pdu_t *rpl_dio_pdu = pdu->sdu;
                    return rpl_receive_dio(src_node, dst_node, rpl_dio_pdu);

                    break;
                }

                case ICMP_RPL_CODE_DAO: {
                    rpl_dao_pdu_t *rpl_dao_pdu = pdu->sdu;
                    return rpl_receive_dao(src_node, dst_node, rpl_dao_pdu);

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

static void ip_event_before_pdu_sent(node_t *src_node, node_t *dst_node, ip_pdu_t *pdu)
{
    rs_debug("'%s' -> '%s'", pdu->src_address, pdu->dst_address);
}

static void ip_event_after_pdu_received(node_t *src_node, node_t *dst_node, ip_pdu_t *pdu)
{
    rs_debug("'%s' -> '%s'", pdu->src_address, pdu->dst_address);
}

static void icmp_event_before_pdu_sent(node_t *src_node, node_t *dst_node, icmp_pdu_t *pdu)
{
    rs_debug(NULL);
}

static void icmp_event_after_pdu_received(node_t *src_node, node_t *dst_node, icmp_pdu_t *pdu)
{
    rs_debug(NULL);
}
