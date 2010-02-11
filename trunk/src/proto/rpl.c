
#include <math.h>

#include "rpl.h"
#include "../system.h"
#include "../measure.h"


    /**** global variables ****/

uint16                      rpl_event_id_after_node_wake;
uint16                      rpl_event_id_before_node_kill;
uint16                      rpl_event_id_after_dis_pdu_sent;
uint16                      rpl_event_id_after_dis_pdu_received;
uint16                      rpl_event_id_after_dio_pdu_sent;
uint16                      rpl_event_id_after_dio_pdu_received;
uint16                      rpl_event_id_after_dao_pdu_sent;
uint16                      rpl_event_id_after_dao_pdu_received;


    /**** local function prototypes ****/

static rpl_remote_node_t *  rpl_remote_node_info_create(node_t *node, rpl_dio_pdu_t *dio_message);
static void                 rpl_remote_node_info_destroy(rpl_remote_node_t *remote_node_info);


    /**** exported functions ****/

bool rpl_init()
{
    rpl_event_id_after_node_wake = event_register("rpl_after_node_wake", (event_handler_t) rpl_event_after_node_wake);
    rpl_event_id_before_node_kill = event_register("rpl_before_node_kill", (event_handler_t) rpl_event_before_node_kill);
    rpl_event_id_after_dis_pdu_sent = event_register("rpl_after_dis_pdu_sent", (event_handler_t) rpl_event_after_dis_pdu_sent);
    rpl_event_id_after_dis_pdu_received = event_register("rpl_after_dis_pdu_received", (event_handler_t) rpl_event_after_dis_pdu_received);
    rpl_event_id_after_dio_pdu_sent = event_register("rpl_after_dio_pdu_sent", (event_handler_t) rpl_event_after_dio_pdu_sent);
    rpl_event_id_after_dio_pdu_received = event_register("rpl_after_dio_pdu_received", (event_handler_t) rpl_event_after_dio_pdu_received);
    rpl_event_id_after_dao_pdu_sent = event_register("rpl_after_dao_pdu_sent", (event_handler_t) rpl_event_after_dao_pdu_sent);
    rpl_event_id_after_dao_pdu_received = event_register("rpl_after_dao_pdu_received", (event_handler_t) rpl_event_after_dao_pdu_received);

    return TRUE;
}

bool rpl_done()
{
    return TRUE;
}

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
    node->rpl_info->dag_id = strdup(node->ip_info->address);
    node->rpl_info->seq_num = 0;
    node->rpl_info->rank = RPL_RANK_INFINITY;

    node->rpl_info->pref_parent = NULL;

    node->rpl_info->parent_list = NULL;
    node->rpl_info->parent_count = 0;

    node->rpl_info->sibling_list = NULL;
    node->rpl_info->sibling_count = 0;

    node->rpl_info->neighbor_list = NULL;
    node->rpl_info->neighbor_count = 0;

    return TRUE;
}

void rpl_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->rpl_info != NULL) {
        if (node->rpl_info->neighbor_list != NULL) {
            int i;
            for (i = 0; i < node->rpl_info->neighbor_count; i++) {
                rpl_remote_node_info_destroy(node->rpl_info->neighbor_list[i]);
            }

            free(node->rpl_info->neighbor_list);
        }

        if (node->rpl_info->parent_list != NULL) {
            free(node->rpl_info->parent_list);
        }

        if (node->rpl_info->sibling_list != NULL) {
            free(node->rpl_info->sibling_list);
        }

        free(node->rpl_info);
    }
}

void rpl_node_set_dag_id(node_t *node, const char *dag_id)
{
    rs_assert(node != NULL);
    rs_assert(dag_id != NULL);

    if (node->rpl_info->dag_id!= NULL)
        free(node->rpl_info->dag_id);

    node->rpl_info->dag_id = strdup(dag_id);
}

bool rpl_node_add_parent(node_t *node, rpl_remote_node_t *parent)
{
    rs_assert(node != NULL);
    rs_assert(parent != NULL);

    node->rpl_info->parent_list = realloc(node->rpl_info->parent_list, (node->rpl_info->parent_count + 1) * sizeof(rpl_remote_node_t *));
    node->rpl_info->parent_list[node->rpl_info->parent_count++] = parent;

    return TRUE;
}

