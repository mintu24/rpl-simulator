
#include "ip.h"
#include "rpl.h"


ip_pdu_t *ip_pdu_create(char *dst_address, char *src_address, void *sdu)
{
    if (dst_address == NULL || src_address == NULL) {
        rs_error("invalid argument");
        return NULL;
    }

    ip_pdu_t *pdu = (ip_pdu_t *) malloc(sizeof(ip_pdu_t*));

    pdu->dst_address = strdup(dst_address);
    pdu->src_address = strdup(src_address);

    pdu->next_header = -1;
    pdu->sdu = NULL;

    return pdu;
}

bool ip_pdu_destroy(ip_pdu_t *pdu)
{
    if (pdu == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

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
    if (pdu == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    pdu->next_header = next_header;
    pdu->sdu = sdu;

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
    if (pdu == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

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
    if (pdu == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    pdu->type = type;
    pdu->code = code;
    pdu->sdu = sdu;

    return TRUE;
}

bool ip_init_node(node_t *node, char *ip_address)
{
    if (node == NULL || ip_address == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    node->ip_address = strdup(ip_address);

    return TRUE;
}
