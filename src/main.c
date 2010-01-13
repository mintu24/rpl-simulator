
#include <gtk/gtk.h>
#include <math.h>

#include "system.h"
#include "gui/mainwin.h"


void rs_print(FILE *stream, char *sym, const char *fname, const char *fmt, ...)
{
    char string[256];

    if (fmt == NULL) {
        fmt = "";
    }

    va_list ap;
    va_start(ap, fmt);
    vsnprintf(string, sizeof(string), fmt, ap);
    va_end(ap);

    if (strlen(string) > 0) {
        fprintf(stream, "%s%s(): %s\n", sym, fname, string);
    }
    else {
        fprintf(stream, "%s%s()\n", sym, fname);
    }
}

void rs_quit()
{
    gtk_main_quit();
}

void do_every_second(node_t *node, void *data)
{
    rs_info("    enter: node '%s'", node->name);
    sleep(1);
    rs_debug("dequeued a pdu: 0x%X", node_process_pdu(node));
    rs_info("    exit: node '%s'", node->name);
}

int main(int argc, char *argv[]) {
	rs_info("hello");

	g_thread_init(NULL);

	node_t *node1 = node_create("one", 10, 15);
	node_start(node1);
	node_schedule(node1, "later", do_every_second, NULL, 1000000, FALSE);

	phy_pdu_t *phy_pdu = phy_pdu_create();
	node_receive_pdu(node1, phy_pdu, PHY_TRANSMIT_MODE_BLOCK);
	node_receive_pdu(node1, phy_pdu, PHY_TRANSMIT_MODE_BLOCK);

	rs_debug("process = 0x%X", node_process_pdu(node1));
	rs_debug("process = 0x%X", node_process_pdu(node1));
	rs_debug("process = 0x%X", node_process_pdu(node1));

//    node_execute(node1, "blocking test", do_every_second, "executed blocking", FALSE);
//    rs_debug("---------------");

//	gtk_init(&argc, &argv);
//	GtkWidget *main_window = main_window_create();
//	gtk_widget_show_all(main_window);
	gtk_main();

	rs_info("bye!");

	return 0;
}