bool rpl_node_remove_parent(node_t *node, rpl_remote_node_t *parent)
{
    rs_assert(node != NULL);
    rs_assert(parent != NULL);

    int pos = -1, i;
    for (i = 0; i < node->rpl_info->parent_count; i++) {
        if (node->rpl_info->parent_list[i] == parent) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        if (parent->node != NULL)
            rs_error("node '%s' does not have node '%s' as a parent", node->phy_info->name, parent->node->phy_info->name);

        return FALSE;
    }

    for (i = pos; i < node->rpl_info->parent_count - 1; i++) {
        node->rpl_info->parent_list[i] = node->rpl_info->parent_list[i + 1];
    }

    node->rpl_info->parent_count--;
    node->rpl_info->parent_list = realloc(node->rpl_info->parent_list, node->rpl_info->parent_count * sizeof(rpl_remote_node_t *));

    return TRUE;
}

rpl_remote_node_t *rpl_node_find_parent_by_node(node_t *node, node_t *parent_node)
{
    rs_assert(node != NULL);
    rs_assert(parent_node != NULL);

    int i;
    for (i = 0; i < node->rpl_info->parent_count; i++) {
        if (node->rpl_info->parent_list[i]->node == parent_node) {
            return node->rpl_info->parent_list[i];
        }
    }

    return NULL;
}

bool rpl_node_add_sibling(node_t *node, rpl_remote_node_t *sibling)
{
    rs_assert(node != NULL);
    rs_assert(sibling != NULL);

    node->rpl_info->sibling_list = realloc(node->rpl_info->sibling_list, (node->rpl_info->sibling_count + 1) * sizeof(rpl_remote_node_t *));
    node->rpl_info->sibling_list[node->rpl_info->sibling_count++] = sibling;

    return TRUE;
}

bool rpl_node_remove_sibling(node_t *node, rpl_remote_node_t *sibling)
{
    rs_assert(node != NULL);
    rs_assert(sibling != NULL);

    int pos = -1, i;
    for (i = 0; i < node->rpl_info->sibling_count; i++) {
        if (node->rpl_info->sibling_list[i] == sibling) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        if (sibling->node != NULL)
            rs_error("node '%s' does not have node '%s' as a sibling", node->phy_info->name, sibling->node->phy_info->name);

        return FALSE;
    }

    for (i = pos; i < node->rpl_info->sibling_count - 1; i++) {
        node->rpl_info->sibling_list[i] = node->rpl_info->sibling_list[i + 1];
    }

    node->rpl_info->sibling_count--;
    node->rpl_info->sibling_list = realloc(node->rpl_info->sibling_list, node->rpl_info->sibling_count * sizeof(rpl_remote_node_t *));

    return TRUE;
}

rpl_remote_node_t *rpl_node_find_sibling_by_node(node_t *node, node_t *sibling_node)
{
    rs_assert(node != NULL);
    rs_assert(sibling_node != NULL);

    int i;
    for (i = 0; i < node->rpl_info->sibling_count; i++) {
        if (node->rpl_info->sibling_list[i]->node == sibling_node) {
            return node->rpl_info->sibling_list[i];
        }
    }

    return NULL;
}

bool rpl_node_add_neighbor(node_t *node, node_t *neighbor_node, rpl_dio_pdu_t *dio_message)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);
    rs_assert(dio_message != NULL);

    rpl_remote_node_t *remote_node_info = rpl_remote_node_info_create(neighbor_node, dio_message);

    node->rpl_info->neighbor_list = realloc(node->rpl_info->neighbor_list, (node->rpl_info->neighbor_count + 1) * sizeof(rpl_remote_node_t *));
    node->rpl_info->neighbor_list[node->rpl_info->neighbor_count++] = remote_node_info;

    return TRUE;
}

bool rpl_node_remove_neighbor(node_t *node, rpl_remote_node_t *neighbor)
{
    rs_assert(node != NULL);
    rs_assert(neighbor != NULL);

    int pos = -1, i;
    for (i = 0; i < node->rpl_info->neighbor_count; i++) {
        if (node->rpl_info->neighbor_list[i] == neighbor) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        if (neighbor->node != NULL)
            rs_error("node '%s' does not have node '%s' as a neighbor", node->phy_info->name, neighbor->node->phy_info->name);

        return FALSE;
    }

    rpl_remote_node_info_destroy(node->rpl_info->neighbor_list[pos]);

    for (i = pos; i < node->rpl_info->neighbor_count - 1; i++) {
        node->rpl_info->neighbor_list[i] = node->rpl_info->neighbor_list[i + 1];
    }

    node->rpl_info->neighbor_count--;
    node->rpl_info->neighbor_list = realloc(node->rpl_info->neighbor_list, node->rpl_info->neighbor_count * sizeof(rpl_remote_node_t *));

    return TRUE;
}

