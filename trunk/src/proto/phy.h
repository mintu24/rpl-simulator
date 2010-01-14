
#ifndef PHY_H_
#define PHY_H_

#include "../base.h"
#include "../node.h"


    /* info that a node supporting PHY layer should store */
typedef struct phy_node_info_t {

    char *name;
    coord_t cx;
    coord_t cy;

    percent_t battery_level;
    percent_t power_level;
    bool mains_powered;

} phy_node_info_t;

    /* fields contained in a PHY message */
typedef struct {

    void *sdu;

} phy_pdu_t;


phy_pdu_t *             phy_pdu_create();
bool                    phy_pdu_destroy(phy_pdu_t *pdu);
bool                    phy_pdu_set_sdu(phy_pdu_t *pdu, void *sdu);

phy_node_info_t *       phy_node_info_create(char *name, coord_t cx, coord_t cy);
bool                    phy_node_info_destroy(phy_node_info_t *node_info);

bool                    phy_init_node(node_t *node, phy_node_info_t *node_info);

    /* PHY events */
void                    phy_event_before_pdu_sent(node_t *node, phy_pdu_t *pdu);
void                    phy_event_after_pdu_received(node_t *node, phy_pdu_t *pdu);


#endif /* PHY_H_ */
