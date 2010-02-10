
#ifndef RPL_H_
#define RPL_H_

#include "../base.h"
#include "../node.h"

#define ICMP_TYPE_RPL                           0x9B

#define ICMP_RPL_CODE_DIS                       0x01
#define ICMP_RPL_CODE_DIO                       0x02
#define ICMP_RPL_CODE_DAO                       0x04

#define RPL_DIO_SUBOPTION_TYPE_DAG_CONFIG       0x04

#define RPL_DEFAULT_DAG_PREF                    0
#define RPL_RANK_ROOT                           1
#define RPL_RANK_INFINITY                       255
#define RPL_MINIMUM_RANK_INCREMENT              1
#define RPL_MAXIMUM_RANK_INCREMENT              16

#define rpl_node_is_root(node)                  ((node)->rpl_info->rank == RPL_RANK_ROOT)

#define rpl_node_has_parent(node, parent)       (rpl_node_find_parent_by_node(node, parent) != NULL)
#define rpl_node_has_sibling(node, sibling)     (rpl_node_find_sibling_by_node(node, sibling) != NULL)
#define rpl_node_has_neighbor(node, neighbor)   (rpl_node_find_neighbor_by_node(node, neighbor) != NULL)


    /* data structure that holds remote RPL node information, for avoiding a node_t * reference */
typedef struct rpl_remote_node_t {

    node_t *            node;   /* identifies the interface */

    struct rpl_dio_pdu_t *
                        last_dio_message;

} rpl_remote_node_t;

    /* info that a node supporting RPL should store */
typedef struct rpl_node_info_t {

    uint8               dag_pref;
    char *              dag_id;
    uint8               seq_num;
    uint8               rank;

    rpl_remote_node_t * pref_parent;
    rpl_remote_node_t **parent_list;
    uint16              parent_count;

    rpl_remote_node_t **sibling_list;
    uint16              sibling_count;

    rpl_remote_node_t **neighbor_list;
    uint16              neighbor_count;

} rpl_node_info_t;


    /* fields of a DAG configuration suboption in a RPL DIO message */
typedef struct rpl_dio_suboption_dag_config_t {

    int8                interval_doublings;
    int8                interval_min;
    int8                redundancy_constant;
    int8                max_rank_increase;

} rpl_dio_suboption_dag_config_t;

    /* a generic structure holding a RPL DIO message suboption */
typedef struct rpl_dio_suboption_t {

    int8                type;
    void *              content;

    struct rpl_dio_suboption_t *
                        next_suboption;

} rpl_dio_suboption_t;

    /* fields contained in a RPL DIO message */
typedef struct rpl_dio_pdu_t {

    bool                grounded;
    bool                da_trigger;
    bool                da_support;
    uint8               dag_pref;
    uint8               seq_number;
    uint8               instance_id;
    uint8               rank;
    char *              dag_id;

    rpl_dio_suboption_t *
                        suboptions;

} rpl_dio_pdu_t;

    /* fields contained in a RPL DAO message */
typedef struct rpl_dao_pdu_t {

    uint16              sequence;
    uint8               instance_id;
    uint8               rank;
    uint32              dao_lifetime;
    char *              prefix;
    uint8               prefix_len;
    char **             rr_stack;
    int                 rr_count;

} rpl_dao_pdu_t;


extern uint16       rpl_event_id_after_node_wake;
extern uint16       rpl_event_id_before_node_kill;
extern uint16       rpl_event_id_after_dis_pdu_sent;
extern uint16       rpl_event_id_after_dis_pdu_received;
extern uint16       rpl_event_id_after_dio_pdu_sent;
extern uint16       rpl_event_id_after_dio_pdu_received;
extern uint16       rpl_event_id_after_dao_pdu_sent;
extern uint16       rpl_event_id_after_dao_pdu_received;


bool                rpl_init();
bool                rpl_done();

