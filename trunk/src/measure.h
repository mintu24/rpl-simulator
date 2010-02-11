
#ifndef MEASURE_H_
#define MEASURE_H_

#include "base.h"
#include "node.h"


    /* connectivity measurement output info  */
typedef struct measure_connect_output_t {

    sim_time_t              connected_time;
    sim_time_t              total_time;
    sim_time_t              measure_time;

} measure_connect_output_t;

    /* connectivity measurement info */
typedef struct measure_connect_t {

    node_t *                src_node;
    node_t *                dst_node;

    int32                   last_connected_event_time;

    measure_connect_output_t
                            output;

} measure_connect_t;


    /* shortest path (Dijkstra) comparison measurement output info  */
typedef struct measure_sp_comp_output_t {

    float                   rpl_cost;
    float                   sp_cost;
    sim_time_t              measure_time;

} measure_sp_comp_output_t;

    /* shortest path (Dijkstra) comparison measurement info  */
typedef struct measure_sp_comp_t {

    node_t *                src_node;
    node_t *                dst_node;

    measure_sp_comp_output_t
                            output;

} measure_sp_comp_t;


    /* convergence measurement output info  */
typedef struct measure_converg_output_t {

    uint16                  converged_node_count;
    uint16                  total_node_count;
    sim_time_t              measure_time;

} measure_converg_output_t;

    /* convergence measurement info  */
typedef struct measure_converg_t {

    measure_converg_output_t
                            output;

} measure_converg_t;


bool                        measure_init();
bool                        measure_done();


void                        measure_connect_entry_add(node_t *src_node, node_t *dst_node);
void                        measure_connect_entry_remove(uint16 index);
void                        measure_connect_entry_remove_all();
uint16                      measure_connect_entry_get_count();
measure_connect_t *         measure_connect_entry_get(uint16 index);
void                        measure_connect_reset_output();
void                        measure_connect_update_output();

void                        measure_sp_comp_entry_add(node_t *src_node, node_t *dst_node);
void                        measure_sp_comp_entry_remove(uint16 index);
void                        measure_sp_comp_entry_remove_all();
uint16                      measure_sp_comp_entry_get_count();
measure_sp_comp_t *         measure_sp_comp_entry_get(uint16 index);
void                        measure_sp_comp_reset_output();
void                        measure_sp_comp_update_output();

measure_converg_t *         measure_converg_entry_get();
void                        measure_converg_reset_output();
void                        measure_converg_update_output();


#endif /* MEASURE_H_ */
