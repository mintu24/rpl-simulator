
#include "ip.h"
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

    return node_info;
}

bool ip_node_info_destroy(ip_node_info_t *node_info)
{
    rs_assert(node_info != NULL);

    if (node_info->address != NULL)
        free(node_info->address);

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
