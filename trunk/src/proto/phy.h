
#ifndef PHY_H_
#define PHY_H_

#include "../base.h"
#include "../node.h"


    /* info that a node supporting PHY layer should store */
typedef struct phy_node_info_t {

    char *          name;
    coord_t         cx;
    coord_t         cy;
    node_t**        neighbors_list;
    uint16          neighbors_count;

    percent_t       battery_level;
    percent_t       tx_power;
    bool            mains_powered;

} phy_node_info_t;

    /* fields contained in a PHY message */
typedef struct phy_pdu_t {

    //node_t *        src_node;   /* workaround for obtaining the sending node */
    void *          sdu;

} phy_pdu_t;


extern uint16           phy_event_id_after_node_wake;
extern uint16           phy_event_id_before_node_kill;
extern uint16           phy_event_id_after_pdu_sent;
extern uint16           phy_event_id_after_pdu_received;


bool                    phy_init();
bool                    phy_done();

phy_pdu_t *             phy_pdu_create();
bool                    phy_pdu_destroy(phy_pdu_t *pdu);
phy_pdu_t *             phy_pdu_duplicate(phy_pdu_t *pdu);
bool                    phy_pdu_set_sdu(phy_pdu_t *pdu, void *sdu);

bool                    phy_node_init(node_t *node, char *name, coord_t cx, coord_t cy);
void                    phy_node_done(node_t *node);

void                    phy_node_set_name(node_t *node, const char *name);
bool                    phy_update_coordinate(node_t* node, coord_t cx, coord_t cy);
bool                    phy_update_tx_power(node_t* node, percent_t tw_power);


bool                    phy_send(node_t *node, node_t *dst_node, void *sdu);
bool                    phy_receive(node_t *node, node_t *src_node, phy_pdu_t *pdu);

    /* events */
bool                    phy_event_after_node_wake(node_t *node);
bool                    phy_event_before_node_kill(node_t *node);

bool                    phy_event_after_pdu_sent(node_t *node, node_t *dst_node, phy_pdu_t *pdu);
bool                    phy_event_after_pdu_received(node_t *node, node_t *src_node, phy_pdu_t *pdu);


  /* neighboor management */
node_t*                 phy_find_neighbour(node_t* node, char* name);
bool                    phy_add_neighbour_node(node_t* node, node_t* neighbour);
bool                    phy_del_neighbour_node(node_t* node, char* name);
node_t**                phy_get_neighbors_list_copy(node_t* node, uint16* count);
void                    phy_debug_print_neighbors_list(node_t* node);


#endif /* PHY_H_ */
