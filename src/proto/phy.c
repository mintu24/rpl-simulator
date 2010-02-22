
#include "phy.h"
#include "mac.h"
#include "../system.h"


    /**** global variables ****/

uint16               phy_event_id_after_node_wake;
uint16               phy_event_id_before_node_kill;
uint16               phy_event_id_after_pdu_sent;
uint16               phy_event_id_after_pdu_received;


    /**** local function prototypes ****/


    /**** exported functions ****/

bool phy_init()
{
    phy_event_id_after_node_wake = event_register("after_node_wake", "phy", (event_handler_t) phy_event_after_node_wake, NULL);
    phy_event_id_before_node_kill = event_register("before_node_kill", "phy", (event_handler_t) phy_event_before_node_kill, NULL);
    phy_event_id_after_pdu_sent = event_register("after_pdu_sent", "phy", (event_handler_t) phy_event_after_pdu_sent, NULL);
    phy_event_id_after_pdu_received = event_register("after_pdu_received", "phy", (event_handler_t) phy_event_after_pdu_received, NULL);
    
    return TRUE;
}

bool phy_done()
{
    return TRUE;
}

phy_pdu_t *phy_pdu_create()
{
    phy_pdu_t *pdu = malloc(sizeof(phy_pdu_t));

    pdu->sdu = NULL;

    return pdu;
}

bool phy_pdu_destroy(phy_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    free(pdu);

    return TRUE;
}

phy_pdu_t *phy_pdu_duplicate(phy_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    phy_pdu_t *new_pdu = malloc(sizeof(phy_pdu_t));

    new_pdu->sdu = mac_pdu_duplicate(pdu->sdu);

    return new_pdu;
}

bool phy_pdu_set_sdu(phy_pdu_t *pdu, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->sdu = sdu;

    return TRUE;
}

bool phy_node_init(node_t *node, char *name, coord_t cx, coord_t cy)
{
    rs_assert(node != NULL);
    rs_assert(name != NULL);

    node->phy_info = malloc(sizeof(phy_node_info_t));

    node->phy_info->name = strdup(name);
    node->phy_info->cx = cx;
    node->phy_info->cy = cy;

    node->phy_info->mains_powered = FALSE;
    node->phy_info->tx_power = 0.5;
    node->phy_info->battery_level = 0.5;

    node->phy_info->neighbors_list = NULL;
    node->phy_info->neighbors_count = 0;

    return TRUE;
}

void phy_node_done(node_t *node)
{
    rs_assert(node != NULL);

    if (node->phy_info != NULL) {
        if (node->phy_info->name != NULL)
            free(node->phy_info->name);

        free(node->phy_info);
        node->phy_info = NULL;
    }
}

void phy_node_set_name(node_t *node, const char *name)
{
    rs_assert(node != NULL);
    rs_assert(name != NULL);

    if (node->phy_info->name != NULL)
        free(node->phy_info->name);

    node->phy_info->name = strdup(name);
}

bool phy_send(node_t *node, node_t *dst_node, void *sdu)
{
    rs_assert(node != NULL);
    rs_assert(sdu != NULL);

    phy_pdu_t *phy_pdu = phy_pdu_create();
    phy_pdu_set_sdu(phy_pdu, sdu);

    if (!event_execute(phy_event_id_after_pdu_sent, node, dst_node, phy_pdu)) {
        phy_pdu_destroy(phy_pdu);
        return FALSE;
    }

    return TRUE;
}

bool phy_receive(node_t *node, node_t *src_node, phy_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    bool all_ok = event_execute(phy_event_id_after_pdu_received, node, src_node, pdu);

    phy_pdu_destroy(pdu);

    return all_ok;
}


bool phy_event_after_node_wake(node_t *node)
{
    return TRUE;
}

bool phy_event_before_node_kill(node_t *node)
{
    rs_assert(node != NULL);

    /* removing all the phy neighboring reference to this node */
    uint16 i;
    for(i = 0; i < node->phy_info->neighbors_count; i++){
        node_t* other = node->phy_info->neighbors_list[i];
        phy_del_neighbour_node(other, node->phy_info->name);
        phy_del_neighbour_node(node, other->phy_info->name);
    }
    return TRUE;
}

