/*
   RPL Simulator.

   Copyright (c) Calin Crisan 2010

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <unistd.h>
#include <math.h>
#include <gtk/gtk.h>
#include <ctype.h>
#include <libgen.h> /* for dirname() */
#include <sys/resource.h> /* for rlimit */

#include "main.h"
#include "system.h"
#include "scenario.h"

#include "gui/mainwin.h"
#include "gui/simfield.h"
#include "gui/dialogs.h"


    /**** global variables ****/

char *              rs_app_dir;
char *              rs_scenario_file_name = NULL;


    /**** local function prototypes ****/

static char *       get_next_name(char *name);
static char *       get_next_mac_address(char *address);
static char *       get_next_ip_address(char *address);


    /**** exported functions ****/

void rs_new()
{
    rs_rem_all_nodes();

    if (rs_scenario_file_name != NULL) {
        free(rs_scenario_file_name);
        rs_scenario_file_name = NULL;
    }

    main_win_system_to_gui();
    main_win_display_to_gui();
    main_win_events_to_gui();
}

char *rs_open(char *filename)
{
    rs_rem_all_nodes();

    if (rs_scenario_file_name != NULL) {
        free(rs_scenario_file_name);
        rs_scenario_file_name = NULL;
    }

    char *msg = scenario_load(filename);
    if (msg == NULL) {
        rs_scenario_file_name = strdup(filename);
    }

    main_win_system_to_gui();
    main_win_display_to_gui();
    main_win_events_to_gui();

    return msg;
}

char* rs_save(char *filename)
{
    char *msg = scenario_save(filename);

    if (msg == NULL) {
        if (rs_scenario_file_name == NULL || strcmp(rs_scenario_file_name, filename) != 0) {
            if (rs_scenario_file_name != NULL) {
                free(rs_scenario_file_name);
            }

            rs_scenario_file_name = strdup(filename);
        }
    }

    main_win_system_to_gui();

    return msg;
}

void rs_quit()
{
    rs_system_stop();

    gtk_main_quit();
}

void rs_start(bool start_paused)
{
    if (!rs_system->paused) {
        if (rs_scenario_file_name != NULL) { /* this overwrites the old log, if it exists */
            char *ext = rindex(rs_scenario_file_name, '.');
            char *path_no_ext;
            if (ext != NULL) {
                path_no_ext = strndup(rs_scenario_file_name, ext - rs_scenario_file_name);
            }
            else {
                path_no_ext = strdup(rs_scenario_file_name);
            }

            char path[256];
            snprintf(path, sizeof(path), "%s.log", path_no_ext);
            event_set_log_file(path);

            free(path_no_ext);
        }
        else {
            event_set_log_file(NULL);
        }
    }

    rs_system_start(start_paused);
    main_win_update_sim_status();
}

void rs_pause()
{
    rs_system_pause();

    main_win_update_sim_status();
}

void rs_step()
{
    if (!rs_system->started) {
        rs_start(TRUE);
    }
    else {
        rs_system_step();
    }

    main_win_update_sim_status();
}

void rs_stop()
{
    rs_system_stop();

    main_win_update_sim_status();
}

node_t *rs_add_node(coord_t x, coord_t y)
{
    nodes_lock();

    char *new_name = NULL;
    char *new_mac_address = NULL;
    char *new_ip_address = NULL;

    int32 index;

    for (index = rs_system->node_count - 1; index >= 0; index--) {
        node_t *node = rs_system->node_list[index];

        if (new_name == NULL)
            new_name = get_next_name(node->phy_info->name);

        if (new_mac_address == NULL)
            new_mac_address = get_next_mac_address(node->mac_info->address);

        if (new_ip_address == NULL)
            new_ip_address = get_next_ip_address(node->ip_info->address);


        if (rs_system_find_node_by_name(new_name) != NULL) {
            free(new_name);
            new_name = NULL;
        }

        if (rs_system_find_node_by_mac_address(new_mac_address) != NULL) {
            free(new_mac_address);
            new_mac_address = NULL;
        }

        if (rs_system_find_node_by_ip_address(new_ip_address) != NULL) {
            free(new_ip_address);
            new_ip_address = NULL;
        }
    }

    nodes_unlock();

    if (new_name == NULL) {
        new_name = get_next_name(NULL);
    }

    if (new_mac_address == NULL) {
        new_mac_address = get_next_mac_address(NULL);
    }

    if (new_ip_address == NULL) {
        new_ip_address = get_next_ip_address(NULL);
    }

    node_t *node = node_create();

    measure_node_init(node);
    phy_node_init(node, new_name, x, y);
    mac_node_init(node, new_mac_address);
    ip_node_init(node, new_ip_address);
    icmp_node_init(node);
    rpl_node_init(node);

    rs_system_add_node(node);

    free(new_name);
    free(new_mac_address);
    free(new_ip_address);


    return node;
}