rpl_remote_node_t *rpl_node_find_neighbor_by_node(node_t *node, node_t *neighbor_node)
{
    rs_assert(node != NULL);
    rs_assert(neighbor_node != NULL);

    rpl_remote_node_t *remote_node = NULL;
    int i;
    for (i = 0; i < node->rpl_info->neighbor_count; i++) {
        if (node->rpl_info->neighbor_list[i]->node == neighbor_node) {
            remote_node = node->rpl_info->neighbor_list[i];
            break;
        }
    }

    return remote_node;
}

node_t *rpl_node_get_next_hop(node_t *node, char *dst_address)
{
    return NULL;
}

bool rpl_send_dis(node_t *node, node_t *dst_node)
{
    rs_assert(node != NULL);

    return event_execute(rpl_event_id_after_dis_pdu_sent, node, dst_node, NULL);
}

bool rpl_receive_dis(node_t *node, node_t *src_node)
{
    rs_assert(node != NULL);

    bool all_ok = event_execute(rpl_event_id_after_dis_pdu_received, node, src_node, NULL);

    return all_ok;
}

bool rpl_send_dio(node_t *node, node_t *dst_node, rpl_dio_pdu_t *pdu)
{
    rs_assert(node != NULL);

    return event_execute(rpl_event_id_after_dio_pdu_sent, node, dst_node, pdu);
}

bool rpl_receive_dio(node_t *node, node_t *src_node, rpl_dio_pdu_t *pdu)
{
    rs_assert(node != NULL);

    bool all_ok = event_execute(rpl_event_id_after_dio_pdu_received, node, src_node, pdu);

    rpl_dio_pdu_destroy(pdu);

    return all_ok;
}

bool rpl_send_dao(node_t *node, node_t *dst_node, rpl_dao_pdu_t *pdu)
{
    rs_assert(node != NULL);

    return event_execute(rpl_event_id_after_dao_pdu_sent, node, dst_node, pdu);
}

bool rpl_receive_dao(node_t *node, node_t *src_node, rpl_dao_pdu_t *pdu)
{
    rs_assert(node != NULL);

    bool all_ok = event_execute(rpl_event_id_after_dao_pdu_received, node, src_node, pdu);

    rpl_dao_pdu_destroy(pdu);

    return all_ok;
}

bool rpl_event_after_node_wake(node_t *node)
{
    return TRUE;
}

bool rpl_event_before_node_kill(node_t *node)
{
    return TRUE;
}

bool rpl_event_after_dis_pdu_sent(node_t *node, node_t *dst_node)
{
    if (!icmp_send(node, dst_node, ICMP_TYPE_RPL, ICMP_RPL_CODE_DIS, NULL)) {
        rs_error("node '%s': failed to send ICMP message", node->phy_info->name);
        return FALSE;
    }

    return TRUE;
}

bool rpl_event_after_dis_pdu_received(node_t *node, node_t *src_node)
{
    rs_system_schedule_event(node, rpl_event_id_after_dis_pdu_sent, node, NULL, 1000);

    measure_connect_update_output();
    measure_sp_comp_update_output();
    measure_converg_update_output();

    return TRUE;
}

bool rpl_event_after_dio_pdu_sent(node_t *node, node_t *dst_node, rpl_dio_pdu_t *pdu)
{
    if (!icmp_send(node, dst_node, ICMP_TYPE_RPL, ICMP_RPL_CODE_DIO, pdu)) {
        rs_error("node '%s': failed to send ICMP message", node->phy_info->name);
        return FALSE;
    }

    return TRUE;
}

bool rpl_event_after_dio_pdu_received(node_t *node, node_t *src_node, rpl_dio_pdu_t *pdu)
{
    measure_connect_update_output();
    measure_sp_comp_update_output();
    measure_converg_update_output();

    return TRUE;
}

bool rpl_event_after_dao_pdu_sent(node_t *node, node_t *dst_node, rpl_dao_pdu_t *pdu)
{
    if (!icmp_send(node, dst_node, ICMP_TYPE_RPL, ICMP_RPL_CODE_DAO, pdu)) {
        rs_error("node '%s': failed to send ICMP message", node->phy_info->name);
        return FALSE;
    }

    return TRUE;
}

bool rpl_event_after_dao_pdu_received(node_t *node, node_t *src_node, rpl_dao_pdu_t *pdu)
{
    measure_connect_update_output();
    measure_sp_comp_update_output();
    measure_converg_update_output();

    return TRUE;
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
