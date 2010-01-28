
#include <unistd.h>
#include <math.h>
#include <gtk/gtk.h>
#include <ctype.h>

#include "main.h"
#include "system.h"
#include "gui/mainwin.h"
#include "gui/simfield.h"


    /**** global variables ****/

GThread *rs_main_thread = NULL;


    /**** local function prototypes ****/

static char *       get_next_name(char *name);
static void         get_next_coords(coord_t *x, coord_t *y);
static char *       get_next_mac_address(char *address);
static char *       get_next_ip_address(char *address);

static node_t *     find_node_by_thread(GThread *thread);


    /**** exported functions ****/

void rs_open(char *filename)
{
    // todo: implement me
}

void rs_save(char *filename)
{
    // todo: implement me
}

void rs_quit()
{
    gtk_main_quit();
}

void rs_start()
{
//    // todo: implement me
//
//    rs_add_node();
//    rs_add_node();
//    rs_add_node();
//
//    rs_wake_all_nodes();
//
//    node_t *a = rs_system_find_node_by_name("A");
//    node_t *b = rs_system_find_node_by_name("B");
//    node_t *c = rs_system_find_node_by_name("C");
//
//    if (a == NULL || b == NULL || c == NULL) {
//        return;
//    }
//
//    ip_node_add_route(a, 0, "0000", 0, b, FALSE);
//    ip_node_add_route(b, 0, "0000", 0, c, FALSE);
//
//    if (a->alive && b->alive && c->alive)
//        rpl_send_dis(a, c);
//
//    sim_field_redraw();
//
//    /************************************/
}

void rs_stop(char *filename)
{
    // todo: implement me

    system_lock();

    uint16 node_count;
    int32 index;
    node_t **node_list = rs_system_get_node_list(&node_count);

    for (index = 0; index < node_count; index++) {
        node_t *node = node_list[index];
        printf("node '%s':\n", phy_node_get_name(node));

        icmp_node_lock(node);

        uint32 meas_count, i;
        icmp_ping_measure_t **measures = icmp_node_get_ping_measure_list(node, &meas_count);
        for (i = 0; i < meas_count; i++) {
            icmp_ping_measure_t *measure = measures[i];
            printf("    to '%s': failures %d/%d\n",
                    phy_node_get_name(measure->dst_node),
                    measure->failed_count,
                    measure->total_count);
        }

        icmp_node_unlock(node);
    }

    system_unlock();

    /** fixme test **********************/
}

