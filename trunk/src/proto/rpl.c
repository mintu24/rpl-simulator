
#include "rpl.h"


dio_pdu_t *dio_pdu_create(bool grounded, bool da_trigger, bool da_support, int8 dag_pref, int8 seq_number, int8 instance_id, int8 rank, char *dag_id)
{
    if (dag_id == NULL) {
        rs_error("invalid argument");
        return NULL;
    }

    dio_pdu_t *pdu = (dio_pdu_t *) malloc(sizeof(dio_pdu_t));

    pdu->grounded = grounded;
    pdu->da_trigger = da_trigger;
    pdu->da_support = da_support;
    pdu->dag_pref = dag_pref;
    pdu->seq_number = seq_number;
    pdu->instance_id = instance_id;
    pdu->rank = rank;
    pdu->dag_id = strdup(dag_id);
    pdu->suboptions = NULL;

    return pdu;
}

bool dio_pdu_destroy(dio_pdu_t *pdu)
{
    if (pdu == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    if (pdu->dag_id != NULL)
        free(pdu->dag_id);

    if (pdu->suboptions != NULL)
        dio_suboption_destroy(pdu->suboptions);

    free(pdu);

    return TRUE;
}

dio_suboption_t *dio_suboption_dag_config_create(int8 interval_doublings, int8 interval_min, int8 redundancy_constant, int8 max_rank_increase)
{
    dio_suboption_t *suboption = (dio_suboption_t *) malloc(sizeof(dio_suboption_t));

    dio_suboption_dag_config_t *content = (dio_suboption_dag_config_t *) malloc(sizeof(dio_suboption_dag_config_t));

    content->interval_doublings = interval_doublings;
    content->interval_min = interval_min;
    content->redundancy_constant = redundancy_constant;
    content->max_rank_increase = max_rank_increase;

    suboption->type = DIO_SUBOPTION_TYPE_DAG_CONFIG;
    suboption->next_suboption = NULL;
    suboption->content = content;

    return suboption;
}

bool dio_suboption_destroy(dio_suboption_t *suboption)
{
    if (suboption == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    bool all_ok = TRUE;

    if (suboption->next_suboption != NULL) {
        if (!dio_suboption_destroy(suboption->next_suboption))
            all_ok = FALSE;
    }

    if (suboption->content != NULL) {
        switch (suboption->type) {
            case DIO_SUBOPTION_TYPE_DAG_CONFIG: {
                free(suboption->content);
                break;
            }

            default:
                rs_error("unknown suboption type '0x%02X'", suboption->type);
                all_ok = FALSE;
        }
    }

    return all_ok;
}

bool dio_pdu_add_suboption(dio_pdu_t *pdu, dio_suboption_t *suboption)
{
    if (pdu == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    if (pdu->suboptions == NULL) {
        pdu->suboptions = suboption;
    }
    else {
        dio_suboption_t *suboption = pdu->suboptions;

        while (suboption->next_suboption != NULL)
            suboption = suboption->next_suboption;

        suboption->next_suboption = suboption;
    }

    return TRUE;
}

dao_pdu_t *dao_pdu_create(uint16 sequence, uint8 instance_id, uint8 rank, uint32 dao_lifetime, char *prefix, uint8 prefix_len)
{
    if (prefix == NULL) {
        rs_error("invalid argument");
        return NULL;
    }

    dao_pdu_t *pdu = (dao_pdu_t *) malloc(sizeof(dao_pdu_t));

    pdu->sequence = sequence;
    pdu->instance_id = instance_id;
    pdu->rank = rank;
    pdu->dao_lifetime = dao_lifetime;
    pdu->prefix = strdup(prefix);
    pdu->prefix_len = prefix_len;
    pdu->rr_stack = NULL;
    pdu->rr_count = 0;

    return pdu;
}

bool dao_pdu_destroy(dao_pdu_t *pdu)
{
    if (pdu == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    while (pdu->rr_count > 0) {
        free(pdu->rr_stack[--pdu->rr_count]);
    }

    free(pdu->rr_stack);

    free(pdu);

    return TRUE;
}

bool dao_pdu_add_rr(dao_pdu_t *pdu, char *ip_address)
{
    if (pdu == NULL || ip_address == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    pdu->rr_stack = (char **) realloc(pdu->rr_stack, (++pdu->rr_count) * sizeof(char *));
    pdu->rr_stack[pdu->rr_count - 1] = strdup(ip_address);

    return TRUE;
}

bool rpl_init_node(node_t *node, rpl_node_info_t *node_info)
{
    if (node == NULL || node_info == NULL) {
        rs_error("invalid argument");
        return FALSE;
    }

    return TRUE;
}
