
#include <unistd.h>
#include <math.h>
#include <gtk/gtk.h>

#include "main.h"
#include "system.h"
#include "gui/mainwin.h"


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

    if (strlen(string) > 0) {
        if (file != NULL && strlen(file) > 0) {
            //fprintf(stream, "%s[in %s() at %s:%d] %s\n", sym, function, file, line, string);
            fprintf(stream, "%s[in %s() at %s:%d]\n", sym, function, file, line);
            fprintf(stream, "    %s\n", string);
        }
        else {
            fprintf(stream, "%s%s\n", sym, string);
        }
    }
    else {
        fprintf(stream, "%s[in %s() at %s:%d]\n", sym, function, file, line);
    }
}

void rs_quit()
{
    // todo: ask "are you sure" stupid question
    gtk_main_quit();
}

void rs_add_node()
{
    node_t *node = node_create();

    // todo: choose random/next values
    phy_node_info_t *phy_node_info = phy_node_info_create("A", 10, 20);
    phy_node_info->battery_level = 0.5;
    phy_node_info->mains_powered = FALSE;
    phy_node_info->tx_power = 0.5;
    phy_init_node(node, phy_node_info);

    mac_node_info_t *mac_node_info = mac_node_info_create("AA:AA:AA:AA:AA:AA");
    mac_init_node(node, mac_node_info);

    ip_node_info_t *ip_node_info = ip_node_info_create("10.0.0.1");
    ip_init_node(node, ip_node_info);

    rpl_node_info_t *rpl_node_info = rpl_node_info_create();
    rpl_init_node(node, rpl_node_info);

    rs_system_add_node(node);

    node = node_create();

    phy_node_info = phy_node_info_create("B", 30, 100);
    phy_node_info->battery_level = 0.5;
    phy_node_info->mains_powered = FALSE;
    phy_node_info->tx_power = 0.5;
    phy_init_node(node, phy_node_info);

    mac_node_info = mac_node_info_create("BB:BB:BB:BB:BB:BB");
    mac_init_node(node, mac_node_info);

    ip_node_info = ip_node_info_create("10.0.0.2");
    ip_init_node(node, ip_node_info);

    rpl_node_info = rpl_node_info_create();
    rpl_init_node(node, rpl_node_info);

    rs_system_add_node(node);
}


int main(int argc, char *argv[])
{
	rs_info("hello");

	g_thread_init(NULL);

	rs_system_create();

    gtk_init(&argc, &argv);
    main_win_init();
	gtk_main();

	rs_system_destroy();

	rs_info("bye!");

	return 0;
}
