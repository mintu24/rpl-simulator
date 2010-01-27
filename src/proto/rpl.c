
#include "rpl.h"
#include "../system.h"


    /**** local function prototypes ****/

static rpl_remote_node_t *  rpl_remote_node_info_create(node_t *node, rpl_dio_pdu_t *dio_message);
static void                 rpl_remote_node_info_destroy(rpl_remote_node_t *remote_node_info);

static bool                 rpl_send(node_t *node, node_t *dst_node, uint8 code, void *sdu);


    /**** exported functions ****/

rpl_dio_pdu_t *rpl_dio_pdu_create(bool grounded, bool da_trigger, bool da_support, uint8 dag_pref, uint8 seq_number, uint8 instance_id, uint8 rank, char *dag_id)
{
    rs_assert(dag_id != NULL);

    rpl_dio_pdu_t *pdu = malloc(sizeof(rpl_dio_pdu_t));

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

void rpl_dio_pdu_destroy(rpl_dio_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    if (pdu->dag_id != NULL)
        free(pdu->dag_id);

    if (pdu->suboptions != NULL)
        rpl_dio_suboption_destroy(pdu->suboptions);

    free(pdu);
}

rpl_dio_suboption_t *rpl_dio_suboption_dag_config_create(int8 interval_doublings, int8 interval_min, int8 redundancy_constant, int8 max_rank_increase)
{
    rpl_dio_suboption_t *suboption = malloc(sizeof(rpl_dio_suboption_t));

    rpl_dio_suboption_dag_config_t *content = malloc(sizeof(rpl_dio_suboption_dag_config_t));

    content->interval_doublings = interval_doublings;
    content->interval_min = interval_min;
    content->redundancy_constant = redundancy_constant;
    content->max_rank_increase = max_rank_increase;

    suboption->type = RPL_DIO_SUBOPTION_TYPE_DAG_CONFIG;
    suboption->next_suboption = NULL;
    suboption->content = content;

    return suboption;
}

bool rpl_dio_suboption_destroy(rpl_dio_suboption_t *suboption)
{
    rs_assert(suboption != NULL);

    bool all_ok = TRUE;

    if (suboption->next_suboption != NULL) {
        if (!rpl_dio_suboption_destroy(suboption->next_suboption))
            all_ok = FALSE;
    }

    if (suboption->content != NULL) {
        switch (suboption->type) {
            case RPL_DIO_SUBOPTION_TYPE_DAG_CONFIG: {
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

bool rpl_dio_pdu_add_suboption(rpl_dio_pdu_t *pdu, rpl_dio_suboption_t *suboption)
{
    rs_assert(pdu != NULL);

    if (pdu->suboptions == NULL) {
        pdu->suboptions = suboption;
    }
    else {
        rpl_dio_suboption_t *suboption = pdu->suboptions;

        while (suboption->next_suboption != NULL)
            suboption = suboption->next_suboption;

        suboption->next_suboption = suboption;
    }

    return TRUE;
}

rpl_dao_pdu_t *rpl_dao_pdu_create(uint16 sequence, uint8 instance_id, uint8 rank, uint32 dao_lifetime, char *prefix, uint8 prefix_len)
{
    rs_assert(prefix != NULL);

    rpl_dao_pdu_t *pdu = malloc(sizeof(rpl_dao_pdu_t));

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

void rpl_dao_pdu_destroy(rpl_dao_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    while (pdu->rr_count > 0) {
        free(pdu->rr_stack[--pdu->rr_count]);
    }

    free(pdu->rr_stack);

    free(pdu);
}

bool rpl_dao_pdu_add_rr(rpl_dao_pdu_t *pdu, char *ip_address)
{
    rs_assert(pdu != NULL);
    rs_assert(ip_address != NULL);

    pdu->rr_stack = realloc(pdu->rr_stack, (++pdu->rr_count) * sizeof(char *));
    pdu->rr_stack[pdu->rr_count - 1] = strdup(ip_address);

    return TRUE;
}

bool rpl_node_init(node_t *node)
{
    rs_assert(node != NULL);

    node->rpl_info = malloc(sizeof(rpl_node_info_t));

    node->rpl_info->rank = 0;
    node->rpl_info->seq_num = 0;
    node->rpl_info->pref_parent = NULL;
    node->rpl_info->parent_list = NULL;
    node->rpl_info->parent_count = 0;
    node->rpl_info->sibling_list = NULL;
    node->rpl_info->sibling_count = 0;

    g_static_rec_mutex_init(&node->rpl_info->mutex);

    return TRUE;
}

void rpl_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->rpl_info != NULL) {
        if (node->rpl_info->parent_list != NULL) {
            int i;
            for (i = 0; i < node->rpl_info->parent_count; i++) {
                rpl_remote_node_info_destroy(node->rpl_info->parent_list[i]);
            }

            free(node->rpl_info->parent_list);
        }

        if (node->rpl_info->sibling_list != NULL) {
            int i;
            for (i = 0; i < node->rpl_info->sibling_count; i++) {
                rpl_remote_node_info_destroy(node->rpl_info->sibling_list[i]);
            }

            free(node->rpl_info->sibling_list);
        }

        g_static_rec_mutex_free(&node->rpl_info->mutex);

        free(node->rpl_info);
    }
}

uint8 rpl_node_get_rank(node_t *node)
{
    rs_assert(node != NULL);

    return node->rpl_info->rank;
}

void rpl_node_set_rank(node_t *node, uint8 rank)
{
    rs_assert(node != NULL);

    node->rpl_info->rank = rank;
}

uint8 rpl_node_get_seq_num(node_t *node)
{
    rs_assert(node != NULL);

    return node->rpl_info->seq_num;
}

void rpl_node_set_seq_num(node_t *node, uint8 seq_num)
{
    rs_assert(node != NULL);

    node->rpl_info->seq_num = seq_num;
}

rpl_remote_node_t *rpl_node_get_pref_parent(node_t *node)
{
    rs_assert(node != NULL);

    return node->rpl_info->pref_parent;
}

void rpl_node_set_pref_parent(node_t *node, rpl_remote_node_t *parent)
{
    rs_assert(node != NULL);

    node->rpl_info->pref_parent = parent;
}

bool rpl_node_add_parent(node_t *node, node_t *parent_node, rpl_dio_pdu_t *dio_message)
{
    rs_assert(node != NULL);
    rs_assert(parent_node != NULL);
    rs_assert(dio_message != NULL);

    rpl_remote_node_t *remote_node_info = rpl_remote_node_info_create(parent_node, dio_message);

    rpl_node_lock(node);

    node->rpl_info->parent_list = realloc(node->rpl_info->parent_list, (node->rpl_info->parent_count + 1) * sizeof(rpl_remote_node_t *));
    node->rpl_info->parent_list[node->rpl_info->parent_count++] = remote_node_info;

    rpl_node_unlock(node);

    return TRUE;
}

bool rpl_node_remove_parent(node_t *node, rpl_remote_node_t *parent)
{
    rs_assert(node != NULL);
    rs_assert(parent != NULL);

    rpl_node_lock(node);

    int pos = -1, i;
    for (i = 0; i < node->rpl_info->parent_count; i++) {
        if (node->rpl_info->parent_list[i] == parent) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' does not have node '%s' as a parent", phy_node_get_name(node), phy_node_get_name(parent->node));
        rpl_node_unlock(node);

        return FALSE;
    }

    rpl_remote_node_info_destroy(node->rpl_info->parent_list[pos]);

    for (i = pos; i < node->rpl_info->parent_count - 1; i++) {
        node->rpl_info->parent_list[i] = node->rpl_info->parent_list[i + 1];
    }

    node->rpl_info->parent_count--;
    node->rpl_info->parent_list = realloc(node->rpl_info->parent_list, node->rpl_info->parent_count * sizeof(rpl_remote_node_t *));

    rpl_node_unlock(node);

    return TRUE;
}

rpl_remote_node_t **rpl_node_get_parent_list(node_t *node, uint16 *parent_count)
{
    rs_assert(node != NULL);

    if (parent_count != NULL)
        *parent_count = node->rpl_info->parent_count;

    return node->rpl_info->parent_list;
}

rpl_remote_node_t *rpl_node_find_parent_by_node(node_t *node, node_t *parent_node)
{
    rs_assert(node != NULL);
    rs_assert(parent_node != NULL);

    rpl_node_lock(node);

    int i;
    for (i = 0; i < node->rpl_info->parent_count; i++) {
        if (node->rpl_info->parent_list[i]->node == parent_node) {
            rpl_node_unlock(node);

            return node->rpl_info->parent_list[i];
        }
    }

    rpl_node_unlock(node);

    return NULL;
}

bool rpl_node_add_sibling(node_t *node, node_t *sibling_node, rpl_dio_pdu_t *dio_message)
{
    rs_assert(node != NULL);
    rs_assert(sibling_node != NULL);
    rs_assert(dio_message != NULL);

    rpl_remote_node_t *remote_node_info = rpl_remote_node_info_create(sibling_node, dio_message);

    rpl_node_lock(node);

    node->rpl_info->sibling_list = realloc(node->rpl_info->sibling_list, (node->rpl_info->sibling_count + 1) * sizeof(rpl_remote_node_t *));
    node->rpl_info->sibling_list[node->rpl_info->sibling_count++] = remote_node_info;

    rpl_node_unlock(node);

    return TRUE;
}

bool rpl_node_remove_sibling(node_t *node, rpl_remote_node_t *sibling)
{
    rs_assert(node != NULL);
    rs_assert(sibling != NULL);

    rpl_node_lock(node);

    int pos = -1, i;
    for (i = 0; i < node->rpl_info->sibling_count; i++) {
        if (node->rpl_info->sibling_list[i] == sibling) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' does not have node '%s' as a sibling", phy_node_get_name(node), phy_node_get_name(sibling->node));
        rpl_node_unlock(node);

        return FALSE;
    }

    rpl_remote_node_info_destroy(node->rpl_info->sibling_list[pos]);

    for (i = pos; i < node->rpl_info->sibling_count - 1; i++) {
        node->rpl_info->sibling_list[i] = node->rpl_info->sibling_list[i + 1];
    }

    node->rpl_info->sibling_count--;
    node->rpl_info->sibling_list = realloc(node->rpl_info->sibling_list, node->rpl_info->sibling_count * sizeof(rpl_remote_node_t *));

    rpl_node_unlock(node);

    return TRUE;
}

rpl_remote_node_t **rpl_node_get_sibling_list(node_t *node, uint16 *sibling_count)
{
    rs_assert(node != NULL);

    if (sibling_count != NULL)
        *sibling_count = node->rpl_info->sibling_count;

    return node->rpl_info->sibling_list;
}

rpl_remote_node_t *rpl_node_find_sibling_by_node(node_t *node, node_t *sibling_node)
{
    rs_assert(node != NULL);
    rs_assert(sibling_node != NULL);

    rpl_node_lock(node);

    int i;
    for (i = 0; i < node->rpl_info->sibling_count; i++) {
        if (node->rpl_info->sibling_list[i]->node == sibling_node) {
            rpl_node_unlock(node);

            return node->rpl_info->sibling_list[i];
        }
    }

    rpl_node_unlock(node);

    return NULL;
}

bool rpl_send_dis(node_t *node, node_t *dst_node)
{
    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);

    node_execute_event(
            node,
            "rpl_event_before_dis_pdu_sent",
            (node_event_t) rpl_event_before_dis_pdu_sent,
            dst_node, NULL,
            TRUE);

    return rpl_send(node, dst_node, ICMP_RPL_CODE_DIS, NULL);
}

bool rpl_receive_dis(node_t *node, node_t *src_node)
{
    rs_assert(node != NULL);

    node_execute_event(
            node,
            "rpl_event_after_dis_pdu_received",
            (node_event_t) rpl_event_after_dis_pdu_received,
            src_node, NULL,
            TRUE);

    return TRUE;
}

bool rpl_send_dio(node_t *node, node_t *dst_node, rpl_dio_pdu_t *pdu)
{
    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    node_execute_event(
            node,
            "rpl_event_before_dio_pdu_sent",
            (node_event_t) rpl_event_before_dio_pdu_sent,
            dst_node, pdu,
            TRUE);

    return rpl_send(node, dst_node, ICMP_RPL_CODE_DIO, pdu);
}

bool rpl_receive_dio(node_t *node, node_t *src_node, rpl_dio_pdu_t *pdu)
{
    rs_assert(pdu != NULL);
    rs_assert(node != NULL);

    node_execute_event(
            node,
            "rpl_event_after_dio_pdu_received",
            (node_event_t) rpl_event_after_dio_pdu_received,
            src_node, pdu,
            TRUE);

    rpl_dio_pdu_destroy(pdu);

    return TRUE;
}

bool rpl_send_dao(node_t *node, node_t *dst_node, rpl_dao_pdu_t *pdu)
{
    rs_assert(rs_system != NULL);
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    node_execute_event(
            node,
            "rpl_event_before_dao_pdu_sent",
            (node_event_t) rpl_event_before_dao_pdu_sent,
            dst_node, pdu,
            TRUE);

    return rpl_send(node, dst_node, ICMP_RPL_CODE_DAO, pdu);
}

bool rpl_receive_dao(node_t *node, node_t *src_node, rpl_dao_pdu_t *pdu)
{
    rs_assert(pdu != NULL);
    rs_assert(node != NULL);

    node_execute_event(
            node,
            "rpl_event_after_dao_pdu_received",
            (node_event_t) rpl_event_after_dao_pdu_received,
            src_node, pdu,
            TRUE);

    rpl_dao_pdu_destroy(pdu);

    return TRUE;
}

void rpl_event_after_node_wake(node_t *node)
{
}

void rpl_event_before_node_kill(node_t *node)
{
}

void rpl_event_before_dis_pdu_sent(node_t *node, node_t *dst_node)
{
    rs_debug(DEBUG_RPL, "'%s' -> '%s'", phy_node_get_name(node), (dst_node != NULL ? phy_node_get_name(dst_node) : "*"));
}

void rpl_event_after_dis_pdu_received(node_t *node, node_t *src_node)
{
    rs_debug(DEBUG_RPL, "'%s' -> '%s'", (src_node != NULL ? phy_node_get_name(src_node) : "?"), phy_node_get_name(node));
}

void rpl_event_before_dio_pdu_sent(node_t *node, node_t *dst_node, rpl_dio_pdu_t *pdu)
{
    rs_debug(DEBUG_RPL, "'%s' -> '%s'", phy_node_get_name(node), (dst_node != NULL ? phy_node_get_name(dst_node) : "*"));
}

void rpl_event_after_dio_pdu_received(node_t *node, node_t *src_node, rpl_dio_pdu_t *pdu)
{
    rs_debug(DEBUG_RPL, "'%s' -> '%s'", (src_node != NULL ? phy_node_get_name(src_node) : "?"), phy_node_get_name(node));
}

void rpl_event_before_dao_pdu_sent(node_t *node, node_t *dst_node, rpl_dao_pdu_t *pdu)
{
    rs_debug(DEBUG_RPL, "'%s' -> '%s'", phy_node_get_name(node), (dst_node != NULL ? phy_node_get_name(dst_node) : "*"));
}

void rpl_event_after_dao_pdu_received(node_t *node, node_t *src_node, rpl_dao_pdu_t *pdu)
{
    rs_debug(DEBUG_RPL, "'%s' -> '%s'", (src_node != NULL ? phy_node_get_name(src_node) : "?"), phy_node_get_name(node));
}


    /**** local functions ****/

static rpl_remote_node_t *rpl_remote_node_info_create(node_t *node, rpl_dio_pdu_t *message)
{
    rs_assert(node != NULL);
    rs_assert(message != NULL);

    rpl_remote_node_t *remote_node_info = malloc(sizeof(rpl_remote_node_t));

    remote_node_info->node = node;
    remote_node_info->last_dio_message = rpl_dio_pdu_create(
            message->grounded,
            message->da_trigger,
            message->da_support,
            message->dag_pref,
            message->seq_number,
            message->instance_id,
            message->rank,
            message->dag_id);

    return remote_node_info;
}

static void rpl_remote_node_info_destroy(rpl_remote_node_t *remote_node_info)
{
    rs_assert(remote_node_info != NULL);

    if (remote_node_info->last_dio_message != NULL)
        rpl_dio_pdu_destroy(remote_node_info->last_dio_message);

    free(remote_node_info);
}


static bool rpl_send(node_t *node, node_t *dst_node, uint8 code, void *sdu)
{
    if (!icmp_send(node, dst_node, ICMP_TYPE_RPL, code, sdu)) {
        rs_error("failed to send ICMP message");
        return FALSE;
    }

    return TRUE;
}
