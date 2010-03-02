
#ifndef RPL_H_
#define RPL_H_

#include "../base.h"
#include "../node.h"
#include "ip.h"

#define ICMP_TYPE_RPL                           0x9B

#define ICMP_RPL_CODE_DIS                       0x01
#define ICMP_RPL_CODE_DIO                       0x02
#define ICMP_RPL_CODE_DAO                       0x04

#define RPL_DIO_SUBOPTION_TYPE_DAG_CONFIG       0x04

#define RPL_DEFAULT_DAG_PREF                    0 /* least preferred */

#define RPL_DEFAULT_NODE_STORING                TRUE

#define RPL_RANK_ROOT                           1
#define RPL_RANK_INFINITY                       0xFF

#define RPL_MINIMUM_RANK_INCREMENT              1
#define RPL_MAXIMUM_RANK_INCREMENT              16


#define rpl_node_has_parent(node, parent)       (rpl_node_find_parent_by_node(node, parent) != NULL)
#define rpl_node_has_sibling(node, sibling)     (rpl_node_find_sibling_by_node(node, sibling) != NULL)
#define rpl_node_has_neighbor(node, neighbor)   (rpl_node_find_neighbor_by_node(node, neighbor) != NULL)

#define rpl_node_is_pref_parent(node, neighbor) ((node)->rpl_info->joined_dodag != NULL && (node)->rpl_info->joined_dodag->pref_parent == neighbor)

#define rpl_node_is_isolated(node)              ((node)->rpl_info->joined_dodag == NULL && (node)->rpl_info->root_info->dodag_id == NULL)
#define rpl_node_is_root(node)                  ((node)->rpl_info->joined_dodag == NULL && (node)->rpl_info->root_info->dodag_id != NULL)
#define rpl_node_is_joined(node)                ((node)->rpl_info->joined_dodag != NULL && (node)->rpl_info->joined_dodag->rank < RPL_RANK_INFINITY)
#define rpl_node_is_poisoning(node)             ((node)->rpl_info->joined_dodag != NULL && (node)->rpl_info->joined_dodag->rank == RPL_RANK_INFINITY)


    /* data structure that holds remote RPL node information, for avoiding a node_t * reference */
typedef struct rpl_neighbor_t {

    node_t *                node;
    bool                    is_dao_parent;

    struct rpl_dio_pdu_t *  last_dio_message;

} rpl_neighbor_t;

typedef struct rpl_root_info_t {

    char *                  dodag_id;
    uint8                   dodag_pref;
    bool                    grounded;
    bool                    dao_supported;
    bool                    dao_trigger;

    uint8                   dio_interval_doublings;
    uint8                   dio_interval_min;
    uint8                   dio_redundancy_constant;

    uint8                   max_rank_inc;
    uint8                   min_hop_rank_inc;

} rpl_root_info_t;

typedef struct rpl_dodag_t {

    char *                  dodag_id;
    uint8                   dodag_pref;
    bool                    grounded;
    bool                    dao_supported;
    bool                    dao_trigger;

    uint8                   dio_interval_doublings;
    uint8                   dio_interval_min;
    uint8                   dio_redundancy_constant;

    uint8                   max_rank_inc;
    uint8                   min_hop_rank_inc;

    uint8                   seq_num;
    uint16                  lowest_rank;
    uint16                  rank;

    rpl_neighbor_t**        parent_list;
    uint16                  parent_count;
    rpl_neighbor_t**        sibling_list;
    uint16                  sibling_count;
    rpl_neighbor_t*         pref_parent;

} rpl_dodag_t;

    /* info that a node supporting RPL should store */
typedef struct rpl_node_info_t {

    rpl_root_info_t *       root_info;
    rpl_dodag_t*            joined_dodag;

    bool                    storing;

    uint8                   trickle_i_doublings_so_far;
    sim_time_t              trickle_i;
    uint8                   trickle_c;
    sim_time_t              last_trickle_i_schedule_time; /* scheduler performance boost */
    sim_time_t              last_trickle_t_schedule_time;

    rpl_neighbor_t**        neighbor_list;
    uint16                  neighbor_count;

    uint8                   poison_count_so_far;

    sim_time_t              last_dio_send_time; /* used for detecting collisions by simulating a risk window */

} rpl_node_info_t;


    /* fields of a DAG configuration suboption in a RPL DIO message */
typedef struct rpl_dio_suboption_dodag_config_t {

    uint8                   dio_interval_doublings;
    uint8                   dio_interval_min;
    uint8                   dio_redundancy_constant;

    uint8                   max_rank_inc;
    uint8                   min_hop_rank_inc;

} rpl_dio_suboption_dodag_config_t;

    /* fields contained in a RPL DIO message */
typedef struct rpl_dio_pdu_t {

    char *                  dodag_id;
    uint8                   dodag_pref;
    uint8                   seq_num;

    uint16                  rank;
    uint8                   dstn;
    bool                    dao_stored;

    bool                    grounded;
    bool                    dao_supported;
    bool                    dao_trigger;

    rpl_dio_suboption_dodag_config_t *
                            dodag_config_suboption;

} rpl_dio_pdu_t;

    /* fields contained in a RPL DAO message */
