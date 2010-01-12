
#ifndef PHY_H_
#define PHY_H_

#include "../base.h"
#include "../node.h"


typedef struct phy_node_info_t {

    percent_t battery_level;
    percent_t power_level;
    bool mains_powered;

} phy_node_info_t;

typedef struct {

    void *sdu;

} phy_pdu_t;

phy_pdu_t *             phy_pdu_create();
bool                    phy_pdu_destroy(phy_pdu_t *pdu);
bool                    phy_pdu_set_sdu(phy_pdu_t *pdu, void *sdu);

bool                    phy_init_node(node_t *node, phy_node_info_t *node_info);


#endif /* PHY_H_ */
