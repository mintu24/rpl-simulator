
#ifndef RPL_H_
#define RPL_H_

#include "../base.h"
#include "../node.h"

#define ICMP_TYPE_RPL                   0x9B

#define ICMP_CODE_DIS                   0x01
#define ICMP_CODE_DIO                   0x02
#define ICMP_CODE_DAO                   0x04

#define DIO_SUBOPTION_TYPE_DAG_CONFIG   0x04


typedef struct rpl_node_info_t {

    node_t *parent_list;
    uint16 parent_count;

    node_t *sibling_list;
    uint16 sibling_count;

} rpl_node_info_t;

typedef struct {

    int8 interval_doublings;
    int8 interval_min;
    int8 redundancy_constant;
    int8 max_rank_increase;

} dio_suboption_dag_config_t;


typedef struct dio_suboption_t {

    int8 type;
    void *content;

    struct dio_suboption_t *next_suboption;

} dio_suboption_t;

typedef struct {

    bool grounded;
    bool da_trigger;    // not used for now
    bool da_support;
    uint8 dag_pref;
    uint8 seq_number;
    uint8 instance_id;
    uint8 rank;
    char *dag_id;

    dio_suboption_t *suboptions;

} dio_pdu_t;

typedef struct {

    uint16 sequence;
    uint8 instance_id;
    uint8 rank;
    uint32 dao_lifetime;
    char *prefix;
    uint8 prefix_len;
    char **rr_stack;
    int rr_count;

} dao_pdu_t;

dio_pdu_t *         dio_pdu_create(bool grounded, bool da_trigger, bool da_support, int8 dag_pref, int8 seq_number, int8 instance_id, int8 rank, char *dag_id);
bool                dio_pdu_destroy(dio_pdu_t *pdu);

dio_suboption_t *   dio_suboption_dag_config_create(int8 interval_doublings, int8 interval_min, int8 redundancy_constant, int8 max_rank_increase);
bool                dio_suboption_destroy(dio_suboption_t *suboption);
bool                dio_pdu_add_suboption(dio_pdu_t *pdu, dio_suboption_t *suboption);

dao_pdu_t *         dao_pdu_create(uint16 sequence, uint8 instance_id, uint8 rank, uint32 dao_lifetime, char *prefix, uint8 prefix_len);
bool                dao_pdu_destroy(dao_pdu_t *pdu);
bool                dao_pdu_add_rr(dao_pdu_t *pdu, char *ip_address);

bool                rpl_init_node(node_t *node, rpl_node_info_t *node_info);


#endif /* RPL_H_ */