typedef struct rpl_dao_pdu_t {

    uint16                  seq_num;
    uint16                  rank;

    char *                  dest;
    uint8                   prefix_len;
    uint32                  life_time;
    char **                 rr_stack;
    uint16                  rr_count;

} rpl_dao_pdu_t;


extern uint16               rpl_event_node_wake;
extern uint16               rpl_event_node_kill;

extern uint16               rpl_event_dis_pdu_send;
extern uint16               rpl_event_dis_pdu_receive;
extern uint16               rpl_event_dio_pdu_send;
extern uint16               rpl_event_dio_pdu_receive;
extern uint16               rpl_event_dao_pdu_send;
extern uint16               rpl_event_dao_pdu_receive;

extern uint16               rpl_event_neighbor_attach;
extern uint16               rpl_event_neighbor_detach;

extern uint16               rpl_event_forward_failure;
extern uint16               rpl_event_forward_inconsistency;

extern uint16               rpl_event_trickle_t_timeout;
extern uint16               rpl_event_trickle_i_timeout;
extern uint16               rpl_event_seq_num_autoinc;


bool                        rpl_init();
bool                        rpl_done();

rpl_neighbor_t *            rpl_neighbor_create(node_t *node);
void                        rpl_neighbor_destroy(rpl_neighbor_t* neighbor);

rpl_root_info_t *           rpl_root_info_create();
void                        rpl_root_info_destroy(rpl_root_info_t *root_info);

rpl_dodag_t *               rpl_dodag_create(rpl_dio_pdu_t *dio_pdu);
void                        rpl_dodag_destroy(rpl_dodag_t *dodag);

rpl_dio_pdu_t *             rpl_dio_pdu_create();
void                        rpl_dio_pdu_destroy(rpl_dio_pdu_t *pdu);
rpl_dio_pdu_t *             rpl_dio_pdu_duplicate(rpl_dio_pdu_t *pdu);

rpl_dio_suboption_dodag_config_t *
                            rpl_dio_suboption_dodag_config_create();
void                        rpl_dio_suboption_dodag_config_destroy(rpl_dio_suboption_dodag_config_t *suboption);

rpl_dao_pdu_t *             rpl_dao_pdu_create();
void                        rpl_dao_pdu_destroy(rpl_dao_pdu_t *pdu);
rpl_dao_pdu_t *             rpl_dao_pdu_duplicate(rpl_dao_pdu_t *pdu);
void                        rpl_dao_pdu_add_rr(rpl_dao_pdu_t *pdu, char *ip_address);

uint8                       rpl_seq_num_get(char *dodag_id);

void                        rpl_node_init(node_t *node);
void                        rpl_node_done(node_t *node);

void                        rpl_node_add_neighbor(node_t *node, node_t *neighbor_node);
bool                        rpl_node_remove_neighbor(node_t *node, rpl_neighbor_t *neighbor);
void                        rpl_node_remove_all_neighbors(node_t *node);
rpl_neighbor_t *            rpl_node_find_neighbor_by_node(node_t *node, node_t *neighbor_node);

void                        rpl_node_add_parent(node_t *node, rpl_neighbor_t *parent);
bool                        rpl_node_remove_parent(node_t *node, rpl_neighbor_t *parent);
void                        rpl_node_remove_all_parents(node_t *node);
rpl_neighbor_t *            rpl_node_find_parent_by_node(node_t *node, node_t *parent_node);
bool                        rpl_node_neighbor_is_parent(node_t *node, rpl_neighbor_t *neighbor);

void                        rpl_node_add_sibling(node_t *node, rpl_neighbor_t *sibling);
bool                        rpl_node_remove_sibling(node_t *node, rpl_neighbor_t *sibling);
void                        rpl_node_remove_all_siblings(node_t *node);
rpl_neighbor_t *            rpl_node_find_sibling_by_node(node_t *node, node_t *sibling_node);
bool                        rpl_node_neighbor_is_sibling(node_t *node, rpl_neighbor_t *neighbor);

void                        rpl_node_start_as_root(node_t *node);
void                        rpl_node_isolate(node_t *node);
void                        rpl_node_reset_trickle_timer(node_t *node);

node_t **                   rpl_node_get_next_hop_list(node_t *node, uint16 *node_count);
bool                        rpl_node_process_incoming_flow_label(node_t *node, node_t *incoming_node, ip_pdu_t *ip_pdu);
node_t *                    rpl_node_process_outgoing_flow_label(node_t *node, node_t *incoming_node, node_t *proposed_dst_node, ip_pdu_t *ip_pdu);

bool                        rpl_node_send_dis(node_t *node, char *dst_ip_address);
bool                        rpl_node_receive_dis(node_t *node, node_t *incoming_node);
bool                        rpl_node_send_dio(node_t *node, char *dst_ip_address, rpl_dio_pdu_t *pdu);
bool                        rpl_node_receive_dio(node_t *node, node_t *incoming_node, rpl_dio_pdu_t *pdu);
bool                        rpl_node_send_dao(node_t *node, char *dst_ip_address, rpl_dao_pdu_t *pdu);
bool                        rpl_node_receive_dao(node_t *node, node_t *incoming_node, rpl_dao_pdu_t *pdu);


#endif /* RPL_H_ */