rpl_dio_pdu_t *     rpl_dio_pdu_create(bool grounded, bool da_trigger, bool da_support, uint8 dag_pref, uint8 seq_number, uint8 instance_id, uint8 rank, char *dag_id);
void                rpl_dio_pdu_destroy(rpl_dio_pdu_t *pdu);
rpl_dio_pdu_t *     rpl_dio_pdu_duplicate(rpl_dio_pdu_t *pdu);

rpl_dio_suboption_t *
                    rpl_dio_suboption_dag_config_create(int8 interval_doublings, int8 interval_min, int8 redundancy_constant, int8 max_rank_increase);
bool                rpl_dio_suboption_destroy(rpl_dio_suboption_t *suboption);
bool                rpl_dio_pdu_add_suboption(rpl_dio_pdu_t *pdu, rpl_dio_suboption_t *suboption);

rpl_dao_pdu_t *     rpl_dao_pdu_create(uint16 sequence, uint8 instance_id, uint8 rank, uint32 dao_lifetime, char *prefix, uint8 prefix_len);
void                rpl_dao_pdu_destroy(rpl_dao_pdu_t *pdu);
rpl_dao_pdu_t *     rpl_dao_pdu_duplicate(rpl_dao_pdu_t *pdu);
bool                rpl_dao_pdu_add_rr(rpl_dao_pdu_t *pdu, char *ip_address);

bool                rpl_node_init(node_t *node);
void                rpl_node_done(node_t *node);

void                rpl_node_set_dag_id(node_t *node, const char *dag_id);

bool                rpl_node_add_parent(node_t *node, rpl_remote_node_t *parent);
bool                rpl_node_remove_parent(node_t *node, rpl_remote_node_t *parent);
rpl_remote_node_t * rpl_node_find_parent_by_node(node_t *node, node_t *parent_node);

bool                rpl_node_add_sibling(node_t *node, rpl_remote_node_t *sibling);
bool                rpl_node_remove_sibling(node_t *node, rpl_remote_node_t *sibling);
rpl_remote_node_t * rpl_node_find_sibling_by_node(node_t *node, node_t *sibling_node);

bool                rpl_node_add_neighbor(node_t *node, node_t *neighbor_node, rpl_dio_pdu_t *dio_message);
bool                rpl_node_remove_neighbor(node_t *node, rpl_remote_node_t *neighbor);
rpl_remote_node_t * rpl_node_find_neighbor_by_node(node_t *node, node_t *neighbor_node);

bool                rpl_send_dis(node_t *node, node_t *dst_node);
bool                rpl_receive_dis(node_t *node, node_t *src_node);
bool                rpl_send_dio(node_t *node, node_t *dst_node, rpl_dio_pdu_t *pdu);
bool                rpl_receive_dio(node_t *node, node_t *src_node, rpl_dio_pdu_t *pdu);
bool                rpl_send_dao(node_t *node, node_t *dst_node, rpl_dao_pdu_t *pdu);
bool                rpl_receive_dao(node_t *node, node_t *src_node, rpl_dao_pdu_t *pdu);


    /* events */
bool                rpl_event_after_node_wake(node_t *node);
bool                rpl_event_before_node_kill(node_t *node);

bool                rpl_event_after_dis_pdu_sent(node_t *node, node_t *dst_node);
bool                rpl_event_after_dis_pdu_received(node_t *node, node_t *src_node);

bool                rpl_event_after_dio_pdu_sent(node_t *node, node_t *dst_node, rpl_dio_pdu_t *pdu);
bool                rpl_event_after_dio_pdu_received(node_t *node, node_t *src_node, rpl_dio_pdu_t *pdu);

bool                rpl_event_after_dao_pdu_sent(node_t *node, node_t *dst_node, rpl_dao_pdu_t *pdu);
bool                rpl_event_after_dao_pdu_received(node_t *node, node_t *src_node, rpl_dao_pdu_t *pdu);


#endif /* RPL_H_ */