bool phy_event_after_pdu_sent(node_t *node, node_t *dst_node, phy_pdu_t *pdu)
{
    return rs_system_send(node, dst_node, pdu);
}

bool phy_event_after_pdu_received(node_t *node, node_t *src_node, phy_pdu_t *pdu)
{
    mac_pdu_t *mac_pdu = pdu->sdu;

    bool all_ok = TRUE;

    if (!mac_receive(node, src_node, mac_pdu)) {
        all_ok = FALSE;
    }

    return all_ok;
}



bool phy_update_coordinate(node_t* node, coord_t cx, coord_t cy)
{
  rs_assert(node != NULL);

  nodes_lock();
  
  node->phy_info->cx = cx;
  node->phy_info->cy = cy;

  nodes_unlock();
  
  rs_system_update_neighbors_list(node);

  return TRUE;
}

bool phy_update_tx_power(node_t* node, percent_t tx_power)
{
  rs_assert(node != NULL);

  nodes_lock();
  
  node->phy_info->tx_power = tx_power;

  nodes_unlock();
  
  rs_system_update_neighbors_list(node);

  return TRUE;

}



node_t* phy_find_neighbour(node_t* node, char* name)
{
 
    rs_assert(node != NULL);
    rs_assert(name != NULL);

    int i;
    node_t *ngb = NULL;
    for (i = 0; i < node->phy_info->neighbors_count; i++) {
        if (!strcmp(node->phy_info->neighbors_list[i]->phy_info->name, name)) {
            ngb = node->phy_info->neighbors_list[i];
            break;
        }
    }

    return ngb;
  
}

bool phy_add_neighbour_node(node_t* node, node_t* neighbour)
{
  
  rs_assert(node != NULL);
  rs_assert(node->phy_info != NULL);
  rs_assert(neighbour != NULL);

  if( phy_find_neighbour(node,neighbour->phy_info->name) != NULL) return FALSE;
  node->phy_info->neighbors_list = realloc(node->phy_info->neighbors_list, (++node->phy_info->neighbors_count) * sizeof(node_t*));
  node->phy_info->neighbors_list[node->phy_info->neighbors_count -1] = neighbour; 

  return TRUE;
}

bool phy_del_neighbour_node(node_t* node, char* name)
{

    rs_assert(node != NULL);
    rs_assert(name != NULL);

    int i, pos = -1;
    for (i = 0; i < node->phy_info->neighbors_count; i++) {
        if (!strcmp(node->phy_info->neighbors_list[i]->phy_info->name, name)) {
            pos = i;
            break;
        }
    }

    if (pos == -1) {
        rs_error("node '%s' not found in %s's neighbors list", name, node->phy_info->name);
        nodes_unlock();

        return FALSE;
    }

    for (i = pos; i < node->phy_info->neighbors_count - 1; i++) {
        node->phy_info->neighbors_list[i] = node->phy_info->neighbors_list[i + 1];
    }

    node->phy_info->neighbors_count--;
    node->phy_info->neighbors_list = realloc(node->phy_info->neighbors_list, (node->phy_info->neighbors_count) * sizeof(node_t *));
    if (node->phy_info->neighbors_count == 0) {
        node->phy_info->neighbors_list = NULL;
    }

    return TRUE;
}


node_t** phy_get_neighbors_list_copy(node_t* node, uint16 *count)
{
  rs_assert(node != NULL);
  rs_assert(count != NULL);

  *count = node->phy_info->neighbors_count;
  node_t **copy = malloc(sizeof(node_t*) * node->phy_info->neighbors_count);
  uint16 i;
  for(i = 0; i < node->phy_info->neighbors_count; i++){
    copy[i] = node->phy_info->neighbors_list[i];
  }
 
  return copy;

}

void phy_debug_print_neighbors_list(node_t* node)
{
  rs_assert(node != NULL);

  char  debug[2000];
  uint16  i;

  sprintf(debug, "node %s's neighbors list : ",node->phy_info->name);
  for(i = 0; i < node->phy_info->neighbors_count; i++){
    char nghb[100];
    sprintf(nghb, " %s ", node->phy_info->neighbors_list[i]->phy_info->name);
    strcat(debug, nghb);
  }

  rs_debug(DEBUG_PHY, debug); 

}



/**** local functions ****/

