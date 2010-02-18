
#include "measure.h"
#include "system.h"


    /**** global variables ****/

static measure_connect_t *          measure_connect_list = NULL;
static uint16                       measure_connect_count = 0;

static measure_sp_comp_t *          measure_sp_comp_list = NULL;
static uint16                       measure_sp_comp_count = 0;

static measure_converg_t            measure_converg;


    /**** local function prototypes ****/

static measure_connect_output_t     measure_connect_compute_output(measure_connect_t *measure);
static measure_sp_comp_output_t     measure_sp_comp_compute_output(measure_sp_comp_t *measure);
static measure_converg_output_t     measure_converg_compute_output();


    /**** exported functios ****/

bool measure_init()
{
    measure_converg.output.converged_node_count = 0;
    measure_converg.output.total_node_count = 0;
    measure_converg.output.measure_time = 0;

    return TRUE;
}

bool measure_done()
{
    return TRUE;
}

void measure_connect_entry_add(node_t *src_node, node_t *dst_node)
{
    measures_lock();

    measure_connect_list = realloc(measure_connect_list, (measure_connect_count + 1) * sizeof(measure_connect_t));

    measure_connect_list[measure_connect_count].src_node = src_node;
    measure_connect_list[measure_connect_count].dst_node = dst_node;
    measure_connect_list[measure_connect_count].last_connected_event_time = -1;
    measure_connect_list[measure_connect_count].output.connected_time = 0;
    measure_connect_list[measure_connect_count].output.total_time = 0;
    measure_connect_list[measure_connect_count].output.measure_time = 0;

    measure_connect_count++;

    measures_unlock();
}

void measure_connect_entry_remove(uint16 index)
{
    measures_lock();

    rs_assert(index < measure_connect_count);

    uint16 i;
    for (i = index; i < measure_connect_count - 1; i++) {
        measure_connect_list[i] = measure_connect_list[i + 1];
    }

    measure_connect_count--;
    measure_connect_list = realloc(measure_connect_list, measure_connect_count * sizeof(measure_connect_t));
    if (measure_connect_count == 0) {
        measure_connect_list = NULL;
    }

    measures_unlock();
}

void measure_connect_entry_remove_all()
{
    measures_lock();

    if (measure_connect_list != NULL)
        free(measure_connect_list);

    measure_connect_count = 0;

    measures_unlock();
}

uint16 measure_connect_entry_get_count()
{
    return measure_connect_count;
}

measure_connect_t *measure_connect_entry_get(uint16 index)
{
    rs_assert(index < measure_connect_count);

    return &measure_connect_list[index];
}

void measure_connect_reset_output()
{
    measures_lock();

    uint16 i;
    for (i = 0; i < measure_connect_count; i++) {
        measure_connect_t *measure = &measure_connect_list[i];
        measure->output.connected_time = 0;
        measure->output.total_time = 0;
        measure->output.measure_time = 0;
    }

    measures_unlock();
}

void measure_connect_update_output()
{
    measures_lock();

    uint16 i;
    for (i = 0; i < measure_connect_count; i++) {
        measure_connect_t *measure = &measure_connect_list[i];
        measure->output = measure_connect_compute_output(measure);
    }

    measures_unlock();
}

void measure_sp_comp_entry_add(node_t *src_node, node_t *dst_node)
{
    measures_lock();

    measure_sp_comp_list = realloc(measure_sp_comp_list, (measure_sp_comp_count + 1) * sizeof(measure_sp_comp_t));

    measure_sp_comp_list[measure_sp_comp_count].src_node = src_node;
    measure_sp_comp_list[measure_sp_comp_count].dst_node = dst_node;
    measure_sp_comp_list[measure_sp_comp_count].output.rpl_cost = 0;
    measure_sp_comp_list[measure_sp_comp_count].output.sp_cost = 0;
    measure_sp_comp_list[measure_sp_comp_count].output.measure_time = 0;

    measure_sp_comp_count++;

    measures_unlock();
}

void measure_sp_comp_entry_remove(uint16 index)
{
    measures_lock();

    rs_assert(index < measure_sp_comp_count);

    uint16 i;
    for (i = index; i < measure_sp_comp_count - 1; i++) {
        measure_sp_comp_list[i] = measure_sp_comp_list[i + 1];
    }

    measure_sp_comp_count--;
    measure_sp_comp_list = realloc(measure_sp_comp_list, measure_sp_comp_count * sizeof(measure_sp_comp_t));
    if (measure_sp_comp_count == 0) {
        measure_sp_comp_list = NULL;
    }

    measures_unlock();
}

void measure_sp_comp_entry_remove_all()
{
    measures_lock();

    if (measure_sp_comp_list != NULL)
        free(measure_sp_comp_list);

    measure_sp_comp_count = 0;

    measures_unlock();
}

uint16 measure_sp_comp_entry_get_count()
{
    return measure_sp_comp_count;
}

measure_sp_comp_t *measure_sp_comp_entry_get(uint16 index)
{
    rs_assert(index < measure_sp_comp_count);

    return &measure_sp_comp_list[index];
}

void measure_sp_comp_reset_output()
{
    measures_lock();

    uint16 i;
    for (i = 0; i < measure_sp_comp_count; i++) {
        measure_sp_comp_t *measure = &measure_sp_comp_list[i];
        measure->output.rpl_cost = 0;
        measure->output.sp_cost = 0;
        measure->output.measure_time = 0;
    }

    measures_unlock();
}

void measure_sp_comp_update_output()
{
    measures_lock();

    uint16 i;
    for (i = 0; i < measure_sp_comp_count; i++) {
        measure_sp_comp_t *measure = &measure_sp_comp_list[i];
        measure->output = measure_sp_comp_compute_output(measure);
    }

    measures_unlock();
}

measure_converg_t *measure_converg_entry_get()
{
    return &measure_converg;
}

void measure_converg_reset_output()
{
    measure_converg.output.converged_node_count = 0;
    measure_converg.output.total_node_count = 0;
    measure_converg.output.measure_time = 0;
}

void measure_converg_update_output()
{
    measure_converg.output = measure_converg_compute_output();
}


    /**** local functions ****/

static measure_connect_output_t measure_connect_compute_output(measure_connect_t *measure)
{
    measure_connect_output_t output;

    output.connected_time = 0;
    output.total_time = rs_system->now;
    output.measure_time = rs_system->now;

    return output;
}

static measure_sp_comp_output_t measure_sp_comp_compute_output(measure_sp_comp_t *measure)
{
    measure_sp_comp_output_t output;

    output.rpl_cost = 0;
    output.sp_cost = 0;
    output.measure_time = rs_system->now;

    return output;
}

static measure_converg_output_t measure_converg_compute_output()
{
    measure_converg_output_t output;

    output.measure_time = rs_system->now;
    output.converged_node_count = 0;
    output.total_node_count = rs_system->node_count;

    return output;
}