node_t *rs_add_node()
{
    char *new_name = NULL;
    char *new_mac_address = NULL;
    char *new_ip_address = NULL;
    coord_t new_x, new_y;

    uint16 node_count;
    int32 index;
    node_t **node_list = rs_system_get_node_list_copy(&node_count);

    for (index = node_count - 1; index >= 0; index--) {
        if (new_name == NULL)
            new_name = get_next_name(phy_node_get_name(node_list[index]));

        if (new_mac_address == NULL)
            new_mac_address = get_next_mac_address(mac_node_get_address(node_list[index]));

        if (new_ip_address == NULL)
            new_ip_address = get_next_ip_address(ip_node_get_address(node_list[index]));


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

    if (new_name == NULL) {
        new_name = get_next_name(NULL);
    }

    if (new_mac_address == NULL) {
        new_mac_address = get_next_mac_address(NULL);
    }

    if (new_ip_address == NULL) {
        new_ip_address = get_next_ip_address(NULL);
    }

    if (node_count > 0) {
        new_x = phy_node_get_x(node_list[node_count - 1]);
        new_y = phy_node_get_y(node_list[node_count - 1]);
    }
    else {
        new_x = -1;
        new_y = -1;
    }

    get_next_coords(&new_x, &new_y);

    node_t *node = node_create();

    phy_node_init(node, new_name, new_x, new_y);
    mac_node_init(node, new_mac_address);
    ip_node_init(node, new_ip_address);
    icmp_node_init(node);
    rpl_node_init(node);

    if (node_count == 0) {
        rpl_node_set_rank(node, RPL_RANK_ROOT);
    }

    /* add routes to everyone */

    for (index = 0; index < node_count; index++) {
        node_t *dst_node = node_list[index];

        ip_node_add_route(node, IP_ROUTE_TYPE_MANUAL, ip_node_get_address(dst_node), 16, dst_node, FALSE);
        ip_node_add_route(dst_node, IP_ROUTE_TYPE_MANUAL, ip_node_get_address(node), 16, node, FALSE);
    }

    free(node_list);

    rs_system_add_node(node);


    free(new_name);
    free(new_mac_address);
    free(new_ip_address);

    main_win_system_to_gui();

    return node;
}

void rs_rem_node(node_t *node)
{
    rs_assert(node != NULL);

    if (!rs_system_remove_node(node)) {
        rs_error("failed to remove node '%s' from the system", phy_node_get_name(node));
        return;
    }

    main_win_system_to_gui();

    /* this won't actually destroy the node, but just kill it.
     * instead, a garbage collector thread will check from time to time
     * for completely unreferenced nodes, and effectively destroy them.
     */
    if (node->alive && !node_kill(node)) {
        rs_error("failed to kill node '%s'", phy_node_get_name(node));
    }
}

void rs_wake_node(node_t *node)
{
    if (!node_wake(node)) {
        rs_error("failed to wake node '%s'", phy_node_get_name(node));
    }
}

void rs_kill_node(node_t *node)
{
    if (!node_kill(node)) {
        rs_error("failed to kill node '%s'", phy_node_get_name(node));
    }
}

void rs_add_more_nodes(uint16 node_count)
{
    int i;
    for (i = 0; i < node_count; i++) {
        rs_add_node();
    }
}

void rs_rem_all_nodes()
{
    system_lock();

    node_t **node_list;
    uint16 node_count;

    node_list = rs_system_get_node_list(&node_count);
    while (node_count > 0) {
        node_t *node = node_list[node_count - 1];

        rs_rem_node(node);
        node_list = rs_system_get_node_list(&node_count);
    }

    system_unlock();
}

void rs_wake_all_nodes()
{
    system_lock();

    node_t **node_list;
    uint16 node_count, index;

    node_list = rs_system_get_node_list(&node_count);

    for (index = 0; index < node_count; index++) {
        node_t *node = node_list[index];
        if (!node->alive && !node_wake(node)) {
            rs_error("failed to wake node '%s'", phy_node_get_name(node));
        }
    }

    system_unlock();
}

void rs_kill_all_nodes()
{
    system_lock();

    node_t **node_list;
    uint16 node_count, index;

    node_list = rs_system_get_node_list(&node_count);

    for (index = 0; index < node_count; index++) {
        node_t *node = node_list[index];
        if (node->alive && !node_kill(node)) {
            rs_error("failed to kill node '%s'", phy_node_get_name(node));
        }
    }

    system_unlock();
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

    node_t *node = NULL;
    if (g_thread_get_initialized() && (rs_system != NULL) && (g_thread_self() != rs_system->main_thread))
        node = find_node_by_thread(g_thread_self());

    char *context;
    if (node == NULL) {
        if (rs_system != NULL && (g_thread_self() == rs_system->gc_thread)) {
            context = "garbage collector";
        }
        else {
            context = "main";
        }
    }
    else {
        context = phy_node_get_name(node);
    }

    if (strlen(string) > 0) {
        if (file != NULL && strlen(file) > 0) {
            fprintf(stream, "%s%s: [in %s() at %s:%d]\n", sym, context, function, file, line);
            fprintf(stream, "    %s\n", string);
        }
        else {
            fprintf(stream, "%s%s: %s\n", sym, context, string);
        }
    }
    else {
        fprintf(stream, "%s%s: [in %s() at %s:%d]\n", sym, context, function, file, line);
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

static void get_next_coords(coord_t *x, coord_t *y)
{
    rs_assert(x != NULL);
    rs_assert(y != NULL);

    // todo arrange the nodes a little bit more nice

    if (*x == -1)
        *x = rs_system_get_width() / 2;
    else
        *x = rand() % (uint32) rs_system_get_width();

    if (*y == -1)
        *y = rs_system_get_height() / 2;
    else
        *y = rand() % (uint32) rs_system_get_height();
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


static node_t *find_node_by_thread(GThread *thread)
{
    uint16 node_count, index;
    node_t **node_list;
    node_list = rs_system_get_node_list_copy(&node_count);

    for (index = 0; index < node_count; index++) {
        node_t *node = node_list[index];

        if (node->life == thread) {
            free(node_list);

            return node;
        }
    }

    free(node_list);

    return NULL;
}


int main(int argc, char *argv[])
{
	rs_info("hello");

	g_thread_init(NULL);
	rs_main_thread = g_thread_self();

	rs_system_create();

    gtk_init(&argc, &argv);
    main_win_init();
	gtk_main();

	rs_system_destroy();

	rs_info("bye!");

	return 0;
}
