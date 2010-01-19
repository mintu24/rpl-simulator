
#ifndef RPL_H_
#define RPL_H_

#include "../base.h"
#include "../node.h"

#define ICMP_TYPE_RPL                   0x9B

#define ICMP_CODE_DIS                   0x01
#define ICMP_CODE_DIO                   0x02
#define ICMP_CODE_DAO                   0x04

#define DIO_SUBOPTION_TYPE_DAG_CONFIG   0x04


    /* info that a node supporting RPL should store */
typedef struct rpl_node_info_t {

    node_t **parent_list;
    uint16 parent_count;

    node_t **sibling_list;
    uint16 sibling_count;

} rpl_node_info_t;

    /* fields of a DAG configuration suboption in a RPL DIO message */
typedef struct {

    int8 interval_doublings;
    int8 interval_min;
    int8 redundancy_constant;
    int8 max_rank_increase;

} dio_suboption_dag_config_t;

    /* a generic structure holding a RPL DIO message suboption */
typedef struct dio_suboption_t {

    int8 type;
    void *content;

    // todo make this list a simple array-based list instead of a linked list
    struct dio_suboption_t *next_suboption;

} dio_suboption_t;

    /* fields contained in a RPL DIO message */
typedef struct {

    bool grounded;
    bool da_trigger;
    bool da_support;
    uint8 dag_pref;
    uint8 seq_number;
    uint8 instance_id;
    uint8 rank;
    char *dag_id;

    dio_suboption_t *suboptions;

} rpl_dio_pdu_t;

    /* fields contained in a RPL DAO message */
typedef struct {

    uint16 sequence;
    uint8 instance_id;
    uint8 rank;
    uint32 dao_lifetime;
    char *prefix;
    uint8 prefix_len;
    char **rr_stack;
    int rr_count;

} rpl_dao_pdu_t;


rpl_dio_pdu_t *     dio_pdu_create(bool grounded, bool da_trigger, bool da_support, int8 dag_pref, int8 seq_number, int8 instance_id, int8 rank, char *dag_id);
bool                dio_pdu_destroy(rpl_dio_pdu_t *pdu);

dio_suboption_t *   dio_suboption_dag_config_create(int8 interval_doublings, int8 interval_min, int8 redundancy_constant, int8 max_rank_increase);
bool                dio_suboption_destroy(dio_suboption_t *suboption);
bool                dio_pdu_add_suboption(rpl_dio_pdu_t *pdu, dio_suboption_t *suboption);

rpl_dao_pdu_t *     dao_pdu_create(uint16 sequence, uint8 instance_id, uint8 rank, uint32 dao_lifetime, char *prefix, uint8 prefix_len);
bool                dao_pdu_destroy(rpl_dao_pdu_t *pdu);
bool                dao_pdu_add_rr(rpl_dao_pdu_t *pdu, char *ip_address);

rpl_node_info_t *   rpl_node_info_create();
bool                rpl_node_info_destroy(rpl_node_info_t *node_info);

bool                rpl_init_node(node_t *node, rpl_node_info_t *node_info);

node_t **           rpl_node_get_parent_list(node_t *node, uint16 *parent_count);
bool                rpl_node_add_parent(node_t *node, node_t *parent);
bool                rpl_node_remove_parent(node_t *node, node_t *parent);

node_t **           rpl_node_get_sibling_list(node_t *node, uint16 *sibling_count);
bool                rpl_node_add_sibling(node_t *node, node_t *sibling);
bool                rpl_node_remove_sibling(node_t *node, node_t *sibling);

    /* RPL events */
void                rpl_event_before_dis_pdu_sent(node_t *node, void *data);
void                rpl_event_after_dis_pdu_received(node_t *node, void *data);

void                rpl_event_before_dio_pdu_sent(node_t *node, rpl_dio_pdu_t *pdu);
void                rpl_event_after_dio_pdu_received(node_t *node, rpl_dio_pdu_t *pdu);

void                rpl_event_before_dao_pdu_sent(node_t *node, rpl_dao_pdu_t *pdu);
void                rpl_event_after_dao_pdu_received(node_t *node, rpl_dao_pdu_t *pdu);


#endif /* RPL_H_ */
