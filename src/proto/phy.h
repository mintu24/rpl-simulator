
#ifndef PHY_H_
#define PHY_H_

#include "../base.h"
#include "../node.h"


    /* info that a node supporting PHY layer should store */
typedef struct phy_node_info_t {

    char *              name;
    coord_t             cx;
    coord_t             cy;

    percent_t           battery_level;
    percent_t           tx_power;
    bool                mains_powered;

    node_t **           neighbor_list;
    uint16              neighbor_count;

} phy_node_info_t;


    /* fields contained in a PHY message */
typedef struct phy_pdu_t {

    void *              sdu;

} phy_pdu_t;


extern uint16           phy_event_node_wake;
extern uint16           phy_event_node_kill;

extern uint16           phy_event_pdu_send;
extern uint16           phy_event_pdu_receive;

extern uint16           phy_event_neighbor_attach;
extern uint16           phy_event_neighbor_detach;


bool                    phy_init();
bool                    phy_done();

phy_pdu_t *             phy_pdu_create();
void                    phy_pdu_destroy(phy_pdu_t *pdu);
phy_pdu_t *             phy_pdu_duplicate(phy_pdu_t *pdu);
void                    phy_pdu_set_sdu(phy_pdu_t *pdu, void *sdu);

void                    phy_node_init(node_t *node, char *name, coord_t cx, coord_t cy);
void                    phy_node_done(node_t *node);

void                    phy_node_set_name(node_t *node, const char *name);
void                    phy_node_set_coordinates(node_t* node, coord_t cx, coord_t cy);
void                    phy_node_set_tx_power(node_t* node, percent_t tw_power);

bool                    phy_send(node_t *node, node_t *outgoing_node, void *sdu);
bool                    phy_receive(node_t *node, node_t *incoming_node, phy_pdu_t *pdu);

bool                    phy_node_add_neighbor(node_t* node, node_t* neighbor_node);
bool                    phy_node_rem_neighbor(node_t* node, node_t *neighbor_node);
bool                    phy_node_has_neighbor(node_t* node, node_t *neighbor_node);

#endif /* PHY_H_ */
