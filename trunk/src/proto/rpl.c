
#include <math.h>

#include "rpl.h"
#include "../system.h"


    /**** local function prototypes ****/

static rpl_remote_node_t *  rpl_remote_node_info_create(node_t *node, rpl_dio_pdu_t *dio_message);
static void                 rpl_remote_node_info_destroy(rpl_remote_node_t *remote_node_info);

static bool                 rpl_send(node_t *node, node_t *dst_node, uint8 code, void *sdu);

static void                 follow_node(node_t *node, rpl_remote_node_t *parent);
static uint8                compute_rank(node_t *node, rpl_remote_node_t *potential_parent);
static void                 refresh_parents_and_siblings(node_t *node);
static void                 trickle_timer_fired(node_t *node, void *data);
static void                 remove_neighbor(node_t *node, rpl_remote_node_t *remote_node);


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

rpl_dio_pdu_t *rpl_dio_pdu_duplicate(rpl_dio_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    rpl_dio_pdu_t *new_pdu = malloc(sizeof(rpl_dio_pdu_t));

    new_pdu->grounded = pdu->grounded;
    new_pdu->da_trigger = pdu->da_trigger;
    new_pdu->da_support = pdu->da_support;
    new_pdu->dag_pref = pdu->dag_pref;
    new_pdu->seq_number = pdu->seq_number;
    new_pdu->instance_id = pdu->instance_id;
    new_pdu->rank = pdu->rank;
    new_pdu->dag_id = strdup(pdu->dag_id);
    new_pdu->suboptions = NULL;

    rpl_dio_suboption_t *suboption = pdu->suboptions;
    while (suboption != NULL) {
        switch (suboption->type) {
            case RPL_DIO_SUBOPTION_TYPE_DAG_CONFIG : {
                rpl_dio_suboption_t *new_suboption = rpl_dio_suboption_dag_config_create(
                        ((rpl_dio_suboption_dag_config_t *) suboption->next_suboption)->interval_doublings,
                        ((rpl_dio_suboption_dag_config_t *) suboption->next_suboption)->interval_min,
                        ((rpl_dio_suboption_dag_config_t *) suboption->next_suboption)->redundancy_constant,
                        ((rpl_dio_suboption_dag_config_t *) suboption->next_suboption)->max_rank_increase);

                rpl_dio_pdu_add_suboption(new_pdu, new_suboption);

                break;
            }

            default :
                rs_error("unknown DIO suboption type '0x%02X'", suboption->type);
        }
    }

    return new_pdu;
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

rpl_dao_pdu_t *rpl_dao_pdu_duplicate(rpl_dao_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    rpl_dao_pdu_t *new_pdu = malloc(sizeof(rpl_dao_pdu_t));

    new_pdu->sequence = pdu->sequence;
    new_pdu->instance_id = pdu->instance_id;
    new_pdu->rank = pdu->rank;
    new_pdu->dao_lifetime = pdu->dao_lifetime;
    new_pdu->prefix = strdup(pdu->prefix);
    new_pdu->prefix_len = pdu->prefix_len;

    if (pdu->rr_count == 0) {
        new_pdu->rr_stack = NULL;
    }
    else {
        new_pdu->rr_stack = malloc(pdu->rr_count * sizeof(char *));

        int i;
        for (i = 0; i < pdu->rr_count; i++) {
            new_pdu->rr_stack[i] = strdup(pdu->rr_stack[i]);
        }
    }

    new_pdu->rr_count = pdu->rr_count;

    return new_pdu;
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

    node->rpl_info->dag_pref = RPL_DEFAULT_DAG_PREF;
    node->rpl_info->dag_id = strdup(ip_node_get_address(node));
    node->rpl_info->seq_num = 0;
    node->rpl_info->rank = RPL_RANK_INFINITY;

    node->rpl_info->pref_parent = NULL;

    node->rpl_info->parent_list = NULL;
    node->rpl_info->parent_count = 0;

    node->rpl_info->sibling_list = NULL;
    node->rpl_info->sibling_count = 0;

    node->rpl_info->neighbor_list = NULL;
    node->rpl_info->neighbor_count = 0;

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

uint8 rpl_node_get_dag_pref(node_t *node)
{
    rs_assert(node != NULL);

    return node->rpl_info->dag_pref;
}

void rpl_node_set_dag_pref(node_t *node, uint8 dag_pref)
{
    rs_assert(node != NULL);

    node->rpl_info->dag_pref = dag_pref;
}

char *rpl_node_get_dag_id(node_t *node)
{
    rs_assert(node != NULL);

    return node->rpl_info->dag_id;
}

void rpl_node_set_dag_id(node_t *node, char *dag_id)
{
    rs_assert(node != NULL);
    rs_assert(dag_id != NULL);

    rpl_node_lock(node);

    if (node->rpl_info->dag_id!= NULL)
        free(node->rpl_info->dag_id);

    node->rpl_info->dag_id = strdup(dag_id);

    rpl_node_unlock(node);
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

bool rpl_node_add_parent(node_t *node, rpl_remote_node_t *parent)
{
    rs_assert(node != NULL);
    rs_assert(parent != NULL);

    rpl_node_lock(node);

    node->rpl_info->parent_list = realloc(node->rpl_info->parent_list, (node->rpl_info->parent_count + 1) * sizeof(rpl_remote_node_t *));
    node->rpl_info->parent_list[node->rpl_info->parent_count++] = parent;

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

bool rpl_node_add_sibling(node_t *node, rpl_remote_node_t *sibling)
{
    rs_assert(node != NULL);
    rs_assert(sibling != NULL);

    rpl_node_lock(node);

    node->rpl_info->sibling_list = realloc(node->rpl_info->sibling_list, (node->rpl_info->sibling_count + 1) * sizeof(rpl_remote_node_t *));
    node->rpl_info->sibling_list[node->rpl_info->sibling_count++] = sibling;

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

bool rpl_node_add_neighbor(node_t *node, node_t *neighbor_node, rpl_dio_pdu_t *dio_message)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);
    rs_assert(dio_message != NULL);

    rpl_remote_node_t *remote_node_info = rpl_remote_node_info_create(neighbor_node, dio_message);

    rpl_node_lock(node);

    node->rpl_info->neighbor_list = realloc(node->rpl_info->neighbor_list, (node->rpl_info->neighbor_count + 1) * sizeof(rpl_remote_node_t *));
    node->rpl_info->neighbor_list[node->rpl_info->neighbor_count++] = remote_node_info;

    rpl_node_unlock(node);

    return TRUE;
}

bool rpl_node_remove_neighbor(node_t *node, rpl_remote_node_t *neighbor)
{
    rs_assert(node != NULL);
    rs_assert(neighbor != NULL);

    rpl_node_lock(node);

    int pos = -1, i;
    for (i = 0; i < node->rpl_info->neighbor_count; i++) {
        if (node->rpl_info->neighbor_list[i] == neighbor) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' does not have node '%s' as a neighbor", phy_node_get_name(node), phy_node_get_name(neighbor->node));
        rpl_node_unlock(node);

        return FALSE;
    }

    rpl_remote_node_info_destroy(node->rpl_info->neighbor_list[pos]);

    for (i = pos; i < node->rpl_info->neighbor_count - 1; i++) {
        node->rpl_info->neighbor_list[i] = node->rpl_info->neighbor_list[i + 1];
    }

    node->rpl_info->neighbor_count--;
    node->rpl_info->neighbor_list = realloc(node->rpl_info->neighbor_list, node->rpl_info->neighbor_count * sizeof(rpl_remote_node_t *));

    rpl_node_unlock(node);

    return TRUE;
}

rpl_remote_node_t **rpl_node_get_neighbor_list(node_t *node, uint16 *neighbor_count)
{
    rs_assert(node != NULL);

    if (neighbor_count != NULL)
        *neighbor_count = node->rpl_info->neighbor_count;

    return node->rpl_info->neighbor_list;
}

rpl_remote_node_t *rpl_node_find_neighbor_by_node(node_t *node, node_t *neighbor_node)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);

    rpl_node_lock(node);

    rpl_remote_node_t *remote_node = NULL;
    int i;
    for (i = 0; i < node->rpl_info->neighbor_count; i++) {
        if (node->rpl_info->neighbor_list[i]->node == neighbor_node) {
            remote_node = node->rpl_info->neighbor_list[i];
            break;
        }
    }

    rpl_node_unlock(node);

    return remote_node;
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
//    if (rpl_node_is_root(node)) {
//
//    }
//    else { /* if the node is not root */
//
//    }

    node_schedule(node, "trickle timer", trickle_timer_fired, NULL, 2000000, TRUE);
}

void rpl_event_before_node_kill(node_t *node)
{
}

void rpl_event_before_dis_pdu_sent(node_t *node, node_t *dst_node)
{
    //rs_debug(DEBUG_RPL, "'%s' -> '%s'", phy_node_get_name(node), (dst_node != NULL ? phy_node_get_name(dst_node) : "*"));
}

void rpl_event_after_dis_pdu_received(node_t *node, node_t *src_node)
{
    //rs_debug(DEBUG_RPL, "'%s' -> '%s'", (src_node != NULL ? phy_node_get_name(src_node) : "?"), phy_node_get_name(node));
}

void rpl_event_before_dio_pdu_sent(node_t *node, node_t *dst_node, rpl_dio_pdu_t *pdu)
{
    //rs_debug(DEBUG_RPL, "'%s' -> '%s'", phy_node_get_name(node), (dst_node != NULL ? phy_node_get_name(dst_node) : "*"));
}

void rpl_event_after_dio_pdu_received(node_t *node, node_t *src_node, rpl_dio_pdu_t *pdu)
{
    rs_debug(DEBUG_RPL, "received DIO message from '%s'", phy_node_get_name(src_node));

//    rpl_node_lock(node);

    /* update last DIO received message from src_node */
    rpl_remote_node_t *remote_node = rpl_node_find_neighbor_by_node(node, src_node);
    if (remote_node != NULL) {
        rpl_dio_pdu_destroy(remote_node->last_dio_message);
        remote_node->last_dio_message = rpl_dio_pdu_duplicate(pdu);
    }
    else {
        rpl_node_add_neighbor(node, src_node, pdu);
    }

    char temp[256];
    snprintf(temp, sizeof(temp), "remove '%s' neighbor", phy_node_get_name(src_node));
    node_schedule(node, temp,  (node_schedule_func_t) remove_neighbor, rpl_node_find_neighbor_by_node(node, src_node), 2000000, FALSE);

    uint16 parent_count;
    rpl_remote_node_t **parent_list = rpl_node_get_parent_list(node, &parent_count);

    if (rpl_node_is_root(node)) {
        rpl_node_unlock(node);
        return;
    }

    if (parent_count == 0) {
        follow_node(node, rpl_node_find_neighbor_by_node(node, src_node));
    }
    else {
        if (pdu->rank > rpl_node_get_rank(node)) {
            return;
        }
        else {
            refresh_parents_and_siblings(node);
        }
    }

//    rpl_node_unlock(node);
}

void rpl_event_before_dao_pdu_sent(node_t *node, node_t *dst_node, rpl_dao_pdu_t *pdu)
{
//    rs_debug(DEBUG_RPL, "'%s' -> '%s'", phy_node_get_name(node), (dst_node != NULL ? phy_node_get_name(dst_node) : "*"));
}

void rpl_event_after_dao_pdu_received(node_t *node, node_t *src_node, rpl_dao_pdu_t *pdu)
{
//    rs_debug(DEBUG_RPL, "'%s' -> '%s'", (src_node != NULL ? phy_node_get_name(src_node) : "?"), phy_node_get_name(node));
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

static void follow_node(node_t *node, rpl_remote_node_t *parent)
{
    rs_assert(node != NULL);
    rs_assert(parent != NULL);

    rpl_node_set_dag_id(node, parent->last_dio_message->dag_id);
    rpl_node_set_seq_num(node, parent->last_dio_message->seq_number);
    rpl_node_set_dag_pref(node, parent->last_dio_message->dag_pref);

    refresh_parents_and_siblings(node);
}

static uint8 compute_rank(node_t *node, rpl_remote_node_t *potential_parent)
{
    percent_t link_quality = rs_system_get_link_quality(node, potential_parent->node);

    uint8 rank_increment = RPL_MAXIMUM_RANK_INCREMENT - pow(link_quality, 0.2) * (RPL_MAXIMUM_RANK_INCREMENT - RPL_MINIMUM_RANK_INCREMENT);
    uint8 potential_rank = potential_parent->last_dio_message->rank + rank_increment;

    return potential_rank;
}

static void refresh_parents_and_siblings(node_t *node)
{
    rpl_node_lock(node);

    uint16 neighbor_count, i;
    rpl_remote_node_t **neighbor_list = rpl_node_get_neighbor_list(node, &neighbor_count);

    uint16 compatible_count = 0;
    rpl_remote_node_t **compatible_list = malloc(neighbor_count * sizeof(rpl_remote_node_t *));

    for (i = 0; i < neighbor_count; i++) {
        rpl_remote_node_t *neighbor = neighbor_list[i];

        if (strcmp(neighbor->last_dio_message->dag_id, rpl_node_get_dag_id(node)) != 0) {
            continue;
        }
        if (neighbor->last_dio_message->seq_number != rpl_node_get_seq_num(node)) {
            continue;
        }

        compatible_list[compatible_count++] = neighbor;
    }

    uint8 best_rank = 0xFF;
    rpl_remote_node_t *pref_parent = NULL;

    for (i = 0; i < compatible_count; i++) {
        rpl_remote_node_t *compatible = compatible_list[i];

        uint8 rank = compute_rank(node, compatible);
        if (rank < best_rank) {
            best_rank = rank;
            pref_parent = compatible;
        }
    }

    rpl_node_set_pref_parent(node, pref_parent);
    rpl_node_set_rank(node, best_rank);

    for (i = 0; i < compatible_count; i++) {
        rpl_remote_node_t *compatible = compatible_list[i];

        if (compatible->last_dio_message->rank == best_rank) {
            rpl_remote_node_t *parent = rpl_node_find_parent_by_node(node, compatible->node);
            if (parent != NULL) {
                rpl_node_remove_parent(node, parent);
            }

            rpl_remote_node_t *sibling = rpl_node_find_sibling_by_node(node, compatible->node);
            if (sibling == NULL) {
                rpl_node_add_sibling(node, compatible);
                //printf("%s.add_sibling(%s)\n", node->phy_info->name, compatible->node->phy_info->name);
            }
        }
        else if (compatible->last_dio_message->rank < best_rank) {
            rpl_remote_node_t *parent = rpl_node_find_parent_by_node(node, compatible->node);
            if (parent == NULL) {
                rpl_node_add_parent(node, compatible);
            }

            rpl_remote_node_t *sibling = rpl_node_find_sibling_by_node(node, compatible->node);
            if (sibling != NULL) {
                rpl_node_remove_sibling(node, sibling);
                //printf("%s.remove_sibling(%s)\n", node->phy_info->name, sibling->node->phy_info->name);
            }
        }
        else {
            rpl_remote_node_t *parent = rpl_node_find_parent_by_node(node, compatible->node);
            if (parent != NULL) {
                rpl_node_remove_parent(node, parent);
                //printf("%s.remove_parent(%s)\n", node->phy_info->name, parent->node->phy_info->name);
            }

            rpl_remote_node_t *sibling = rpl_node_find_sibling_by_node(node, compatible->node);
            if (sibling != NULL) {
                rpl_node_remove_sibling(node, sibling);
                //printf("%s.remove_sibling(%s)\n", node->phy_info->name, sibling->node->phy_info->name);
            }
        }
    }

    free(compatible_list);

    rpl_node_unlock(node);
}

static void trickle_timer_fired(node_t *node, void *data)
{
    // todo change the hardcoded values of the DIO message
    rpl_dio_pdu_t *dio_pdu = rpl_dio_pdu_create(
            FALSE, FALSE, FALSE,
            rpl_node_get_dag_pref(node),
            rpl_node_get_seq_num(node),
            0,
            rpl_node_get_rank(node),
            rpl_node_get_dag_id(node));

    rpl_send_dio(node, NULL, dio_pdu);
}

static void remove_neighbor(node_t *node, rpl_remote_node_t *remote_node)
{
    rs_assert(node != NULL);
    rs_assert(remote_node != NULL);

    rpl_node_lock(node);

    if (rpl_node_has_neighbor(node, remote_node->node))
        rpl_node_remove_neighbor(node, remote_node);

    if (rpl_node_has_parent(node, remote_node->node))
        rpl_node_remove_parent(node, remote_node);

    if (rpl_node_has_sibling(node, remote_node->node))
        rpl_node_remove_sibling(node, remote_node);

    rpl_node_unlock(node);
}