void rs_rem_node(node_t *node)
{
    rs_assert(node != NULL);

    if (!rs_system_remove_node(node)) {
        rs_error("failed to remove node '%s' from the system", node->phy_info->name);
        return;
    }

    node_destroy(node);

    main_win_system_to_gui();
    main_win_update_nodes_status();
}

void rs_wake_node(node_t *node)
{
    if (!node_wake(node)) {
        rs_error("failed to wake node '%s'", node->phy_info->name);
    }

    main_win_update_nodes_status();
}

void rs_kill_node(node_t *node)
{
    if (!node_kill(node)) {
        rs_error("failed to kill node '%s'", node->phy_info->name);
    }

    main_win_update_nodes_status();
}

void rs_add_more_nodes(uint16 node_number, uint8 pattern, coord_t horiz_dist, coord_t vert_dist, uint16 row_length)
{
    switch (pattern) {
        case ADD_MORE_DIALOG_PATTERN_RECTANGULAR : {
            uint16 i;
            coord_t x, y;

            for (i = 0; i < node_number; i++) {
                x = horiz_dist * (i % row_length);
                y = vert_dist * (i / row_length);

                x += (rs_system->width - (row_length - 1) * horiz_dist) / 2;
                y += (rs_system->height - ((node_number - 1) / row_length * vert_dist)) / 2;

                rs_add_node(x, y);
            }

            break;
        }

        case ADD_MORE_DIALOG_PATTERN_TRIANGULAR : {
            uint16 i, row_count, row_index, max_row_count;
            coord_t x, y;

            row_index = 0;
            row_count = 1;
            for (i = 0; i < node_number; i++) {
                if (row_index < row_count - 1) {
                    row_index++;
                }
                else {
                    row_index = 0;
                    row_count++;
                }
            }
            max_row_count = row_count;

            row_index = 0;
            row_count = 1;
            for (i = 0; i < node_number; i++) {
                x = rs_system->width / 2 + (horiz_dist * (row_index - row_count / 2));
                if (row_count % 2 == 0) {
                    x += horiz_dist / 2;
                }

                y = (rs_system->height - (max_row_count - 1) * vert_dist) / 2 + vert_dist * (row_count - 1);

                rs_add_node(x, y);

                if (row_index < row_count - 1) {
                    row_index++;
                }
                else {
                    row_index = 0;
                    row_count++;
                }
            }

            break;
        }

        case ADD_MORE_DIALOG_PATTERN_RANDOM : {
            uint16 i;

            for (i = 0; i < node_number; i++) {
                rs_add_node(rand() % (uint16) rs_system->width, rand() % (uint16) rs_system->height);
            }

            break;
        }
    }

    main_win_system_to_gui();
    main_win_update_nodes_status();
}

void rs_rem_all_nodes()
{
    nodes_lock();
    while (rs_system->node_count > 0) {
        node_t *node = rs_system->node_list[rs_system->node_count - 1];

        nodes_unlock();
        rs_system_remove_node(node);
        nodes_lock();
    }
    nodes_unlock();

    main_win_set_selected_node(NULL);
    main_win_system_to_gui();
    main_win_update_nodes_status();

    /* remove all scheduled events */
    rs_system_cancel_event(NULL, -1, NULL, NULL, 0);
}

void rs_wake_all_nodes()
{
    uint16 node_count;
    node_t **node_list = rs_system_get_node_list_copy(&node_count);

    uint16 i;
    for (i = 0; i < node_count; i++) {
        node_t *node = node_list[i];
        if (!node->alive && !node_wake(node)) {
            rs_error("failed to wake node '%s'", node->phy_info->name);
        }
    }

    if (node_list != NULL) {
        free(node_list);
    }

    main_win_update_nodes_status();
}

void rs_kill_all_nodes()
{
    uint16 node_count;
    node_t **node_list = rs_system_get_node_list_copy(&node_count);

    uint16 i;
    for (i = 0; i < node_count; i++) {
        node_t *node = node_list[i];
        if (node->alive && !node_kill(node)) {
            rs_error("failed to kill node '%s'", node->phy_info->name);
        }
    }

    if (node_list != NULL) {
        free(node_list);
    }

    main_win_update_nodes_status();
}

