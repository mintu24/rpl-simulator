
#include "phy.h"
#include "mac.h"
#include "../system.h"


    /**** global variables ****/

uint16                  phy_event_node_wake;
uint16                  phy_event_node_kill;

uint16                  phy_event_pdu_send;
uint16                  phy_event_pdu_receive;


    /**** local function prototypes ****/

static bool             event_handler_node_wake(node_t *node);
static bool             event_handler_node_kill(node_t *node);

static bool             event_handler_pdu_send(node_t *node, node_t *outgoing_node, phy_pdu_t *pdu);
static bool             event_handler_pdu_receive(node_t *node, node_t *incoming_node, phy_pdu_t *pdu);

static void             debug_print_neighbor_list(node_t* node);
static void             event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len);


    /**** exported functions ****/

bool phy_init()
{
    phy_event_node_wake = event_register("node_wake", "phy", (event_handler_t) event_handler_node_wake, NULL);
    phy_event_node_kill = event_register("node_kill", "phy", (event_handler_t) event_handler_node_kill, NULL);

    phy_event_pdu_send = event_register("pdu_send", "phy", (event_handler_t) event_handler_pdu_send, event_arg_str);
    phy_event_pdu_receive = event_register("pdu_receive", "phy", (event_handler_t) event_handler_pdu_receive, event_arg_str);
    
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

void phy_pdu_destroy(phy_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    free(pdu);
}

phy_pdu_t *phy_pdu_duplicate(phy_pdu_t *pdu)
{
    rs_assert(pdu != NULL);

    phy_pdu_t *new_pdu = malloc(sizeof(phy_pdu_t));

    new_pdu->sdu = mac_pdu_duplicate(pdu->sdu);

    return new_pdu;
}

void phy_pdu_set_sdu(phy_pdu_t *pdu, void *sdu)
{
    rs_assert(pdu != NULL);

    pdu->sdu = sdu;
}

void phy_node_init(node_t *node, char *name, coord_t cx, coord_t cy)
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

void phy_node_set_coordinates(node_t* node, coord_t cx, coord_t cy)
{
  rs_assert(node != NULL);

  nodes_lock();

  node->phy_info->cx = cx;
  node->phy_info->cy = cy;

  nodes_unlock();

  phy_node_update_neighbor_list(node);
}

void phy_node_set_tx_power(node_t* node, percent_t tx_power)
{
  rs_assert(node != NULL);

  nodes_lock();

  node->phy_info->tx_power = tx_power;

  nodes_unlock();

  phy_node_update_neighbor_list(node);

}

bool phy_send(node_t *node, node_t *outgoing_node, void *sdu)
{
    rs_assert(node != NULL);
    rs_assert(sdu != NULL);

    phy_pdu_t *phy_pdu = phy_pdu_create();
    phy_pdu_set_sdu(phy_pdu, sdu);

    if (!event_execute(phy_event_pdu_send, node, outgoing_node, phy_pdu)) {
        phy_pdu_destroy(phy_pdu);
        return FALSE;
    }

    return TRUE;
}

bool phy_receive(node_t *node, node_t *incoming_node, phy_pdu_t *pdu)
{
    rs_assert(node != NULL);
    rs_assert(pdu != NULL);

    bool all_ok = event_execute(phy_event_pdu_receive, node, incoming_node, pdu);

    phy_pdu_destroy(pdu);

    return all_ok;
}

node_t* phy_node_find_neighbor(node_t* node, char* name)
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

bool phy_node_add_neighbor(node_t* node, node_t* neighbour)
{
  
  rs_assert(node != NULL);
  rs_assert(node->phy_info != NULL);
  rs_assert(neighbour != NULL);

  if( phy_node_find_neighbor(node,neighbour->phy_info->name) != NULL) return FALSE;
  node->phy_info->neighbors_list = realloc(node->phy_info->neighbors_list, (++node->phy_info->neighbors_count) * sizeof(node_t*));
  node->phy_info->neighbors_list[node->phy_info->neighbors_count -1] = neighbour; 

  return TRUE;
}

bool phy_node_rem_neighbor(node_t* node, char* name)
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

//node_t** phy_get_neighbors_list_copy(node_t* node, uint16 *count)
//{
//  rs_assert(node != NULL);
//  rs_assert(count != NULL);
//
//  *count = node->phy_info->neighbors_count;
//  node_t **copy = malloc(sizeof(node_t*) * node->phy_info->neighbors_count);
//  uint16 i;
//  for(i = 0; i < node->phy_info->neighbors_count; i++){
//    copy[i] = node->phy_info->neighbors_list[i];
//  }
//
//  return copy;
//
//}

bool phy_node_update_neighbor_list(node_t* node) {

    rs_assert(node != NULL);

    if (!node->alive)
        return FALSE;

    nodes_lock();

    int has_changed = 0;
    uint16 i;
    node_t* other = NULL;

    for (i = 0; i < rs_system->node_count; i++) {
        other = rs_system->node_list[i];

        if (other == node) {
            continue;
        }

        if (!other->alive) {
            continue;
        }

        percent_t quality = rs_system_get_link_quality(node, other);
        if (quality < rs_system->no_link_quality_thresh) {
            if (phy_node_find_neighbor(node, other->phy_info->name) != NULL) {
                phy_node_rem_neighbor(node, other->phy_info->name);
                phy_node_rem_neighbor(other, node->phy_info->name);
                if (has_changed == 0)
                    has_changed = 1;
            }
        } else {
            if (phy_node_find_neighbor(node, other->phy_info->name) == NULL) {
                phy_node_add_neighbor(node, other);
                phy_node_add_neighbor(other, node);
                if (has_changed == 0)
                    has_changed = 1;
            }
        }

    }

    nodes_unlock();

    debug_print_neighbor_list(node);

    if (has_changed == 1) {
        return TRUE;
    } else {
        return FALSE;
    }
}


/**** local functions ****/

static bool event_handler_node_wake(node_t *node)
{
    return TRUE;
}

static bool event_handler_node_kill(node_t *node)
{
    rs_assert(node != NULL);

    /* removing all the phy neighboring reference to this node */
    uint16 i;
    for(i = 0; i < node->phy_info->neighbors_count; i++){
        node_t* other = node->phy_info->neighbors_list[i];
        phy_node_rem_neighbor(other, node->phy_info->name);
        phy_node_rem_neighbor(node, other->phy_info->name);
    }
    return TRUE;
}

static bool event_handler_pdu_send(node_t *node, node_t *outgoing_node, phy_pdu_t *pdu)
{
    return rs_system_send(node, outgoing_node, pdu);
}

static bool event_handler_pdu_receive(node_t *node, node_t *incoming_node, phy_pdu_t *pdu)
{
    mac_pdu_t *mac_pdu = pdu->sdu;

    return mac_receive(node, incoming_node, mac_pdu);
}

static void debug_print_neighbor_list(node_t* node)
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

static void event_arg_str(uint16 event_id, void *data1, void *data2, char *str1, char *str2, uint16 len)
{
    str1[0] = '\0';
    str2[0] = '\0';

    if (event_id == phy_event_pdu_send) {
        node_t *node = data1;
        snprintf(str1, len, "outgoing_node = '%s'", (node != NULL ? node->phy_info->name : "<<unknown>>"));
    }
    else if (event_id == phy_event_pdu_receive) {
        node_t *node = data1;
        snprintf(str1, len, "incoming_node = '%s'", (node != NULL ? node->phy_info->name : "<<unknown>>"));
    }
}
