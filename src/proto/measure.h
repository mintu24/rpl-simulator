
#ifndef MEASURE_H_
#define MEASURE_H_

#include "../base.h"
#include "../node.h"

#define IP_NEXT_HEADER_MEASURE              0x01

#define MEASURE_TYPE_CONNECT                0

#define MEASURE_STAT_TYPE_NODE              0
#define MEASURE_STAT_TYPE_AVG               1
#define MEASURE_STAT_TYPE_TOTAL             2



    /* information that a node keeps in order to be measured */
typedef struct measure_node_info_t {

    /* connectivity */
    node_t *                connect_dst_node;
    bool                    connect_busy;
    bool                    connect_dst_reachable;
    int32                   connect_global_start_time;
    int32                   connect_update_start_time;
    int32                   connect_last_establish_time;
    sim_time_t              connect_connected_time;

    /* statistics */
    uint32                  forward_inconsistency_count;
    uint32                  forward_failure_count;
    uint32                  rpl_r_dis_message_count;
    uint32                  rpl_r_dio_message_count;
    uint32                  rpl_r_dao_message_count;
    uint32                  rpl_s_dis_message_count;
    uint32                  rpl_s_dio_message_count;
    uint32                  rpl_s_dao_message_count;
    uint32                  ping_successful_count;
    uint32                  ping_timeout_count;

} measure_node_info_t;


    /* fields contained in a measure message */
typedef struct measure_pdu_t {

    node_t *                measuring_node;
    node_t *                dst_node;
    uint8                   type;

} measure_pdu_t;


    /* convergence measurement info  */
typedef struct measure_converg_t {

    uint16                  total_node_count;
    uint16                  connected_node_count;
    uint16                  floating_node_count;
    uint16                  stable_node_count;

} measure_converg_t;


extern uint16               measure_event_node_wake;
extern uint16               measure_event_node_kill;

extern uint16               measure_event_pdu_send;
extern uint16               measure_event_pdu_receive;

extern uint16               measure_event_connect_update;
extern uint16               measure_event_connect_hop_passed;
extern uint16               measure_event_connect_hop_failed;
extern uint16               measure_event_connect_hop_timeout;
extern uint16               measure_event_connect_established;
extern uint16               measure_event_connect_lost;


bool                        measure_init();
bool                        measure_done();

measure_pdu_t *             measure_pdu_create(node_t *node, node_t *dst_node, uint8 type);
void                        measure_pdu_destroy(measure_pdu_t *pdu);
measure_pdu_t *             measure_pdu_duplicate(measure_pdu_t *pdu);

void                        measure_node_init(node_t *node);
void                        measure_node_done(node_t *node);

bool                        measure_node_send(node_t *node, node_t *dst_node, uint8 type);
bool                        measure_node_receive(node_t *node, node_t *incoming_node, measure_pdu_t *pdu);

void                        measure_node_add_forward_inconsistency(node_t *node);
void                        measure_node_add_forward_failure(node_t *node);
void                        measure_node_add_rpl_dis_message(node_t *node, bool sent);
void                        measure_node_add_rpl_dio_message(node_t *node, bool sent);
void                        measure_node_add_rpl_dao_message(node_t *node, bool sent);
void                        measure_node_add_ping(node_t *node, bool successful);
void                        measure_node_reset(node_t *node);

void                        measure_node_connect_update(node_t *node);

void                        measure_connect_update();

measure_converg_t *         measure_converg_get();
void                        measure_converg_reset();
void                        measure_converg_update();


#endif /* MEASURE_H_ */