void rs_print(FILE *stream, char *sym, const char *file, int line, const char *function, const char *fmt, ...)
{
    char string[1024];

    if (fmt == NULL) {
        fmt = "";
    }

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(string, sizeof(string), fmt, ap);
    va_end(ap);

    char *thread;
    if (g_thread_self()->func == 0) {
        thread = "main";
    }
    else {
        thread = "system";
    }

    if (strlen(string) > 0) {
        if (file != NULL && strlen(file) > 0) {
            fprintf(stream, "%s[in %s/%s() at %s:%d]\n", sym, thread, function, file, line);
            fprintf(stream, "    %s\n", string);
        }
        else {
            fprintf(stream, "%s%s\n", sym, string);
        }
    }
    else {
        fprintf(stream, "%s[in %s/%s() at %s:%d]\n", sym, thread, function, file, line);
    }
}


    /**** local functions ****/

static char *get_next_name(char *name)
{
    char *new_name = malloc(256);

    if (name == NULL || strlen(name) == 0) {
        return strdup(DEFAULT_NODE_NAME);
    }
    else {
        int length = strlen(name);

        if (isdigit(name[length - 1])) { /* increment the trailing decimal */
            int number = 0, pos = length - 1;

            while (pos >= 0 && isdigit(name[pos])) {
                number += (name[pos] - '0') * pow(10, length - pos - 1);
                pos--;
            }

            number++;

            char base[256];
            strncpy(base, name, pos + 1);
            base[pos + 1] = '\0';

            snprintf(new_name, 256, "%s%d", base, number);
        }
        else if (isalpha(name[length - 1])) { /* increment last letter */
            char last_letter = name[length - 1];

            if (last_letter < 'Z') {
                char base[256];
                strncpy(base, name, length - 1);
                base[length - 1] = '\0';

                snprintf(new_name, 256, "%s%c", base, last_letter + 1);
            }
            else {
                snprintf(new_name, 256, "1");
            }

        }
        else { /* add a 1, whatever the name is */
            snprintf(new_name, 256, "%s1", name);
        }
    }

    return new_name;
}

static char *get_next_mac_address(char *address)
{
    char *new_address = malloc(256);

    if (address == NULL || strlen(address) == 0) {
        return strdup(DEFAULT_NODE_MAC_ADDRESS);
    }
    else {
        int length = strlen(address);
        int autoinc = (AUTOINC_ADDRESS_PART >> 2);

        char base[256], tail[256];
        if (length >= autoinc) {
            strncpy(base, address, length - autoinc);
            strncpy(tail, address + length - autoinc, autoinc);
            base[length - autoinc] = '\0';
            tail[autoinc] = '\0';
        }
        else {
            strcpy(tail, address);
            base[0] = '\0';
            tail[autoinc] = '\0';
        }

        uint32 number = strtoll(tail, NULL, 16);

        number++;

        char fmt[256];
        sprintf(fmt, "%%s%%0%dX", autoinc);
        snprintf(new_address, 256, fmt, base, number);
    }

    return new_address;
}

static char *get_next_ip_address(char *address)
{
    if (address == NULL || strlen(address) == 0) {
        return strdup(DEFAULT_NODE_IP_ADDRESS);
    }
    else {
        return get_next_mac_address(address);
    }
}


void makecoreifcrash(){
	struct rlimit rlim;
	if(!getrlimit(RLIMIT_CORE, &rlim) && rlim.rlim_cur != RLIM_INFINITY){
		rlim.rlim_cur = RLIM_INFINITY;
		rlim.rlim_max = RLIM_INFINITY;
		setrlimit(RLIMIT_CORE, &rlim);
	}
}

int main(int argc, char *argv[])
{
	makecoreifcrash();
	rs_info("hello");

	rs_app_dir = strdup(dirname(argv[0]));

	g_thread_init(NULL);
	gdk_threads_init();

	if (!rs_system_create()) {
	    rs_error("failed to initialize the system");
	    return -1;
	}

    gtk_init(&argc, &argv);

    if (!main_win_init()) {
        rs_error("failed to initialize main window");
        return -1;
    }

	gtk_main();

	if (!rs_system_destroy()) {
	    rs_error("failed to destroy the system");
	    return -1;
	}

    rs_info("bye!");

	return 0;
}
