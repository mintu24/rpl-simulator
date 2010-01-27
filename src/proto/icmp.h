
#ifndef ICMP_H_
#define ICMP_H_

#include "../base.h"
#include "../node.h"

#define IP_NEXT_HEADER_ICMP         0x0058

#define ICMP_DEFAULT_PING_INTERVAL  1000000
#define ICMP_DEFAULT_PING_TIMEOUT   1000000

#define ICMP_TYPE_ECHO_REQUEST      0x80
#define ICMP_TYPE_ECHO_REPLY        0x81

#define icmp_node_lock(node)        g_static_rec_mutex_lock(&node->icmp_info->mutex)
#define icmp_node_unlock(node)      g_static_rec_mutex_unlock(&node->icmp_info->mutex)


    /* a set of measurement results obtained when pinging a specified node */
typedef struct node_ping_measure_t {

    node_t *                dst_node;
    uint32                  total_count;
    uint32                  failed_count;

} icmp_ping_measure_t;

    /* info that a node supporting ICMP should store */
typedef struct icmp_node_info_t {

    bool                    ping_measures_enabled;
    icmp_ping_measure_t **  ping_measure_list;
    uint32                  ping_measure_count;
    uint32                  ping_interval;
    uint32                  ping_timeout;
    uint32                  ping_current_seq;
    node_t *                ping_current_node;

    GStaticRecMutex         mutex;

} icmp_node_info_t;

    /* fields contained in a ICMP message */
typedef struct icmp_pdu_t {

    uint8                   type;
    uint8                   code;
    void *                  sdu;

} icmp_pdu_t;


icmp_pdu_t *        icmp_pdu_create();
void                icmp_pdu_destroy(icmp_pdu_t *pdu);
bool                icmp_pdu_set_sdu(icmp_pdu_t *pdu, uint8 type, uint8 code, void *sdu);

void                icmp_node_init(node_t *node);
void                icmp_node_done(node_t *node);

void                icmp_node_set_enable_ping_measurement(node_t *node, bool enabled);
bool                icmp_node_is_enabled_ping_measurement(node_t *node);

void                icmp_node_set_ping_interval(node_t *node, uint32 interval);
uint32              icmp_node_get_ping_interval(node_t *node);

void                icmp_node_set_ping_timeout(node_t *node, uint32 timeout);
uint32              icmp_node_get_ping_timeout(node_t *node);

icmp_ping_measure_t *
                    icmp_node_get_ping_measure(node_t *node, node_t *dst_node);
icmp_ping_measure_t **
                    icmp_node_get_ping_measure_list(node_t *node, uint32 *ping_measure_count);

bool                icmp_send(node_t *node, node_t *dst_node, uint8 type, uint8 code, void *sdu);
bool                icmp_receive(node_t *node, node_t *src_node, icmp_pdu_t *pdu);

    /* events */
void                icmp_event_after_node_wake(node_t *node);
void                icmp_event_before_node_kill(node_t *node);

void                icmp_event_before_pdu_sent(node_t *node, node_t *dst_node, icmp_pdu_t *pdu);
void                icmp_event_after_pdu_received(node_t *node, node_t *src_node, icmp_pdu_t *pdu);


#endif /* ICMP_H_ */
