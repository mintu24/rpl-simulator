
#include "rpl.h"
#include "../system.h"


rpl_dio_pdu_t *dio_pdu_create(bool grounded, bool da_trigger, bool da_support, int8 dag_pref, int8 seq_number, int8 instance_id, int8 rank, char *dag_id)
{
    rs_assert(dag_id != NULL);

    rpl_dio_pdu_t *pdu = (rpl_dio_pdu_t *) malloc(sizeof(rpl_dio_pdu_t));

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

bool dio_pdu_destroy(rpl_dio_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

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
    rs_assert(suboption != NULL);

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

bool dio_pdu_add_suboption(rpl_dio_pdu_t *pdu, dio_suboption_t *suboption)
{
    rs_assert(pdu != NULL);

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

rpl_dao_pdu_t *dao_pdu_create(uint16 sequence, uint8 instance_id, uint8 rank, uint32 dao_lifetime, char *prefix, uint8 prefix_len)
{
    rs_assert(prefix != NULL);

    rpl_dao_pdu_t *pdu = (rpl_dao_pdu_t *) malloc(sizeof(rpl_dao_pdu_t));

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

bool dao_pdu_destroy(rpl_dao_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    while (pdu->rr_count > 0) {
        free(pdu->rr_stack[--pdu->rr_count]);
    }

    free(pdu->rr_stack);

    free(pdu);

    return TRUE;
}

bool dao_pdu_add_rr(rpl_dao_pdu_t *pdu, char *ip_address)
{
    rs_assert(pdu != NULL);
    rs_assert(ip_address != NULL);

    pdu->rr_stack = (char **) realloc(pdu->rr_stack, (++pdu->rr_count) * sizeof(char *));
    pdu->rr_stack[pdu->rr_count - 1] = strdup(ip_address);

    return TRUE;
}

rpl_node_info_t *rpl_node_info_create()
{
    rpl_node_info_t *node_info = (rpl_node_info_t *) malloc(sizeof(rpl_node_info_t));

    node_info->parent_list = NULL;
    node_info->parent_count = 0;
    node_info->sibling_list = NULL;
    node_info->sibling_count = 0;

    return node_info;
}

bool rpl_node_info_destroy(rpl_node_info_t *node_info)
{
    rs_assert(node_info != NULL);

    // todo: free the parent list & the sibling list
    free(node_info);

    return TRUE;
}

bool rpl_init_node(node_t *node, rpl_node_info_t *node_info)
{
    rs_assert(node != NULL);
    rs_assert(node_info != NULL);

    node->rpl_info = node_info;

    return TRUE;
}

node_t **rpl_node_get_parent_list(node_t *node, uint16 *parent_count)
{
    rs_assert(node != NULL);

    g_mutex_lock(node->proto_info_mutex);

    node_t **list = node->rpl_info->parent_list;
    *parent_count = node->rpl_info->parent_count;

    g_mutex_unlock(node->proto_info_mutex);

    return list;
}

bool rpl_node_add_parent(node_t *node, node_t *parent)
{
    rs_assert(node != NULL);
    rs_assert(parent != NULL);

    g_mutex_lock(node->proto_info_mutex);

    int i;
    for (i = 0; i < node->rpl_info->parent_count; i++) {
        if (node->rpl_info->parent_list[i] == parent) {
            rs_error("node '%s' already has node '%s' as a parent", phy_node_get_name(node), phy_node_get_name(parent));
            g_mutex_unlock(node->proto_info_mutex);

            return FALSE;
        }
    }

    node->rpl_info->parent_list = realloc(node->rpl_info->parent_list, (node->rpl_info->parent_count + 1) * sizeof(node_t));
    node->rpl_info->parent_list[node->rpl_info->parent_count++] = parent;

    g_mutex_unlock(node->proto_info_mutex);

    return TRUE;
}

bool rpl_node_remove_parent(node_t *node, node_t *parent)
{
    rs_assert(node != NULL);
    rs_assert(parent != NULL);

    g_mutex_lock(node->proto_info_mutex);

    int pos = -1, i;
    for (i = 0; i < node->rpl_info->parent_count; i++) {
        if (node->rpl_info->parent_list[i] == parent) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' does not have node '%s' as a parent", phy_node_get_name(node), phy_node_get_name(parent));
        g_mutex_unlock(node->proto_info_mutex);

        return FALSE;
    }

    for (i = pos; i < node->rpl_info->parent_count - 1; i++) {
        node->rpl_info->parent_list[i] = node->rpl_info->parent_list[i + 1];
    }

    // todo: call realloc()
    node->rpl_info->parent_count--;

    g_mutex_unlock(node->proto_info_mutex);

    return TRUE;
}

node_t **rpl_node_get_sibling_list(node_t *node, uint16 *sibling_count)
{
    rs_assert(node != NULL);

    g_mutex_lock(node->proto_info_mutex);

    node_t **list = node->rpl_info->sibling_list;
    *sibling_count = node->rpl_info->sibling_count;

    g_mutex_unlock(node->proto_info_mutex);

    return list;
}

bool rpl_node_add_sibling(node_t *node, node_t *sibling)
{
    rs_assert(node != NULL);
    rs_assert(sibling != NULL);

    g_mutex_lock(node->proto_info_mutex);

    int i;
    for (i = 0; i < node->rpl_info->sibling_count; i++) {
        if (node->rpl_info->sibling_list[i] == sibling) {
            rs_error("node '%s' already has node '%s' as a sibling", phy_node_get_name(node), phy_node_get_name(sibling));
            g_mutex_unlock(node->proto_info_mutex);

            return FALSE;
        }
    }

    node->rpl_info->sibling_list = realloc(node->rpl_info->sibling_list, (node->rpl_info->sibling_count + 1) * sizeof(node_t));
    node->rpl_info->sibling_list[node->rpl_info->sibling_count++] = sibling;

    g_mutex_unlock(node->proto_info_mutex);

    return TRUE;
}

bool rpl_node_remove_sibling(node_t *node, node_t *sibling)
{
    rs_assert(node != NULL);
    rs_assert(sibling != NULL);

    g_mutex_lock(node->proto_info_mutex);

    int pos = -1, i;
    for (i = 0; i < node->rpl_info->sibling_count; i++) {
        if (node->rpl_info->sibling_list[i] == sibling) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' does not have node '%s' as a sibling", phy_node_get_name(node), phy_node_get_name(sibling));
        g_mutex_unlock(node->proto_info_mutex);

        return FALSE;
    }

    for (i = pos; i < node->rpl_info->sibling_count - 1; i++) {
        node->rpl_info->sibling_list[i] = node->rpl_info->sibling_list[i + 1];
    }

    // todo: call realloc()
    node->rpl_info->sibling_count--;

    g_mutex_unlock(node->proto_info_mutex);

    return TRUE;
}

void rpl_event_before_dis_pdu_sent(node_t *node, void *data)
{
    rs_debug(NULL);
}

void rpl_event_after_dis_pdu_received(node_t *node, void *data)
{
    rs_debug(NULL);
}

void rpl_event_before_dio_pdu_sent(node_t *node, rpl_dio_pdu_t *pdu)
{
    rs_debug(NULL);
}

void rpl_event_after_dio_pdu_received(node_t *node, rpl_dio_pdu_t *pdu)
{
    rs_debug(NULL);
}

void rpl_event_before_dao_pdu_sent(node_t *node, rpl_dao_pdu_t *pdu)
{
    rs_debug(NULL);
}

void rpl_event_after_dao_pdu_received(node_t *node, rpl_dao_pdu_t *pdu)
{
    rs_debug(NULL);
}
