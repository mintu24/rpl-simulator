
#include <unistd.h>
#include <math.h>
#include <gtk/gtk.h>

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
    gtk_main_quit();
}

node_t *create_node(char *name, char *mac, char *ip, coord_t cx, coord_t cy)
{
    node_t *node = node_create(name, 0, 0);

    phy_node_info_t *phy_node_info = phy_node_info_create(name, cx, cy);
    phy_node_info->battery_level = 0.5;
    phy_node_info->mains_powered = FALSE;
    phy_node_info->power_level = 0.5;
    phy_init_node(node, phy_node_info);

    mac_node_info_t *mac_node_info = mac_node_info_create(mac);
    mac_init_node(node, mac_node_info);

    ip_node_info_t *ip_node_info = ip_node_info_create(ip);
    ip_init_node(node, ip_node_info);

    rpl_node_info_t *rpl_node_info = rpl_node_info_create();
    rpl_init_node(node, rpl_node_info);

    return node;
}

int main(int argc, char *argv[]) {
	rs_info("hello");

	g_thread_init(NULL);

	rs_system_create();

	/****************************************/

	node_t *node_a = create_node("A", "AA:AA:AA:AA:AA:AA", "10.0.0.1", 10, 20);
	node_t *node_b = create_node("B", "BB:BB:BB:BB:BB:BB", "10.0.0.2", 30, 40);

	node_start(node_a);
	node_start(node_b);

	rs_system_send_rpl_dis(node_a, node_b);

    /****************************************/

//    gtk_init(&argc, &argv);
//    GtkWidget *main_window = main_window_create();
//    gtk_widget_show_all(main_window);
	gtk_main();

	rs_system_destroy();

	rs_info("bye!");

	return 0;
}
