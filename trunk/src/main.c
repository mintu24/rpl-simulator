
#include <gtk/gtk.h>
#include <math.h>

#include "node.h"
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
    rs_info("node '%s', event '%s'", node->name, (char *) data);
}

int main(int argc, char *argv[]) {
	rs_info("hello");

	g_thread_init(NULL);

	node_t *node1 = node_create("one", 10, 15);

	node_start(node1);
	int i;
	for (i = 1; i <= 100; i++) {
	    char name[256];
	    sprintf(name, "event%d", i);
	    node_schedule(node1, name, do_every_second, strdup(name), 2000000, TRUE);
	}
//	node_schedule(node1, "event1", NULL, NULL, 0, FALSE);
//	node_schedule(node1, "event2", NULL, NULL, 0, FALSE);

//	node_born(node2);
//	node_schedule(node2, "event2", do_every_second, "event2", 2000000, TRUE);

//	gtk_init(&argc, &argv);
//	GtkWidget *main_window = main_window_create();
//	gtk_widget_show_all(main_window);
	gtk_main();

	rs_info("bye!");

	return 0;
}
