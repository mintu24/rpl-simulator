
#ifndef PHY_H_
#define PHY_H_

#include "../base.h"
#include "../node.h"


    /* info that a node supporting PHY layer should store */
typedef struct phy_node_info_t {

    char *          name;
    coord_t         cx;
    coord_t         cy;

    percent_t       battery_level;
    percent_t       tx_power;
    bool            mains_powered;

} phy_node_info_t;

    /* fields contained in a PHY message */
typedef struct phy_pdu_t {

    node_t *        src_node;   /* workaround for obtaining the sending node */
    void *          sdu;

} phy_pdu_t;


phy_pdu_t *             phy_pdu_create();
bool                    phy_pdu_destroy(phy_pdu_t *pdu);
bool                    phy_pdu_set_sdu(phy_pdu_t *pdu, node_t *src_node, void *sdu);

bool                    phy_node_init(node_t *node, char *name, coord_t cx, coord_t cy);
void                    phy_node_done(node_t *node);

char *                  phy_node_get_name(node_t *node);
void                    phy_node_set_name(node_t *node, const char *name);

coord_t                 phy_node_get_x(node_t *node);
coord_t                 phy_node_get_y(node_t *node);
void                    phy_node_set_xy(node_t *node, coord_t x, coord_t y);

percent_t               phy_node_get_battery_level(node_t *node);
void                    phy_node_set_battery_level(node_t *node, percent_t level);

percent_t               phy_node_get_tx_power(node_t *node);
void                    phy_node_set_tx_power(node_t *node, percent_t tx_power);

bool                    phy_node_is_mains_powered(node_t *node);
void                    phy_node_set_mains_powered(node_t *node, bool value);

bool                    phy_send(node_t *node, node_t *dst_node, void *sdu);
bool                    phy_receive(node_t *node, node_t *src_node, phy_pdu_t *pdu);


#endif /* PHY_H_ */
